#pragma once
#include <stdint.h>
#include <string.h>

/*
  Kaseikyo, NEC42, Pioneer and RCA, rendered to raw IR timings.

  These four are the protocols the Flipper IRDB carries that IRremote has no
  sender for - it covers Kaseikyo, but not with the field split the database
  uses, and RCA, Pioneer and NEC42 not at all. Together they are 2,280 of the
  169,910 parsed buttons in the 14,551-remote database (Kaseikyo 1680, RCA 274,
  Pioneer 189, NEC42 137), and until now neither the remote nor the dock could
  send any of them.

  ONE implementation, compiled into both firmwares from this file. The remote
  hands the result to IrSender.sendRaw() and the dock hands it to RMT, so the
  two cannot drift apart - which is the failure mode that made an RF433 button
  come out of an infrared LED, and that a duplicated encoder would invite back.

  Field packing was derived from the database rather than assumed. Kaseikyo is
  the one worth recording: across all 1,680 buttons the command never exceeds
  0x3FF (exactly the 10-bit data field), bits 8..23 of the address are always a
  vendor id (0x2002 Panasonic, 0x3254 Denon), and bits 24..25 only ever hold
  0..3 - the 2-bit id. The on-air layout and checksum then come from IRremote's
  ir_Kaseikyo.hpp, so a frame built here is the frame that library would build.

  Timings were measured from the database's own raw captures of each family
  rather than taken from memory:
    Kaseikyo  header 3459/1729, unit 432, one space 1297   (n=400 medians)
    Pioneer   header 8446/4207, unit 526, one space 1578   (n=400 medians)
  which round to the published 3456/1728/432/1296 and 8500/4250/550/1700.
*/

// Emits mark/space pairs for a pulse-distance frame, LSB first.
inline void orIrPulseDistanceLsb(uint16_t *out, uint16_t &n, uint16_t cap,
                                 uint64_t data, uint8_t bits, uint16_t mark,
                                 uint16_t zeroSpace, uint16_t oneSpace) {
  for (uint8_t i = 0; i < bits && n + 1 < cap; i++) {
    out[n++] = mark;
    out[n++] = (data >> i) & 1ULL ? oneSpace : zeroSpace;
  }
}

inline void orIrPulseDistanceMsb(uint16_t *out, uint16_t &n, uint16_t cap,
                                 uint32_t data, uint8_t bits, uint16_t mark,
                                 uint16_t zeroSpace, uint16_t oneSpace) {
  for (int8_t i = (int8_t)bits - 1; i >= 0 && n + 1 < cap; i--) {
    out[n++] = mark;
    out[n++] = (data >> i) & 1UL ? oneSpace : zeroSpace;
  }
}

/*
  48 bits, LSB first:
    [0..15] vendor id   [16..19] vendor parity   [20..23] genre1
    [24..27] genre2     [28..37] data (10)       [38..39] id (2)
    [40..47] checksum
  The checksum is the XOR of bytes 2, 3 and 4 of that frame, and the vendor
  parity is the vendor id folded to a nibble - both exactly as IRremote does
  them, so a receiver expecting a Panasonic frame accepts this one.
*/
inline uint16_t orIrEncodeKaseikyo(uint32_t address, uint32_t command,
                                   uint16_t *out, uint16_t cap, uint16_t &khz) {
  khz = 37;
  uint16_t vendor = (uint16_t)((address >> 8) & 0xFFFFU);
  uint8_t genre = (uint8_t)(address & 0xFFU);          // genre1 low nibble, genre2 high
  uint8_t id = (uint8_t)((address >> 24) & 0x03U);
  uint16_t data = (uint16_t)(command & 0x03FFU);

  uint8_t vparity = (uint8_t)(vendor ^ (vendor >> 8));
  vparity = (uint8_t)((vparity ^ (vparity >> 4)) & 0x0FU);

  uint64_t frame = 0;
  frame |= (uint64_t)vendor;                    // bits 0..15
  frame |= (uint64_t)vparity << 16;             // bits 16..19
  frame |= (uint64_t)genre << 20;               // bits 20..27
  frame |= (uint64_t)data << 28;                // bits 28..37
  frame |= (uint64_t)id << 38;                  // bits 38..39
  uint8_t b2 = (uint8_t)((frame >> 16) & 0xFF);
  uint8_t b3 = (uint8_t)((frame >> 24) & 0xFF);
  uint8_t b4 = (uint8_t)((frame >> 32) & 0xFF);
  frame |= (uint64_t)(uint8_t)(b2 ^ b3 ^ b4) << 40;

  uint16_t n = 0;
  if (cap < 100) return 0;
  out[n++] = 3456; out[n++] = 1728;
  orIrPulseDistanceLsb(out, n, cap, frame, 48, 432, 432, 1296);
  out[n++] = 432;                                // Stop bit.
  return n;
}

