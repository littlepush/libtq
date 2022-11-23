/*
    task_queue.h
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

#pragma once

#ifndef LIBTQ_TASK_QUEUE_H__
#define LIBTQ_TASK_QUEUE_H__

#include <list>
#include <memory>

#include "event_queue.h"
#include "worker_group.h"
#include "task.h"

#ifdef _MSC_VER
#include <intrin.h>
#define NOP() __nop()       // _emit 0x90
#else
// assume __GNUC__ inline asm
#define NOP() asm("nop")    // implicitly volatile
#endif

namespace libtq {

typedef std::shared_ptr<worker_group> wg_st;
typedef std::weak_ptr<worker_group>   wg_wt;

/**
 * @brief Inner data storage of a task queue
*/
struct task_queue_impl {
  std::mutex        lock;
  std::list<task>   tq;
  std::atomic_bool  valid;
  std::atomic_bool  running;
  eq_wt             related_eq;
  wg_wt             related_wg;
};

class task_queue : public std::enable_shared_from_this<task_queue> {
public:
  /**
   * @brief Force create task queue with shared ptr
  */
  static std::shared_ptr<task_queue> create(eq_wt related_eq, wg_wt related_wg);
  /**
   * @brief Block until all task done
  */
  ~task_queue();

  /**
   * @brief Cancel all task
  */
  void cancel();

  /**
   * @brief Break the queue, no more tasks are accepted
  */
  void break_queue();

  /**
   * @brief Post a async task
  */
  void post_task(task_location loc, task_t t);

  /**
   * @brief Wait for current task to be done
  */
  void sync_task(task_location loc, task_t t);

public:
  task_queue(const task_queue&) = delete;
  task_queue(task_queue&&) = delete;
  task_queue& operator = (const task_queue&) = delete;
  task_queue& operator = (task_queue&&) = delete;
protected:
  /**
   * @brief Initialize a task queue bind to event queue and worker group
  */
  task_queue(eq_wt related_eq, wg_wt related_wg);
  
private:
  std::shared_ptr<task_queue_impl>  impl_;
};

typedef task_queue  tq_t;
typedef std::shared_ptr<tq_t>   tq_st;
typedef std::weak_ptr<tq_t>     tq_wt;

} // namespace libtq

#endif

// Push Chen
