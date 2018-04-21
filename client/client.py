#!/usr/bin/env python3
import network

CHARACTER_NAME = "Alice"
PASSWORD = "1"

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

    print("Closing connection")
    conn.close()
