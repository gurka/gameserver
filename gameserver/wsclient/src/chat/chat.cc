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

#include "chat.h"

#include <algorithm>

#include "utils/logger.h"

namespace chat
{

void Chat::openChannel(std::uint16_t id, const std::string& name)
{
  m_channels.push_back({ id, name, {} });
}

void Chat::openPrivateChannel(const std::string& name)
{
  // TODO: when do messages go here? when talker == private channel name?
  m_private_channels.push_back({ name, {} });
}

void Chat::message(const std::string& talker, std::uint8_t talk_type, const std::string& text)
{
  m_default_messages.push_back({ talker, talk_type, text });
}

void Chat::message(const std::string& talker, std::uint8_t talk_type, std::uint16_t channel_id, const std::string& text)
{
  auto it = std::find_if(m_channels.begin(),
                         m_channels.end(),
                         [channel_id](const Channel& channel) { return channel.id == channel_id; });
  if (it == m_channels.end())
  {
    LOG_ERROR("%s: could not find channel with id: %d", __func__, channel_id);
    return;
  }

  it->messages.push_back({ talker, talk_type, text });
}

}  // namespace chat