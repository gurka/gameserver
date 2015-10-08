/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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

#include "accountmgr.h"

#include <utility>

const Account AccountManager::ACCOUNT_NOT_FOUND(Account::NOT_FOUND, {}, 0);
const Account AccountManager::ACCOUNT_INVALID_PASSWORD(Account::INVALID_PASSWORD, {}, 0);

std::unordered_map<uint32_t, Account> AccountManager::accounts_;
std::unordered_map<uint32_t, std::string> AccountManager::passwords_;
std::unordered_map<std::string, uint32_t> AccountManager::characterToAccountNumber_;

void AccountManager::initialize()
{
  // Create static accounts
  accounts_.insert(std::pair<uint32_t, Account>(1,
                                                {
                                                  Account::Status::OK,
                                                  {
                                                    // 192.168.1.3 => 0xC0 0xA8 0x01 0x03
                                                    // 10.0.0.1 => 0x0A 0x00 0x00 0x01
                                                    { "Alice", "Default", 0x0100000A, 7172, },
                                                    { "Bob", "Default", 0x0100000A, 7172, },
                                                  },
                                                  90
                                                }));
  passwords_.insert(std::pair<uint32_t, std::string>(1, "1"));

  accounts_.insert(std::pair<uint32_t, Account>(2,
                                                {
                                                  Account::Status::OK,
                                                  {
                                                    { "Gamemaster", "Default", 0x0100000A, 7172, },
                                                  },
                                                  1337,
                                                }));
  passwords_.insert(std::pair<uint32_t, std::string>(2, "2"));

  for (const auto& accountsPair : accounts_)
  {
    for (const auto& character : accountsPair.second.characters)
    {
      characterToAccountNumber_.insert(std::make_pair(character.name, accountsPair.first));
    }
  }
}

const Account& AccountManager::getAccount(uint32_t account_name,
                                          const std::string& password)
{
  auto account_cit = accounts_.find(account_name);
  auto password_cit = passwords_.find(account_name);

  // Check if both account and password exist
  if (account_cit == accounts_.end() || password_cit == passwords_.end())
  {
    return ACCOUNT_NOT_FOUND;
  }

  // Check if password matches
  if (password_cit->second != password)
  {
    return ACCOUNT_INVALID_PASSWORD;
  }

  return account_cit->second;
}

bool AccountManager::verifyPassword(const std::string& character_name,
                                    const std::string& password)
{
  auto character_cit = characterToAccountNumber_.find(character_name);
  if (character_cit == characterToAccountNumber_.end())
  {
    return false;  // Character not found
  }

  auto password_cit = passwords_.find(character_cit->second);
  if (password_cit == passwords_.end())
  {
    return false;  // Error (account without password)
  }

  return password_cit->second == password;
}
