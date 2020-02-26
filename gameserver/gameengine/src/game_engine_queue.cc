/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Simon Sandström
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

#include "game_engine_queue.h"

namespace gameengine
{

GameEngineQueue::GameEngineQueue(GameEngine* game_engine, asio::io_context* io_context)
  : m_game_engine(game_engine),
    m_timer(*io_context),
    m_timer_started(false)
{
}

void GameEngineQueue::addTask(int tag, const Task& task)
{
  addTask(tag, 0, task);
}

void GameEngineQueue::addTask(int tag, std::int64_t expire_ms, const Task& task)
{
  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) +
                boost::posix_time::millisec(expire_ms);

  // Locate a task with greater, or equal, expire than the given expire
  auto it = std::find_if(m_queue.cbegin(), m_queue.cend(), [expire](const TaskWrapper& tw)
  {
    return tw.expire >= expire;
  });

  // Add the new task before the task we found
  m_queue.emplace(it, task, tag, expire);

  if (!m_timer_started)
  {
    // If the timer isn't started, start it!
    startTimer();
  }
  else if (it == m_queue.cbegin())
  {
    // If the timer is started but we added a task with lower expire than the
    // previously lowest expire, then cancel the timer and let it restart
    m_timer.cancel();
  }
}

void GameEngineQueue::cancelAllTasks(int tag)
{
  if (!m_queue.empty())
  {
    // If the first task in the queue has this tag we need to restart the timer after removing them
    bool restart_timer = false;
    if (m_queue.front().tag == tag)
    {
      restart_timer = true;
    }

    // Erase all tasks with given tag
    auto pred = [tag](const TaskWrapper& tw) { return tw.tag == tag; };
    m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(), pred), m_queue.end());

    // Restart the timer if restart_timer is true AND if the queue is not empty
    if (restart_timer && !m_queue.empty())
    {
      m_timer.cancel();
    }
  }
}

void GameEngineQueue::startTimer()
{
  // Start timer
  boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
  boost::posix_time::ptime task_expire(m_queue.front().expire);
  m_timer.expires_from_now(task_expire - now);

  m_timer.async_wait([this](const std::error_code& ec)
  {
    onTimeout(ec);
  });

  m_timer_started = true;
}

void GameEngineQueue::onTimeout(const std::error_code& ec)
{
  if (ec == asio::error::operation_aborted)
  {
    // Canceled by addTask, so just restart the timer
    startTimer();
    return;
  }

  if (ec)
  {
    // TODO(simon): abort() isn't good.
    abort();
  }

  // Call all tasks that have expired
  boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
  while (!m_queue.empty())
  {
    if (m_queue.front().expire > now)
    {
      break;
    }

    // More tasks can be added to the queue when calling task()
    // So copy the task and remove it from the queue before calling task(), to avoid problems
    auto tw = m_queue.front();
    m_queue.erase(m_queue.begin());
    tw.task(m_game_engine);
  }

  // Start the timer again if there are more tasks in the queue
  if (!m_queue.empty())
  {
    startTimer();
  }
  else
  {
    m_timer_started = false;
  }
}

}  // namespace gameengine
