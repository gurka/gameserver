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

#ifndef GAMEENGINE_EXPORT_GAME_ENGINE_QUEUE_H_
#define GAMEENGINE_EXPORT_GAME_ENGINE_QUEUE_H_

#include <functional>
#include <vector>

#include <boost/asio.hpp>  //NOLINT
#include <boost/date_time/posix_time/posix_time.hpp>  //NOLINT

class GameEngine;

class GameEngineQueue
{
 public:
  using Task = std::function<void(GameEngine*)>;

  GameEngineQueue(GameEngine* gameEngine, boost::asio::io_service* io_service);

  // Delete copy constructors
  GameEngineQueue(const GameEngineQueue&) = delete;
  GameEngineQueue& operator=(const GameEngineQueue&) = delete;

  void addTask(int tag, const Task& task);
  void addTask(int tag, std::int64_t expire_ms, const Task& task);
  void cancelAllTasks(int tag);

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

  GameEngine* gameEngine_;

  // The vector should be sorted on TaskWrapper.expire
  // This is handled by addTask()
  std::vector<TaskWrapper> queue_;

  boost::asio::deadline_timer timer_;
  bool timer_started_;
};

#endif  // GAMEENGINE_EXPORT_GAME_ENGINE_QUEUE_H_
