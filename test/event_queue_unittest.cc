/*
    event_queue_unittest.cc
    libtq
    2022-11-12
    Push Chen
*/

#include "gtest/gtest.h"
#include "event_queue.h"

class event_queue_test : public testing::Test {
public:
  virtual void SetUp() {
    std::cout << "SetUp" << std::endl;
  }

  virtual void TearDown() {
    std::cout << "TearDown" << std::endl;
  }

protected:
  libtq::event_queue<std::string> test_eq_;
};

TEST_F(event_queue_test, infinitive_wait_1) {
  std::thread t([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    this->test_eq_.emplace_back("hellotq");
  });
  auto result = test_eq_.wait();
  EXPECT_TRUE(result);
  EXPECT_EQ(result->i, "hellotq");
  t.join();
}