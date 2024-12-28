#pragma once

#ifndef ENDGAME_API
#ifdef __GNUC__
#define ENDGAME_API /* nothing */
#elif defined(_MSC_VER)
#define ENDGAME_API __declspec(dllimport)
#else
#define ENDGAME_API /* nothing */
#endif
#endif

#include <endgame/screen.h>
