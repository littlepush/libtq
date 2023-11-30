/*
  rwlock.cc
  libtq
  2023-11-30
  Push Chen
*/

/*
MIT License

Copyright (c) 2023 Push Chen

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

#include "rwlock.h"

namespace libtq {

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
// Do nothing because we can use std::shared_mutex
#else
/**
  * @brief Write lock, block any new reader, and wait all current readers are done
*/
void rwlock::lock() {
  std::unique_lock<std::mutex> _(mutex_);
  ++write_cnt_;
  while (read_cnt_ != 0 || is_writing_) {
    write_cond_.wait(_);
  }
  is_writing_ = true;
}
/**
  * @brief Write unlock
*/
void rwlock::unlock() {
  std::unique_lock< std::mutex > _(mutex_);
  if (write_cnt_ == 0) return;
  is_writing_ = false;
  if ((--write_cnt_) == 0) {
    read_cond_.notify_all();
  } else {
    write_cond_.notify_one();
  }
}
/**
  * @brief Read Lock, shared with other readers
*/
void rwlock::lock_shared() {
  std::unique_lock< std::mutex > _(mutex_);
  while (write_cnt_ != 0) {
    read_cond_.wait(_);
  }
  ++read_cnt_;
}
/**
  * @brief Read unlock
*/
void rwlock::unlock_shared() {
  std::unique_lock< std::mutex > _(mutex_);
  if (read_cnt_ == 0) return;
  if ((--read_cnt_) == 0) {
    write_cond_.notify_one();
  };
}
/**
  * @brief Try to write lock, if has any other reader or writer, return false directly
*/
bool rwlock::try_lock() {
  std::unique_lock< std::mutex > _(mutex_);
  if (read_cnt_ != 0 || is_writing_) {
    return false;
  }
  ++write_cnt_;
  is_writing_ = true;
  return true;
}
/**
  * @brief Try to read lock, if has any writer, return false directly
*/
bool rwlock::try_lock_shared() {
  std::unique_lock< std::mutex > _(mutex_);
  if (write_cnt_ != 0) {
    return false;
  }
  ++read_cnt_;
  return true;
}

#endif

} // namespace libtq

// Push Chen
