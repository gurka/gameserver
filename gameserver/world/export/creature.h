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

using CreatureId = int;

struct Outfit
{
  int type;
  int ext;  // item
  int head;
  int body;
  int legs;
  int feet;
};

class Creature
{
 public:
  static const Creature INVALID;

  Creature();
  explicit Creature(const std::string& name);
  virtual ~Creature() = default;

  bool operator==(const Creature& other) const;
  bool operator!=(const Creature& other) const;

  CreatureId getCreatureId() const { return creatureId_; }
  const std::string& getName() const { return name_; }

  const Direction& getDirection() const { return direction_; }
  void setDirection(const Direction& direction) { direction_ = direction; }

  int getMaxHealth() const { return maxHealth_; }
  void setMaxHealth(int maxHealth) { maxHealth_ = maxHealth; }

  int getHealth() const { return health_; }
  void setHealth(int health) { health_ = health; }

  virtual int getSpeed() const { return speed_; }
  void setSpeed(int speed) { speed_ = speed; }

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
  int maxHealth_;
  int health_;
  int speed_;
  Outfit outfit_;
  int lightColor_;
  int lightLevel_;

  std::int64_t nextWalkTick_;

  static CreatureId nextCreatureId_;
};

#endif  // WORLD_EXPORT_CREATURE_H_
