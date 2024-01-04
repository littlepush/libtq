/*
    worker_group_unittest.cc
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

#include "worker_group.h"
#include "gtest/gtest.h"

class worker_group_test : public testing::Test {
public:
  worker_group_test() : eq_(new libtq::eq_t), wg_(eq_) {}
protected:
  libtq::eq_st eq_;
  libtq::worker_group wg_;
};

TEST_F(worker_group_test, validate_count) {
  EXPECT_EQ(wg_.size(), 2);
  // Wait for the worker group to initialized
  while (eq_->waiter_count() != 2) {
    std::this_thread::yield();
  }
  EXPECT_EQ(eq_->waiter_count(), 2);
  std::vector<std::thread::id> worker_tids;
  std::mutex tids_lock;
  libtq::event_queue<int> iq;
  for (int i = 0; i < 2; ++i) {
    libtq::task st;
    st.t = [&worker_tids, &tids_lock]() {
      {
        std::lock_guard<std::mutex> _(tids_lock);
        worker_tids.push_back(std::this_thread::get_id());
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };
    st.after = [&iq](libtq::task *ptask) {
      iq.emplace_back(1);
    };
    eq_->emplace_back(std::move(st));
  }
  for (int i = 0; i < 2; ++i) {
    auto r = iq.wait_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(r);
    EXPECT_EQ(r->i, 1);
  }
  auto this_tid = std::this_thread::get_id();
  EXPECT_EQ(worker_tids.size(), 2);
  EXPECT_NE(worker_tids[0], worker_tids[1]);
  EXPECT_NE(worker_tids[0], this_tid);
  EXPECT_NE(worker_tids[1], this_tid);
}

TEST_F(worker_group_test, increase) {
  EXPECT_EQ(wg_.size(), 2);
  while (eq_->waiter_count() != 2) {
    std::this_thread::yield();
  }
  EXPECT_EQ(eq_->waiter_count(), 2);
  wg_.increase_worker();
  EXPECT_EQ(wg_.size(), 3);
  while (eq_->waiter_count() != 3) {
    std::this_thread::yield();
  }
  EXPECT_EQ(eq_->waiter_count(), 3);
}

TEST_F(worker_group_test, decrease) {
  EXPECT_EQ(wg_.size(), 2);
  // Wait for the worker group to initialized
  while (eq_->waiter_count() != 2) {
    std::this_thread::yield();
  }
  wg_.decrease_worker();
  EXPECT_EQ(wg_.size(), 1);
  EXPECT_EQ(eq_->waiter_count(), 1);
  wg_.decrease_worker();
  EXPECT_EQ(wg_.size(), 0);
  EXPECT_EQ(eq_->waiter_count(), 0);
  wg_.decrease_worker();
  EXPECT_EQ(wg_.size(), 0);
  EXPECT_EQ(eq_->waiter_count(), 0);
}

TEST_F(worker_group_test, in_worker_group) {
  EXPECT_FALSE(wg_.in_worker_group());
  libtq::task t;
  std::atomic<int> count(0);
  t.t = [this, &count]() {
    EXPECT_TRUE(this->wg_.in_worker_group());
    ++count;
  };
  eq_->emplace_back(std::move(t));
  while (count == 0) {
    std::this_thread::yield();
  }
}
