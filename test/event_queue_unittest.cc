/*
    event_queue_unittest.cc
    libtq
    2022-11-12
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

#include "gtest/gtest.h"
#include "event_queue.h"

class event_queue_test : public testing::Test {
protected:
  libtq::event_queue<std::string> test_eq_;
};

TEST_F(event_queue_test, infinitive_wait_base) {
  std::thread t([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    this->test_eq_.emplace_back("hellotq");
  });
  auto result = test_eq_.wait();
  EXPECT_TRUE(result);
  EXPECT_EQ(result->i, "hellotq");
  t.join();
}

TEST_F(event_queue_test, infinitive_wait_break) {
  std::thread::id tid = std::this_thread::get_id();
  std::thread t([this, tid]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    this->test_eq_.break_waiter(tid);
  });
  auto result = test_eq_.wait();
  EXPECT_FALSE(result);
  t.join();
}

TEST_F(event_queue_test, infinitive_wait_cancel_and_repub) {
  this->test_eq_.emplace_back("hellotq");
  this->test_eq_.cancel_all();
  this->test_eq_.emplace_back("hellotq2");
  auto result = test_eq_.wait();
  EXPECT_TRUE(result);
  EXPECT_EQ(result->i, "hellotq2");
}

TEST_F(event_queue_test, wait_for_base) {
  std::thread t([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    this->test_eq_.emplace_back("hellotq");
  });
  auto result = test_eq_.wait_for(std::chrono::seconds(1));
  EXPECT_TRUE(result);
  EXPECT_EQ(result->i, "hellotq");
  t.join();
}

TEST_F(event_queue_test, wait_for_timeout) {
  auto result = this->test_eq_.wait_for(std::chrono::milliseconds(30));
  EXPECT_FALSE(result);
}

TEST_F(event_queue_test, cancel_event) {
  auto i = this->test_eq_.emplace_back("hellotq");
  this->test_eq_.cancel(i);
  auto result = test_eq_.wait_for(std::chrono::milliseconds(10));
  EXPECT_FALSE(result);
}

TEST_F(event_queue_test, emplace_order) {
  this->test_eq_.emplace_back("1");
  this->test_eq_.emplace_back("2");
  this->test_eq_.emplace_front("3");
  this->test_eq_.emplace_front("4");
  std::vector< libtq::event_queue<std::string>::item_strong_t > data;
  for (int i = 0; i < 4; ++i) {
    auto result = this->test_eq_.wait();
    data.push_back(result);
  }
  std::vector<std::string> expect_data = {"4", "3", "1", "2"};
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_TRUE(data[i]);
    EXPECT_EQ(data[i]->i, expect_data[i]);
  }
}

TEST_F(event_queue_test, destroy) {
  libtq::event_queue<std::string>* eq = new libtq::event_queue<std::string>;
  std::thread t([eq]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    delete eq;
  });
  auto result = eq->wait();
  EXPECT_FALSE(result);
  if (t.joinable()) t.join();
}