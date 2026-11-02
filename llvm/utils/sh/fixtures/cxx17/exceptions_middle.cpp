#include "exceptions.hpp"
std::string eh_middle(int n) {
  EhTracked t;
  try { return eh_leaf(n); }
  catch (...) { throw; }
}
