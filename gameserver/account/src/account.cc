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

#include "account.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include "logger.h"
#include "loader.h"

namespace account
{

bool AccountReader::load(const std::string& accounts_filename)
{
  return loader::load(accounts_filename, &m_account_data);
}

bool AccountReader::accountExists(int account_number) const
{
  return m_account_data.accounts.count(account_number) == 1;
}

bool AccountReader::verifyPassword(int account_number, const std::string& password) const
{
  if (m_account_data.passwords.count(account_number) == 1)
  {
    return m_account_data.passwords.at(account_number) == password;
  }
  return false;
}

const Account* AccountReader::getAccount(int account_number) const
{
  if (accountExists(account_number))
  {
    return &m_account_data.accounts.at(account_number);
  }
  return nullptr;
}

bool AccountReader::characterExists(const std::string& character_name) const
{
  return m_account_data.char_to_acc_num.count(character_name) == 1;
}

bool AccountReader::verifyPassword(const std::string& character_name, const std::string& password) const
{
  if (m_account_data.char_to_acc_num.count(character_name) == 1)
  {
    return verifyPassword(m_account_data.char_to_acc_num.at(character_name), password);
  }
  return false;
}

const Character* AccountReader::getCharacter(const std::string& character_name) const
{
  if (characterExists(character_name))
  {
    const auto* account = getAccount(m_account_data.char_to_acc_num.at(character_name));
    const auto pred = [&character_name](const Character& c)
    {
      return c.name == character_name;
    };
    const auto& it = std::find_if(account->characters.begin(),
                                  account->characters.end(),
                                  pred);
    if (it != account->characters.end())
    {
      return &(*it);
    }

    LOG_DEBUG("%s: Character: %s not found in accounts, but exists in characterToAccountNumber map",
              __func__, character_name.c_str());
  }
  return nullptr;
}

const Account* AccountReader::getAccount(const std::string& character_name) const
{
  if (characterExists(character_name))
  {
    return getAccount(m_account_data.char_to_acc_num.at(character_name));
  }
  return nullptr;
}

}  // namespace account
