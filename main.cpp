#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <deque>
#include <climits>
#include <chrono>
#include <thread>
#include <getopt.h>
#include <fstream>
#include <cstdio>

using uint = unsigned int;
using ushort = unsigned short;
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
  update(std::vector<uchar>& inputs, std::vector<uchar>& outputs, bool cycleStart = false) {
    switch (type) {
      // starter blocks
      case 's': // source (Note: in the original game there should be only one white soure.)
        out[dir] = color;
        break;
      case 'b': // buffer
        if(cycleStart) {
          out[dir] = color;
        } else {
          color = in[dir];
        }
        break;
      // normal blocks:
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
      case 'i': // input
        for (uchar i = 0; i < 4; ++i) {
          out[i] = in[i] & inputs[dir];
        }
        break;
      case 'o': // output
        outputs[dir] = 0;
        for (uchar i = 0; i < 4; ++i) {
          outputs[dir] |= in[i];
        }
        break;
      default: // empty
        out = in;
        break;
    }
  }
};

const std::vector<uchar> starters {'s', 'b', 'i'};

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
  update(std::deque<uint>& cIds, std::deque<Tile>& cTiles, std::vector<uchar>& inputs, std::vector<uchar>& outputs, bool cycleStart = false) {
    uint x = cIds.size();
    for (uint i = 0; i < x; ++i) {
      Tile& t = cTiles.front();
      t.update(inputs, outputs, cycleStart);
      std::vector<uint> n = neighbours(cIds.front());
      for (uint j = 0; j < 4; ++j) {
        if (n[j] != MAX && t.out[j] != v[cIds.front()].out[j]) {
          auto newBegin = cIds.begin() + (x - i);
          auto k = std::find(newBegin, cIds.end(), n[j]);
          if (k == cIds.end()) {
            cIds.push_back(n[j]);
            cTiles.push_back(v[n[j]]);
          }
          cTiles[k - cIds.begin()].in[j] = t.out[j];
        }
      }
      v[cIds.front()] = t;
      cIds.pop_front();
      cTiles.pop_front();
    }
  }

  void
  draw(bool clear = false) {
    if (clear) {
      std::cout << "\r\e[A";
      for (uint i = 0; i < h; ++i) {
        std::cout << "\e[A\e[A";
      }
    }
    const char fmtH[] = "  \033[1;3%um|\033[0m ";
    const char fmtV[] = "\033[1;3%um—\033[0m";
    for (uint i = 0; i < w; ++i) {
      std::printf(fmtH, v[i].out[0]);
    }
    std::cout << '\n';
    for (uint y = 0; y < h; ++y) {
      std::printf(fmtV, v[y * w].out[3]);
      for (uint i = y * w; i < (y + 1) * w; ++i) {
        std::cout << v[i].type << (uint)v[i].dir << (uint)v[i].color;
        if (i % w != w - 1) {
          std::printf(fmtV, v[i].out[1] | v[i + 1].out[3]);
        }
      }
      std::printf(fmtV, v[(y + 1) * w - 1].out[1]);
      std::cout << '\n';
      if (y != h - 1) {
        for (uint i = y * w; i < (y + 1) * w; ++i) {
          std::printf(fmtH, v[i].out[2] | v[i + w].out[0]);
        }
        std::cout << '\n';
      }
    }
    for (uint i = (h - 1) * w; i <  h * w; ++i) {
      std::printf(fmtH, v[i].out[2]);
    }
    std::cout << std::endl;
  }
};

int
main(int argc, char* argv[]) {
  int opt;
  std::ifstream boardFile;
  std::vector<std::ifstream> inFiles;
  std::vector<std::ofstream> outFiles;
  uint cycles = 1, delay = 100;

  bool optErrors = false, useFile = false, animate = false, quiet = false;

  while ((opt = getopt(argc, argv, "b:i:o:c:ad:q")) != -1) {
    switch (opt) {
      case 'b':
        boardFile = std::ifstream(optarg);
        useFile = true;
        break;
      case 'i':
        inFiles.push_back(std::ifstream(optarg));
        break;
      case 'o':
        outFiles.push_back(std::ofstream(optarg, std::ios::trunc));
        break;
      case 'c':
        cycles = atoi(optarg);
        break;
      case 'a':
        animate = true;
        break;
      case 'd':
        delay = atoi(optarg);
        break;
      case 'q':
        quiet = true;
        break;
      default:
        optErrors = true;
        break;
    }
  }
  
  if (optErrors) {
    return 1;
  }
  
  Board board(useFile ? boardFile : std::cin);

  std::deque<uint> startIds;
  for (uint i = 0; i < board.size(); ++i) {
    if (isStarter(board[i].type)) {
      startIds.push_back(i);
    }
  }

  std::vector<uchar> inputs(inFiles.size()), outputs(outFiles.size());
  for (uint cycle = 0; cycle < cycles; ++cycle) {
    for (uint i = 0; i < inFiles.size(); ++i) {
      inFiles[i] >> inputs[i];
      inputs[i] -= '0';
    }

    std::deque<uint> cIds = startIds;
    std::deque<Tile> cTiles;
    for (uint i = 0; i < startIds.size(); ++i) {
      cTiles.push_back(board[startIds[i]]);
    }
    board.update(cIds, cTiles, inputs, outputs, true);
    
    bool drawnOne = false;
    while (!cIds.empty()) {
      if (animate && !quiet) {
        board.draw(drawnOne);
        drawnOne = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
      }
      board.update(cIds, cTiles, inputs, outputs);
    }
    if (!quiet) {
      board.draw(drawnOne);
    }

    for (uint i = 0; i < outFiles.size(); ++i) {
      outFiles[i] << + outputs[i];
    }
  }
  for (uint i = 0; i < outFiles.size(); ++i) {
    outFiles[i] << '\n';
  }

  return 0;
}
