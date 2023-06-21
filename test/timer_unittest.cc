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

#include "timer.h"
#include "task_queue_manager.h"
#include "gtest/gtest.h"

class timer_test : public testing::Test {
public:
  // create a default task queue
  timer_test() : tq_(libtq::task_queue_manager::create_task_queue()) {}
protected:
  libtq::tq_st tq_;
};

TEST_F(timer_test, loop_rate) {
  libtq::timer t(tq_);
  libtq::event_queue<int> eq;
  int count = 0;
  auto begin = std::chrono::steady_clock::now();

  t.start(__TQ_TASK_LOC, [&eq]() {
    eq.emplace_back(1);
  }, 10);

  while (eq.wait()) {
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
  libtq::timer t(tq_);
  libtq::event_queue<int> eq;
  int count = 0;
  auto begin = std::chrono::steady_clock::now();
  t.start(__TQ_TASK_LOC, [&eq]() {
    eq.emplace_back(1);
  }, 10, true);

  while (eq.wait()) {
    ++count;
    if (count == 10) {
      t.stop();
      break;
    }
  }
  auto end = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  EXPECT_GE(d, 10 * 9);
  EXPECT_LT(d, 10 * 10);
}

TEST_F(timer_test, fire_once_delay) {
  libtq::timer t(tq_);
  libtq::event_queue<int> eq;
  t.start_once_after(__TQ_TASK_LOC, [&eq]() {
    eq.emplace_back(1);
  }, 10);
  int count = 0;
  while (eq.wait_for(std::chrono::milliseconds(20))) {
    ++count;
    if (count > 1) {
      t.stop();
      break;
    }
  }
  EXPECT_EQ(count, 1);
}
