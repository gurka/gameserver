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

#ifndef COMMON_ACCOUNTMANAGER_ACCOUNTMGR_H_
#define COMMON_ACCOUNTMANAGER_ACCOUNTMGR_H_

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>

struct Character
{
  std::string name;
  std::string worldName;
  uint32_t worldIp;
  uint16_t worldPort;
};

struct Account
{
  enum Status
  {
    NOT_FOUND,
    INVALID_PASSWORD,
    OK,
  };

  Account(Status status, int premiumDays, const std::vector<Character>& characters)
    : status(status),
      premiumDays(premiumDays),
      characters(characters)
  {
  }

  Status status;
  int premiumDays;
  std::vector<Character> characters;
};

class AccountManager
{
 public:
  // Not instantiable
  AccountManager() = delete;
  AccountManager(const AccountManager&) = delete;
  AccountManager& operator=(const AccountManager&) = delete;

  static bool initialize(const std::string& accountsFilename);
  static const Account& getAccount(uint32_t account_name, const std::string& password);
  static bool verifyPassword(const std::string& character_name, const std::string& password);

 private:
  static bool loadAccounts(const std::string& accountsFilename);

  static const Account ACCOUNT_NOT_FOUND;
  static const Account ACCOUNT_INVALID_PASSWORD;

  static std::unordered_map<uint32_t, Account> accounts_;
  static std::unordered_map<uint32_t, std::string> passwords_;
  static std::unordered_map<std::string, uint32_t> characterToAccountNumber_;
};

#endif  // COMMON_ACCOUNTMANAGER_ACCOUNTMGR_H_
