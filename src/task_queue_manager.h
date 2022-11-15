/*
    task_queue_manager.h
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

#pragma once

#ifndef LIBTQ_TASK_QUEUE_MANAGER_H__
#define LIBTQ_TASK_QUEUE_MANAGER_H__

#include "task_queue.h"

namespace libtq {

class task_queue_manager {
public: 
  typedef std::shared_ptr<task_queue>   tq_st;

public:
  /**
   * @brief Change default worker group's worker count to given value
  */
  static void adjust_default_worker_count(int wc);

  /**
   * @brief Create a task queue and bind to default worker group
  */
  static tq_st create_task_queue();

  /**
   * @brief Create a task queue with specified event queue and worker group
  */
  static tq_st create_task_queue(eq_st related_eq, worker_group_st related_wg);
};

} // namespace libtq

#endif

// Push Chen
