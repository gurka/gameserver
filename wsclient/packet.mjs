export class IncomingPacket {
  constructor(buffer, length) {
    this.buffer = buffer;
    this.length = length;
    this.pos = 0;
  }

  peekU8() {
    return this.buffer[this.pos];
  }

  getU8() {
    const val = this.peekU8();
    this.pos += 1;
    return val;
  }

  peekU16() {
    return this.buffer[this.pos] |
          (this.buffer[this.pos + 1] << 8);
  }

  getU16() {
    const val = this.peekU16();
    this.pos += 2;
    return val;
  }

  peekU32() {
    return this.buffer[this.pos] |
          (this.buffer[this.pos + 1] << 8) |
          (this.buffer[this.pos + 2] << 16) |
          (this.buffer[this.pos + 3] << 24);
  }

  getU32() {
    const val = this.peekU32();
    this.pos += 4;
    return val;
  }

  getString() {
    const len = this.getU16();
    const str = String.fromCharCode.apply(null, this.buffer.slice(this.pos, this.pos + len));
    this.pos += len;
    return str;
  }

  skipBytes(n) {
    this.pos += n;
  }

  getPosition() {
    const x = this.getU16();
    const y = this.getU16();
    const z = this.getU8();
    return { "x": x, "y": y, "z": z };
  }

  isEmpty() {
    return this.pos == this.length;
  }
}

export class OutgoingPacket {
  constructor() {
    this.buffer = new Uint8Array(1024);
    this.length = 0;
  }

  addU8(val) {
    this.buffer[this.length] = val;
    this.length += 1;
  }

  addU16(val) {
    this.addU8(val & 0xFF);
    this.addU8(val >> 8);
  }

  addU32(val) {
    this.addU8(val & 0xFF);
    this.addU8((val >> 8) & 0xFF);
    this.addU8((val >> 16) & 0xFF);
    this.addU8((val >> 24) & 0xFF);
  }

  addString(str) {
    this.addU16(str.length);
    for (let i = 0; i < str.length; i++) {
      this.addU8(str.charCodeAt(i));
    }
  }

  addPadding(n) {
    for (let i = 0; i < n; i++) {
      this.addU8(0x00);
    }
  }

  get_buffer() {
    let buffer = new Uint8Array(2 + this.length);
    buffer[0] = this.length & 0xFF;
    buffer[1] = (this.length >> 8) & 0xFF;
    buffer.set(this.buffer.slice(0, this.length), 2);
    return buffer;
  }
}
