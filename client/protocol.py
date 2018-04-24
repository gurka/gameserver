import time

from network import Connection, OutgoingPacket, IncomingPacket
from world import Item, Creature, Tile

class Protocol():
    def __init__(self, client):
        self._client = client
        self._conn = None

    def connected(self):
        return self._conn != None

    def login(self, character_name, password):
        # Connect
        self._conn = Connection()
        self._conn.connect()

        # Send login packet
        login_packet = OutgoingPacket()
        login_packet.add_u8(0x0A)
        login_packet.add_u8(0)
        login_packet.add_u8(0)
        login_packet.add_u16(0)
        login_packet.add_u8(0)
        login_packet.add_string(character_name)
        login_packet.add_string(password)
        self._conn.send_packet(login_packet)

        # Receive login response
        while True:
            packet = self._conn.recv_packet()
            if packet:
                break
            time.sleep(0.1)

        packet_type = packet.get_u8()
        if packet_type == 0x14:
            error_message = packet.get_string()
            print("login: could not login: \"{}\"".format(error_message))
            self._conn.close()
            self._conn = None
            return False

        elif packet_type == 0x0A:
            self._parse_login_packet(packet)
            return True

        else:
            print("login: unknown packet type: {}".format(packet_type))
            self._conn.close()
            self._conn = None
            return False

    def logout(self):
        # Gracefully logout
        logout_packet = OutgoingPacket()
        logout_packet.add_u8(0x14)
        self._conn.send_packet(logout_packet)

        # Just wait for the socket to close by the server
        try:
            while True:
                handle_packet()
        except:
            self._conn = None

    def disconnect(self):
        # Just close the socket
        self._conn.close()
        self._conn = None

    def move(self, direction):
        packet = OutgoingPacket()
        packet.add_u8(0x65 + direction)
        self._conn.send_packet(packet)

    def handle_packet(self):
        packet = self._conn.recv_packet()
        if not packet:
            return False

        packet_type = packet.get_u8()
        #print("parse_packet: skipping unknown packet type: 0x{:02x}".format(packet_type))
        return True

    # Helpers

    def _parse_position(self, packet):
        x = packet.get_u16()
        y = packet.get_u16()
        z = packet.get_u8()
        return x, y, z

    def _parse_item(self, packet):
        item_type_id = packet.get_u16()
        # TODO(simon): parse count if stackable & parse subtype if multitype
        return Item(item_type_id)

    def _parse_creature(self, packet):
        creature_type = packet.get_u16()
        if creature_type == 0x0061:
            # New creature
            packet.get_u32()  # creature ids to remove
            creature_id = packet.get_u32()
            creature_name = packet.get_string()

        elif creature_type == 0x0062:
            # TODO(simon): Handle known creature
            raise Exception()

        else:
            print("Unknown creature_type: {}".format(creature_type))
            raise Exception()

        # Skip health, direction, outfit, 0xDC00 ?? and speed
        packet.skip(11)

        return Creature(creature_id, creature_name)

    def _parse_tile(self, packet):
        tile = Tile()
        tile.ground = self._parse_item(packet)

        for stackpos in range(10):
            if packet.peek_u16() >= 0xFF00:
                packet.get_u16()
                return tile

            elif packet.peek_u16() == 0x0061 or packet.peek_u16() == 0x0062:
                tile.creatures.append(self._parse_creature(packet))

            else:
                tile.items.append(self._parse_item(packet))

        raise Exception()

    def _parse_login_packet(self, packet):
        self._client.player_id = packet.get_u32()

        packet.skip(2)

        tmp = packet.get_u8()
        if tmp != 0x64:
            print("Expected full map (0x64) but got: {}".format(tmp))
            raise Exception()

        self._client.player_position = self._parse_position(packet)
        x, y, z = self._client.player_position
        for x_ in range(x - 9, x + 9):
            for y_ in range(y - 7, y + 7):
                self._client.tiles[(x_, y_)] = self._parse_tile(packet)

        # Skip 12 0xFF, light, magic effect, player stats, light (?), player skills, equipment
