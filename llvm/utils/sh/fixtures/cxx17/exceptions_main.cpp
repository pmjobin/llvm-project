#include "exceptions.hpp"
#include <iostream>
#include <stdexcept>
int main() {
  try { (void)eh_middle(9); return 21; }
  catch (const std::runtime_error &e) {
    if (std::string(e.what())!="needle" || eh_dtor_count!=2) return 22;
    std::cout << "exceptions:what=" << e.what() << ":unwound=" << eh_dtor_count << ":PASS\n";
  }
  return 0;
}
