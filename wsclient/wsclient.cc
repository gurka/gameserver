#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "logger.h"
#include "outgoing_packet.h"
#include "incoming_packet.h"

emscripten::val ws = emscripten::val::null();

void handle_packet(IncomingPacket* packet)
{
  LOG_INFO("%s", __func__);

  while (!packet->isEmpty())
  {
    const auto type = packet->getU8();
    if (type == 0x0A)
    {
      const auto reason = packet->getString();
      LOG_INFO("Could not login: %s", reason.c_str());
    }
    else if (type == 0x14)
    {
      const auto motd = packet->getString();
      LOG_INFO("MOTD: %s", motd.c_str());
    }
    else if (type == 0x64)
    {
      const auto num_chars = packet->getU8();
      LOG_INFO("Number of characters: %d", num_chars);
      for (auto i = 0; i < num_chars; i++)
      {
        const auto name = packet->getString();
        const auto world = packet->getString();
        const auto ip = packet->getU32();
        const auto port = packet->getU16();
        LOG_INFO("Character: %s World: %s IP: %d port: %d", name.c_str(), world.c_str(), ip, port);
      }
      const auto prem_days = packet->getU16();
      LOG_INFO("Premium days: %d", prem_days);
    }
    else
    {
      LOG_ERROR("%s: unknown packet type: 0x%X", __func__, type);
      break;
    }
  }
}

void connect()
{
  ws = emscripten::val::global("WebSocket").new_(emscripten::val("ws://192.168.1.4:8171"));
  ws.set("onopen", emscripten::val::module_property("onopen"));
  ws.set("onmessage", emscripten::val::module_property("onmessage"));
}

void send_packet(const OutgoingPacket& packet)
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
  OutgoingPacket packet;
  packet.addU8(0x01);
  packet.addU16(0x1234);
  packet.addU16(0x5678);
  packet.skipBytes(12);
  packet.addU32(1);
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

  LOG_INFO("%s: BEFORE: %d AFTER: %d ADDED: %d", __func__, buffer.size() - str.length(), buffer.size(), str.length());

  // Check if we have enough for next packet
  while (buffer.size() >= 2u)
  {
    const auto next_packet_length = buffer[0] | (buffer[1] << 8);
    LOG_INFO("%s: next_packet_length: %d", __func__, next_packet_length);
    if (buffer.size() >= 2u + next_packet_length)
    {
      IncomingPacket packet(buffer.data() + 2, next_packet_length);
      handle_packet(&packet);
      buffer.erase(buffer.begin(), buffer.begin() + 2u + next_packet_length);
    }
    else
    {
      break;
    }
  }

  LOG_INFO("%s: AFTER HANDLE: %d", __func__, buffer.size());
}

EMSCRIPTEN_BINDINGS(websocket_callbacks)
{
  emscripten::function("onopen", &onopen);
  emscripten::function("onmessage", &onmessage);
  emscripten::function("onmessage_buffer", &onmessage_buffer);
}

int main()
{
  connect();
  return 0;
}
