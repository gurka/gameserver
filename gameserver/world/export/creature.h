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

#ifndef WORLD_EXPORT_CREATURE_H_
#define WORLD_EXPORT_CREATURE_H_

#include <cstdint>
#include <string>

#include "direction.h"

namespace world
{

using CreatureId = std::uint32_t;

struct Outfit
{
  std::uint8_t type;
  std::uint8_t ext;  // ???
  std::uint8_t head;
  std::uint8_t body;
  std::uint8_t legs;
  std::uint8_t feet;
};

class Creature
{
 public:
  static const Creature INVALID;

  Creature();
  explicit Creature(std::string name);
  virtual ~Creature() = default;

  bool operator==(const Creature& other) const;
  bool operator!=(const Creature& other) const;

  CreatureId getCreatureId() const { return m_creature_id; }
  const std::string& getName() const { return m_name; }

  const Direction& getDirection() const { return m_direction; }
  void setDirection(const Direction& direction) { m_direction = direction; }

  std::uint16_t getMaxHealth() const { return m_max_health; }
  void setMaxHealth(std::uint16_t max_health) { m_max_health = max_health; }

  std::uint16_t getHealth() const { return m_health; }
  void setHealth(std::uint16_t health) { m_health = health; }

  virtual std::uint16_t getSpeed() const { return m_speed; }
  void setSpeed(std::uint16_t speed) { m_speed = speed; }

  const Outfit& getOutfit() const { return m_outfit; }
  void setOutfit(const Outfit& outfit) { m_outfit = outfit; }

  int getLightColor() const { return m_light_color; }
  void setLightColor(int light_color) { m_light_color = light_color; }

  int getLightLevel() const { return m_light_level; }
  void setLightLevel(int light_level) { m_light_level = light_level; }

  std::int64_t getNextWalkTick() const { return m_next_walk_tick; }
  void setNextWalkTick(std::int64_t tick) { m_next_walk_tick = tick; }

  static const CreatureId INVALID_ID;
  static int getFreeCreatureId() { return Creature::next_creature_id++; }

 private:
  CreatureId m_creature_id;
  std::string m_name;
  Direction m_direction{Direction::SOUTH};
  std::uint16_t m_max_health{0};
  std::uint16_t m_health{0};
  std::uint16_t m_speed{0};
  Outfit m_outfit;
  int m_light_color{0};
  int m_light_level{0};

  std::int64_t m_next_walk_tick{0};

  static CreatureId next_creature_id;
};

}  // namespace world

#endif  // WORLD_EXPORT_CREATURE_H_
