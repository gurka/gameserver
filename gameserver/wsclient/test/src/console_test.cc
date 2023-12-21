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

#include "../../src/console.h"

#include "gtest/gtest.h"

namespace wsclient
{

TEST(ConsoleTest, test)
{
  Console console;

  bool testCalled = false;
  std::string testCalledArgs;
  console.addCommand("test", [&](const std::string& args)
  {
    testCalled = true;
    testCalledArgs = args;
    return "OK";
  });

  std::string input = "test foo bar";
  for (auto c : input)
  {
    console.addInput(c);
  }
  console.executeInput();

  ASSERT_TRUE(testCalled);
  ASSERT_EQ("foo bar", testCalledArgs);
  ASSERT_EQ(2u, console.getHistory().size());
  ASSERT_TRUE(console.getInput().empty());
}

}  // namespace wsclient
