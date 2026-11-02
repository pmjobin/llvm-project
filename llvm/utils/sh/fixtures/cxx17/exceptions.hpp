#pragma once
#include <string>
extern int eh_dtor_count;
struct EhTracked {
  ~EhTracked() noexcept { ++eh_dtor_count; }
};
std::string eh_leaf(int n);
std::string eh_middle(int n);
