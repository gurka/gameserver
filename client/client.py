#!/usr/bin/env python3
import signal
import sys

import network

CHARACTER_NAME = "Alice"
PASSWORD = "1"

THING_ITEM = 0
THING_CREATURE = 1

def create_login_packet(character_name, password):
    # 1: 0x0A
    # 1: unknown
    # 1: client_os
    # 2: client_version
    # 1: unknown
    # s: character_name
    # s: password
    login_packet = network.OutgoingPacket()
    login_packet.add_u8(0x0A)
    login_packet.add_u8(0)
    login_packet.add_u8(0)
    login_packet.add_u16(0)
    login_packet.add_u8(0)
    login_packet.add_string(character_name)
    login_packet.add_string(password)
    return login_packet

def parse_position(packet):
    x = packet.get_u16()
    y = packet.get_u16()
    z = packet.get_u8()
    return x, y, z

def parse_item(packet):
    item_type_id = packet.get_u16()
    # TODO(simon): parse count if stackable & parse subtype if multitype
    return THING_ITEM, item_type_id, "ITEM"

def parse_creature(packet):
    creature_type = packet.get_u16()
    if creature_type == 0x0061:
        # New creature
        packet.get_u32()  # creature ids to remove
        creature_id = packet.get_u32()
        creature_name = packet.get_string()

    elif creature_type == 0x0062:
        # Known creature
        creature_id = packet.get_u32()
        creature_name = "KNOWN_CREATURE"

    else:
        print("Unknown creature_type: {}".format(creature_type))
        raise Exception()

    # Skip health, direction, outfit, 0xDC00 ?? and speed
    packet.skip(11)

    return THING_CREATURE, creature_id, creature_name

def parse_tile(packet):
    tile = []
    tile.append(parse_item(packet))

    for stackpos in range(10):
        if packet.peek_u16() >= 0xFF00:
            packet.get_u16()
            return tile

        elif packet.peek_u16() == 0x0061 or packet.peek_u16() == 0x0062:
            tile.append(parse_creature(packet))

        else:
            tile.append(parse_item(packet))

    return tile

def parse_login(packet):
    player_id = packet.get_u32()

    packet.skip(2)

    tmp = packet.get_u8()
    if tmp != 0x64:
        print("Expected full map (0x64) but got: {}".format(tmp))
        raise Exception()

    position = parse_position(packet)
    tiles = {}
    for x in range(18):
        for y in range(14):
            tiles[(x, y)] = parse_tile(packet)

    # Skip 12 0xFF, light, magic effect, player stats, light (?), player skills, equipment

    return player_id, tiles


if __name__ == '__main__':
    conn = network.Connection()
    conn.connect()
    print("Connected")

    # Send login packet
    login_packet = create_login_packet(CHARACTER_NAME, PASSWORD)
    conn.send_packet(login_packet)
    print("Login packet sent")

    # Receive response
    response_packet = conn.recv_packet()
    print("Received response")

    # Parse login response
    packet_type = response_packet.get_u8()
    if packet_type == 0x14:
        error_message = response_packet.get_string()
        print("Could not login: \"{}\"".format(error_message))

    elif packet_type == 0x0A:
        player_id, tiles = parse_login(response_packet)
        print("Player id: {}".format(player_id))
        print("Tiles:")
        print(tiles)

    else:
        print("Unknown packet type: {}".format(packet_type))

    def signal_handler(*args):
        print("Closing connection")
        conn.close()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    while True:
        conn.recv_packet()

