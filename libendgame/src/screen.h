#pragma once

#include <endgame/screen.h>
#include <stdbool.h>
#include <stddef.h>
#include <termios.h>

struct eg_screen {
  bool active;    ///< has the backing TTY been modified?
  size_t columns; ///< terminal width;
  size_t rows;    ///< terminal height

  struct termios original_termios; ///< state of the terminal prior to init
};
