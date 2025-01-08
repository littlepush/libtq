/*
    timer_unittest.cc
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

#include "task_timer.h"
#include "task_queue_manager.h"
#include "gtest/gtest.h"

#include <cmath>

class timer_test : public testing::Test {
public:
  // create a default task queue
  timer_test() : tq_(libtq::task_queue_manager::create_task_queue(libtq::thread_priority::k_normal)) {
    libtq::task_queue_manager::default_worker_group()->increase_worker(libtq::thread_priority::k_normal);
    tq_->set_recent_trace_keep_count(10);
  }
  ~timer_test() {
    libtq::task_queue_manager::default_worker_group()->decrease_worker(libtq::thread_priority::k_normal);
  }
protected:
  LIBTQ_DISABLE_COPY(timer_test)
  LIBTQ_DISABLE_MOVE(timer_test)
protected:
  libtq::tq_st tq_;
};

TEST_F(timer_test, loop_rate) {
  libtq::timer t(tq_);
  libtq::event_queue<int> eq;
  int count = 0;
  auto begin = std::chrono::steady_clock::now();

  t.start(__TQ_TASK_LOC, [&eq]() {
    eq.emplace_back(1, 2);
  }, 10);

  while (eq.wait(2)) {
    ++count;
    if (count == 10) {
      t.stop();
      break;
    }
  }
  auto end = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  EXPECT_GE(d, 10 * 10);
  EXPECT_LT(d, 10 * 11);
}

TEST_F(timer_test, fire_now) {
  static double avg_delta = 0.f;
  static std::list<double> all_delta;
  libtq::timer t(tq_);
  libtq::event_queue<libtq::task_time_t> eq;
  int count = 0;
  auto begin = std::chrono::steady_clock::now();
  t.start(__TQ_TASK_LOC, [&eq]() {
    // eq.emplace_back(1);
    eq.emplace_back(std::chrono::steady_clock::now());
  }, 10, true);

  std::list<double> delta_list;
  auto last_tick = begin;
  libtq::event_queue<libtq::task_time_t>::item_strong_t tick_item;
  while ((tick_item = eq.wait()) != nullptr) {
    auto delta = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tick_item->i - last_tick).count();
    delta_list.push_back(delta);
    last_tick = tick_item->i;
    ++count;
    if (count == 10) {
      t.stop();
      break;
    }
  }
  auto end = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  EXPECT_GE(d, 10 * 9 - 1);
  EXPECT_LT(d, 10 * 10);
  
  delta_list.pop_front();
  
  tq_->sync_task(TQ_TASK_LOC, [&]() {
    auto ti_qu = tq_->recent_trace_info();
    auto last = ti_qu.front();
    ti_qu.pop();
    while (ti_qu.size() > 0) {
      auto& recent = ti_qu.front();
      auto pdelta = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(recent.post_time - last.post_time).count();
      auto pickup_time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(recent.begin_time - recent.post_time).count();
      auto exec_time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(recent.end_time - recent.begin_time).count();
      auto delta = delta_list.front();
      delta_list.pop_front();
      avg_delta = ((avg_delta * (double)all_delta.size()) + delta) / (double)(all_delta.size() + (size_t)1);
      all_delta.push_back(delta);
      printf("post delta: %f; pickup used: %f, exec used: %f, delta: %f\n", pdelta, pickup_time, exec_time, delta);
      last = recent;
      ti_qu.pop();
    }
    double variance = 0.f;
    for (auto& d : all_delta) {
      variance += pow((d - avg_delta), 2.f);
    }
    printf("delta variance is: %f, data count: %u\n", (variance / (double)all_delta.size()), (unsigned int)all_delta.size());
  });
}

TEST_F(timer_test, fire_once_delay) {
  std::shared_ptr<libtq::event_queue<int, 2>> eq = std::make_shared<libtq::event_queue<int, 2>>();
  libtq::timer::once_after(tq_, __TQ_TASK_LOC, [eq]() {
    eq->emplace_back(1, 2);
  }, 10);
  int count = 0;
  while (eq->wait_for(std::chrono::milliseconds(20), 2)) {
    ++count;
    if (count > 1) {
      // t.stop();
      break;
    }
  }
  EXPECT_EQ(count, 1);
}

TEST_F(timer_test, fire_once_cancel) {
  libtq::event_queue<int> eq;
  auto job_id = libtq::timer::once_after(tq_, __TQ_TASK_LOC, [&eq]() {
    eq.emplace_back(1, 2);
  }, 10);
  libtq::timer::cancel_once(job_id);
  EXPECT_FALSE(eq.wait_for(std::chrono::milliseconds(20), 2));
}
