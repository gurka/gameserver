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
    timer_started_(false)
{
}

void TaskQueueImpl::addTask(const Task& task, unsigned tag)
{
  addTask(task, tag, 0u);
}

void TaskQueueImpl::addTask(const Task& task, unsigned tag, unsigned expire_ms)
{
  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) +
                boost::posix_time::millisec(expire_ms);

  // Locate a task with greater, or equal, expire than the given expire
  auto it = std::find_if(queue_.cbegin(), queue_.cend(), [expire](const TaskWrapper& tw)
  {
    return tw.expire >= expire;
  });

  // Add the new task before the task we found
  queue_.emplace(it, task, tag, expire);

  if (!timer_started_)
  {
    // If the timer isn't started, start it!
    startTimer();
  }
  else if (it == queue_.cbegin())
  {
    // If the timer is started but we added a task with lower expire than the
    // previously lowest expire, then cancel the timer and let it restart
    timer_.cancel();
  }
}

void TaskQueueImpl::cancelAllTasks(unsigned tag)
{
  if (!queue_.empty())
  {
    // If the first task in the queue has this tag we need to restart the timer after removing them
    bool restart_timer = false;
    if (queue_.front().tag == tag)
    {
      restart_timer = true;
    }

    // Erase all tasks with given tag
    auto pred = [tag](const TaskWrapper& tw) { return tw.tag == tag; };
    queue_.erase(std::remove_if(queue_.begin(), queue_.end(), pred), queue_.end());

    // Restart the timer if restart_timer is true AND if the queue is not empty
    if (restart_timer && !queue_.empty())
    {
      timer_.cancel();
    }
  }
}

void TaskQueueImpl::startTimer()
{
  // Start timer
  boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
  boost::posix_time::ptime taskExpire(queue_.front().expire);
  timer_.expires_from_now(taskExpire - now);
  timer_.async_wait(std::bind(&TaskQueueImpl::onTimeout, this, std::placeholders::_1));

  timer_started_ = true;
}

void TaskQueueImpl::onTimeout(const boost::system::error_code& ec)
{
  if (ec == boost::asio::error::operation_aborted)
  {
    // Canceled by addTask, so just restart the timer
    startTimer();
    return;
  }
  else if (ec)
  {
    // TODO(gurka): abort() isn't good.
    abort();
  }

  // Call all tasks that have expired
  boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
  while (!queue_.empty())
  {
    if (queue_.front().expire > now)
    {
      break;
    }

    // More tasks can be added to the queue when calling task()
    // So copy the task and remove it from the queue before calling task(), to avoid problems
    auto tw = queue_.front();
    queue_.erase(queue_.begin());
    tw.task();
  }

  // Start the timer again if there are more tasks in the queue
  if (!queue_.empty())
  {
    startTimer();
  }
  else
  {
    timer_started_ = false;
  }
}
