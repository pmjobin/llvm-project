#include "abi.hpp"
#include <iostream>
#include <typeinfo>
int main() {
  {
    auto p = abi_make(17);
    const auto *derived = dynamic_cast<const AbiDerived *>(p.get());
    if (!derived || derived->raw()!=17 || p->value()!=34 || typeid(*p)!=typeid(AbiDerived) || abi_join("abi")!="abi!" || abi_bridge(5)!=6) return 23;
  }
  if (abi_dtor_count()!=1) return 24;
  std::cout << "abi:virt=34:raw=17:joined=abi!:bridge=6:dtor=" << abi_dtor_count() << ":PASS\n";
  return 0;
}
