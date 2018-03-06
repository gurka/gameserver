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

#ifndef UTILS_CONFIGPARSER_H_
#define UTILS_CONFIGPARSER_H_

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "logger.h"

class ConfigParser
{
 public:
  bool hasSection(const std::string& section) const
  {
    return values_.count(section) == 1;
  }

  bool hasValue(const std::string& section, const std::string& key) const
  {
    if (!hasSection(section))
    {
      return false;
    }

    return values_.at(section).count(key) == 1;
  }

  int getInteger(const std::string& section, const std::string& key, int default_ = 0) const
  {
    if (hasValue(section, key))
    {
      return std::stoi(values_.at(section).at(key));
    }
    else
    {
      return default_;
    }
  }

  std::string getString(const std::string& section, const std::string& key, const std::string& default_ = "") const
  {
    if (hasValue(section, key))
    {
      return values_.at(section).at(key);
    }
    else
    {
      return default_;
    }
  }

  bool getBoolean(const std::string& section, const std::string& key, bool default_ = false) const
  {
    if (hasValue(section, key))
    {
      return values_.at(section).at(key) == "true";
    }
    else
    {
      return default_;
    }
  }

  bool parsedOk() const
  {
    return !error_;
  }

  std::string getErrorMessage() const
  {
    return errorMessage_;
  }

  static ConfigParser parseFile(const std::string& fileName)
  {
    // Open file
    std::ifstream fileStream(fileName);
    if (!fileStream.is_open())
    {
      ConfigParser result;
      result.error_ = true;
      result.errorMessage_ = "Could not open file: " + fileName;
      return result;
    }

    ConfigParser result = parseStream(&fileStream);
    if (result.error_)
    {
      auto temp = result.errorMessage_;
      result.errorMessage_ = fileName + ":" + temp;
    }

    return result;
  }

  static ConfigParser parseStream(std::istream* stream)
  {
    ConfigParser result;

    std::unordered_map<std::string, std::string>* currentSection = nullptr;

    // Read lines
    std::string line;
    auto lineNumber = 0;
    while (std::getline(*stream, line))
    {
      lineNumber++;

      std::istringstream lineStream(line);

      // Discard leading whitespace
      lineStream >> std::ws;

      if (lineStream.eof())
      {
        // Empty line
      }
      else if (lineStream.peek() == ';')
      {
        // Comment
      }
      else if (lineStream.peek() == '[')
      {
        // Read section name
        std::stringbuf sectionNameBuf;
        lineStream.ignore(1, '[');                // Discard '['
        lineStream.get(sectionNameBuf, ']');      // Read until ']'
        auto sectionName = sectionNameBuf.str();

        // Get pointer to section's map
        if (sectionName.length() == 0)
        {
          std::ostringstream errorMessageStream;
          errorMessageStream << lineNumber << ": Invalid section name (empty)";

          result.error_ = true;
          result.errorMessage_ = errorMessageStream.str();
          return result;
        }

        LOG_DEBUG("Read section name: {%s}", sectionName.c_str());

        // operator[] creates a new element (if one not already exists)
        // and returns it, we keep the pointer
        currentSection = &(result.values_[sectionName]);
      }
      else if (currentSection == nullptr)
      {
        std::ostringstream errorMessageStream;
        errorMessageStream << lineNumber << ": Key-value pair without section";

        result.error_ = true;
        result.errorMessage_ = errorMessageStream.str();
        return result;
      }
      else
      {
        // Read key-value pair
        std::stringbuf keyBuf;
        std::stringbuf valueBuf;

        lineStream.get(keyBuf, '=');  // Read until '=' (can contain trailing whitespace)
        lineStream.ignore(1, '=');    // Discard '='
        lineStream >> std::ws;        // Discard leading whitespace
        lineStream.get(valueBuf);     // Read until EOL (can contain trailing whitespace)

        auto key = keyBuf.str();
        auto value = valueBuf.str();

        // Remove trailing whitespace from both key and value
        auto isSpace = [](int ch) { return std::isspace(ch); };

        auto keyTrailingWs = std::find_if_not(key.rbegin(), key.rend(), isSpace);
        if (keyTrailingWs != key.rend())
        {
          key = std::string(key.begin(), keyTrailingWs.base());
        }

        auto valueTrailingWs = std::find_if_not(value.rbegin(), value.rend(), isSpace);
        if (valueTrailingWs != value.rend())
        {
          value = std::string(value.begin(), valueTrailingWs.base());
        }

        // Verify that it is a key-value pair line
        if (key.length() == 0 || value.length() == 0)
        {
          std::ostringstream errorMessageStream;
          errorMessageStream << lineNumber << ": Invalid key-value pair";

          result.error_ = true;
          result.errorMessage_ = errorMessageStream.str();
          return result;
        }

        // Verify that the key isn't already read
        if (currentSection->count(key) == 1)
        {
          std::ostringstream errorMessageStream;
          errorMessageStream << lineNumber << ": Key \"" << key << "\" read multiple times";

          result.error_ = true;
          result.errorMessage_ = errorMessageStream.str();
          return result;
        }

        currentSection->insert(std::make_pair(key, value));
      }
    }

    return result;
  }

 private:
  ConfigParser()
    : error_(false)
  {
  }

  bool error_;
  std::string errorMessage_;

  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::string>> values_;
};

#endif  // UTILS_CONFIGPARSER_H_
