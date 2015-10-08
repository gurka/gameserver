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
    return ok_;
  }

  std::string getErrorMessage() const
  {
    return errorMessage_;
  }

  static ConfigParser parse(const std::string& fileName)
  {
    ConfigParser result;

    // Open file
    std::ifstream fileStream(fileName);
    if (!fileStream.is_open())
    {
      result.ok_ = false;
      result.errorMessage_ = "Could not open file: " + fileName;
      return result;
    }

    std::unordered_map<std::string, std::string>* currentSection = nullptr;

    // Read lines
    std::string line;
    auto lineNumber = 0;
    while (std::getline(fileStream, line))
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
          errorMessageStream << fileName << ":" << lineNumber << ": Invalid section name (empty)";

          LOG_ERROR("%s", errorMessageStream.str().c_str());

          result.ok_ = false;
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
        errorMessageStream << fileName << ":" << lineNumber << ": Key-value pair without section";

        LOG_ERROR("%s", errorMessageStream.str().c_str());

        result.ok_ = false;
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
          errorMessageStream << fileName << ":" << lineNumber << ": Invalid key-value pair";

          LOG_ERROR("%s", errorMessageStream.str().c_str());

          result.ok_ = false;
          result.errorMessage_ = errorMessageStream.str();
          return result;
        }

        // Verify that the key isn't already read
        if (currentSection->count(key) == 1)
        {
          std::ostringstream warningMessageStream;
          warningMessageStream << fileName << ":" << lineNumber << ": Warning: key \"" << key << "\" read multiple times";
          LOG_INFO("%s", warningMessageStream.str().c_str());
        }

        LOG_DEBUG("Read value {%s} = {%s}", key.c_str(), value.c_str());
        currentSection->insert(std::make_pair(key, value));
      }
    }

    return result;
  }

 private:
  ConfigParser()
    : ok_(true)
  {
  }

  bool ok_;
  std::string errorMessage_;

  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::string>> values_;
};

#endif  // UTILS_CONFIGPARSER_H_