// 42 bits, LSB first: address(13), ~address(13), command(8), ~command(8).
inline uint16_t orIrEncodeNec42(uint32_t address, uint32_t command,
                                uint16_t *out, uint16_t cap, uint16_t &khz) {
  khz = 38;
  uint32_t a = address & 0x1FFFU;
  uint32_t c = command & 0xFFU;
  uint64_t frame = (uint64_t)a
                 | ((uint64_t)((~a) & 0x1FFFU) << 13)
                 | ((uint64_t)c << 26)
                 | ((uint64_t)((~c) & 0xFFU) << 34);
  uint16_t n = 0;
  if (cap < 90) return 0;
  out[n++] = 9000; out[n++] = 4500;
  orIrPulseDistanceLsb(out, n, cap, frame, 42, 560, 560, 1690);
  out[n++] = 560;
  return n;
}

// NEC-shaped but with its own header, unit and a 40kHz carrier.
inline uint16_t orIrEncodePioneer(uint32_t address, uint32_t command,
                                  uint16_t *out, uint16_t cap, uint16_t &khz) {
  khz = 40;
  uint32_t a = address & 0xFFU;
  uint32_t c = command & 0xFFU;
  uint64_t frame = (uint64_t)a
                 | ((uint64_t)((~a) & 0xFFU) << 8)
                 | ((uint64_t)c << 16)
                 | ((uint64_t)((~c) & 0xFFU) << 24);
  uint16_t n = 0;
  if (cap < 70) return 0;
  out[n++] = 8500; out[n++] = 4250;
  orIrPulseDistanceLsb(out, n, cap, frame, 32, 550, 550, 1700);
  out[n++] = 550;
  return n;
}

// 24 bits, MSB first: address(4), command(8), ~address(4), ~command(8).
inline uint16_t orIrEncodeRca(uint32_t address, uint32_t command,
                              uint16_t *out, uint16_t cap, uint16_t &khz) {
  khz = 38;
  uint32_t a = address & 0x0FU;
  uint32_t c = command & 0xFFU;
  uint32_t frame = (a << 20) | (c << 12) | (((~a) & 0x0FU) << 8) | ((~c) & 0xFFU);
  uint16_t n = 0;
  if (cap < 56) return 0;
  out[n++] = 4000; out[n++] = 4000;
  orIrPulseDistanceMsb(out, n, cap, frame, 24, 500, 1000, 2000);
  out[n++] = 500;
  return n;
}

// Returns 0 when the protocol is not one of these four.
inline uint16_t orIrEncodeExtraProtocol(const char *protocol, uint32_t address,
                                        uint32_t command, uint16_t *out,
                                        uint16_t cap, uint16_t &khz) {
  if (!protocol || !out) return 0;
  if (strcmp(protocol, "Kaseikyo") == 0) return orIrEncodeKaseikyo(address, command, out, cap, khz);
  if (strcmp(protocol, "NEC42") == 0 ||
      strcmp(protocol, "NEC42ext") == 0)  return orIrEncodeNec42(address, command, out, cap, khz);
  if (strcmp(protocol, "Pioneer") == 0)   return orIrEncodePioneer(address, command, out, cap, khz);
  if (strcmp(protocol, "RCA") == 0)       return orIrEncodeRca(address, command, out, cap, khz);
  return 0;
}
