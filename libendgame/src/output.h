#pragma once

#include <endgame/output.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <termios.h>

struct eg_output {
  FILE *out; ///< output handle to the TTY

  bool active;    ///< has the backing TTY been modified?
  bool debug;     ///< are we currently in debug mode?
  size_t columns; ///< terminal width;
  size_t rows;    ///< terminal height

  struct termios original_termios; ///< state of the terminal prior to init
};
