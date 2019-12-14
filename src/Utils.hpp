#pragma once
#include <string>

namespace Utils {
inline std::string fromChar(const char &c) { return std::string(1, c); }
inline std::string fold(const std::string &a, const std::string &b) {
  return a + b;
}
} // namespace Utils