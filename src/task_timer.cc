/*
    timer.cc
    libtq
    2022-11-15
    Push Chen
*/

/*
MIT License

Copyright (c) 2019 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <queue>
#include <iostream>
#include <algorithm>
#include <cmath>
#ifdef _WIN32
#include <Windows.h>
#include <timeapi.h>
#include <bcrypt.h>
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "ntdll.lib")
#endif
#include "task_timer.h"
#include "task.h"
#include "task_event_queue.h"
#include "task_thread.h"

#ifdef _WIN32
extern "C" NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(
  ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);
extern "C" NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(
  OUT PULONG MinimumResolution, OUT PULONG MaximumResolution, OUT PULONG CurrentResolution);
#endif
namespace libtq {

#ifdef _WIN32
ULONG* cached_time_resolution() {
  static ULONG g_cached_res;
  return &g_cached_res;
}

void adjust_timer_resolution_high() {
  ULONG min_res, max_res, cur_res;
  NtQueryTimerResolution(&min_res, &max_res, &cur_res);
  NtSetTimerResolution(max_res, TRUE, cached_time_resolution());
}

void adjust_timer_resolution_default() {
  NtSetTimerResolution(*cached_time_resolution(), TRUE, cached_time_resolution());
}
#endif

#if defined(_WIN32)
#define LIBTQ_PICKUP_EST_TIME   24u
#else
#define LIBTQ_PICKUP_EST_TIME   12u
#endif

typedef std::function<void(task_time_t)> timer_job_t;

template <typename T, typename M>
class removable_priority_queue : public std::priority_queue<T, std::vector<T>, std::greater<T>> {
public:
  bool erase(M m) {
    auto it = std::find_if(this->c.begin(), this->c.end(), [m](const T& item) {
      return item.match(m);
    });
    if (it == this->c.end()) {
      return false;
    }
    if (it == this->c.begin()) {
      this->pop();
    } else {
      this->c.erase(it);
      std::make_heap(this->c.begin(), this->c.end(), this->comp);
    }
    return true;
  }
};

struct priority_job {
  uint64_t      job_id;
  task_time_t   fire_time;
  timer_job_t   job;
  task_location loc;

  bool operator > (const priority_job& rh) const {
    return fire_time > rh.fire_time;
  }
  bool match(uint64_t jid) const {
    return this->job_id == jid;
  }
};

class timer_inner_worker : public thread {
public:
  static timer_inner_worker& instance() {
    static timer_inner_worker g_tiw;
    return g_tiw;
  }
  /**
   * @brief Force to stop the loop
  */
  ~timer_inner_worker() {
    // std::lock_guard<std::mutex> _(cv_l_);
    this->invalidate_();
    cv_.notify_all();
  }
  timer_inner_worker(const timer_inner_worker&) = delete;
  timer_inner_worker& operator = (const timer_inner_worker&) = delete;

  /**
   * @brief Add a job to the timer poll, will re-order the pending list
  */
  uint64_t add_next_job(task_time_t t, task_location loc, timer_job_t job) {
    static uint64_t s_timer_job_id = 0;
    priority_job pj{};
    uint64_t jid = s_timer_job_id++;
    pj.job_id = jid;
    pj.fire_time = t;
    pj.job = job;
    pj.loc = loc;
    std::lock_guard<std::mutex> _(cv_l_);
    pq_.emplace(std::move(pj));
    cv_.notify_all();
    return jid;
  }

  /**
   * @brief Remove an unfired job in the queue
  */
  void remove_unfired_job(uint64_t job_id) {
    std::lock_guard<std::mutex> _(cv_l_);
    pq_.erase(job_id);
  }

  void fire_job_wrapper(task_location loc, 
    task_time_t ft, unsigned int interval, 
    tq_wt related_tq, std::shared_ptr<bool> st, task_t job
  ) {
    if (!st || *st == false) {
      // stop the timer
      return;
    }
    if (job) {
      if (auto tq = related_tq.lock()) {
        tq->post_task(loc, job, 1); // the job should be set to the header of the task queue
      }
    }
    auto now = std::chrono::steady_clock::now();
    auto next_ft = ft + std::chrono::milliseconds(interval);
    if (next_ft <= now) {
      auto catch_up = std::chrono::duration_cast<std::chrono::milliseconds>(now - next_ft).count() / interval;
      next_ft += std::chrono::milliseconds(interval * catch_up);
    }
    while (next_ft <= now) {
      next_ft += std::chrono::milliseconds(interval);
    }
    this->add_next_job(next_ft, loc, [=](task_time_t last_ft) {
      timer_inner_worker::instance().fire_job_wrapper(loc, last_ft, interval, related_tq, st, job);
    });
  }
