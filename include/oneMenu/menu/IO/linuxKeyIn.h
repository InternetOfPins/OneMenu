/* -*- C++ -*- */
#pragma once

/**
 * @file linuxKeyIn.h
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief POSIX PC Keyboard driver, usefull when running the menu on a native system (linux tested)
 * https://stackoverflow.com/a/448982/1329075
 * @version 5
 * @date 2024-11-13
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include "oneMenu/menu/in.h"

static struct termios orig_termios;

static void reset_terminal_mode() {tcsetattr(0, TCSANOW, &orig_termios);}

static void set_conio_terminal_mode() {
    struct termios new_termios;
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));
    atexit(reset_terminal_mode);
    new_termios.c_lflag &= ~(ECHO|ICANON);
    new_termios.c_cc[VTIME] = 0;
    new_termios.c_cc[VMIN] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

static int kbhit() {
  struct timeval tv = { 0L, 0L };
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  return select(1, &fds, NULL, NULL, &tv);
}

static int getch() {
  unsigned char c;
  int r=read(STDIN_FILENO, &c, sizeof(c));
  return r<0?r:c;
}

/// @brief POSIX system key reader
struct LinuxKeyIn {
  template<typename In>
  struct Part : In {
    inline Part() {set_conio_terminal_mode();}
    inline ~Part() {reset_terminal_mode();}

    static bool available() { return In::available() || kbhit(); }

    oneMenu::CKE cmd() {
      // Drain any pending event from the inner component first.
      if (In::available()) return In::cmd();
      // Otherwise read stdin if ready.
      return kbhit() ? In::parseKey(oneMenu::Key(getch())) : In::cmd();
    }
  };
};
