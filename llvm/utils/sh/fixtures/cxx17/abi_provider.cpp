#include "abi.hpp"
static int abi_dtor = 0;
AbiBase::~AbiBase() = default;
AbiDerived::AbiDerived(int x) : n(x) {}
AbiDerived::~AbiDerived() { ++abi_dtor; }
int AbiDerived::value() const { return n * 2; }
int AbiDerived::raw() const { return n; }
std::unique_ptr<AbiBase> abi_make(int n) { return std::make_unique<AbiDerived>(n); }
std::string abi_join(const std::string &s) { return s + "!"; }
int abi_dtor_count() { return abi_dtor; }
extern "C" int abi_bridge(int x) { return x + 1; }
