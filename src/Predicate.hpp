#pragma once
#include "Parser.hpp"
#include "Utils.hpp"
#include <functional>

namespace Parser {

constexpr int NONE = -4;
constexpr int OPTIONAL = -3;
constexpr int MORE = -2;
constexpr int ANY = -1;
constexpr int ONCE = 1;

template <typename T> T identity(const T &v) { return v; }

template <typename S, typename T, T convert(const S &),
          T fold(const T &, const T &),
          std::string toStr(const T &) = identity<T>>
class PredicateParser final : public AbstractParser<S, T> {
private:
  std::function<std::function<bool(const S &)>()> predicateGen;
  std::function<bool(const S &)> predicate;
  int q;
  int count = 0;
  T aggregated;
  std::string name;

  class PredicateParserResult final : public AbstractParserResult<S, T> {
  private:
    S token;
    T value;
    bool tokenLeft;
    bool valueLeft;

  public:
    PredicateParserResult() : tokenLeft(false), valueLeft(false) {}

    PredicateParserResult(S token)
        : token(token), tokenLeft(true), valueLeft(false) {}

    PredicateParserResult(T value)
        : value(value), tokenLeft(false), valueLeft(true) {}

    PredicateParserResult(S token, T value)
        : token(token), value(value), tokenLeft(true), valueLeft(true) {}

    std::optional<S> getRemaining() override {
      if (tokenLeft) {
        tokenLeft = false;
        return std::make_optional(token);
      }
      return {};
    }

    std::optional<T> get() override {
      if (valueLeft) {
        valueLeft = false;
        return std::make_optional(value);
      }
      return {};
    }
  };

public:
  PredicateParser(decltype(predicateGen) predicateGen, const int quantifier,
                  const std::string &name)
      : predicateGen(predicateGen), predicate(predicateGen()), q(quantifier),
        name(name) {}

  PredicateParser(const S s, const int quantifier, const std::string &name)
      : predicateGen([s]() { return [s](const S &v) { return v == s; }; }),
        predicate(predicateGen()), q(quantifier), name(name) {}

  void reset() override {
    count = 0;
    predicate = predicateGen();
  }

  AbstractParserPtr<S, T> clone() override {
    return std::make_unique<PredicateParser>(predicateGen, q, name);
  }

  ParserResult<S, T> operator()(const S &value) override {
    if (predicate(value)) {
      T v = convert(value);
      if (count++ == 0)
        aggregated = v;
      else
        aggregated = fold(aggregated, v);
      if (q == NONE)
        return ParsingError::get<S, T>("Unexpected " + toStr(v), name);
      if (q == ONCE || q == OPTIONAL || q == count) {
        auto parsed = castResult<PredicateParserResult, S, T>(aggregated);
        reset();
        return parsed;
      }
      return {};
    }
    if (count < q || (count == 0 && (q == ONCE || q == MORE))) {
      reset();
      return ParsingError::get<S, T>("Insufficient tokens", name);
    }
    auto result =
        count == 0 ? castResult<PredicateParserResult, S, T>(value)
                   : castResult<PredicateParserResult, S, T>(value, aggregated);
    reset();
    return result;
  }

  ParserResult<S, T> operator()() override {
    if (count < q || (count == 0 && (q == ONCE || q == MORE))) {
      reset();
      return ParsingError::get<S, T>("Insufficient tokens", name);
    }
    auto result = count == 0
                      ? castResult<PredicateParserResult, S, T>()
                      : castResult<PredicateParserResult, S, T>(aggregated);
    reset();
    return result;
  }

  const std::string &getName() override { return name; }

  static AbstractParserPtr<S, T> get(const S &s, const int quantifier,
                                     const std::string &name) {
    return std::make_unique<PredicateParser<S, T, convert, fold, toStr>>(
        s, quantifier, name);
  }
};

auto StringPredicate(const std::string &str, const std::string &name) {
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