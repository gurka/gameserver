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

#include "account.h"

#include <fstream>
#include <sstream>
#include <utility>

#include "logger.h"
#include "rapidxml.hpp"

namespace
{
  // Converts an IP address on the form "xxx.xxx.xxx.xxx" to uint32
  uint32_t ipAddressToUint32(const std::string& ipAddress)
  {
    // We can't use uint8_t since the istringstream will read only one
    // character instead of a number (since uint8_t == char)
    uint16_t a;
    uint16_t b;
    uint16_t c;
    uint16_t d;

    std::istringstream ss(ipAddress);
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

bool AccountReader::loadFile(const std::string& accountsFilename)
{
  // Open XML and read into string
  std::ifstream xmlFileStream(accountsFilename);
  if (!xmlFileStream.is_open())
  {
    LOG_ERROR("%s: Could not open file %s", __func__, accountsFilename.c_str());
    return false;
  }

  return loadFile(&xmlFileStream);
}

bool AccountReader::loadFile(std::istream* accountsFileStream)
{
  std::string tempString;
  std::ostringstream xmlStringStream;
  while (std::getline(*accountsFileStream, tempString))
  {
    xmlStringStream << tempString << "\n";
  }

  // Convert the std::string to a char*
  char* xmlString = strdup(xmlStringStream.str().c_str());

  // Parse the XML string with Rapidxml
  rapidxml::xml_document<> accountsXml;
  accountsXml.parse<0>(xmlString);

  // Get top node (<accounts>)
  rapidxml::xml_node<>* accountsNode = accountsXml.first_node("accounts");
  if (accountsNode == nullptr)
  {
    LOG_ERROR("%s: Invalid file: Could not find node <accounts>", __func__);
    free(xmlString);
    return false;
  }

  // Iterate over all <account> nodes
  for (auto* accountNode = accountsNode->first_node("account");
       accountNode != nullptr;
       accountNode = accountNode->next_sibling())
  {
    // Get account number
    auto* numberAttr = accountNode->first_attribute("number");
    if (numberAttr == nullptr)
    {
      LOG_ERROR("%s: Invalid file: <account> has no attribute \"number\"", __func__);
      free(xmlString);
      return false;
    }
    auto number = std::stoi(numberAttr->value());

    // Get account password
    auto* passwordAttr = accountNode->first_attribute("password");
    if (passwordAttr == nullptr)
    {
      LOG_ERROR("%s: Invalid file: <account> has no attribute \"password\"", __func__);
      free(xmlString);
      return false;
    }
    auto password = numberAttr->value();

    // Get account paid days
    auto* paidDaysAttr = accountNode->first_attribute("paid_days");
    if (paidDaysAttr == nullptr)
    {
      LOG_ERROR("%s: Invalid file: <account> has no attribute \"paid_days\"", __func__);
      free(xmlString);
      return false;
    }
    auto paidDays = std::stoi(paidDaysAttr->value());

    // Create Account object
    Account account(paidDays, {});

    // Iterate over all <character> nodes
    for (auto* characterNode = accountNode->first_node("character");
         characterNode != nullptr;
         characterNode = characterNode->next_sibling())
    {
      Character character;

      // Get name
      auto* charNameAttr = characterNode->first_attribute("name");
      if (charNameAttr == nullptr)
      {
        LOG_ERROR("%s: Invalid file: <character> has no attribute \"name\"", __func__);
        free(xmlString);
        return false;
      }
      character.name = charNameAttr->value();

      // Get world
      auto* worldNameAttr = characterNode->first_attribute("world_name");
      if (worldNameAttr == nullptr)
      {
        LOG_ERROR("%s: Invalid file: <character> has no attribute \"world_name\"", __func__);
        free(xmlString);
        return false;
      }
      character.worldName = worldNameAttr->value();

      // Get address
      auto* worldIpAttr = characterNode->first_attribute("world_ip");
      if (worldIpAttr == nullptr)
      {
        LOG_ERROR("%s: Invalid file: <character> has no attribute \"world_ip\"", __func__);
        free(xmlString);
        return false;
      }
      character.worldIp = ipAddressToUint32(worldIpAttr->value());

      // Get port
      auto* worldPortAttr = characterNode->first_attribute("world_port");
      if (worldPortAttr == nullptr)
      {
        LOG_ERROR("%s: Invalid file: <character> has no attribute \"world_port\"", __func__);
        free(xmlString);
        return false;
      }
      character.worldPort = std::stoi(worldPortAttr->value());

      // Insert character
      account.characters.push_back(character);
      characterToAccountNumber_.insert(std::make_pair(character.name, number));
    }

    // Insert account and password
    accounts_.insert(std::make_pair(number, account));
    passwords_.insert(std::make_pair(number, password));
  }

  LOG_INFO("%s: Successfully loaded %zu accounts with a total of %zu characters",
           __func__, accounts_.size(), characterToAccountNumber_.size());

  free(xmlString);
  return true;
}

bool AccountReader::accountExists(uint32_t accountNumber) const
{
  return accounts_.count(accountNumber) == 1;
}

bool AccountReader::verifyPassword(uint32_t accountNumber, const std::string& password) const
{
  if (passwords_.count(accountNumber) == 1)
  {
    return passwords_.at(accountNumber) == password;
  }
  return false;
}

const Account* AccountReader::getAccount(uint32_t accountNumber) const
{
  if (accountExists(accountNumber))
  {
    return &accounts_.at(accountNumber);
  }
  return nullptr;
}

bool AccountReader::characterExists(const std::string& characterName) const
{
  return characterToAccountNumber_.count(characterName) == 1;
}

bool AccountReader::verifyPassword(const std::string& characterName, const std::string& password) const
{
  if (characterToAccountNumber_.count(characterName) == 1)
  {
    return verifyPassword(characterToAccountNumber_.at(characterName), password);
  }
  return false;
}

const Character* AccountReader::getCharacter(const std::string& characterName) const
{
  if (characterExists(characterName))
  {
    const auto* account = getAccount(characterToAccountNumber_.at(characterName));
    for (const auto& character : account->characters)
    {
      if (character.name == characterName)
      {
        return &character;
      }
    }

    LOG_DEBUG("%s: Character: %s not found in accounts, but exists in characterToAccountNumber map",
              __func__, characterName.c_str());
  }
  return nullptr;
}

const Account* AccountReader::getAccount(const std::string& characterName) const
{
  if (characterExists(characterName))
  {
    return getAccount(characterToAccountNumber_.at(characterName));
  }
  return nullptr;
}
