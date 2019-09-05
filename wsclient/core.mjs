import { Login } from './login.mjs';
import { Game } from './game.mjs';

document.getElementById("login_button").addEventListener('click', core_login);

export function core_login() {
  new Login((res) => {
    if ("error" in res) {
      document.getElementById("login_error").innerHTML = res["error"];
    } else {
      let chars_el = document.getElementById("characters");

      chars_el.appendChild(document.createTextNode("MOTD:"));
      chars_el.appendChild(document.createTextNode(res["motd"]));
      chars_el.appendChild(document.createElement("br"));

      chars_el.appendChild(document.createTextNode("Characters:"));
      chars_el.appendChild(document.createElement("br"));

      let chars = res["characters"];
      for (let i = 0; i < chars.length; i++) {
        let char_el = document.createElement("button");
        char_el.innerHTML = chars[i]["name"] + " (" + chars[i]["world"] + ")";
        char_el.onclick = function() {
          core_game(chars[i]["name"], chars[i]["url"]);
        };
        chars_el.appendChild(char_el);
        chars_el.appendChild(document.createElement("br"));
      }

      chars_el.appendChild(document.createTextNode("Premium days: " + res["prem_days"]));
      chars_el.appendChild(document.createElement("br"));

      let cancel_el = document.createElement("button");
      cancel_el.innerHTML = "Cancel";
      cancel_el.onclick = function() {
        let chars_el = document.getElementById("characters");
        while (chars_el.firstChild) {
          chars_el.removeChild(chars_el.firstChild);
        }
        chars_el.hidden = true;
        document.getElementById("login").hidden = false;
      };
      chars_el.appendChild(cancel_el);

      document.getElementById("login").hidden = true;
      chars_el.hidden = false;
    }
  });
}

export function core_game(name, url) {
  let password = document.getElementById("login_password").value;
  new Game(name, password, url);
}
