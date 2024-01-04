/*
    timer.h
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

#ifndef LIBTQ_TIMER_H__
#define LIBTQ_TIMER_H__

#include "task_queue.h"

namespace libtq {

class timer {
public: 
  timer(tq_wt related_tq);
  ~timer();

  timer(const timer&) = delete;
  timer(timer&&) = delete;
  timer& operator =(const timer&) = delete;
  timer& operator =(timer&&) = delete;

  /**
   * @brief Start the timer
  */
  void start(task_location loc, task_t job, unsigned int ms, bool fire_now = false);

  /**
   * @brief Start a job after some time
  */
  void start_once_after(task_location loc, task_t job, unsigned int ms, 
    std::function<bool()> pred = nullptr);

  /**
   * @brief Stop the timer
  */
  void stop();

protected:
  /**
   * @brief Timer status
  */
  std::shared_ptr<bool> status_;
  /**
   * @brief Related task queue
  */
  tq_wt related_tq_;
};

} // namespace libtq

#endif

// Push Chen
