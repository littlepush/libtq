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

namespace libtq {

struct task;

typedef std::function<void()>                     task_t;
typedef std::function<void(task*)>                task_hook_t;
typedef std::chrono::steady_clock                 task_clock_t;
typedef std::chrono::time_point<task_clock_t>     task_time_t;
typedef std::chrono::nanoseconds                  duration_t;

struct task_location {
  const char*   file;
  int           line;
};

#if (defined WIN32 | defined _WIN32 | defined WIN64 | defined _WIN64)
inline struct task_location __build_location(const char* f, int l) {
  struct task_location loc;
  loc.file = f;
  loc.line = l;
  return loc;
}
#define __TQ_TASK_LOC     (libtq::__build_location(__FILE__, __LINE__))
#else
#define __TQ_TASK_LOC     (libtq::task_location){__FILE__, __LINE__}
#endif

#define TQ_TASK_LOC       __TQ_TASK_LOC

struct task {
  task_t        t;
  task_hook_t   before;
  task_hook_t   after;
  task_location loc;
  task_time_t   ptime;
  task_time_t   btime;
};

} // namespace libtq

#endif

// Push Chen
