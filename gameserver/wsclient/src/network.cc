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
#include "network.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cstdint>
#include <string>
#include <vector>

#include "logger.h"
#include "outgoing_packet.h"
#include "incoming_packet.h"

namespace
{
  emscripten::val ws = emscripten::val::null();
  std::function<void(network::IncomingPacket*)> handle_packet;

  void connect(const std::string& uri)
  {
    ws = emscripten::val::global("WebSocket").new_(emscripten::val(uri));
    ws.set("onopen", emscripten::val::module_property("onopen"));
    ws.set("onmessage", emscripten::val::module_property("onmessage"));
  }

  void send_packet(const network::OutgoingPacket& packet)
  {
    static std::uint8_t header[2];
    header[0] = packet.getLength();
    header[1] = packet.getLength() >> 8;
    ws.call<void>("send", std::string(reinterpret_cast<const char*>(header), 2));
    ws.call<void>("send", std::string(reinterpret_cast<const char*>(packet.getBuffer()), packet.getLength()));
  }

  void onopen(emscripten::val event)
  {
    (void)event;

    // Send login packet
    network::OutgoingPacket packet;
    packet.addU8(0x0A);
    packet.skipBytes(5);
    packet.addString("Alice");
    packet.addString("1");
    send_packet(packet);
  }

  void onmessage(emscripten::val event)
  {
    // Convert to Uint8Array
    auto reader = emscripten::val::global("FileReader").new_();
    reader.call<void>("readAsArrayBuffer", event["data"]);
    reader.call<void>("addEventListener", std::string("loadend"), emscripten::val::module_property("onmessage_buffer"));
  }

  void onmessage_buffer(emscripten::val event)
  {
    static std::vector<std::uint8_t> buffer;

    const auto str = event["target"]["result"].as<std::string>();
    buffer.insert(buffer.end(),
                  reinterpret_cast<const std::uint8_t*>(str.data()),
                  reinterpret_cast<const std::uint8_t*>(str.data() + str.length()));

    //LOG_INFO("%s: BEFORE: %d AFTER: %d ADDED: %d", __func__, buffer.size() - str.length(), buffer.size(), str.length());

    // Check if we have enough for next packet
    while (buffer.size() >= 2u)
    {
      const auto next_packet_length = buffer[0] | (buffer[1] << 8);
      //LOG_INFO("%s: next_packet_length: %d", __func__, next_packet_length);
      if (buffer.size() >= 2u + next_packet_length)
      {
        network::IncomingPacket packet(buffer.data() + 2, next_packet_length);
        handle_packet(&packet);
        buffer.erase(buffer.begin(), buffer.begin() + 2u + next_packet_length);
      }
      else
      {
        break;
      }
    }

    //LOG_INFO("%s: AFTER HANDLE: %d", __func__, buffer.size());
  }

  EMSCRIPTEN_BINDINGS(websocket_callbacks)
  {
    emscripten::function("onopen", &onopen);
    emscripten::function("onmessage", &onmessage);
    emscripten::function("onmessage_buffer", &onmessage_buffer);
  }

}

namespace wsclient::network
{

void start(const std::string& uri, const std::function<void(IncomingPacket*)> callback)
{
  handle_packet = callback;
  connect(uri);
}

}  // namespace wsclient::network
