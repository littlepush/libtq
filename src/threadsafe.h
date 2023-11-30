/*
  threadsafe.h
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

#pragma once

#ifndef LIBTQ_THREADSAFE_H__
#define LIBTQ_THREADSAFE_H__

#include <memory>
#include <thread>
#include "rwlock.h"
#include "task_queue.h"

namespace libtq {

typedef std::shared_ptr<rwlock>             raw_lock_t;
typedef std::shared_ptr<bool>               raw_flag_t;

template <typename T>
class rawptr {
public:
  rawptr(raw_lock_t lock, raw_flag_t state, T* ptr)
    : __raw_lock_(lock)
  {
    if (__raw_lock_) {
      __raw_lock_->lock_shared();
    }
    if (state) {
      __raw_this_ = (*state ? ptr : nullptr);
    }
  }
  ~rawptr() {
    if (__raw_lock_) {
      __raw_lock_->unlock_shared();
    }
  }
  rawptr(const rawptr&) = delete;
  rawptr(rawptr&& r) : 
    __raw_lock_(std::move(r.__raw_lock_)), __raw_this_(r.__raw_this_)
  {
    r.__raw_this_ = nullptr;
  }
  rawptr& operator =(const rawptr&) = delete;
  rawptr& operator =(rawptr&&) = delete;

  T* get() { return __raw_this_; }
  operator bool() const { return __raw_this_ != nullptr; }

  T* operator ->() { return __raw_this_; }
  T& operator *() { return *__raw_this_; }
private:
  raw_lock_t  __raw_lock_;
  T*          __raw_this_ = nullptr;
};

template <typename T>
class weak_protector {
public:
  weak_protector() = default;
  weak_protector(const weak_protector& r) :
    __raw_lock_(r.__raw_lock_),
    __raw_state_(r.__raw_state_),
    __raw_this_(r.__raw_this_)
  {}
  weak_protector(weak_protector&& r) : 
    __raw_lock_(std::move(r.__raw_lock_)),
    __raw_state_(std::move(r.__raw_state_)),
    __raw_this_(std::move(r.__raw_this_))
  {}
  weak_protector(
    raw_lock_t lock,
    raw_flag_t state,
    T* ptr
  ) : __raw_lock_(lock), __raw_state_(state), __raw_this_(ptr)
  {}
  weak_protector& operator = (const weak_protector& wp) {
    if (this == &wp) return *this;
    __raw_lock_ = wp.__raw_lock_;
    __raw_state_ = wp.__raw_state_;
    __raw_this_ = wp.__raw_this_;
    return *this;
  }
  weak_protector& operator = (weak_protector&& wp) {
    if (this == &wp) return *this;
    __raw_lock_ = std::move(wp.__raw_lock_);
    __raw_state_ = std::move(wp.__raw_state_);
    __raw_this_ = std::move(wp.__raw_this_);
    return *this;
  }
  rawptr<T> lock() const {
    return rawptr<T>(__raw_lock_, __raw_state_, __raw_this_);
  }

private:
  raw_lock_t   __raw_lock_;
  raw_flag_t   __raw_state_;
  T*           __raw_this_;
};

template <typename T>
class rawptr_protector {
public:
  rawptr_protector() : 
    __raw_lock_(new rwlock),
    __raw_state_(new bool(true)),
    __raw_this_(static_cast<T*>(this))
  { }
  ~rawptr_protector() {
    this->destroy_protection();
  }
  void destroy_protection() {
    std::lock_guard<rwlock> _(*__raw_lock_);
    *__raw_state_ = false;
    __raw_this_ = nullptr;
  }
  weak_protector<T> protector() {
    return weak_protector<T>(__raw_lock_, __raw_state_, __raw_this_);
  }
private:
  raw_lock_t   __raw_lock_;
  raw_flag_t   __raw_state_;
  T*           __raw_this_;
};

template <typename T>
std::function<void()> weak_protector_wrapper(
    weak_protector<T> wp,
    std::function<void()> job,
    std::function<void()> lock_failed = nullptr
) {
  if (!job) return nullptr;
  return [wp, job, lock_failed]() {
    if (auto sp = wp.lock()) {
      job();
    } else {
      if (lock_failed) {
        lock_failed();
      }
    }
  };
}

template <typename T>
std::function<void()> protector_wrapper(
    rawptr_protector<T>* rp, 
    std::function<void()> job, 
    std::function<void()> lock_failed = nullptr
) {
  if (rp == nullptr || !job) return nullptr;
  auto w_rp = rp->protector();
  return weak_protector_wrapper(w_rp, job, lock_failed);
}

template<typename T>
class task_helper {
public:
  task_helper(libtq::tq_wt q, rawptr_protector<T>* p, libtq::task_location l) : 
    wq_(q), wptr_(p->protector()), loc_(l)
  {}
  task_helper(libtq::tq_wt q, weak_protector<T> wp, libtq::task_location l) : 
    wq_(q), wptr_(wp), loc_(l)
  {}
  task_helper(task_helper&& rth) : wq_(rth.wq_), wptr_(rth.wptr_), loc_(rth.loc_) {}

  task_helper& operator << (libtq::task_t task) {
    if (auto sq = wq_.lock()) {
      sq->post_task(loc_, weak_protector_wrapper(wptr_, task));
    }
    return *this;
  }

  task_helper& operator () (libtq::task_t task, std::function<void()> lock_failed) {
    if (auto sq = wq_.lock()) {
      sq->post_task(loc_, weak_protector_wrapper(wptr_, task, lock_failed));
    } else {
      if (lock_failed) {
        lock_failed();
      }
    }
    return *this;
  }

  task_helper& operator <<= (libtq::task_t task) {
    if (auto sq = wq_.lock()) {
      sq->post_task(loc_, task);
    }
    return *this;
  }

  task_helper& operator >> (libtq::task_t task) {
    if (auto sq = wq_.lock()) {
      sq->sync_task(loc_, weak_protector_wrapper(wptr_, task));
    }
    return *this;
  }
protected:
  libtq::tq_wt          wq_;
  weak_protector<T>     wptr_;
  libtq::task_location  loc_;
};
template <typename T>
inline task_helper<T> __build_task_helper__(
    libtq::tq_wt q, rawptr_protector<T>* p, libtq::task_location l)
{
  return task_helper<T>(q, p, l);
}
template <typename T>
inline task_helper<T> __build_task_helper__(
    libtq::tq_wt q, weak_protector<T> wp, libtq::task_location l)
{
  return task_helper<T>(q, wp, l);
}

#define POST_TASK_TO_QUEUE(q, rp)   \
  yqt::__build_task_helper__(q, rp, __TQ_TASK_LOC)

#define POST_TASK(q)  POST_TASK_TO_QUEUE(q, this)

} // namespace libtq

#endif

// Push Chen
