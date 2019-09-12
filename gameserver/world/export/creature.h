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

  CreatureId getCreatureId() const { return creatureId_; }
  const std::string& getName() const { return name_; }

  const Direction& getDirection() const { return direction_; }
  void setDirection(const Direction& direction) { direction_ = direction; }

  std::uint16_t getMaxHealth() const { return maxHealth_; }
  void setMaxHealth(std::uint16_t maxHealth) { maxHealth_ = maxHealth; }

  std::uint16_t getHealth() const { return health_; }
  void setHealth(std::uint16_t health) { health_ = health; }

  virtual std::uint16_t getSpeed() const { return speed_; }
  void setSpeed(std::uint16_t speed) { speed_ = speed; }

  const Outfit& getOutfit() const { return outfit_; }
  void setOutfit(const Outfit& outfit) { outfit_ = outfit; }

  int getLightColor() const { return lightColor_; }
  void setLightColor(int lightColor) { lightColor_ = lightColor; }

  int getLightLevel() const { return lightLevel_; }
  void setLightLevel(int lightLevel) { lightLevel_ = lightLevel; }

  std::int64_t getNextWalkTick() const { return nextWalkTick_; }
  void setNextWalkTick(std::int64_t tick) { nextWalkTick_ = tick; }

  static const CreatureId INVALID_ID;
  static int getFreeCreatureId() { return Creature::nextCreatureId_++; }

 private:
  CreatureId creatureId_;
  std::string name_;
  Direction direction_;
  std::uint16_t maxHealth_;
  std::uint16_t health_;
  std::uint16_t speed_;
  Outfit outfit_;
  int lightColor_;
  int lightLevel_;

  std::int64_t nextWalkTick_;

  static CreatureId nextCreatureId_;
};

#endif  // WORLD_EXPORT_CREATURE_H_
