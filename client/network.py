import binascii
import socket
import struct

class Connection():
    def __init__(self):
        self.sock = None

    def connect(self):
        if self.sock is None:
            self.sock = socket.create_connection(('127.0.0.1', 7172))
            self.sock.setblocking(0)

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None

    def send_packet(self, packet):
        if self.sock is None:
            raise Exception

        packet_header = struct.pack('<H', len(packet.data))

        #print("send_packet: header: {}".format(binascii.hexlify(packet_header)))
        #print("send_packet: packet: {}".format(binascii.hexlify(packet.data)))

        self.sock.sendall(packet_header)
        self.sock.sendall(packet.data)

    def recv_packet(self):
        if self.sock is None:
            raise Exception

        try:
            packet_header_bytes = self.sock.recv(2)
            packet_header, = struct.unpack('<H', packet_header_bytes)
            packet_bytes = self.sock.recv(packet_header)
        except Exception:
            return None

        #print("recv_packet: header: {}".format(binascii.hexlify(packet_header_bytes)))
        #print("recv_packet: packet: {}".format(binascii.hexlify(packet_bytes)))

        return IncomingPacket(packet_bytes)

class OutgoingPacket():
    def __init__(self):
        self.data = b''

    def add_u8(self, value):
        self.data += struct.pack('<B', value)

    def add_u16(self, value):
        self.data += struct.pack('<H', value)

    def add_u32(self, value):
        self.data += struct.pack('<I', value)

    def add_string(self, string):
        string_bytes = bytearray(string, 'ascii')
        self.add_u16(len(string_bytes))
        self.data += string_bytes

class IncomingPacket():
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def peek_u8(self):
        value, = struct.unpack('<B', self.data[self.pos:self.pos+1])
        return value

    def get_u8(self):
        value = self.peek_u8()
        self.pos += 1
        return value

    def peek_u16(self):
        value, = struct.unpack('<H', self.data[self.pos:self.pos+2])
        return value

    def get_u16(self):
        value = self.peek_u16()
        self.pos += 2
        return value

    def peek_u32(self):
        value, = struct.unpack('<I', self.data[self.pos:self.pos+4])
        return value

    def get_u32(self):
        value = self.peek_u32()
        self.pos += 4
        return value

    def get_string(self):
        string_len = self.get_u16()
        string = self.data[self.pos:self.pos+string_len].decode('ascii')
        self.pos += string_len
        return string

    def skip(self, n):
        self.pos += n
