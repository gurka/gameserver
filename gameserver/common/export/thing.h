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

#ifndef COMMON_EXPORT_THING_H_
#define COMMON_EXPORT_THING_H_

#include <variant>

namespace common
{

class Creature;
class Item;

class Thing
{
 public:
  Thing(const Creature* creature)  // NOLINT
      : m_thing(creature)
  {
  }

  Thing(const Item* item)  // NOLINT
      : m_thing(item)
  {
  }

  bool hasCreature() const { return std::holds_alternative<const Creature*>(m_thing); }

  const Creature* creature() const
  {
    return hasCreature() ? std::get<const Creature*>(m_thing) : nullptr;
  }

  bool hasItem() const { return std::holds_alternative<const Item*>(m_thing); }

  const Item* item() const
  {
    return hasItem() ? std::get<const Item*>(m_thing) : nullptr;
  }

 private:
  std::variant<const Creature*, const Item*> m_thing;
};

}  // namespace common

#endif  // COMMON_EXPORT_THING_H_
