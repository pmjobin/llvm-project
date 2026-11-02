#pragma once
#include <memory>
#include <string>
struct AbiBase {
  virtual ~AbiBase();
  virtual int value() const = 0;
};
struct AbiDerived final : AbiBase {
  explicit AbiDerived(int x);
  ~AbiDerived() override;
  int value() const override;
  int raw() const;
private:
  int n;
};
std::unique_ptr<AbiBase> abi_make(int n);
std::string abi_join(const std::string &s);
int abi_dtor_count();
extern "C" int abi_bridge(int x);
