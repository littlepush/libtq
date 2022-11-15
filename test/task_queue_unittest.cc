/*
    task_queue_unittest.cc
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

#include "task_queue.h"
#include "gtest/gtest.h"

class task_queue_test : public testing::Test {
public:
  task_queue_test() : 
    eq_(new libtq::eq_t),
    wg_(new libtq::worker_group(eq_)),
    tq_(libtq::task_queue::create(eq_, wg_))
  {}
protected:
  libtq::eq_st eq_;
  libtq::wg_st wg_;
  libtq::tq_st tq_;
};

TEST_F(task_queue_test, test_post_tasks) {
  std::vector<int> result;
  for (int i = 0; i < 10; ++i) {
    tq_->post_task(__TQ_TASK_LOC, [&result, i]() {
      result.push_back(i);
    });
  }
  while(result.size() != 10) {
    std::this_thread::yield();
  }
  EXPECT_EQ(result.size(), 10);
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(result[i], i);
  }
}

TEST_F(task_queue_test, test_sync_task) {
  std::vector<int> result;
  std::mutex rlock;
  for (int i = 0; i < 5; ++i) {
    tq_->post_task(__TQ_TASK_LOC, [&result, &rlock, i]() {
      {
        std::lock_guard<std::mutex> _(rlock);
        result.push_back(i);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
  }
  tq_->sync_task(__TQ_TASK_LOC, [&result, &rlock]() {
    std::lock_guard<std::mutex> _(rlock);
    result.push_back(5);
  });
  EXPECT_EQ(result.size(), 6);
  for (int i = 0; i < 6; ++i) {
    EXPECT_EQ(result[i], i);
  }
}

TEST_F(task_queue_test, cancel_task) {
  std::vector<int> result;
  std::mutex rlock;
  for (int i = 0; i < 5; ++i) {
    tq_->post_task(__TQ_TASK_LOC, [&result, &rlock, i]() {
      {
        std::lock_guard<std::mutex> _(rlock);
        result.push_back(i);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
  }
  std::this_thread::yield();
  tq_->cancel();
  tq_->sync_task(__TQ_TASK_LOC, [&result, &rlock]() {
    std::lock_guard<std::mutex> _(rlock);
    result.push_back(5);
  });
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0);
  EXPECT_EQ(result[1], 5);
}

TEST_F(task_queue_test, break_queue) {
  std::vector<int> result;
  std::mutex rlock;
  for (int i = 0; i < 5; ++i) {
    tq_->post_task(__TQ_TASK_LOC, [&result, &rlock, i]() {
      std::lock_guard<std::mutex> _(rlock);
      result.push_back(i);
    });
    if (i == 2) {
      tq_->break_queue();
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(result.size(), 3);
}
