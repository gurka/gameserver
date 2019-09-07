#ifndef NETWORK_H_
#define NETWORK_H_

#include <functional>

class IncomingPacket;

namespace Network
{
  void start(const std::function<void(IncomingPacket*)> callback);
}

#endif  // NETWORK_H_
