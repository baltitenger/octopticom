#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <climits>

using uint = unsigned int;
using uchar = unsigned char;

const uint MAX = UINT_MAX;

struct Tile {
  uchar type;
  uchar dir;
  uchar color;
  std::array<uchar, 4> in, out;

  Tile()
    : type{'.'}, dir{0}, color{0}, in{0}, out{0} {
  }

  void
  update() {
    switch (type) {
      case 's': // source (Note: in the original game there should be only one white soure.)
        out[dir] = color;
        break;
      case 'f': // filter (& block)
        for (uchar i = 0; i < 4; ++i) {
          out[i] = in[i] & color;
        }
        break;
      case 'm': // filter / double mirror (color is what gets reflected)
        for (uchar i = dir % 2; i < 4; i += 2) {
          out[i]           = (in[i] & ~color) | (in[(i + 1) % 4] &  color);
          out[(i + 1) % 4] = (in[i] &  color) | (in[(i + 1) % 4] & ~color);
        }
        break;
      case 'M': // let light pass, mirror color (white: beam splitter) Note: colors other than white are not originally supported by the game.
        for (uchar i = dir % 2; i < 4; i += 2) {
          out[i]           =  in[i]          | (in[(i + 1) % 4] & color);
          out[(i + 1) % 4] = (in[i] & color) |  in[(i + 1) % 4];
        }
        break;
      case 'b': // buffer
        break;
      case 't': //tunnel
        for (uchar i = dir % 2; i < 4; i += 2) {
          out[i] = in[(i + 2) % 4];
        }
        break;
      case '?': // type 1 conditional
        if (in[dir] == 0) {
          out[(dir + 1) % 4] = out[(dir - 1) % 4] = 0;
        } else {
          out[(dir + 1) % 4] = in[(dir + 1) % 4];
          out[(dir - 1) % 4] = in[(dir - 1) % 4];
        }
        break;
      case ':': // type 2 conditional
        if (in[dir] != 0) {
          out[(dir + 1) % 4] = out[(dir - 1) % 4] = 0;
        } else {
          out[(dir + 1) % 4] = in[(dir + 1) % 4];
          out[(dir - 1) % 4] = in[(dir - 1) % 4];
        }
        break;
      default: // empty
        out = in;
        break;
    }
  }
};

const std::vector<uchar> starters {'s', 'b'};

bool
isStarter(uchar type) {
  return std::find(starters.begin(), starters.end(), type) != starters.end();
}

struct Board {
  uint w, h;
  std::vector<Tile> v;

  Board(uint w, uint h)
    : w(w), h(h), v(w * h) {
  }

  Board(std::istream& is) {
    is >> w >> h;
    v.resize(w * h);
    for (uint i = 0; i < w * h; ++i) {
      uchar type, dir, color;
      is >> type >> dir >> color;
      v[i].type = type;
      if (dir != '.') {
        v[i].dir = dir - '0';
      }
      if (color != '.') {
        v[i].color = color - '0';
      }
    }
  }

  Tile&
  operator[](uint i) {
    return v[i];
  }

  uint
  size() {
    return w * h;
  }

  std::vector<uint>
  neighbours(uint i) {
    std::vector<uint> res (4, -1);
    const std::vector<uint> x {i - w, i + 1, i + w, i - 1};
    for (uint j = 0; j < 4; ++j) {
      uint n = x[j];
      if (n < size() && ((i % w == n % w) || (i / w == n / w))) {
        res[j] = n;
      }
    }
    return res;
  }

  void
  update(std::queue<uint>& changed) {
    Tile& t = v[changed.front()];
    t.update();
    auto n = neighbours(changed.front());
    for (uint i = 0; i < 4; ++i) {
      if (n[i] != MAX && v[n[i]].in[i] != t.out[i]) {
        changed.push(n[i]);
        v[n[i]].in[i] = t.out[i];
      }
    }
    changed.pop();
  }

  void
  draw() {
    for (uint i = 0; i < w; ++i) {
      std::cout << "  \033[1;" << (v[i].out[0] + 30) << "m|\033[0m ";
    }
    std::cout << '\n';
    for (uint y = 0; y < h; ++y) {
      std::cout << "\033[1;" << (v[y * w].out[3] + 30) << "m-\033[0m";
      for (uint i = y * w; i < (y + 1) * w; ++i) {
        std::cout << v[i].type << (uint)v[i].dir << (uint)v[i].color;
        if (i % w != w - 1) {
          std::cout << "\033[1;" << ((v[i].out[1] | v[i + 1].out[3]) + 30) << "m-\033[0m";
        }
      }
      std::cout << "\033[1;" << (v[(y + 1) * w - 1].out[1] + 30) << "m-\033[0m\n";
      if (y == h - 1) {
        break;
      }
      for (uint i = y * w; i < (y + 1) * w; ++i) {
        std::cout << "  \033[1;" << ((v[i].out[2] | v[i + w].out[0]) + 30) << "m|\033[0m ";
      }
      std::cout << '\n';
    }
    for (uint i = (h - 1) * w; i <  h * w; ++i) {
      std::cout << "  \033[1;" << (v[i].out[2] + 30) << "m|\033[0m ";
    }
    std::cout << '\n';
  }
};

int
main() {
  Board board(std::cin);

  std::queue<uint> changed;
  for (uint i = 0; i < board.size(); ++i) {
    if (isStarter(board[i].type)) {
      changed.push(i);
    }
  }

  while (changed.size() > 0) {
    board.update(changed);
  }
  board.draw();

  return 0;
}