protected:

  /**
   * @brief start the inner waiting loop
  */
  timer_inner_worker() : 
    thread(make_thread_attribute(k_thread_attribute_default_stack_size, nullptr, thread_priority::k_realtime, "libtq_timer"))
  {
    this->start();
  }

  std::tuple<bool, priority_job, duration_t> get_top_fire_job() {
    bool has_fire_job = false;
    priority_job pj{};
    duration_t d = std::chrono::milliseconds(1000);
    std::lock_guard<std::mutex> _(cv_l_);
    if (pq_.size() > 0) {
      const auto& i = pq_.top();
      auto n = std::chrono::steady_clock::now();
      auto dlt = std::abs(std::chrono::duration_cast<std::chrono::microseconds>(n - i.fire_time).count());
      // Already timedout
      if (i.fire_time <= n || dlt <= LIBTQ_PICKUP_EST_TIME) {
        pj = i;
        pq_.pop();
        has_fire_job = true;
      } else {
        d = i.fire_time - n;
      }
    }
    return std::make_tuple(has_fire_job, std::move(pj), d);
  }

  void main() override {
    this->started_();
#ifdef _WIN32
    // timeBeginPeriod(1);
    adjust_timer_resolution_high();
#endif
    while (this->is_validate()) {  
      auto r = get_top_fire_job();
      if (std::get<0>(r) == true) {
        std::get<1>(r).job(std::get<1>(r).fire_time);
      } else {
#ifdef __APPLE__
        // We don't need to sleep if the delta is less than 100us
        size_t wait_offset = 0;
        if (std::get<2>(r) <= std::chrono::microseconds(LIBTQ_PICKUP_EST_TIME * 3)) {
          // let the loop to re-run 3 times, and post the task directly
          // use a spin loop to imporve the reslution
          continue;
        } else if (std::get<2>(r) <= std::chrono::microseconds(500)) {
          // [pku_est * 5, 500]
          wait_offset = LIBTQ_PICKUP_EST_TIME * 3;
        } else if (std::get<2>(r) <= std::chrono::microseconds(1000)) {
          // (500, 1000]
          wait_offset = 449;
        } else {
          // 
          wait_offset = 899;
        }
        auto wait_delta = std::get<2>(r) - std::chrono::microseconds(wait_offset);
#elif defined(_WIN32)
        size_t wait_offset = 0;
        if (std::get<2>(r) <= std::chrono::microseconds(LIBTQ_PICKUP_EST_TIME * 3)) {
          continue;
        } else if (std::get<2>(r) <= std::chrono::microseconds(500)) {
          // Nt Timer Resolution is 500 microseconds, try to yield the thread and retry the loop
          for (int i = 0; i < 3; ++i) {
            std::this_thread::yield();
          }
          continue;
        } else {
          wait_offset = (size_t)(std::chrono::duration_cast<std::chrono::microseconds>(std::get<2>(r)).count() / 500l - 1) * 500u;
          wait_offset -= LIBTQ_PICKUP_EST_TIME;
        }
        auto wait_delta = std::chrono::microseconds(wait_offset);
#endif
        std::unique_lock<std::mutex> _(cv_l_);
        // if the wait_for broken before timeout, next loop will 
        // continue to wait until timeout, so do not need pred here.
#ifdef __APPLE__
        cv_.wait_for(_, wait_delta);
#elif defined(_WIN32)
        cv_.wait_for(_, wait_delta);
#else
        cv_.wait_for(_, std::get<2>(r));
#endif
      }
    }
#ifdef _WIN32
    // timeEndPeriod(1);
    adjust_timer_resolution_default();
#endif
  }
protected:
  /**
   * @brief Inner Poll wrapper
  */
  std::condition_variable cv_;
  std::mutex cv_l_;

  /**
   * @brief Inner min-queue, order by task's fire time
  */
  removable_priority_queue<priority_job, uint64_t> pq_;
};

timer::timer(tq_wt related_tq) : status_(new bool), related_tq_(related_tq) {}
timer::~timer() {
  this->stop();
}
/**
 * @brief Get the related queue
*/
tq_wt timer::related_queue() const {
  return this->related_tq_;
}

/**
 * @brief Start the timer
*/
void timer::start(task_location loc, task_t job, unsigned int ms, bool fire_now) {
  if (!job || ms == 0) return;
  *status_ = true;
  auto now = std::chrono::steady_clock::now();
  auto next_ft = now + std::chrono::milliseconds(ms);
  auto rtq = this->related_tq_;
  auto st = this->status_;
  timer_inner_worker::instance().add_next_job(next_ft, loc, [=](task_time_t) {
    timer_inner_worker::instance().fire_job_wrapper(loc, next_ft, ms, rtq, st, job);
  });
  if (fire_now) {
    if (auto tq = this->related_tq_.lock()) {
      tq->post_task(loc, job);
    }
  }
}

/**
 * @brief Stop the timer
*/
void timer::stop() {
  if (status_) {
    *status_ = false;
  }
}

/**
 * @brief Start a job after some time to related task queue
*/
uint64_t timer::once_after(tq_wt related_tq, task_location loc, task_t job, unsigned int delay_ms) {
  if (!job || delay_ms == 0) return (uint64_t)-1;
  auto next_fire_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
  return timer_inner_worker::instance().add_next_job(next_fire_time, loc, [=](task_time_t) {
    if (auto tq = related_tq.lock()) {
      tq->post_task(loc, job, 1);
    }
  });
}
/**
 * @brief Cancel an un-fired delay job
*/
void timer::cancel_once(uint64_t job_id) {
  timer_inner_worker::instance().remove_unfired_job(job_id);
}


} // namespace libtq

// Push Chen
