/*
    worker_group.h
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

#ifndef LIBTQ_WORKER_GROUP_H__
#define LIBTQ_WORKER_GROUP_H__

#include <vector>
#include "worker.h"

namespace libtq {

typedef std::shared_ptr<worker> worker_st;

class worker_group {
public:
  /**
   * @brief Create a worker group with default 2 workers
  */
  worker_group(eq_wt q, unsigned int worker_count = 2);

  /**
   * @brief Destroy the group
  */
  ~worker_group();

  /**
   * @brief Get current worker group's worker count
  */
  size_t size() const;

  /**
   * @brief Check if current thread is in the worker group
  */
  bool in_worker_grouop() const;

  /**
   * @brief increase a worker
  */
  void increase_worker();

  /**
   * @brief remove a worker if still has
  */
  void decrease_worker();

protected:
  /**
   * @brief Worker storage
  */
  std::vector<worker_st>    workers_;

  /**
   * @brief Related event queue for all worker
  */
  eq_wt related_eq_;

  /**
   * @brief Lock for workers
  */
  mutable std::mutex worker_lock_;
};

} // namespace libtq

#endif

// Push Chen
