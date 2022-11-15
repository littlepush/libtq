/*
    task_queue_manager.cc
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

#include "task_queue_manager.h"
#include "worker_group.h"

namespace libtq {

eq_st global_event_queue() {
  static eq_st g_eq(new eq_t);
  return g_eq;
}

wg_st global_worker_group() {
  static wg_st g_wg(new worker_group(global_event_queue()));
  return g_wg;
}

/**
 * @brief Change default worker group's worker count to given value
*/
void task_queue_manager::adjust_default_worker_count(int wc) {
  auto wg = global_worker_group();
  while (wg->size() > wc) {
    wg->decrease_worker();
  }
  while (wg->size() < wc) {
    wg->increase_worker();
  }
}

/**
 * @brief Create a task queue and bind to default worker group
*/
task_queue_manager::tq_st task_queue_manager::create_task_queue() {
  return task_queue::create(global_event_queue(), global_worker_group());
}

/**
 * @brief Create a task queue with specified event queue and worker group
*/
task_queue_manager::tq_st task_queue_manager::create_task_queue(eq_st related_eq, wg_st related_wg) {
  return task_queue::create(related_eq, related_wg);
}

} // namespace libtq

// Push Chen
