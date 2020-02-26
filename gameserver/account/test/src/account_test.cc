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

#include <algorithm>
#include "gtest/gtest.h"
#include "account.h"

namespace account
{

class AccountTest : public ::testing::Test
{
 public:

  static constexpr auto ACCOUNT_1 = 123456;
  static constexpr auto ACCOUNT_2 = 0;
  static constexpr auto ACCOUNT_INVALID = 1337;

  static constexpr auto PASSWORD_1 = "hunter2";
  static constexpr auto PASSWORD_2 = "root";
  static constexpr auto PASSWORD_INVALID = "god";

  AccountTest()
  {
    Character alice { "Alice", "Default", 123456U, 7172 };
    Character bob { "Bob", "Default", 123456U, 7172 };
    Character gamemaster { "Gamemaster", "Default", 123456U, 7172 };

    Account acc1(90, { alice, bob });
    Account acc2(1337, { gamemaster });

    m_account_reader.m_account_data.accounts.emplace(ACCOUNT_1, acc1);
    m_account_reader.m_account_data.accounts.emplace(ACCOUNT_2, acc2);

    m_account_reader.m_account_data.passwords.emplace(ACCOUNT_1, PASSWORD_1);
    m_account_reader.m_account_data.passwords.emplace(ACCOUNT_2, PASSWORD_2);

    m_account_reader.m_account_data.char_to_acc_num.emplace("Alice", ACCOUNT_1);
    m_account_reader.m_account_data.char_to_acc_num.emplace("Bob", ACCOUNT_1);
    m_account_reader.m_account_data.char_to_acc_num.emplace("Gamemaster", ACCOUNT_2);
  }

  AccountReader m_account_reader;
};

TEST_F(AccountTest, Accounts)
{
  EXPECT_TRUE(m_account_reader.accountExists(ACCOUNT_1));
  EXPECT_TRUE(m_account_reader.accountExists(ACCOUNT_2));
  EXPECT_FALSE(m_account_reader.accountExists(ACCOUNT_INVALID));

  EXPECT_TRUE(m_account_reader.verifyPassword(ACCOUNT_1, PASSWORD_1));
  EXPECT_TRUE(m_account_reader.verifyPassword(ACCOUNT_2, PASSWORD_2));
  EXPECT_FALSE(m_account_reader.verifyPassword(ACCOUNT_INVALID, PASSWORD_INVALID));
  EXPECT_FALSE(m_account_reader.verifyPassword(ACCOUNT_1, PASSWORD_2));
  EXPECT_FALSE(m_account_reader.verifyPassword(ACCOUNT_2, PASSWORD_1));

  EXPECT_NE(nullptr, m_account_reader.getAccount(ACCOUNT_1));
  EXPECT_NE(nullptr, m_account_reader.getAccount(ACCOUNT_2));
  EXPECT_EQ(nullptr, m_account_reader.getAccount(ACCOUNT_INVALID));

  EXPECT_EQ(90, m_account_reader.getAccount(ACCOUNT_1)->premium_days);
  EXPECT_EQ(1337, m_account_reader.getAccount(ACCOUNT_2)->premium_days);
}

TEST_F(AccountTest, Characters)
{
  EXPECT_TRUE(m_account_reader.characterExists("Alice"));
  EXPECT_TRUE(m_account_reader.characterExists("Bob"));
  EXPECT_TRUE(m_account_reader.characterExists("Gamemaster"));
  EXPECT_FALSE(m_account_reader.characterExists("Simon"));

  EXPECT_TRUE(m_account_reader.verifyPassword("Alice", PASSWORD_1));
  EXPECT_TRUE(m_account_reader.verifyPassword("Bob", PASSWORD_1));
  EXPECT_TRUE(m_account_reader.verifyPassword("Gamemaster", PASSWORD_2));
  EXPECT_FALSE(m_account_reader.verifyPassword("Simon", PASSWORD_INVALID));
  EXPECT_FALSE(m_account_reader.verifyPassword("Alice", PASSWORD_2));
  EXPECT_FALSE(m_account_reader.verifyPassword("Gamemaster", PASSWORD_1));

  EXPECT_NE(nullptr, m_account_reader.getCharacter("Alice"));
  EXPECT_NE(nullptr, m_account_reader.getCharacter("Bob"));
  EXPECT_NE(nullptr, m_account_reader.getCharacter("Gamemaster"));
  EXPECT_EQ(nullptr, m_account_reader.getCharacter("Simon"));

  const auto* accountAlice = m_account_reader.getAccount("Alice");
  const auto* accountBob = m_account_reader.getAccount("Bob");
  const auto* accountGamemaster = m_account_reader.getAccount("Gamemaster");

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

  const auto* characterBob = m_account_reader.getCharacter("Bob");
  EXPECT_EQ("Default", characterBob->world_name);
  EXPECT_EQ(7172, characterBob->world_port);
}

}  // namespace account
