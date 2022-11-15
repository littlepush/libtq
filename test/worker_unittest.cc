/*
    worker_unittest.cc
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

#include "worker.h"
#include "gtest/gtest.h"

class worker_test : public testing::Test {
public:
  worker_test() : eq_(new libtq::eq_t), w_(eq_) {}
protected:
  libtq::eq_st eq_;
  libtq::worker w_;
};

TEST_F(worker_test, start_stop) {
  w_.start();
  EXPECT_TRUE(w_.is_running());
  w_.stop();
  EXPECT_FALSE(w_.is_running());
}

TEST_F(worker_test, async_thread) {
  w_.start();
  libtq::task st;
  auto c_tid = std::this_thread::get_id();
  int count = 0;
  st.before = [&count](libtq::task* ptask) {
    count += 1;
  };
  st.t = [c_tid, &count]() {
    EXPECT_NE(c_tid, std::this_thread::get_id());
    count += 1;
  };
  st.after = [&count](libtq::task* ptask) {
    count += 1;
  };
  eq_->emplace_back(std::move(st));
  while (eq_->pending_count() > 0) {
    std::this_thread::yield();
  }
  w_.stop();
  EXPECT_EQ(count, 3);
}

