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
#include "container.h"

#include <string>
#include <sstream>

#include "player_ctrl.h"
#include "item.h"
#include "creature.h"

namespace gameengine
{

std::string Container::toString(int indent) const
{
  const auto indents = std::string(indent, ' ');
  std::ostringstream ss;
  ss << indents << "Weight:             " << weight << '\n';
  ss << indents << "ItemUniqueId:       " << item->getItemUniqueId()
     << " (" << (item->getItemType().is_container ? "" : "not ") << "container)\n";
  ss << indents << "parent_item_unique_id: " << parent_item_unique_id << '\n';
  ss << indents << "root_game_position:   " << root_game_position.toString() << '\n';
  ss << indents << "items:\n";
  for (const auto* item : items)
  {
    ss << indents << indents << "ItemUniqueId: " << item->getItemUniqueId()
       << " (" << (item->getItemType().is_container ? "" : "not ") << "container)\n";
  }
  ss << indents << "related_players:\n";
  for (const auto* player_ctrl : related_players)
  {
    ss << indents << indents << "PlayerID: " << player_ctrl->getPlayerId() << '\n';
  }
  return ss.str();
}

}  // namespace gameengine
