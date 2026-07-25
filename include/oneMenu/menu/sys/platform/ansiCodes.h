#pragma once

static constexpr int ESCAPE= 0x1B;
static constexpr int BRACE= '[';

//color codes will be added a delta to be foreground or background
static constexpr int BLACK= 0;
static constexpr int RED= 1;
static constexpr int GREEN= 2;
static constexpr int YELLOW= 3;
static constexpr int BLUE= 4;
static constexpr int MAGENTA= 5;
static constexpr int CYAN= 6;
static constexpr int WHITE= 7;

//this command will go raw
static constexpr int BOLD_ON= 1;
static constexpr int BOLD_OFF= 22;
static constexpr int UNDERLINE_ON= 4;
static constexpr int UNDERLINE_OFF= 24;
static constexpr int ITALICS_ON= 3;
static constexpr int ITALICS_OFF= 23;
static constexpr int STRIKETHROUGH_ON= 9;
static constexpr int STRIKETHROUGH_OFF= 29;
static constexpr int INVERSE_ON= 7;
static constexpr int INVERSE_OFF= 27;
// RESET removed 2026-07-25: declared but never referenced anywhere in
// OneMenu, and collides with STM32 CMSIS's own `enum FlagStatus { RESET,
// SET }` (stm32f1xx.h) the moment a real STM32duino sketch includes both
// <SPI.h>/<Arduino.h> and <menu.h> in the same TU — same class of bare-
// global-constant collision as feedback_ansi_codes_global_namespace (that
// one was BLACK/WHITE colliding with vendor GFX headers, fixed per-port via
// #undef since those were macros; this one can't be #undef'd, since neither
// side is a macro — a real redeclaration conflict, not a preprocessor one).
static constexpr int DEFAULT_FOREGROUND= 39;
static constexpr int DEFAULT_BACKGROUND= 49;

