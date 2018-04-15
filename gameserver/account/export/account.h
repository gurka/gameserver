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

#ifndef ACCOUNT_EXPORT_ACCOUNT_H_
#define ACCOUNT_EXPORT_ACCOUNT_H_

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
  Account(int premiumDays, const std::vector<Character>& characters)
    : premiumDays(premiumDays),
      characters(characters)
  {
  }

  int premiumDays;
  std::vector<Character> characters;
};

class AccountReader
{
 public:
  bool loadFile(const std::string& accountsFilename);
  bool loadFile(std::istream* accountsFileStream);

  bool accountExists(uint32_t accountNumber) const;
  bool verifyPassword(uint32_t accountNumber, const std::string& password) const;
  const Account* getAccount(uint32_t accountNumber) const;

  bool characterExists(const std::string& characterName) const;
  bool verifyPassword(const std::string& characterName, const std::string& password) const;
  const Character* getCharacter(const std::string& characterName) const;
  const Account* getAccount(const std::string& characterName) const;

 private:
  std::unordered_map<uint32_t, Account> accounts_;
  std::unordered_map<uint32_t, std::string> passwords_;
  std::unordered_map<std::string, uint32_t> characterToAccountNumber_;
};

#endif  // ACCOUNT_EXPORT_ACCOUNT_H_
