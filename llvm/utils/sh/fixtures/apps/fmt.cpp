#include <fmt/format.h>
#include <fmt/ranges.h>
#include <iostream>
#include <vector>
int main() {
  auto s=fmt::format("{:04x}|{:.3f}|{}",42,1.25,fmt::join(std::vector<int>{3,1,4},","));
  if(s!="002a|1.250|3,1,4") return 1;
  bool caught=false; try {(void)fmt::format(fmt::runtime("{:q}"),17);} catch(const fmt::format_error &) {caught=true;}
  if(!caught) return 2;
  std::cout<<"fmt:"<<s<<":caught="<<caught<<":PASS\n";
}
