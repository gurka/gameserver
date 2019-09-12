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

#include "account.h"

#include <cstring>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

#include "logger.h"
#include "rapidxml.hpp"

namespace
{

// Converts an IP address on the form "xxx.xxx.xxx.xxx" to uint32
std::uint32_t ipAddressToUint32(const std::string& ip_address)
{
  // We can't use uint8_t since the istringstream will read only one
  // character instead of a number (since uint8_t == char)
  std::uint16_t a;
  std::uint16_t b;
  std::uint16_t c;
  std::uint16_t d;

  std::istringstream ss(ip_address);
  ss >> a;
  ss.ignore(1, '.');
  ss >> b;
  ss.ignore(1, '.');
  ss >> c;
  ss.ignore(1, '.');
  ss >> d;
  ss.ignore(1, '.');

  return ((d & 0xFF) << 24) |
         ((c & 0xFF) << 16) |
         ((b & 0xFF) << 8)  |
         ((a & 0xFF) << 0);
}

}  // namespace

bool AccountReader::loadFile(const std::string& accounts_filename)
{
  // Open XML and read into string
  std::ifstream xml_file_stream(accounts_filename);
  if (!xml_file_stream.is_open())
  {
    LOG_ERROR("%s: Could not open file %s", __func__, accounts_filename.c_str());
    return false;
  }

  return loadFile(&xml_file_stream);
}

bool AccountReader::loadFile(std::istream* accounts_file_stream)
{
  std::string temp_string;
  std::ostringstream xml_stream;
  while (std::getline(*accounts_file_stream, temp_string))
  {
    xml_stream << temp_string << "\n";
  }

  // Convert the std::string to a char*
  char* xml_string = strdup(xml_stream.str().c_str());

  // Parse the XML string with Rapidxml
  rapidxml::xml_document<> m_accountsxml;
  m_accountsxml.parse<0>(xml_string);

  // Get top node (<accounts>)
  rapidxml::xml_node<>* m_accountsnode = m_accountsxml.first_node("accounts");
  if (m_accountsnode == nullptr)
  {
    LOG_ERROR("%s: Invalid file: Could not find node <accounts>", __func__);
    free(xml_string);
    return false;
  }

  // Iterate over all <account> nodes
  for (auto* account_node = m_accountsnode->first_node("account");
       account_node != nullptr;
       account_node = account_node->next_sibling())
  {
    // Get account number
    auto* number_attr = account_node->first_attribute("number");
    if (number_attr == nullptr)
    {
      LOG_ERROR("%s: Invalid file: <account> has no attribute \"number\"", __func__);
      free(xml_string);
      return false;
    }
    auto number = std::stoi(number_attr->value());

    // Get account password
    auto* password_attr = account_node->first_attribute("password");
    if (password_attr == nullptr)
    {
      LOG_ERROR("%s: Invalid file: <account> has no attribute \"password\"", __func__);
      free(xml_string);
      return false;
    }
    auto password = password_attr->value();

    // Get account paid days
    auto* paid_days_attr = account_node->first_attribute("paid_days");
    if (paid_days_attr == nullptr)
    {
      LOG_ERROR("%s: Invalid file: <account> has no attribute \"paid_days\"", __func__);
      free(xml_string);
      return false;
    }
    auto paid_days = std::stoi(paid_days_attr->value());

    // Create Account object
    Account account(paid_days, {});

    // Iterate over all <character> nodes
    for (auto* character_node = account_node->first_node("character");
         character_node != nullptr;
         character_node = character_node->next_sibling())
    {
      Character character;

      // Get name
      auto* char_name_attr = character_node->first_attribute("name");
      if (char_name_attr == nullptr)
      {
        LOG_ERROR("%s: Invalid file: <character> has no attribute \"name\"", __func__);
        free(xml_string);
        return false;
      }
      character.name = char_name_attr->value();

      // Get world
      auto* world_name_attr = character_node->first_attribute("world_name");
      if (world_name_attr == nullptr)
      {
        LOG_ERROR("%s: Invalid file: <character> has no attribute \"world_name\"", __func__);
        free(xml_string);
        return false;
      }
      character.world_name = world_name_attr->value();

      // Get address
      auto* world_ip_addr = character_node->first_attribute("world_ip");
      if (world_ip_addr == nullptr)
      {
        LOG_ERROR("%s: Invalid file: <character> has no attribute \"world_ip\"", __func__);
        free(xml_string);
        return false;
      }
      character.world_ip = ipAddressToUint32(world_ip_addr->value());

      // Get port
      auto* world_port_attr = character_node->first_attribute("world_port");
      if (world_port_attr == nullptr)
      {
        LOG_ERROR("%s: Invalid file: <character> has no attribute \"world_port\"", __func__);
        free(xml_string);
        return false;
      }
      character.world_port = std::stoi(world_port_attr->value());

      // Insert character
      account.characters.push_back(character);
      m_char_to_acc_num.insert(std::make_pair(character.name, number));
    }

    // Insert account and password
    LOG_DEBUG("%s: Adding account: %d, password: %s", __func__, number, password);
    m_accounts.insert(std::make_pair(number, account));
    m_passwords.insert(std::make_pair(number, password));
  }

  LOG_INFO("%s: Successfully loaded %zu accounts with a total of %zu characters",
           __func__, m_accounts.size(), m_char_to_acc_num.size());

  free(xml_string);
  return true;
}

bool AccountReader::accountExists(int account_number) const
{
  return m_accounts.count(account_number) == 1;
}

bool AccountReader::verifyPassword(int account_number, const std::string& password) const
{
  if (m_passwords.count(account_number) == 1)
  {
    return m_passwords.at(account_number) == password;
  }
  return false;
}

const Account* AccountReader::getAccount(int account_number) const
{
  if (accountExists(account_number))
  {
    return &m_accounts.at(account_number);
  }
  return nullptr;
}

bool AccountReader::characterExists(const std::string& character_name) const
{
  return m_char_to_acc_num.count(character_name) == 1;
}

bool AccountReader::verifyPassword(const std::string& character_name, const std::string& password) const
{
  if (m_char_to_acc_num.count(character_name) == 1)
  {
    return verifyPassword(m_char_to_acc_num.at(character_name), password);
  }
  return false;
}

const Character* AccountReader::getCharacter(const std::string& character_name) const
{
  if (characterExists(character_name))
  {
    const auto* account = getAccount(m_char_to_acc_num.at(character_name));
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
    return getAccount(m_char_to_acc_num.at(character_name));
  }
  return nullptr;
}
