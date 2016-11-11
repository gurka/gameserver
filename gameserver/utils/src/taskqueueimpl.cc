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

#include "taskqueueimpl.h"

TaskQueueImpl::TaskQueueImpl(boost::asio::io_service* io_service)
  : timer_(*io_service),
    timerStarted_(false)
{
}

void TaskQueueImpl::addTask(const Task& task)
{
  addTask(task, 0u);
}

void TaskQueueImpl::addTask(const Task& task, unsigned expire_ms)
{
  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) +
                boost::posix_time::millisec(expire_ms);

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

void TaskQueueImpl::startTimer()
{
  boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
  boost::posix_time::ptime taskExpire(queue_.top().expire);
  timer_.expires_from_now(taskExpire - now);
  timer_.async_wait(std::bind(&TaskQueueImpl::onTimeout, this, std::placeholders::_1));
  timerStarted_ = true;
}

void TaskQueueImpl::onTimeout(const boost::system::error_code& ec)
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

  boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
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
