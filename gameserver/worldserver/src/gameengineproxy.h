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

#ifndef WORLDSERVER_GAMEENGINEPROXY_H_
#define WORLDSERVER_GAMEENGINEPROXY_H_

#include <string>
#include <utility>

#include "gameengine.h"

class TaskQueue;
class World;

class GameEngineProxy
{
 public:
  GameEngineProxy(TaskQueue* taskQueue, const std::string& loginMessage, World* world)
    : gameEngine_(taskQueue, loginMessage, world)
  {
  }

  // Delete copy constructors
  GameEngineProxy(const GameEngineProxy&) = delete;
  GameEngineProxy& operator=(const GameEngineProxy&) = delete;

  template<class F, class... Args>
  void addTask(CreatureId playerId, F&& f, Args&&... args)
  {
    // TODO(gurka): Fix this...
    gameEngine_.addTask(playerId, f, args...);
  }

 private:
  GameEngine gameEngine_;
};

#endif  // WORLDSERVER_GAMEENGINEPROXY_H_
