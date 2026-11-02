#include "tinyxml2.h"
#include <iostream>
#include <string>
int main() {
  tinyxml2::XMLDocument d;
  if (d.Parse("<root id='17'><item>caf&#233; &amp; tea</item><item>42</item></root>")) return 1;
  auto *r=d.RootElement(); int id=0;
  if (r->QueryIntAttribute("id", &id) || id!=17 || std::string(r->FirstChildElement()->GetText())!="caf\xc3\xa9 & tea") return 2;
  int n=0; if(r->LastChildElement()->QueryIntText(&n) || n!=42) return 3;
  tinyxml2::XMLPrinter p; d.Print(&p);
  tinyxml2::XMLDocument copy; if(copy.Parse(p.CStr()) || copy.RootElement()->IntAttribute("id")!=id) return 4;
  tinyxml2::XMLDocument bad; if(bad.Parse("<a></b>")==tinyxml2::XML_SUCCESS) return 5;
  std::cout<<"tinyxml2:id="<<id<<":value="<<n<<":PASS\n";
}
