class IncomingPacket {
  constructor(buffer, length) {
    this.buffer = buffer;
    this.length = length;
    this.pos = 0;
  }

  getU8() {
    let val = this.buffer[this.pos];
    this.pos += 1;
    return val;
  }

  getU16() {
    let val = this.getU8();
    val |= (this.getU8() << 8);
    return val;
  }

  getU32() {
    let val = this.getU8();
    val |= (this.getU8() << 8);
    val |= (this.getU8() << 16);
    val |= (this.getU8() << 24);
    return val;
  }

  getString() {
    let len = this.getU16();
    let str = String.fromCharCode.apply(null, this.buffer.slice(this.pos, this.pos + len));
    this.pos += len;
    return str;
  }

  skipBytes(n) {
    this.pos += n;
  }
}

class OutgoingPacket {
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
