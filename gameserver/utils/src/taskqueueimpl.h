/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Simon Sandström
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef UTIL_TASKQUEUEIMPL_H_
#define UTIL_TASKQUEUEIMPL_H_

#include <functional>
#include <deque>
#include <queue>

#include <boost/asio.hpp>  //NOLINT
#include <boost/date_time/posix_time/posix_time.hpp>  //NOLINT

#include "taskqueue.h"

class TaskQueueImpl : public TaskQueue
{
 public:
  explicit TaskQueueImpl(boost::asio::io_service* io_service);

  // Delete copy constructors
  TaskQueueImpl(const TaskQueueImpl&) = delete;
  TaskQueueImpl& operator=(const TaskQueueImpl&) = delete;

  void addTask(const Task& task) override;
  void addTask(const Task& task, unsigned expire_ms) override;

 private:
  struct TaskWrapper
  {
    Task task;
    boost::posix_time::ptime expire;

    bool operator>(const TaskWrapper& other) const
    {
      return this->expire > other.expire;
    }
  };

  void startTimer();
  void onTimeout(const boost::system::error_code& ec);

  // We use std::greater to get reverse priority queue (Task with lowest time first)
  std::priority_queue<TaskWrapper, std::deque<TaskWrapper>, std::greater<TaskWrapper>> queue_;

  boost::asio::deadline_timer timer_;
  bool timerStarted_;
};

#endif  // UTIL_TASKQUEUEIMPL_H_
