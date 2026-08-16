// Host-Test-Stub: minimaler Arduino-Ersatz, um RotelCommand.h zu testen
#pragma once
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <deque>

static unsigned long g_millis = 0;
inline unsigned long millis() { return g_millis; }

#define LOG0(...) do{}while(0)
#define LOG1(...) do{}while(0)

#define SERIAL_8N1 0x800001c

inline size_t strlcpy(char *dst, const char *src, size_t size) {
  size_t srclen = strlen(src);
  if (size) {
    size_t n = (srclen >= size) ? size - 1 : srclen;
    memcpy(dst, src, n);
    dst[n] = '\0';
  }
  return srclen;
}

// Fake UART: rxFeed() schiebt Bytes rein, txLog sammelt gesendete Strings
struct HardwareSerial {
  int num;
  std::deque<char> rx;
  std::string txRaw;
  std::vector<std::string> txFrames;

  explicit HardwareSerial(int n) : num(n) {}
  void setRxBufferSize(size_t) {}
  void begin(unsigned long b, uint32_t = 0, int = -1, int = -1) { baud = b; }
  unsigned long baud = 0;

  int available() { return (int)rx.size(); }
  int read() { if (rx.empty()) return -1; char c = rx.front(); rx.pop_front(); return (unsigned char)c; }
  void print(const char *s) { txRaw += s; txFrames.push_back(std::string(s)); }
  void write(uint8_t c) { txRaw += (char)c; }

  void feed(const char *s) { for (const char *p = s; *p; ++p) rx.push_back(*p); }
};

// Preferences-Stub (NVS)
class Preferences {
public:
  bool begin(const char *, bool = false) { return true; }
  void end() {}
  uint8_t getUChar(const char *, uint8_t def = 0) { return def; }
  void putUChar(const char *, uint8_t) {}
  void remove(const char *) {}
};
