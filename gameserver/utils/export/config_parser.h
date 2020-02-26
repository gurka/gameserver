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

#ifndef UTILS_EXPORT_CONFIG_PARSER_H_
#define UTILS_EXPORT_CONFIG_PARSER_H_

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "logger.h"

namespace utils
{

class ConfigParser
{
 public:
  bool hasSection(const std::string& section) const
  {
    return m_values.count(section) == 1;
  }

  bool hasValue(const std::string& section, const std::string& key) const
  {
    if (!hasSection(section))
    {
      return false;
    }

    return m_values.at(section).count(key) == 1;
  }

  int getInteger(const std::string& section, const std::string& key, int default_value = 0) const
  {
    if (hasValue(section, key))
    {
      return std::stoi(m_values.at(section).at(key));
    }
    return default_value;
  }

  std::string getString(const std::string& section, const std::string& key, const std::string& default_value = "") const
  {
    if (hasValue(section, key))
    {
      return m_values.at(section).at(key);
    }
    return default_value;
  }

  bool getBoolean(const std::string& section, const std::string& key, bool default_value = false) const
  {
    if (hasValue(section, key))
    {
      return m_values.at(section).at(key) == "true";
    }
    return default_value;
  }

  bool parsedOk() const
  {
    return !m_error;
  }

  std::string getErrorMessage() const
  {
    return m_error_message;
  }

  static ConfigParser parseFile(const std::string& file_name)
  {
    // Open file
    std::ifstream file_stream(file_name);
    if (!file_stream.is_open())
    {
      ConfigParser result;
      result.m_error = true;
      result.m_error_message = "Could not open file: " + file_name;
      return result;
    }

    ConfigParser result = parseStream(&file_stream);
    if (result.m_error)
    {
      auto temp = result.m_error_message;
      result.m_error_message = file_name + ":" + temp;
    }

    return result;
  }

  static ConfigParser parseStream(std::istream* stream)
  {
    ConfigParser result;

    std::unordered_map<std::string, std::string>* current_section = nullptr;

    // Read lines
    std::string line;
    auto line_number = 0;
    while (std::getline(*stream, line))
    {
      line_number++;

      std::istringstream line_stream(line);

      // Discard leading whitespace
      line_stream >> std::ws;

      if (line_stream.eof())
      {
        // Empty line
      }
      else if (line_stream.peek() == ';')
      {
        // Comment
      }
      else if (line_stream.peek() == '[')
      {
        // Read section name
        std::stringbuf section_name_buf;
        line_stream.ignore(1, '[');                // Discard '['
        line_stream.get(section_name_buf, ']');      // Read until ']'
        auto section_name = section_name_buf.str();

        // Get pointer to section's map
        if (section_name.length() == 0)
        {
          std::ostringstream m_errormessage_stream;
          m_errormessage_stream << line_number << ": Invalid section name (empty)";

          result.m_error = true;
          result.m_error_message = m_errormessage_stream.str();
          return result;
        }

        LOG_DEBUG("Read section name: {%s}", section_name.c_str());

        // operator[] creates a new element (if one not already exists)
        // and returns it, we keep the pointer
        current_section = &(result.m_values[section_name]);
      }
      else if (current_section == nullptr)
      {
        std::ostringstream m_errormessage_stream;
        m_errormessage_stream << line_number << ": Key-value pair without section";

        result.m_error = true;
        result.m_error_message = m_errormessage_stream.str();
        return result;
      }
      else
      {
        // Read key-value pair
        std::stringbuf key_buf;
        std::stringbuf value_buf;

        line_stream.get(key_buf, '=');  // Read until '=' (can contain trailing whitespace)
        line_stream.ignore(1, '=');    // Discard '='
        line_stream >> std::ws;        // Discard leading whitespace
        line_stream.get(value_buf);     // Read until EOL (can contain trailing whitespace)

        auto key = key_buf.str();
        auto value = value_buf.str();

        // Remove trailing whitespace from both key and value
        auto is_space = [](int ch) { return std::isspace(ch); };

        auto key_trailing_ws = std::find_if_not(key.rbegin(), key.rend(), is_space);
        if (key_trailing_ws != key.rend())
        {
          key = std::string(key.begin(), key_trailing_ws.base());
        }

        auto value_trailing_ws = std::find_if_not(value.rbegin(), value.rend(), is_space);
        if (value_trailing_ws != value.rend())
        {
          value = std::string(value.begin(), value_trailing_ws.base());
        }

        // Verify that it is a key-value pair line
        if (key.length() == 0 || value.length() == 0)
        {
          std::ostringstream error_message_stream;
          error_message_stream << line_number << ": Invalid key-value pair";

          result.m_error = true;
          result.m_error_message = error_message_stream.str();
          return result;
        }

        // Verify that the key isn't already read
        if (current_section->count(key) == 1)
        {
          std::ostringstream error_message_stream;
          error_message_stream << line_number << ": Key \"" << key << "\" read multiple times";

          result.m_error = true;
          result.m_error_message = error_message_stream.str();
          return result;
        }

        current_section->insert(std::make_pair(key, value));
      }
    }

    return result;
  }

 private:
  ConfigParser() = default;

  bool m_error{false};
  std::string m_error_message;

  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::string>> m_values;
};

}  // namespace utils

#endif  // UTILS_EXPORT_CONFIG_PARSER_H_
