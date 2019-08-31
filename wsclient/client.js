class Client {

  constructor(url, onOpen, packetHandler) {
    this.url = url;
    this.onOpen = onOpen;
    this.packetHandler = packetHandler;

    this.receivedData = new Uint8Array(0);
    this.ws = new WebSocket(url);

    this.ws.onopen = (event) => { this.onOpen(); };
    this.ws.onmessage = (event) => {
      // Convert to Uint8Array and call handleReceivedData
      let reader = new FileReader();
      reader.readAsArrayBuffer(event.data);
      reader.addEventListener("loadend", (event) => {
        this.handleReceivedData(new Uint8Array(event.target.result));
      });
    }
  }

  send(buffer) {
    this.ws.send(buffer);
  }

  handleReceivedData(buffer) {
    // Add data to receivedData
    let tmp = new Uint8Array(this.receivedData.length + buffer.length);
    tmp.set(this.receivedData);
    tmp.set(buffer, this.receivedData.length);
    this.receivedData = tmp;

    // Check if we have enough data to read packet length
    if (this.receivedData.length >= 2) {
      let packet_len = this.receivedData[0] | (this.receivedData[1] << 8);

      // Check if we have enough data for the whole packet
      if (this.receivedData.length - 2 >= packet_len) {
        let packet = new IncomingPacket(this.receivedData.slice(2, packet_len + 2));

        // Delete packet data from receivedData
        this.receivedData = this.receivedData.slice(packet_len + 2);

        // Call packet handler
        this.packetHandler(packet);
      }
    }
  }
}
