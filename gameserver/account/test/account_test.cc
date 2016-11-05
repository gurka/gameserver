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

#include <algorithm>
#include "gtest/gtest.h"
#include "account.h"

class AccountTest : public ::testing::Test
{
 public:
  AccountTest()
  {
    // Create valid xml-file stream
    xmlStream <<
    "<?xml version=\"1.0\"?>\n"
    "<accounts>\n"
    "  <account number=\"1\" password=\"1\" paid_days=\"90\">\n"
    "    <character name=\"Alice\" world_name=\"Default\" world_ip=\"192.168.0.4\" world_port=\"7172\" />\n"
    "    <character name=\"Bob\" world_name=\"Default\" world_ip=\"192.168.0.4\" world_port=\"7172\" />\n"
    "  </account>\n"
    "  <account number=\"2\" password=\"2\" paid_days=\"1337\">\n"
    "    <character name=\"Gamemaster\" world_name=\"Default\" world_ip=\"192.168.0.4\" world_port=\"7172\" />\n"
    "  </account>\n"
    "</accounts>\n";
  }

  std::stringstream xmlStream;
};

TEST_F(AccountTest, LoadFile)
{
  // Create AccountReader and load file sream
  AccountReader accountReader;
  EXPECT_TRUE(accountReader.loadFile(&xmlStream));
}

TEST_F(AccountTest, Accounts)
{
  AccountReader accountReader;
  accountReader.loadFile(&xmlStream);

  EXPECT_TRUE(accountReader.accountExists(1));
  EXPECT_TRUE(accountReader.accountExists(2));
  EXPECT_FALSE(accountReader.accountExists(3));

  EXPECT_TRUE(accountReader.verifyPassword(1, "1"));
  EXPECT_TRUE(accountReader.verifyPassword(2, "2"));
  EXPECT_FALSE(accountReader.verifyPassword(3, "3"));
  EXPECT_FALSE(accountReader.verifyPassword(1, "2"));
  EXPECT_FALSE(accountReader.verifyPassword(2, "1"));

  EXPECT_NE(nullptr, accountReader.getAccount(1));
  EXPECT_NE(nullptr, accountReader.getAccount(2));
  EXPECT_EQ(nullptr, accountReader.getAccount(3));

  EXPECT_EQ(90, accountReader.getAccount(1)->premiumDays);
  EXPECT_EQ(1337, accountReader.getAccount(2)->premiumDays);
}

TEST_F(AccountTest, Characters)
{
  AccountReader accountReader;
  accountReader.loadFile(&xmlStream);

  EXPECT_TRUE(accountReader.characterExists("Alice"));
  EXPECT_TRUE(accountReader.characterExists("Bob"));
  EXPECT_TRUE(accountReader.characterExists("Gamemaster"));
  EXPECT_FALSE(accountReader.characterExists("Simon"));

  EXPECT_TRUE(accountReader.verifyPassword("Alice", "1"));
  EXPECT_TRUE(accountReader.verifyPassword("Bob", "1"));
  EXPECT_TRUE(accountReader.verifyPassword("Gamemaster", "2"));
  EXPECT_FALSE(accountReader.verifyPassword("Simon", "3"));
  EXPECT_FALSE(accountReader.verifyPassword("Alice", "2"));
  EXPECT_FALSE(accountReader.verifyPassword("Gamemaster", "1"));

  EXPECT_NE(nullptr, accountReader.getCharacter("Alice"));
  EXPECT_NE(nullptr, accountReader.getCharacter("Bob"));
  EXPECT_NE(nullptr, accountReader.getCharacter("Gamemaster"));
  EXPECT_EQ(nullptr, accountReader.getCharacter("Simon"));

  const auto* accountAlice = accountReader.getAccount("Alice");
  const auto* accountBob = accountReader.getAccount("Bob");
  const auto* accountGamemaster = accountReader.getAccount("Gamemaster");

  EXPECT_TRUE(accountAlice == accountBob);
  EXPECT_FALSE(accountAlice == accountGamemaster);
  EXPECT_FALSE(accountBob == accountGamemaster);

  EXPECT_EQ(2U, accountAlice->characters.size());
  EXPECT_EQ(2U, accountBob->characters.size());
  EXPECT_EQ(1U, accountGamemaster->characters.size());

  auto it = std::find_if(accountAlice->characters.cbegin(), accountAlice->characters.cend(), [](const Character& character)
  {
    return character.name == "Alice";
  });

  EXPECT_NE(accountAlice->characters.cend(), it);

  const auto* characterBob = accountReader.getCharacter("Bob");
  EXPECT_EQ("Default", characterBob->worldName);
  EXPECT_EQ(7172, characterBob->worldPort);
}
