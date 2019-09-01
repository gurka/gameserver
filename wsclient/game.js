class Game {
  constructor(name, password, url) {
    this.name = name;
    this.password = password;
    this.game_client = new Client(url,
                                  () => { this.onOpen(); },
                                  (packet) => { this.packetHandler(packet); });
  }

  onOpen() {
    // Send login packet
    let packet = new OutgoingPacket();
    packet.addU8(0x0A);

    packet.addU8(0x00);  // unknown
    packet.addU8(0x00);  // OS
    packet.addU16(0x00);  // client version
    packet.addU8(0x00);  // unknown

    packet.addString(this.name);
    packet.addString(this.password);

    this.game_client.send(packet.get_buffer());
  }

  packetHandler(packet) {
    let type = packet.getU8();
    if (type == 0x0A) {
      handleLoginPacket(packet);
    } else {
      console.log("Unknown packet type: " + type);
    }
  }

  handleLoginPacket(packet) {
  }
}
