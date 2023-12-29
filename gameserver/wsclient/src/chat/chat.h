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

#ifndef WSCLIENT_SRC_CHAT_CHAT_H_
#define WSCLIENT_SRC_CHAT_CHAT_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace chat
{

class Chat
{
 public:
  struct Message
  {
    std::string talker;
    std::uint8_t talk_type;
    std::string text;
  };

  struct Channel
  {
    std::string name;
    std::vector<Message> messages;
  };

  void openChannel(std::uint16_t id, const std::string& name);
  void openPrivateChannel(const std::string& name);
  void message(const std::string& talker, std::uint8_t talk_type, const std::string& text);
  void message(const std::string& talker, std::uint8_t talk_type, std::uint16_t channel_id, const std::string& text);

  const std::vector<Message>& getDefaultMessages() const { return m_default_messages; }
  const std::unordered_map<std::uint16_t, Channel>& getChannels() const { return m_channels; }
  const std::unordered_map<std::string, std::vector<Message>>& getPrivateChannels() const { return m_private_channels; }
  int getVersion() const { return m_version; }

 private:
  std::vector<Message> m_default_messages;
  std::unordered_map<std::uint16_t, Channel> m_channels;
  std::unordered_map<std::string, std::vector<Message>> m_private_channels;

  int m_version;
};

}  // namespace chat

#endif  // WSCLIENT_SRC_CHAT_CHAT_H_
