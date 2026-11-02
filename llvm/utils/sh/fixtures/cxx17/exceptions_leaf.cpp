#include "exceptions.hpp"
#include <stdexcept>
int eh_dtor_count = 0;
std::string eh_leaf(int n) {
  EhTracked t;
  if (n == 9) throw std::runtime_error("needle");
  return "ok";
}
