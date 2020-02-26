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

#ifndef ACCOUNT_EXPORT_ACCOUNT_H_
#define ACCOUNT_EXPORT_ACCOUNT_H_

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>

namespace account
{

struct Character
{
  std::string name;
  std::string world_name;

  // TODO(simon): these don't belong here
  std::uint32_t world_ip;
  int world_port;
};

struct Account
{
  Account(int premium_days, std::vector<Character> characters)
    : premium_days(premium_days),
      characters(std::move(characters))
  {
  }

  int premium_days;
  std::vector<Character> characters;
};

struct AccountData
{
  std::unordered_map<int, Account> accounts;
  std::unordered_map<int, std::string> passwords;
  std::unordered_map<std::string, int> char_to_acc_num;
};

// TODO(simon): rename to AccountManager or Accounts
class AccountReader
{
 public:
  bool load(const std::string& accounts_filename);

  bool accountExists(int account_number) const;
  bool verifyPassword(int account_number, const std::string& password) const;
  const Account* getAccount(int account_number) const;

  bool characterExists(const std::string& character_name) const;
  bool verifyPassword(const std::string& character_name, const std::string& password) const;
  const Character* getCharacter(const std::string& character_name) const;
  const Account* getAccount(const std::string& character_name) const;

 private:
  AccountData m_account_data;

#ifdef UNITTEST
  friend class AccountTest;
#endif
};

}  // namespace account

#endif  // ACCOUNT_EXPORT_ACCOUNT_H_
