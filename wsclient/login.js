function int2ip(val) {
  return "" + (val & 0xFF) + "." + ((val >> 8) & 0xFF) + "." + ((val >> 16) & 0xFF) + "." + ((val >> 24) & 0xFF);
}

function handlePacket(packet) {
  // Process packet
  if (packet.getU8() != 0x14) {
    console.log("Unknown packet, expected MOTD");
    return;
  }

  let motd = packet.getString();
  console.log("MOTD: " + motd);

  let login_resp = packet.getU8();
  if (login_resp == 0x0A) {
    let error = packet.getString();
    console.log("Error: " + error);
  } else if (login_resp == 0x64) {
    let characters = document.getElementById("characters");
    characters.appendChild(document.createTextNode("Characters:"));
    characters.appendChild(document.createElement("br"));

    let num_chars = packet.getU8();
    for (let i = 0; i < num_chars; i++) {
      let name = packet.getString();
      let world = packet.getString();
      let url = "ws://" + int2ip(packet.getU32());
      url += ":" + (packet.getU16() + 1000);  // TODO: WS server is normal port + 1000
      console.log(name + " (" + world + ", " + url + ")");

      let character = document.createElement("button");
      character.innerHTML = name;
      character.onclick = function() {
        game(name, url);
      };
      characters.appendChild(character);
      characters.appendChild(document.createElement("br"));
    }

    let prem_days = packet.getU16();
    console.log("Premium days: " + prem_days);

    characters.appendChild(document.createTextNode("Premium days: " + prem_days));
    characters.appendChild(document.createElement("br"));

    let cancel_btn = document.createElement("button");
    cancel_btn.innerHTML = "Cancel";
    cancel_btn.onclick = function() {
      let characters = document.getElementById("characters");
      while (characters.firstChild) {
        characters.removeChild(characters.firstChild);
      }
      characters.hidden = true;
      document.getElementById("login").hidden = false;
    };
    characters.appendChild(cancel_btn);

    document.getElementById("login").hidden = true;
    characters.hidden = false;
  } else {
    console.log("Unknown login response: " + login_resp);
  }
}

let receivedData = new Uint8Array(0);
function handleReceivedData(buffer) {
  // Add data to receivedData
  let tmp = new Uint8Array(receivedData.length + buffer.length);
  tmp.set(receivedData);
  tmp.set(buffer, receivedData.length);
  receivedData = tmp;

  // Check if we have enough data to read packet length
  if (receivedData.length >= 2) {
    let packet_len = receivedData[0] | (receivedData[1] << 8);

    // Check if we have enough data for the whole packet
    if (receivedData.length - 2 >= packet_len) {
      let packet = new IncomingPacket(receivedData.slice(2, packet_len + 2));

      // Delete packet data from receivedData
      receivedData = receivedData.slice(packet_len + 2);

      // Call handlePacket
      handlePacket(packet);
    }
  }
}

function login() {
  let ws = new WebSocket("ws://192.168.1.4:7272");

  ws.onopen = function(event) {
    // Parse login info
    let account = parseInt(document.getElementById("login_account").value);
    let password = document.getElementById("login_password").value;

    if (isNaN(account)) {
      console.log("Invalid account number");
      return;
    }

    // Send login packet
    let packet = new OutgoingPacket();
    packet.addU8(0x01);
    packet.addU16(0x1234);
    packet.addU16(0x5678);
    packet.addPadding(12);
    packet.addU32(account);
    packet.addString(password);
    ws.send(packet.get_buffer());
  };

  ws.onmessage = function(event) {
    // Convert to Uint8Array and call handleReceivedData
    let reader = new FileReader();
    reader.readAsArrayBuffer(event.data);
    reader.addEventListener("loadend", function(e) {
      handleReceivedData(new Uint8Array(e.target.result));
    });
  }
}
