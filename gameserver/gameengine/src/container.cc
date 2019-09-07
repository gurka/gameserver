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
#include "container.h"

#include <string>
#include <sstream>

#include "player_ctrl.h"
#include "item.h"
#include "creature.h"

std::string Container::toString(int indent) const
{
  const auto indents = std::string(indent, ' ');
  std::ostringstream ss;
  ss << indents << "Weight:             " << weight << '\n';
  ss << indents << "ItemUniqueId:       " << item->getItemUniqueId()
     << " (" << (item->getItemType().isContainer ? "" : "not ") << "container)\n";
  ss << indents << "parentItemUniqueId: " << parentItemUniqueId << '\n';
  ss << indents << "rootGamePosition:   " << rootGamePosition.toString() << '\n';
  ss << indents << "items:\n";
  for (const auto* item : items)
  {
    ss << indents << indents << "ItemUniqueId: " << item->getItemUniqueId()
       << " (" << (item->getItemType().isContainer ? "" : "not ") << "container)\n";
  }
  ss << indents << "relatedPlayers:\n";
  for (const auto* playerCtrl : relatedPlayers)
  {
    ss << indents << indents << "PlayerID: " << playerCtrl->getPlayerId() << '\n';
  }
  return ss.str();
}
