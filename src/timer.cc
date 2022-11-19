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
#include "timer.h"
#include "task.h"
#include "event_queue.h"

namespace libtq {

typedef std::function<void(task_time_t)> timer_job_t;

struct priority_job {
  task_time_t   fire_time;
  timer_job_t   job;

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
    std::lock_guard<std::mutex> _(cv_l_);
    status_ = false;
    cv_.notify_all();
  }

  /**
   * @brief Add a job to the timer poll, will re-order the pending list
  */
  void add_next_job(task_time_t t, timer_job_t job) {
    std::lock_guard<std::mutex> _(cv_l_);
    priority_job pj;
    pj.fire_time = t;
    pj.job = job;
    pq_.emplace(std::move(pj));
    cv_.notify_all();
  }

  void fire_job_wrapper(task_time_t ft, unsigned int interval, tq_wt related_tq, std::shared_ptr<bool> st, task_t job) {
    if (!st || *st == false) {
      // stop the timer
      return;
    }
    if (job) {
      if (auto tq = related_tq.lock()) {
        tq->post_task(__TQ_TASK_LOC, job);
      }
    }
    auto now = std::chrono::steady_clock::now();
    auto next_ft = ft + std::chrono::milliseconds(interval);
    while (next_ft <= now) {
      next_ft += std::chrono::milliseconds(interval);
    }
    this->add_next_job(next_ft, [=](task_time_t last_ft) {
      timer_inner_worker::instance().fire_job_wrapper(last_ft, interval, related_tq, st, job);
    });
  }
protected:

  /**
   * @brief start the inner waiting loop
  */
  timer_inner_worker() : status_(false) {
    event_queue<int> sig;
    std::thread t([this, &sig]() {
      this->status_ = true;
      sig.emplace_back(1);
      this->waiting();
    });
    t.detach();

    sig.wait();
  }

  void waiting() {
    while (status_) {
      duration_t d = std::chrono::milliseconds(1000);
      if (pq_.size() > 0) {
        auto n = std::chrono::steady_clock::now();
        const auto& i = pq_.top();
        // Already timedout
        if (i.fire_time <= n) {
          i.job(i.fire_time);
          pq_.pop();
          continue;
        }
        d = (i.fire_time - n);
      }
      if (!status_) {
        continue;
      }
      std::unique_lock<std::mutex> _(cv_l_);
      // if the wait_for broken before timeout, next loop will 
      // continue to wait until timeout, so do not need pred here.
      cv_.wait_for(_, d);
    }
  }
protected:
  /**
   * @brief Inner Poll wrapper
  */
  std::condition_variable cv_;
  std::mutex cv_l_;

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
void timer::start(task_t job, unsigned int ms, bool fire_now) {
  if (!job || ms == 0) return;
  *status_ = true;
  auto now = std::chrono::steady_clock::now();
  auto next_ft = now + std::chrono::milliseconds(ms);
  auto rtq = this->related_tq_;
  auto st = this->status_;
  timer_inner_worker::instance().add_next_job(next_ft, [=](task_time_t last_ft) {
    timer_inner_worker::instance().fire_job_wrapper(next_ft, ms, rtq, st, job);
  });
  if (fire_now) {
    if (auto tq = this->related_tq_.lock()) {
      tq->post_task(__TQ_TASK_LOC, job);
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

} // namespace libtq

// Push Chen
