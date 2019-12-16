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

/**
 * This is just a simple predicate class. The convert function takes the input
 * of type S and convert it to output type T. The fold function (actually foldl)
 * aggregate the input (T->T->T), and the toStr function give us a
 * human-readable error message (though I found that I did not use it much).
 */
template <typename S, typename T, T convert(const S &),
          T fold(const T &, const T &),
          std::string toStr(const T &) = identity<T>>
class PredicateParser final : public AbstractParser<S, T> {
private:
  std::function<std::function<bool(const S &)>()> predicateGen;
  std::function<bool(const S &)> predicate;
  int quantifier;
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
      : predicateGen(predicateGen), predicate(predicateGen()),
        quantifier(quantifier), name(name) {}

  PredicateParser(const S s, const int quantifier, const std::string &name)
      : predicateGen([s]() { return [s](const S &v) { return v == s; }; }),
        predicate(predicateGen()), quantifier(quantifier), name(name) {}

  void reset() override {
    count = 0;
    // The predicate function is stateful. We need to generate a new one when we
    // reset the parser.
    predicate = predicateGen();
  }

  AbstractParserPtr<S, T> clone() override {
    return std::make_unique<PredicateParser>(predicateGen, quantifier, name);
  }

  ParserResult<S, T> operator()(const S &value) override {
    // Simple logic: Handle the special quantifiers specifically in each case.
    if (predicate(value)) {
      T v = convert(value);
      if (count++ == 0)
        aggregated = v;
      else
        aggregated = fold(aggregated, v);
      if (quantifier == NONE)
        return ParsingError::get<S, T>("Unexpected " + toStr(v), name);
      if (quantifier == ONCE || quantifier == OPTIONAL || quantifier == count) {
        auto parsed = castResult<PredicateParserResult, S, T>(aggregated);
        reset();
        return parsed;
      }
      return {};
    }
    if (count < quantifier ||
        (count == 0 && (quantifier == ONCE || quantifier == MORE))) {
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
    if (count < quantifier ||
        (count == 0 && (quantifier == ONCE || quantifier == MORE))) {
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

std::unique_ptr<
    PredicateParser<char, std::string, Utils::fromChar, Utils::fold>>
StringPredicate(const std::string &str, const std::string &name);
} // namespace Parser