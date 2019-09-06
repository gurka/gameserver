import { Client } from './client.mjs';
import { OutgoingPacket } from './packet.mjs';

export class Game {
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
    while (!packet.isEmpty()) {
      const type = packet.getU8();
      if (type == 0x0A) {
        this.handleLoginPacket(packet);
      } else if (type == 0x14) {
        const reason = packet.getString();
        console.log("Could not login: " + reason);
      } else if (type == 0x64) {
        this.handleFullMapPacket(packet);
      } else if (type == 0x83) {
        this.handleMagicEffect(packet);
      } else if (type == 0xA0) {
        this.handlePlayerStats(packet);
      } else if (type == 0x82) {
        this.handleWorldLight(packet);
      } else if (type == 0xA1) {
        this.handlePlayerSkills(packet);
      } else if (type == 0x78 || type == 0x79) {
        this.handleEquipmentUpdate(type == 0x78, packet);
      } else {
        console.log("Unknown packet type: " + type + ", skipping rest of this packet");
        return;
      }
    }
  }

  handleLoginPacket(packet) {
    this.playerId = packet.getU32();
    this.serverBeat = packet.getU16();
  }

  handleFullMapPacket(packet) {
    this.playerPosition = packet.getPosition();

    // Parse map (server will send map data from (x - 8, y - 6, z) to and including (x + 9, y + 7, z), where x, y, z is playerPosition
    // e.g. if playerPosition = (8, 6, 0) then it will send from (0, 0, 0) to and including (17, 13, 0)
    // The known map is therefore size (18, 14)
    const mapSizeX = 18;
    const mapSizeY = 14;
    this.map = Array(mapSizeX).fill().map(() => Array(mapSizeY).fill());

    // Assume that we always are on z=7
    var skip = 0;
    for (var z = 7; z >= 0; z--) {
      for (var x = 0; x < mapSizeX; x++) {
        for (var y = 0; y < mapSizeY; y++) {

          if (skip > 0) {
            skip -= 1;
            continue;
          }

          // Parse tile
          for (var stackpos = 0; stackpos < 255; stackpos++) {

            // Check if tile is done, and read how many tiles to skip next
            if (packet.peekU16() >= 0xFF00) {
              skip = packet.getU16() & 0xFF;
              break;
            }

            if (stackpos > 10) {
              console.log("Too many things :(");
              return;
            }

            // Parse thing
            if (packet.peekU16() == 0x0061 || packet.peekU16() == 0x0062) {
              var creatureId = 0;
              var creatureName = "";

              if (packet.getU16() == 0x0061) {
                // New creature
                packet.getU32();  // old creature to remove
                creatureId = packet.getU32();
                creatureName = packet.getString();
              } else if (packet.getU16() == 0x0062) {
                // Known creature
                creatureId = packet.getU32();
              }

              console.log("Parsed creature with id " + creatureId + " and name " + creatureName);

              packet.getU8();  // health %
              packet.getU8();  // direction
              packet.getU8();  // outfit
              packet.getU8();  // outfit
              packet.getU8();  // outfit
              packet.getU8();  // outfit
              packet.getU8();  // outfit
              packet.getU16();  // Unknown (0x00 0xDC)
              packet.getU16();  // speed

            } else {
              // Item
              const itemTypeId = packet.getU16();  // itemTypeId
              // assume not stackable or multitype, otherwise we need to read one more byte

              if (stackpos == 0) {
                // Ground item, save in map
                this.map[x][y] = itemTypeId;
              }
            }
          }
        }
      }
    }
  }

  handleMagicEffect(packet) {
    packet.getPosition();
    packet.getU8();
  }

  handlePlayerStats(packet) {
    packet.getU16();  // health
    packet.getU16();  // max health
    packet.getU16();  // cap
    packet.getU32();  // exp
    packet.getU8();   // level
    packet.getU16();  // mana
    packet.getU16();  // max mana
    packet.getU8();   // magic level
  }

  handleWorldLight(packet) {
    packet.getU8();  // intensity
    packet.getU8();  // color
  }

  handlePlayerSkills(packet) {
    for (var skill = 0; skill < 7; skill++) {
      packet.getU8();
    }
  }

  handleEquipmentUpdate(empty, packet) {
    packet.getU8();  // index
    if (!empty) {
      packet.getU16();  // itemTypeId
      // assume not stackable or multitype
    }
  }
}
