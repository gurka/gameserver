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

#ifndef WORLDSERVER_CONTAINERMANAGER_H_
#define WORLDSERVER_CONTAINERMANAGER_H_

#include <vector>

#include "item.h"
#include "logger.h"

class ContainerManager
{
 public:
  const std::vector<Item>& getContainerContents(int containerId)
  {
    for (const auto& container : containers_)
    {
      if (container.id == containerId)
      {
        return container.items;
      }
    }

    // Create a new empty container
    LOG_DEBUG("%s: creating new container with id: %d", __func__, containerId);
    containers_.push_back({ containerId, {} });
    return containers_.back().items;
  }

  void addItem(int containerId, const Item& item)
  {
    for (auto& container : containers_)
    {
      if (container.id == containerId)
      {
        container.items.push_back(item);
        return;
      }
    }

    LOG_ERROR("%s: could not find container with id: %d", __func__, containerId);
  }

 private:
  struct Container
  {
    int id;
    std::vector<Item> items;
  };

  std::vector<Container> containers_;
};

#endif  // WORLDSERVER_CONTAINERMANAGER_H_
