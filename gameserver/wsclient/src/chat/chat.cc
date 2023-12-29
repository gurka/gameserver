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
  LOG_INFO("%s: id=%u name=%s", __func__, id, name.c_str());

  if (m_channels.count(id) == 0)
  {
    m_channels[id] = { name, {} };
  }

  m_version += 1;
}

void Chat::openPrivateChannel(const std::string& name)
{
  LOG_INFO("%s: name=%s", __func__, name.c_str());

  // TODO: when do messages go here? when talker == private channel name?
  if (m_private_channels.count(name) == 0)
  {
    m_private_channels[name] = {};
  }

  m_version += 1;
}

void Chat::message(const std::string& talker, std::uint8_t talk_type, const std::string& text)
{
  LOG_INFO("%s: talker=%s talk_type=%u text=%s", __func__, talker.c_str(), static_cast<std::uint16_t>(talk_type), text.c_str());
  m_default_messages.push_back({ talker, talk_type, text });
  m_version += 1;
}

void Chat::message(const std::string& talker, std::uint8_t talk_type, std::uint16_t channel_id, const std::string& text)
{
  LOG_INFO("%s: talker=%s talk_type=%u channel_id=%u text=%s", __func__, talker.c_str(), static_cast<std::uint16_t>(talk_type), channel_id, text.c_str());
  if (m_channels.count(channel_id) == 0)
  {
    LOG_ERROR("%s: could not find channel with id: %d", __func__, channel_id);
    return;
  }

  m_channels[channel_id].messages.push_back({ talker, talk_type, text });
  m_version += 1;
}

}  // namespace chat