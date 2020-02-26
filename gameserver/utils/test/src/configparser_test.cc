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

#include "config_parser.h"

#include <sstream>

#include "gtest/gtest.h"

namespace utils
{

TEST(ConfigParserTest, ParseValidFile)
{
  // Create valid xml-file stream
  std::stringstream xmlStream;
  xmlStream <<
  "; This is a comment\n"
  "\n"
  "[sectionA]\n"
  "\n"
  "  number = 1\n"
  "  string = foo\n"
  "  boolean = true\n"
  "\n"
  "[sectionB]\n"
  "\ttabs\t=\ttrue\n"
  "nospaces=true\n"
  "\tmix =     true\n"
  "\n"
  "\t\t    ; Some    whitespace   \n"
  "\n";

  // Parse file
  ConfigParser config = ConfigParser::parseStream(&xmlStream);

  // Should be parsed OK
  ASSERT_EQ(config.parsedOk(), true);

  // Valid sections
  ASSERT_EQ(config.hasSection("sectionA"), true);
  ASSERT_EQ(config.hasSection("sectionB"), true);

  // Validate key-values pairs
  ASSERT_EQ(config.getInteger("sectionA", "number", 0), 1);
  ASSERT_EQ(config.getString("sectionA", "string", "invalid"), "foo");
  ASSERT_EQ(config.getBoolean("sectionA", "boolean", false), true);

  ASSERT_EQ(config.getBoolean("sectionB", "tabs", false), true);
  ASSERT_EQ(config.getBoolean("sectionB", "nospaces", false), true);
  ASSERT_EQ(config.getBoolean("sectionB", "mix", false), true);
}

TEST(ConfigParserTest, ParseInvalidFile)
{
  // No value for key
  std::stringstream xmlStreamOne;
  xmlStreamOne <<
  "[sectionA]\n"
  "  invalid =\n";
  ASSERT_EQ(ConfigParser::parseStream(&xmlStreamOne).parsedOk(), false);

  // No equals character
  std::stringstream xmlStreamTwo;
  xmlStreamTwo <<
  "[sectionA]\n"
  "  invalid invalid\n";
  ASSERT_EQ(ConfigParser::parseStream(&xmlStreamTwo).parsedOk(), false);

  // No key for value
  std::stringstream xmlStreamThree;
  xmlStreamThree <<
  "[sectionA]\n"
  "  = invalid\n";
  ASSERT_EQ(ConfigParser::parseStream(&xmlStreamThree).parsedOk(), false);

  // Invalid section
  std::stringstream xmlStreamFour;
  xmlStreamFour <<
  "[]\n";
  ASSERT_EQ(ConfigParser::parseStream(&xmlStreamFour).parsedOk(), false);

  // Key-value pair before section
  std::stringstream xmlStreamFive;
  xmlStreamFive <<
  "  invalid = invalid\n"
  "[sectionA]\n";
  ASSERT_EQ(ConfigParser::parseStream(&xmlStreamFive).parsedOk(), false);
}

}  // namespace utils
