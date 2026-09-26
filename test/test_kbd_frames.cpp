/**
 * @file test_kbd_frames.cpp
 * @brief PCKbd's ESC parsing when the caller polls slower than the ESC timeout. Drivers read one byte per cmd()
 * call, so a caller running one frame per 30+ ms sees ESC, '[' and 'B' in three different frames: a waiting
 * byte must outrank the parser's ESC timeout, or an arrow key arrives as Esc, '[', 'B'. Uses the real
 * UartSerialIn driver, IdxParser and PCKbd over a fake UART, polled every 40 ms (timeout: 30 ms).
 *
 * Native-only, direct g++ (same convention as the other tests here).
 */

#include <cstdio>
#include <cstring>
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/IOP/uartIn.h>
#include <oneMenu/menu/IO/pcKbdIn.h>
#include <oneMenu/menu/IO/idxParser.h>

using namespace oneMenu;

struct FakeUart {
  static inline unsigned char buf[16];
  static inline int n = 0, pos = 0;
  static void feed(const char* s) { n = (int)strlen(s); pos = 0; memcpy(buf, s, n); }
  static bool available() { return pos < n; }
  static int  getch()     { return buf[pos++]; }
};

InDef<UartSerialIn<FakeUart>, IdxParser, PCKbd> in;

static int failures;
#define CHECK(c) do { if (!(c)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static CKE frame() { hw::delay_ms(40); return in.cmd(); }   // one caller frame, slower than the 30 ms ESC timeout

int main() {
  // an arrow key across three slow frames: ESC, '[' and 'B' each wait a frame; not Esc, '[', 'B'
  FakeUart::feed("\x1b[B");
  CHECK(frame().cmd == Cmd::None);
  CHECK(frame().cmd == Cmd::None);
  CKE r = frame();
  CHECK(r.cmd == Cmd::Up);                 // ESC[B is the increment direction (see pcKbdIn.h)
  CHECK(!in.available());

  FakeUart::feed("\x1b[A");
  (void)frame(); (void)frame();
  CHECK(frame().cmd == Cmd::Down);

  // a lone ESC is still delivered, by its timeout, when no byte follows
  FakeUart::feed("\x1b");
  CHECK(frame().cmd == Cmd::None);         // armed
  CHECK(!in.available());                  // the window is still open
  hw::delay_ms(40);
  CHECK(in.available());                   // timed out: the parser reports it is ready
  CHECK(frame().cmd == Cmd::Esc);
  CHECK(!in.available());

  // ESC followed at once by a non-'[' key: Esc first, then that key, in order
  FakeUart::feed("\x1bx");
  CHECK(frame().cmd == Cmd::None);
  r = frame();
  CHECK(r.cmd == Cmd::Esc);
  CHECK(in.queued());                      // the key waits its turn, ahead of any new input
  r = frame();
  CHECK(r.cmd == Cmd::Key && r.key == 'x');
  CHECK(!in.queued());

  // a queued key is not overtaken by bytes that arrive behind it
  FakeUart::feed("\x1bxy");
  (void)frame();
  CHECK(frame().cmd == Cmd::Esc);
  r = frame(); CHECK(r.cmd == Cmd::Key && r.key == 'x');
  r = frame(); CHECK(r.cmd == Cmd::Key && r.key == 'y');

  // digits and Enter are unaffected
  FakeUart::feed("2");
  r = frame(); CHECK(r.cmd == Cmd::Go && r.key == 2);
  FakeUart::feed("\n");
  CHECK(frame().cmd == Cmd::Enter);

  std::printf(failures ? "FAILED (%d)\n" : "OK: PCKbd across slow frames\n", failures);
  return failures != 0;
}
