#pragma once
#include "Parser.hpp"

namespace Parser {

template <typename S, typename T>
class LazyParser : public AbstractParser<S, T> {
private:
  AbstractParser<S, T> *src;
  std::unique_ptr<AbstractParser<S, T>> instance;

public:
  LazyParser(decltype(src) src) : src(src) { reset(); }
  void reset() override { instance = nullptr; }
  AbstractParserPtr<S, T> clone() override {
    return std::make_unique<LazyParser>(src);
  }
  const std::string &getName() override { return src->getName(); }
  ParserResult<S, T> operator()(const S &value) override {
    if (instance == nullptr)
      instance = std::move(src->clone());
    return (*instance)(value);
  }
  ParserResult<S, T> operator()() override {
    if (instance == nullptr)
      instance = std::move(src->clone());
    return (*instance)();
  }
};

} // namespace Parser