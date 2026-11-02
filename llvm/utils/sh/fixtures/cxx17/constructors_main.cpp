#include "constructors.hpp"
#include <iostream>
int main() {
  if (ctor_global_count()!=1 || ctor_local_count()!=0) return 25;
  if (ctor_local_access()!=1 || ctor_local_access()!=1 || ctor_local_count()!=1) return 26;
  std::cout << "ctors:global=1:local=1:PASS\n";
  return 0;
}
