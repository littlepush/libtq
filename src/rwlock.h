/*
  rwlock.h
  rwlock.h
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

#pragma once

#ifndef LIBTQ_RWLOCK_H__
#define LIBTQ_RWLOCK_H__

// RW lock is available in C++17
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#include <shared_mutex>
#else
#include <mutex>
#include <condition_variable>
#include <atomic>
#endif

namespace libtq {
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
typedef std::shared_mutex rwlock;
#else

/**
 * @brief A simple read-write lock
*/
class rwlock {
public:
  rwlock() = default;
  ~rwlock() = default;

  /**
   * @brief Write lock, block any new reader, and wait all current readers are done
  */
  void lock();
  /**
   * @brief Write unlock
  */
  void unlock();
  /**
   * @brief Read Lock, shared with other readers
  */
  void lock_shared();
  /**
   * @brief Read unlock
  */
  void unlock_shared();
  /**
   * @brief Try to write lock, if has any other reader or writer, return false directly
  */
  bool try_lock();
  /**
   * @brief Try to read lock, if has any writer, return false directly
  */
  bool try_lock_shared();

protected:
  rwlock(const rwlock&) = delete;
  rwlock(rwlock&&) = delete;
  rwlock& operator = (const rwlock&) = delete;
  rwlock& operator = (rwlock&&) = delete;

protected:
  std::mutex mutex_;
  std::condition_variable read_cond_;
  std::condition_variable write_cond_;
  std::atomic<unsigned int> read_cnt_{0};
  std::atomic<unsigned int> write_cnt_{0};
  std::atomic<bool> is_writing_{false};
};

#endif
} // namespace libtq

#endif

// Push Chen
