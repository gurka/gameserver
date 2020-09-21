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

#include "client_factory.h"

#include <memory>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "emscripten_client_backend.h"
#include "connection_impl.h"

namespace
{

emscripten::val ws = emscripten::val::null();

// Only set during connect procedure
std::unique_ptr<network::EmscriptenClient> pending_client;
network::ClientFactory::Callbacks pending_callbacks;

// Only set while connection is established
network::EmscriptenClient* client = nullptr;

void onopen(emscripten::val)
{
  if (client)
  {
    LOG_ERROR("%s: called but we are already connected", __func__);
    return;
  }

  LOG_INFO("%s: connected", __func__);

  client = pending_client.get();
  auto socket = network::EmscriptenClientBackend::Socket(std::move(pending_client));
  pending_callbacks.on_connected(std::make_unique<network::ConnectionImpl<network::EmscriptenClientBackend>>(std::move(socket)));
}

void onerror(emscripten::val)
{
  if (client)
  {
    LOG_ERROR("%s: called but we are already connected", __func__);
    return;
  }

  LOG_INFO("%s: could not connect", __func__);
  pending_client.reset();
  pending_callbacks.on_connect_failure();
}

void onclose(emscripten::val)
{
  if (client)
  {
    LOG_INFO("%s: connection closed", __func__);
    client->handleClose();
    client = nullptr;
  }
}

void onmessage(emscripten::val event)
{
  if (!client)
  {
    LOG_ERROR("%s: called but we are not connected", __func__);
    return;
  }

  // Convert to Uint8Array
  auto reader = emscripten::val::global("FileReader").new_();
  reader.call<void>("readAsArrayBuffer", event["data"]);
  reader.call<void>("addEventListener", std::string("loadend"), emscripten::val::module_property("onmessage_buffer"));
}

void onmessage_buffer(emscripten::val event)
{
  if (!client)
  {
    LOG_ERROR("%s: called but we are not connected", __func__);
    return;
  }

  const auto str = event["target"]["result"].as<std::string>();
  client->handleMessage(reinterpret_cast<const std::uint8_t*>(str.data()), str.size());
}

}

namespace network
{

bool ClientFactory::createWebsocketClient(const std::string& uri, const Callbacks& callbacks)
{
  // Need to figure out how to bind/map callbacks to different websocket objects
  // to be able to support multiple clients/connections (can we use the event parameter?)
  // Until then only allow one client/connection to be made
  if (ws != emscripten::val::null())
  {
    LOG_ERROR("%s: only one client/connection is currently supported", __func__);
    return false;
  }

  ws = emscripten::val::global("WebSocket").new_(emscripten::val(uri));

  pending_client = std::make_unique<network::EmscriptenClient>(ws);
  pending_callbacks = callbacks;

  // Called when connection has been established
  ws.set("onopen", emscripten::val::module_property("onopen"));

  // Called when an established connection closes or
  // when a connect attempt fails (but then preceded by a call to onerror)
  ws.set("onclose", emscripten::val::module_property("onclose"));

  // Called when a connect attempt fails, and then followed by a call to onclose
  ws.set("onerror", emscripten::val::module_property("onerror"));

  // Fired when data is received through a WebSocket.
  ws.set("onmessage", emscripten::val::module_property("onmessage"));

  return true;
}

EMSCRIPTEN_BINDINGS(websocket_callbacks)
{
  emscripten::function("onopen", &onopen);
  emscripten::function("onerror", &onerror);
  emscripten::function("onclose", &onclose);
  emscripten::function("onmessage", &onmessage);
  emscripten::function("onmessage_buffer", &onmessage_buffer);
}

}  // namespace network
