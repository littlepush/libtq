/*
    task.h
    libtq
    2022-09-28
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

#ifndef LIBTQ_TASK_H__
#define LIBTQ_TASK_H__

#include <functional>
#include <chrono>

#if defined(_WIN32)
#pragma warning(disable: 4820)
#pragma warning(disable: 5045)
#endif

namespace libtq {

struct task;

typedef std::function<void()>                     task_t;
typedef std::function<void(task*)>                task_hook_t;
typedef std::chrono::steady_clock                 task_clock_t;
typedef std::chrono::time_point<task_clock_t>     task_time_t;
typedef std::chrono::nanoseconds                  duration_t;

struct alignas(intptr_t) task_location {
  const char*   file;
  intptr_t      line;   // use intptr_t to align the memory of pointer
};

// inline struct task_location __build_location(const char* f, int l) {
//   return {f, l};
// }
// #define __TQ_TASK_LOC     (libtq::__build_location(__FILE__, __LINE__))
#define __TQ_TASK_LOC     libtq::task_location{__FILE__, __LINE__}

#define TQ_TASK_LOC       __TQ_TASK_LOC

struct alignas(intptr_t) task_trace_item {
  task_location   loc;
  task_time_t     post_time;
  task_time_t     begin_time;
  task_time_t     end_time;
};

struct alignas(intptr_t) task : public task_trace_item {
  task_t        t;
  task_hook_t   before;
  task_hook_t   after;
};

#define LIBTQ_DISABLE_COPY(clz)   \
  clz(const clz&) = delete;       \
  clz& operator = (const clz&) = delete;
#define LIBTQ_DISABLE_MOVE(clz)   \
  clz(clz&&) = delete;            \
  clz& operator = (clz&&) = delete;

} // namespace libtq

#endif

// Push Chen
