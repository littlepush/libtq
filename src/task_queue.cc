/*
    task_queue.cc
    libtq
    2022-11-11
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

#include "task_queue.h"

namespace libtq {

class state_semaphore {
public:
  void wait() {
    std::unique_lock<std::mutex> _(l_);
    cv_.wait(_, [this]() {
      return this->st_;
    });
  }
  void notify() {
    std::lock_guard<std::mutex> _(l_);
    st_ = true;
    cv_.notify_all();
  }
protected:
  std::mutex l_;
  std::condition_variable cv_;
  bool st_ = false;
};

class movable_flag {
public:
  movable_flag() : p_ss_(new state_semaphore) {}
  movable_flag(const movable_flag& mf) : p_ss_(std::move(mf.p_ss_)) {}
  movable_flag(movable_flag&& mf) : p_ss_(std::move(mf.p_ss_)) {}
  ~movable_flag() {
    if (p_ss_) p_ss_->notify();
  }
  std::shared_ptr<state_semaphore> state() {
    return p_ss_;
  }
protected:
  mutable std::shared_ptr<state_semaphore> p_ss_;
};

/**
 * @brief Force create task queue with shared ptr
*/
std::shared_ptr<task_queue> task_queue::create(eq_wt related_eq, wg_wt related_wg) {
  return std::shared_ptr<task_queue>(new task_queue(related_eq, related_wg));
}

/**
 * @brief Initialize a task queue bind to event queue and worker group
*/
task_queue::task_queue(eq_wt related_eq, wg_wt related_wg)
  : impl_(new task_queue_impl)
{ 
  impl_->running = false;
  impl_->valid = true;
  impl_->related_eq = related_eq;
  impl_->related_wg = related_wg;
}

/**
 * @brief Block until all task done
*/
task_queue::~task_queue() {
  this->break_queue();
}

/**
 * @brief Cancel all task
*/
void task_queue::cancel() {
  std::lock_guard<std::mutex> _(impl_->lock);
  if (impl_->running) {
    while (impl_->tq.size() > 1) {
      impl_->tq.pop_back();
    }
  } else {
    impl_->tq.clear();
  }
}

/**
 * @brief Break the queue, no more tasks are accepted
*/
void task_queue::break_queue() {
  impl_->valid = false;
}

/**
 * @brief Post a async task
*/
void task_queue::post_task(task_location loc, task_t t) {
  if (!impl_->valid) return;
  task st;
  st.t = t;
  st.loc = loc;
  st.ptime = std::chrono::steady_clock::now();

  std::weak_ptr<task_queue_impl> w_tq_impl = this->impl_;
  st.after = [w_tq_impl](task* ptask) {
    auto impl = w_tq_impl.lock();
    if (!impl || impl->valid == false) {
      // already destroy or break the task_queue
      return;
    }
    std::lock_guard<std::mutex> _(impl->lock);
    impl->tq.pop_front();
    if (impl->tq.size() > 0) {
      if (auto seq = impl->related_eq.lock()) {
        // still running, no need to change
        seq->emplace_back(std::move(impl->tq.front()));
      } else {
        impl->running = false;
      }
    } else {
      impl->running = false;
    }
  };

  std::lock_guard<std::mutex> _(impl_->lock);
  impl_->tq.emplace_back(std::move(st));
  // the only one in the queue is current task
  if (impl_->tq.size() == 1 && impl_->running == false) {
    if (auto seq = impl_->related_eq.lock()) {
      impl_->running = true;
      seq->emplace_back(std::move(impl_->tq.front()));
    }
  }
}

/**
 * @brief Wait for current task to be done
*/
void task_queue::sync_task(task_location loc, task_t t) {
  if (!impl_->valid) return;
  if (auto wg = impl_->related_wg.lock()) {
    // we are the only thread in current worker group
    if (wg->size() == 1 && wg->in_worker_group()) {
      if (t) t();
    } else {
      movable_flag mf;
      auto ss = mf.state();
      this->post_task(loc, [t, mf]() {
        t();
      });
      ss->wait();
    }
  }
}

} // namespace libtq

// Push Chen
