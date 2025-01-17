#pragma once

#include <endgame/input.h>
#include <endgame/io.h>
#include <endgame/output.h>

struct eg_io {
  eg_input_t *in;
  eg_output_t *out;
};
