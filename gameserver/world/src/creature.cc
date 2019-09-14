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

#include "creature.h"

namespace world
{

const Creature Creature::INVALID = Creature();
const CreatureId Creature::INVALID_ID = 0;
CreatureId Creature::next_creature_id = 0x4713;

Creature::Creature()
  : m_creature_id(Creature::INVALID_ID),
    m_outfit({0, 0, 0, 0, 0, 0})
{
}

Creature::Creature(std::string name)
  : m_creature_id(Creature::getFreeCreatureId()),
    m_name(std::move(name)),
    m_max_health(100),
    m_health(100),
    m_speed(110),
    m_outfit({ 128, 0, 20, 30, 40, 50 }),
    m_next_walk_tick(0)
{
}

bool Creature::operator==(const Creature& other) const
{
  return m_creature_id == other.m_creature_id;
}

bool Creature::operator!=(const Creature& other) const
{
  return !(*this == other);
}

}  // namespace world
