#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
int main() {
  std::vector<int> v{7,1,4,1,9};
  std::sort(v.begin(), v.end());
  const int sum = std::accumulate(v.begin(), v.end(), 0);
  const std::string s = std::string("abc") + "def";
  const std::map<std::string,int> m{{"alpha",2},{"beta",5}};
  const std::unordered_map<std::string,int> u{{"hit",6}};
  const std::optional<int> o{12};
  const std::variant<int,std::string> x{std::string("ok")};
  const auto p = std::make_unique<int>(8);
  if (v[0]!=1 || v.back()!=9 || sum!=22 || s!="abcdef" || m.at("alpha")+m.at("beta")!=7 || u.at("hit")!=6 || *o!=12 || std::get<std::string>(x)!="ok" || *p!=8) return 20;
  std::cout << "core17:sum=" << sum << ":str=" << s << ":map=7:variant=" << std::get<std::string>(x) << ":PASS\n";
  return 0;
}
