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
#ifdef _WIN32
#include <Windows.h>
#pragma comment(lib, "Winmm.lib")
#endif
#include "timer.h"
#include "task.h"
#include "event_queue.h"

namespace libtq {

typedef std::function<void(task_time_t)> timer_job_t;

struct priority_job {
  task_time_t   fire_time;
  timer_job_t   job;
  task_location loc;

  bool operator > (const priority_job& rh) const {
    return fire_time > rh.fire_time;
  }
};

class timer_inner_worker {
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
    status_ = false;
    cv_.notify_all();
    if (wt_->joinable()) {
      wt_->join();
    }
  }

  /**
   * @brief Add a job to the timer poll, will re-order the pending list
  */
  void add_next_job(task_time_t t, task_location loc, timer_job_t job) {
    std::lock_guard<std::mutex> _(cv_l_);
    priority_job pj;
    pj.fire_time = t;
    pj.job = job;
    pj.loc = loc;
    pq_.emplace(std::move(pj));
    cv_.notify_all();
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
        tq->post_task(loc, job);
      }
    }
    auto now = std::chrono::steady_clock::now();
    auto next_ft = ft + std::chrono::milliseconds(interval);
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
  timer_inner_worker() : status_(false) {
    event_queue<int> sig;
    wt_ = std::make_shared<std::thread>([this, &sig]() {
      this->status_ = true;
      sig.emplace_back(1);
      this->waiting();
    });

    sig.wait();
  }

  std::tuple<bool, priority_job, duration_t> get_top_fire_job() {
    std::lock_guard<std::mutex> _(cv_l_);
    bool has_fire_job = false;
    priority_job pj;
    duration_t d = std::chrono::milliseconds(1000);
    if (pq_.size() > 0) {
      auto n = std::chrono::steady_clock::now();
      const auto& i = pq_.top();
      // Already timedout
      if (i.fire_time <= n) {
        pj = i;
        pq_.pop();
        has_fire_job = true;
      } else {
        d = i.fire_time - n;
      }
    }
    return std::make_tuple(has_fire_job, std::move(pj), d);
  }

  void waiting() {
    while (status_) {  
      auto r = get_top_fire_job();
      if (std::get<0>(r) == true) {
        std::get<1>(r).job(std::get<1>(r).fire_time);
      } else {
        std::unique_lock<std::mutex> _(cv_l_);
        // if the wait_for broken before timeout, next loop will 
        // continue to wait until timeout, so do not need pred here.
#ifdef _WIN32
        timeBeginPeriod(1);
#endif
        cv_.wait_for(_, std::get<2>(r));
#ifdef _WIN32
        timeEndPeriod(1);
#endif
      }
    }
  }
protected:
  /**
   * @brief Inner Poll wrapper
  */
  std::condition_variable cv_;
  std::mutex cv_l_;
  std::shared_ptr<std::thread> wt_;

  /**
   * @brief status
  */
  std::atomic_bool status_;
  /**
   * @brief Inner min-queue, order by task's fire time
  */
  std::priority_queue<priority_job, std::vector<priority_job>, std::greater<priority_job>>  pq_;
};

timer::timer(tq_wt related_tq) : status_(new bool), related_tq_(related_tq) {}
timer::~timer() {
  this->stop();
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
  timer_inner_worker::instance().add_next_job(next_ft, loc, [=](task_time_t last_ft) {
    timer_inner_worker::instance().fire_job_wrapper(loc, next_ft, ms, rtq, st, job);
  });
  if (fire_now) {
    if (auto tq = this->related_tq_.lock()) {
      tq->post_task(loc, job);
    }
  }
}

/**
 * @brief Start a job after some time
*/
void timer::start_once_after(task_location loc, task_t job, unsigned int ms, 
  std::function<bool()> pred
) {
  if (!job || ms == 0) return;
  auto now = std::chrono::steady_clock::now();
  auto next_ft = now + std::chrono::milliseconds(ms);
  auto rtq = this->related_tq_;
  timer_inner_worker::instance().add_next_job(next_ft, loc, [=](task_time_t last_ft) {
    auto wrapper_job = [=]() {
      if (pred) {
        if (!pred()) return;
      }
      job();
    };
    if (auto tq = rtq.lock()) {
      tq->post_task(loc, wrapper_job);
    }
  });
}

/**
 * @brief Stop the timer
*/
void timer::stop() {
  if (status_) {
    *status_ = false;
  }
}

} // namespace libtq

// Push Chen
