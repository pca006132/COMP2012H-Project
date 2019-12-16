#include "Predicate.hpp"
namespace Parser {
// Actually this is a specific case of constant string matching using stateful
// predicate.
std::unique_ptr<
    PredicateParser<char, std::string, Utils::fromChar, Utils::fold>>
StringPredicate(const std::string &str, const std::string &name) {
  auto fn = [str]() {
    unsigned int i = 0;
    return [str, i](const char &c) mutable {
      return i == str.length() || str[i++] == c;
    };
  };
  return std::make_unique<
      PredicateParser<char, std::string, Utils::fromChar, Utils::fold>>(
      fn, str.length(), name);
};
} // namespace Parser