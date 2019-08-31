let login = null;
let game = null;

function core_login() {
  login = new Login();
}

function core_game(name, url) {
  game = new Game(name, url);
}
