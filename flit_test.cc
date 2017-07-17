// Copyright 2017, Beeri 15.  All rights reserved.
// Author: Roman Gershman (romange@gmail.com)
//
#include <random>

#include "flit.h"

#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

namespace flit {
using namespace std;

inline int flit64enc(void* buf, uint64_t v) {
  int lzc = 64;
  if (v) lzc = __builtin_clzll(v);
  if (lzc > 56) {
    *(uint8_t*)buf = (uint8_t)v << 1 | 1;
    return 1;
  }
  if (lzc < 8) {
    uint8_t* p = (uint8_t*)buf;
    *p++ = 0;
    *(uint64_t*)p = v;
    return 9;
  }

  // count extra bytes
  int e = (63 - lzc) / 7;

  v <<= 1;
  v |= 1;
  v <<= e;
  *(uint64_t*)buf = v;

  return e + 1;
}

inline int flit64dec(uint64_t* v, const void* buf) {
  uint64_t* p = (uint64_t*)buf;
  uint64_t x = *p;

  int tzc = 8;
  if (x) tzc = __builtin_ctzll(x);
  if (tzc > 7) {
    uint8_t c = *(uint8_t*)(++p);
    *v = x >> 8 | (uint64_t)c << 56;
    return 9;
  }

  static const uint64_t mask[8] = {
    0xff,
    0xffff,
    0xffffff,
    0xffffffff,
    0xffffffffff,
    0xffffffffffff,
    0xffffffffffffff,
    0xffffffffffffffff,
  };
  x &= mask[tzc];

  // const here seems to ensure that 'size' is not aliased by '*v'
  const int size = tzc + 1;

  *v = x >> size;

  return size;
}

class FlitTest : public testing::Test {

protected:
  unsigned Flit64(uint64_t val) {
    return EncodeFlit64(val, buf_) - buf_;
  }

  unsigned UnFlit64(uint64_t* val) {
    return ParseFlit64Fast(buf_, val) - buf_;
  }

  uint8_t buf_[9];
};


#define TEST_CONST(x, y) \
  ASSERT_EQ(y, Flit64(x)); \
  ASSERT_EQ(y, UnFlit64(&val)); \
  EXPECT_EQ((x), val)

TEST_F(FlitTest, Flit) {
  uint64_t val = 0;
  TEST_CONST(0, 1);
  TEST_CONST(127, 1);
  TEST_CONST(128, 2);
  TEST_CONST(255, 2);
  TEST_CONST((1<<14) - 1, 2);
  TEST_CONST((1<<21) - 1, 3);
  TEST_CONST((1ULL << 32), 5);
  TEST_CONST((1ULL << 31), 5);
  TEST_CONST((1ULL<<56), 9);
  TEST_CONST((1ULL<<63), 9);
}

static std::mt19937_64 rnd_engine;

uint64_t RandUint64() {
  unsigned bit_len = (rnd_engine() % 64) + 1;
  return rnd_engine() & ((1ULL << bit_len) - 1);
}

static void FillEncoded(uint8_t* buf, unsigned num) {
  for (unsigned i = 0; i < num; ++i) {
    volatile uint64_t val = RandUint64();
    buf = EncodeFlit64(val, buf);
  }
}

template<typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
  return N;
}

static void BM_FlitEncode(benchmark::State& state) {
  uint64_t input[1024];
  std::generate(input, input + countof(input), RandUint64);
  uint8_t buf[1024];

  while (state.KeepRunning()) {
    for (unsigned i = 0; i < countof(input); i +=4) {
      benchmark::DoNotOptimize(EncodeFlit64(input[i], buf));
      benchmark::DoNotOptimize(EncodeFlit64(input[i] + 1, buf + 9));
      benchmark::DoNotOptimize(EncodeFlit64(input[i] + 2, buf + 18));
      benchmark::DoNotOptimize(EncodeFlit64(input[i] + 3, buf + 27));
    }
  }
}
BENCHMARK(BM_FlitEncode);

static void BM_FlitEncodeGold(benchmark::State& state) {
  uint64_t input[1024];
  std::generate(input, input + countof(input), RandUint64);
  uint8_t buf[1024];

  while (state.KeepRunning()) {
    for (unsigned i = 0; i < countof(input); i +=4) {

      benchmark::DoNotOptimize(flit64enc(buf + 0, input[i]));
      benchmark::DoNotOptimize(flit64enc(buf + 9, input[i] + 1));
      benchmark::DoNotOptimize(flit64enc(buf + 18, input[i] + 2));
      benchmark::DoNotOptimize(flit64enc(buf + 27, input[i] + 3));
    }
  }
}
BENCHMARK(BM_FlitEncodeGold);


static void BM_FlitDecode(benchmark::State& state) {
  uint8_t buf[1024 * 9];
  FillEncoded(buf, 1024);

  while (state.KeepRunning()) {
    uint64_t val = 0;
    const uint8_t* rn = buf;
    for (unsigned i = 0; i < 1024 /4; ++i) {
      rn = ParseFlit64Fast(rn, &val);
      rn = ParseFlit64Fast(rn, &val);
      rn = ParseFlit64Fast(rn, &val);
      rn = ParseFlit64Fast(rn, &val);
      benchmark::DoNotOptimize(val);
    }
  }
}
BENCHMARK(BM_FlitDecode);

static void BM_FlitDecodeGold(benchmark::State& state) {
  uint8_t buf[1024 * 9];
  FillEncoded(buf, 1024);

  while (state.KeepRunning()) {
    uint64_t val = 0;
    const uint8_t* rn = buf;
    for (unsigned i = 0; i < 1024 /4; ++i) {
      rn += flit64dec(&val, rn);
      rn += flit64dec(&val, rn);
      rn += flit64dec(&val, rn);
      rn += flit64dec(&val, rn);
      benchmark::DoNotOptimize(val);
    }
  }
}
BENCHMARK(BM_FlitDecodeGold);

}  // namespace flit


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  auto gtest_value = RUN_ALL_TESTS();

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return gtest_value;
}