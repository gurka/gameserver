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

#ifndef WORLDSERVER_TASKQUEUE_H_
#define WORLDSERVER_TASKQUEUE_H_

#include <functional>
#include <deque>
#include <queue>

#include <boost/asio.hpp>  //NOLINT
#include <boost/date_time/posix_time/posix_time.hpp>  //NOLINT

template <class Task>
class TaskQueue
{
 public:
  explicit TaskQueue(boost::asio::io_service* io_service)
    : timer_(*io_service),
      timerStarted_(false)
  {
  }

  // Delete copy constructors
  TaskQueue(const TaskQueue&) = delete;
  TaskQueue& operator=(const TaskQueue&) = delete;

  void addTask(const Task& task)
  {
    addTask(task, boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time()));
  }

  void addTask(const Task& task, const boost::posix_time::ptime& expire)
  {
    TaskWrapper taskWrapper { task, expire };
    queue_.push(taskWrapper);
    if (timerStarted_)
    {
      // onTimeout will handle restart of timer
      timer_.cancel();
    }
    else
    {
      startTimer();
    }
  }

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

  void startTimer()
  {
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    boost::posix_time::ptime taskExpire(queue_.top().expire);
    timer_.expires_from_now(taskExpire - now);
    timer_.async_wait(std::bind(&TaskQueue::onTimeout, this, std::placeholders::_1));
    timerStarted_ = true;
  }

  void onTimeout(const boost::system::error_code& ec)
  {
    if (ec == boost::asio::error::operation_aborted)
    {
      // Canceled by addTask
      startTimer();
      return;
    }
    else if (ec)
    {
      abort();
    }

    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    while (!queue_.empty())
    {
      auto taskWrapper = queue_.top();
      if (taskWrapper.expire > now)
      {
        break;
      }
      queue_.pop();
      taskWrapper.task();
    }

    if (!queue_.empty())
    {
      startTimer();
    }
    else
    {
      timerStarted_ = false;
    }
  }

  // We use std::greater to get reverse priority queue (Task with lowest time first)
  std::priority_queue<TaskWrapper, std::deque<TaskWrapper>, std::greater<TaskWrapper>> queue_;

  boost::asio::deadline_timer timer_;
  bool timerStarted_;
};

#endif  // WORLDSERVER_TASKQUEUE_H_
