/*
    worker.h
    libtq
    2022-09-27
    Push Chen
*/

/*
MIT License

Copyright (c) 2020 Push Chen

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

#pragma once

#ifndef LIBTQ_WORKER_H__
#define LIBTQ_WORKER_H__

#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include "task.h"
#include "task_event_queue.h"
#include "task_thread.h"

namespace libtq {

typedef event_queue<task> eq_t;
typedef std::weak_ptr<eq_t> eq_wt;
typedef std::shared_ptr<eq_t> eq_st;

class worker : public thread {
public:
  /**
   * @brief Init a worker with the task queue.
   * @remarks throw runtime error when the queue is not validate
  */
  worker(eq_wt q, thread_attribute attr);

  /**
   * @brief Stop and quit worker
  */
  virtual ~worker();

public:
  worker(const worker&) = delete;
  worker(worker&&) = delete;
  worker& operator = (const worker&) = delete;
  worker& operator = (worker&&) = delete;

public:
  /**
   * @brief Stop current worker, will wait until current running task to be stopped
  */
  void stop();

protected:
  /**
   * @brief inner thread main function
  */
  virtual void main();

private:
  /**
   * @brief running status lock
  */
  mutable std::mutex running_lock_;

  /**
   * @brief Releated event queue
  */
  eq_wt related_eq_;
  /**
   * @brief Last change priority time
  */
  task_time_t adjust_prio_time_;
};

} // namespace libtq

#endif

// Push Chen
