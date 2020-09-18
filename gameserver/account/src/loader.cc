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

#include "loader.h"

#include <cstring>
#include <fstream>
#include <sstream>

#include "logger.h"
#include "rapidxml.hpp"
#include "account.h"

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

namespace account::loader
{

bool load(const std::string& filename, account::AccountData* account_data)
{
  account_data->accounts.clear();
  account_data->passwords.clear();
  account_data->char_to_acc_num.clear();

  // Open XML and read into string
  std::ifstream xml_file_stream(filename);
  if (!xml_file_stream.is_open())
  {
    LOG_ERROR("%s: Could not open file %s", __func__, filename.c_str());
    return false;
  }

  std::string temp_string;
  std::ostringstream xml_stream;
  while (std::getline(xml_file_stream, temp_string))
  {
    xml_stream << temp_string << "\n";
  }

  // Convert the std::string to a char*
  char* xml_string = strdup(xml_stream.str().c_str());

  // Parse the XML string with Rapidxml
  rapidxml::xml_document<> accounts_xml;
  accounts_xml.parse<0>(xml_string);

  // Get top node (<accounts>)
  rapidxml::xml_node<>* accounts_node = accounts_xml.first_node("accounts");
  if (accounts_node == nullptr)
  {
    LOG_ERROR("%s: Invalid file: Could not find node <accounts>", __func__);
    free(xml_string);
    return false;
  }

  // Iterate over all <account> nodes
  for (auto* account_node = accounts_node->first_node("account");
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
    account::Account acc(paid_days, {});

    // Iterate over all <character> nodes
    for (auto* character_node = account_node->first_node("character");
         character_node != nullptr;
         character_node = character_node->next_sibling())
    {
      account::Character character;

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
      acc.characters.push_back(character);
      account_data->char_to_acc_num.insert(std::make_pair(character.name, number));
    }

    // Insert account and password
    LOG_DEBUG("%s: Adding account: %d, password: %s", __func__, number, password);
    account_data->accounts.insert(std::make_pair(number, acc));
    account_data->passwords.insert(std::make_pair(number, password));
  }

  LOG_INFO("%s: Successfully loaded %zu accounts with a total of %zu characters",
           __func__,
           account_data->accounts.size(),
           account_data->char_to_acc_num.size());

  free(xml_string);
  return true;
}

}  // namespace account::loader
