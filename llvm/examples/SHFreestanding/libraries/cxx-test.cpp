// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <memory>
#include <memory_resource>
#include <new>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" int library_transform(int);
static int State;
static int Constructions;
static int HandlerCalls;

static void allocationFailed() {
  ++HandlerCalls;
  std::set_new_handler(nullptr);
}

struct Lifetime {
  std::string Name;
  Lifetime() : Name("constructed before main with allocated storage") {
    ++Constructions;
  }
  ~Lifetime() {
    assert(State == 1 && Constructions == 2);
    if (--Constructions == 1)
      std::puts("SH4 LIBCXX LOCAL DTOR PASS");
    else
      std::abort();
  }
};

struct GlobalLifetime {
  std::string Name = "global static initialization";
  GlobalLifetime() { ++Constructions; }
  ~GlobalLifetime() {
    assert(State == 1 && Constructions == 1);
    --Constructions;
    std::puts("SH4 LIBCXX PASS");
  }
} Global;

static Lifetime &local() {
  static Lifetime Object;
  return Object;
}

struct alignas(64) Aligned {
  int Value = 19;
};

extern "C" int main() {
  assert(Constructions == 1 && Global.Name == "global static initialization");
  assert(local().Name.size() > 20 && &local() == &local() &&
         Constructions == 2);
  std::vector<int> Values = {9, 1, 7, 3, 5};
  std::sort(Values.begin(), Values.end());
  assert(Values.front() == 1 && Values.back() == 9);
  assert(std::accumulate(Values.begin(), Values.end(), 0) == 25);
  std::map<std::string, int> Ordered;
  Ordered["archive"] = library_transform(7);
  std::unordered_map<std::string, std::vector<int>> Hashed;
  Hashed["values"] = Values;
  assert(Ordered.at("archive") == 52 && Hashed.at("values")[2] == 5);
  auto Unique = std::make_unique<std::string>(80, 'x');
  auto Shared = std::make_shared<std::vector<int>>(Values);
  auto Copy = Shared;
  assert(Shared.use_count() == 2 && Unique->size() == 80 && Copy->size() == 5);
  auto Wide = std::make_unique<Aligned>();
  assert(reinterpret_cast<std::uintptr_t>(Wide.get()) % 64 == 0 &&
         Wide->Value == 19);
  std::function<int(int)> Function = [Values](int X) { return Values[X] + 12; };
  assert(Function(3) == 19);
  std::array<char, 32> Text;
  auto Result =
      std::to_chars(Text.data(), Text.data() + Text.size(), 1234567890123LL);
  assert(Result.ec == std::errc());
  long long Number = 0;
  assert(std::from_chars(Text.data(), Result.ptr, Number).ec == std::errc());
  assert(Number == 1234567890123LL);
  std::array<std::byte, 2048> Storage;
  std::pmr::monotonic_buffer_resource Arena(Storage.data(), Storage.size());
  auto OldResource = std::pmr::set_default_resource(&Arena);
  assert(std::pmr::get_default_resource() == &Arena);
  {
    std::pmr::vector<int> Pooled = {2, 3, 5, 7};
    assert(std::accumulate(Pooled.begin(), Pooled.end(), 0) == 17);
  }
  assert(std::pmr::set_default_resource(OldResource) == &Arena);
  auto Nothrow = new (std::nothrow) Aligned;
  assert(Nothrow && reinterpret_cast<std::uintptr_t>(Nothrow) % 64 == 0);
  delete Nothrow;
  std::set_new_handler(allocationFailed);
  auto TooLarge = new (std::nothrow) char[512 * 1024];
  assert(!TooLarge && HandlerCalls == 1);
  State = 1;
  return 0;
}
