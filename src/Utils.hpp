#pragma once
#include <string>
using std::string;

//functions to manipulate std::string
namespace Utils {
  string fromChar(const char &c) { return string(1, c); }

  string fold(const string &a, const string &b) {
    return a + b;
  }

} // namespace Utils
