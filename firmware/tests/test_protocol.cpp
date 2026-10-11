// Host test for vryo_protocol.h. Builds packets the same way the PC
// side does (VryoGPUController::WriteFrame) and feeds them to the parser.
// Build and run:  g++ -std=c++17 -Wall -I../vryo_gpu_controller test_protocol.cpp -o t && ./t

#include <cstdio>
#include <cstdlib>
#include <vector>
#include "vryo_protocol.h"

static std::vector<uint8_t> MakePacket(const std::vector<uint8_t>& rgb)
{
    std::vector<uint8_t> p;
    const size_t n = rgb.size() / 3;
    p.push_back(0xAA); p.push_back(0x55);
    p.push_back((uint8_t)(n & 0xFF)); p.push_back((uint8_t)((n >> 8) & 0xFF));
    for(uint8_t b : rgb) p.push_back(b);
    uint8_t x = 0; for(uint8_t b : p) x ^= b;
    p.push_back(x);
    return p;
}

static int failures = 0;
#define CHECK(c) do { if(!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while(0)

static int Feed(VryoFrameParser<44>& parser, const std::vector<uint8_t>& bytes)
{
    int frames = 0;
    for(uint8_t b : bytes) if(parser.Feed(b)) frames++;
    return frames;
}

int main()
{
    std::vector<uint8_t> rgb(44 * 3);
    for(size_t i = 0; i < rgb.size(); i++) rgb[i] = (uint8_t)(i * 7 + 3);

    // 1. good frame
    { VryoFrameParser<44> p; CHECK(Feed(p, MakePacket(rgb)) == 1);
      CHECK(p.FrameLedCount() == 44); CHECK(p.StoredCount() == 44);
      for(size_t i = 0; i < rgb.size(); i++) CHECK(p.Colors()[i] == rgb[i]); }

    // 2. two frames back to back
    { VryoFrameParser<44> p; auto a = MakePacket(rgb); auto b = a; b.insert(b.end(), a.begin(), a.end());
      CHECK(Feed(p, b) == 2); }

    // 3. bad checksum is rejected, next good frame still works
    { VryoFrameParser<44> p; auto bad = MakePacket(rgb); bad.back() ^= 0x01;
      CHECK(Feed(p, bad) == 0); CHECK(Feed(p, MakePacket(rgb)) == 1); }

    // 4. garbage before a frame (resync)
    { VryoFrameParser<44> p; std::vector<uint8_t> g = {0x00, 0xAA, 0xAA, 0x13, 0xAA, 0x55, 0x07};
      auto f = MakePacket(rgb); g.insert(g.end(), f.begin(), f.end());
      CHECK(Feed(p, g) == 1); }

    // 5. longer frame than the buffer: accepted, cut to 44 LEDs
    { VryoFrameParser<44> p; std::vector<uint8_t> big(60 * 3, 9);
      CHECK(Feed(p, MakePacket(big)) == 1); CHECK(p.FrameLedCount() == 60); CHECK(p.StoredCount() == 44); }

    // 6. shorter frame: accepted, StoredCount is the short length
    { VryoFrameParser<44> p; std::vector<uint8_t> small(10 * 3, 5);
      CHECK(Feed(p, MakePacket(small)) == 1); CHECK(p.StoredCount() == 10); }

    // 7. zero and absurd counts rejected
    { VryoFrameParser<44> p; std::vector<uint8_t> z = {0xAA, 0x55, 0x00, 0x00, 0xFF};
      CHECK(Feed(p, z) == 0);
      std::vector<uint8_t> h = {0xAA, 0x55, 0xFF, 0xFF};
      CHECK(Feed(p, h) == 0); CHECK(Feed(p, MakePacket(rgb)) == 1); }

    // 8. Reset() mid-frame drops it (timeout path)
    { VryoFrameParser<44> p; auto f = MakePacket(rgb); size_t half = f.size() / 2;
      for(size_t i = 0; i < half; i++) p.Feed(f[i]);
      p.Reset(); CHECK(Feed(p, MakePacket(rgb)) == 1); }

    printf(failures ? "%d FAILED\n" : "all protocol tests passed\n", failures);
    return failures ? 1 : 0;
}
