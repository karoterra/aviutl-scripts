export class Reversi {
  static Width = 8;
  static Height = 8;

  static Empty = 0;
  static Black = 1;
  static White = 2;

  static diff = [
    [-1, 0],
    [-1, -1],
    [0, -1],
    [1, -1],
    [1, 0],
    [1, 1],
    [0, 1],
    [-1, 1]
  ];

  constructor() {
    this.order = false;
    this.field = new Array(Reversi.Height);
    for (let y = 0; y < Reversi.Height; y++) {
      this.field[y] = new Array(Reversi.Width).fill(Reversi.Empty);
    }
    this.field[3][3] = Reversi.White;
    this.field[4][4] = Reversi.White;
    this.field[3][4] = Reversi.Black;
    this.field[4][3] = Reversi.Black;
  }

  takeTurn() {
    this.order = !this.order;
  }

  getColor() {
    return this.order ? Reversi.White : Reversi.Black;
  }

  getRivalColor() {
    return this.order ? Reversi.Black : Reversi.White;
  }

  canReverse(x, y, dir) {
    const color = this.getColor();
    const rival = this.getRivalColor();
    const dx = Reversi.diff[dir][0], dy = Reversi.diff[dir][1];
    let nx = x + dx, ny = y + dy;
    if (nx < 0 || Reversi.Width <= nx || ny < 0 || Reversi.Height <= ny) {
      return false;
    }

    if (this.field[ny][nx] !== rival) {
      return false;
    }
    nx += dx;
    ny += dy;
    while (0 <= nx && nx < Reversi.Width && 0 <= ny && ny < Reversi.Height) {
      if (this.field[ny][nx] === Reversi.Empty) {
        return false;
      }
      if (this.field[ny][nx] === color) {
        return true;
      }
      nx += dx;
      ny += dy;
    }
    return false;
  }

  reverse(x, y, dir) {
    if (!this.canReverse(x, y, dir)) {
      return false;
    }

    const color = this.getColor();
    const dx = Reversi.diff[dir][0], dy = Reversi.diff[dir][1];
    let nx = x + dx, ny = y + dy;
    while (this.field[ny][nx] !== color) {
      this.field[ny][nx] = color;
      nx += dx;
      ny += dy;
    }

    return true;
  }

  put(x, y) {
    if (this.field[y][x] !== Reversi.Empty) {
      return false;
    }

    let reversed = false;
    for (let i = 0; i < Reversi.diff.length; i++) {
      if (this.reverse(x, y, i)) {
        reversed = true;
      }
    }
    if (!reversed) {
      return false;
    }

    this.field[y][x] = this.getColor();
    this.takeTurn();
    return true;
  }

  pass() {
    this.takeTurn();
  }
}
