#pragma once
#include "Parser.hpp"
#include <functional>

namespace Parser {

/**
 * Just a simple wrapper class to handle recursive parser using lazy
 * instantiation of sub-parsers. It also provides a mapping function
 * to alter the parser result conveniently.
 * The sub-parser is only instantiated when the parser is applied.
 */
template <typename S, typename T>
class LazyParser : public AbstractParser<S, T> {
private:
  AbstractParser<S, T> *src;
  std::unique_ptr<AbstractParser<S, T>> instance;
  std::function<ParserResult<S, T>(ParserResult<S, T>)> mapping;

public:
  LazyParser(decltype(src) src, decltype(mapping) mapping = nullptr)
      : src(src), mapping(mapping) {
    reset();
  }
  void reset() override { instance = nullptr; }
  AbstractParserPtr<S, T> clone() override {
    return std::make_unique<LazyParser>(src);
  }
  const std::string &getName() override { return src->getName(); }
  ParserResult<S, T> operator()(const S &value) override {
    if (instance == nullptr)
      instance = std::move(src->clone());
    auto result = (*instance)(value);
    if (mapping != nullptr)
      result = mapping(std::move(result));
    return result;
  }
  ParserResult<S, T> operator()() override {
    if (instance == nullptr)
      instance = std::move(src->clone());
    auto result = (*instance)();
    if (mapping != nullptr)
      result = mapping(std::move(result));
    return result;
  }
};

} // namespace Parser