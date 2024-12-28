#pragma once

#include <endgame/screen.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <termios.h>

struct eg_screen {
  FILE *out; ///< output handle to the TTY
  FILE *in;  ///< input handle to the TTY

  bool active;    ///< has the backing TTY been modified?
  size_t columns; ///< terminal width;
  size_t rows;    ///< terminal height

  struct termios original_termios; ///< state of the terminal prior to init
};
