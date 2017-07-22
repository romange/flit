// FLIT64 Implementation
// This is free and unencumbered software released into the public domain.

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <memory.h>

#if !defined(__amd64__)
#error "Only for  AMD64 little endian architecture"
#endif

namespace flit {

inline constexpr int FindLSBSetNonZero(uint32_t n) {
  return __builtin_ctz(n);
}


inline uint64_t FindMSBSet64NonZero(uint64_t n) {
    uint64_t result;
    asm ("bsrq %1, %0"
        : "=r" (result)
        : "rm" (n));
    return result;
}

// Decodes buf into v and returns the pointer to the next byte after decoded chunk.
// Requires that src points to buffer that has at least 8 bytes and
// it assumes that the input is valid.
inline const unsigned ParseFlit64Fast(const uint8_t* src, uint64_t* v) {
  if (*src == 0) {
    ++src;
    *v = *reinterpret_cast<const uint64_t*>(src);
    return 9;
  }

  *v = *reinterpret_cast<const uint64_t*>(src);
  unsigned index = FindLSBSetNonZero(*src);

#define USE_MASK 0

#if USE_MASK
  static constexpr uint64_t mask[8] = {
    0xff,
    0xffff,
    0xffffff,
    0xffffffff,
    0xffffffffff,
    0xffffffffffff,
    0xffffffffffffff,
    0xffffffffffffffff,
  };

  *v &= mask[index];

  ++index;
#else
  ++index;
  *v &= ((1ULL << index * 8) - 1);
#endif

  *v >>= index;

  return index;
}

#undef USE_MASK

inline const uint8_t* ParseFlit64Safe(const uint8_t* begin, const uint8_t* end, uint64_t* v) {
  size_t sz = end - begin;

  if (*begin == 0) {
    ++begin;
    if (sz < 8)
      return nullptr;

    *v = *reinterpret_cast<const uint64_t*>(begin);
    if (*v < (1ULL << 56))
      return nullptr;
    return begin + 8;
  }

  unsigned index = FindLSBSetNonZero(*begin) + 1;
  if (sz < index)
    return nullptr;
  memcpy(v, begin, index);
  *v &= ((1ULL << index * 8) - 1);
  *v >>= index;

  return begin + index;
}

// Encodes v into buf and returns pointer to the next byte.
// dest must have at least 9 bytes.
inline unsigned EncodeFlit64(uint64_t v, uint8_t* dest) {
  if (v < 128) {
    *(uint8_t*)dest = (uint8_t)v << 1 | 1;
    return 1;
  }
  uint32_t index = FindMSBSet64NonZero(v);
  if (v >= (1ULL << 56)) {
    *dest++ = 0;
    *reinterpret_cast<uint64_t*>(dest) = v;
    return 9;
  }
  uint32_t num_bytes = (index * 2454267027) >> 34;

  v <<= 1;
  v |= 1UL;
  v <<= num_bytes;
  *reinterpret_cast<uint64_t*>(dest) = v;

  return num_bytes + 1;
}

}  // namespace flit
