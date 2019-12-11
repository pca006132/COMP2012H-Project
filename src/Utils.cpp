#include "Utils.hpp"

namespace Utils {
std::string fromChar(const char &c) { return std::string(1, c); }
std::string fold(const std::string &a, const std::string &b) { return a + b; }
}