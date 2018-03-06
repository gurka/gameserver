/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Simon Sandström
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
#include <vector>

#include <boost/asio.hpp>  //NOLINT
#include <boost/date_time/posix_time/posix_time.hpp>  //NOLINT

#include "taskqueue.h"

// TODO(gurka): If we ever want to run multiple threads for network I/O this queue needs to be threadsafe
class TaskQueueImpl : public TaskQueue
{
 public:
  explicit TaskQueueImpl(boost::asio::io_service* io_service);

  // Delete copy constructors
  TaskQueueImpl(const TaskQueueImpl&) = delete;
  TaskQueueImpl& operator=(const TaskQueueImpl&) = delete;

  void addTask(int tag, const Task& task) override;
  void addTask(int tag, unsigned expire_ms, const Task& task) override;
  void cancelAllTasks(int tag) override;

 private:
  struct TaskWrapper
  {
    TaskWrapper(const Task& task, int tag, const boost::posix_time::ptime& expire)
      : task(task),
        tag(tag),
        expire(expire)
    {
    }

    Task task;
    int tag;
    boost::posix_time::ptime expire;
  };

  void startTimer();
  void onTimeout(const boost::system::error_code& ec);

  // The vector should be sorted on TaskWrapper.expire
  // This is handled by addTask()
  std::vector<TaskWrapper> queue_;

  boost::asio::deadline_timer timer_;
  bool timer_started_;
};

#endif  // UTIL_TASKQUEUEIMPL_H_
