#pragma once

#include <endgame/input.h>
#include <endgame/io.h>
#include <endgame/output.h>
#include <stdint.h>

struct eg_io {
  eg_input_t *in;
  eg_output_t *out;

  int tick;           ///< current tick, ≤0 for tickless
  uint64_t last_tick; ///< time we last saw a tick event
};
