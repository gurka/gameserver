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

#include "console.h"

namespace wsclient
{

void Console::addCommand(std::string&& keyword, wsclient::Console::Command&& command)
{
  m_commands.emplace(std::move(keyword), std::move(command));
}

void Console::addInput(char c)
{
  m_input += c;
}

void Console::deleteInput(int n)
{
  if (n <= 0) {
    return;
  }

  if (n >= static_cast<int>(m_input.size()))
  {
    m_input.clear();
  }
  else
  {
    m_input.erase(m_input.size() - n);
  }
}

void Console::clearInput()
{
  m_input.clear();
}

void Console::executeInput()
{
  std::string keyword;
  std::string argument;
  const auto first_space = m_input.find_first_of(' ');
  if (first_space == std::string::npos)
  {
    keyword = m_input;
    argument = "";
  }
  else
  {
    keyword = m_input.substr(0, first_space);
    argument = m_input.substr(first_space + 1);
  }

  m_history.emplace_back("$ " + m_input);

  if (m_commands.count(keyword) == 0)
  {
    m_history.emplace_back("Command not found");
  }
  else
  {
    m_history.emplace_back(m_commands[keyword](argument));
  }

  m_input.clear();

  // TODO cap size of m_history
}

}  // namespace wsclient
