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

#include "gtest/gtest.h"

namespace world
{

TEST(CreatureTest, Constructor)
{
  std::string TestCreatureName("TestCreature");
  Creature creature(TestCreatureName);

  ASSERT_NE(creature.getCreatureId(), Creature::INVALID_ID);
  ASSERT_EQ(creature.getName(), TestCreatureName);
}

TEST(CreatureTest, CreatureId)
{
  Creature creatureFoo("foo");
  Creature creatureBar("bar");

  ASSERT_NE(creatureFoo.getCreatureId(), Creature::INVALID_ID);
  ASSERT_NE(creatureBar.getCreatureId(), Creature::INVALID_ID);
  ASSERT_NE(creatureFoo.getCreatureId(), creatureBar.getCreatureId());
}

TEST(CreatureTest, Equals)
{
  Creature creatureFoo("foo");
  Creature creatureBar("bar");
  Creature& creatureFooRef(creatureFoo);

  ASSERT_NE(creatureFoo, creatureBar);
  ASSERT_NE(creatureBar, creatureFooRef);
  ASSERT_EQ(creatureFoo, creatureFooRef);
}

TEST(CreatureTest, GettersSetters)
{
  Creature creature("TestCreature");

  creature.setDirection(Direction::NORTH);
  ASSERT_EQ(creature.getDirection(), Direction::NORTH);

  creature.setHealth(123);
  ASSERT_EQ(creature.getHealth(), 123);

  creature.setLightColor(45);
  ASSERT_EQ(creature.getLightColor(), 45);

  creature.setLightLevel(67);
  ASSERT_EQ(creature.getLightLevel(), 67);

  creature.setMaxHealth(890);
  ASSERT_EQ(creature.getMaxHealth(), 890);

  creature.setSpeed(123);
  ASSERT_EQ(creature.getSpeed(), 123);

  Outfit outfitSet;
  outfitSet.type = 11;
  outfitSet.ext = 22;
  outfitSet.head = 33;
  outfitSet.body = 44;
  outfitSet.legs = 55;
  outfitSet.feet = 66;
  creature.setOutfit(outfitSet);
  const auto& outfitGet = creature.getOutfit();
  ASSERT_EQ(outfitGet.type, 11);
  ASSERT_EQ(outfitGet.ext, 22);
  ASSERT_EQ(outfitGet.head, 33);
  ASSERT_EQ(outfitGet.body, 44);
  ASSERT_EQ(outfitGet.legs, 55);
  ASSERT_EQ(outfitGet.feet, 66);
}

}  // namespace world
