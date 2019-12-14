#pragma once
#include "Parser.hpp"
#include <queue>

namespace Parser {

template <typename S, typename T, typename U>
class TakeTill : public AbstractParser<S, T> {
private:
  AbstractParserPtr<S, T> parser;
  AbstractParserPtr<S, U> suffix;
  std::deque<std::pair<int, AbstractParserPtr<S, T>>> suffixStates;
  std::unique_ptr<std::queue<AbstractParserResultPtr<S, T>>> results;
  std::queue<S> tokens;
  std::string name;
  bool lastFinished = true;

  class TakeTillResult : public AbstractParserResult<S, T> {
  private:
    decltype(TakeTill::results) results;
    AbstractParserResultPtr<S, T> ending;

  public:
    TakeTillResult(decltype(results) results, decltype(ending) ending)
        : results(std::move(results)), ending(std::move(ending)) {}

    std::optional<T> get() override {
      while (!results->empty()) {
        if (auto v = results->front()->get(); v.has_value())
          return v;
        results->pop();
      }
      return {};
    }

    std::optional<S> getRemaining() override { return ending->getRemaining(); }
  };

public:
  TakeTill(decltype(parser) parser, decltype(suffix) suffix,
           const std::string &name)
      : parser(std::move(parser)), suffix(std::move(suffix)), name(name) {
    reset();
  }

  void reset() override {
    parser->reset();
    suffixStates.clear();
    results = std::make_unique<std::queue<AbstractParserResultPtr<S, T>>>();
    while (!tokens.empty())
      tokens.pop();
  }

  AbstractParserPtr<S, T> clone() override {
    return std::make_unique<TakeTill<S, T, U>>(
        std::move(parser->clone()), std::move(suffix->clone()), name);
  }

  ParserResult<S, T> operator()(const S &value) override {
    tokens.push(value);
    suffixStates.push_back(std::make_pair(0, std::move(suffix->clone())));
    int max = 0;
    auto matched = std::unique_ptr<AbstractParserResult<S, U>>(nullptr);
    for (auto it = suffixStates.begin(); it != suffixStates.end();) {
      if (auto v = (*it->second)(value); v.has_value()) {
        if (!isError(v)) {
          // suffix matched, terminate
          matched = std::move(asResult(v));
          max = it->first + 1;
          break;
        }
        // suffix not match, remove this state
        it = suffixStates.erase(it);
      } else {
        // suffix not yet determined
        if (max < ++(it->first))
          max = it->first;
        ++it;
      }
    }
    int count = tokens.size() - max;
    for (int i = 0; i < count; ++i) {
      auto &t = tokens.front();
      if (lastFinished)
        parser->reset();
      auto result = (*parser)(t);
      if ((lastFinished = result.has_value())) {
        if (isError(result)) {
          auto error = asError(result);
          error.record(name);
          return result;
        }
        results->push(std::move(asResult(result)));
      }
      tokens.pop();
    }
    if (matched != nullptr) {
      if (!lastFinished) {
        auto result = (*parser)();
        if (!result.has_value())
          return ParsingError::get<S, T>("Insufficient Tokens", name);
        if (isError(result)) {
          auto error = asError(result);
          error.record(name);
          return result;
        }
        results->push(std::move(asResult(result)));
      }
      return castResult<TakeTillResult, S, T>(std::move(results),
                                              std::move(matched));
    }
    return {};
  }

  ParserResult<S, T> operator()() override {
    int max = 0;
    auto matched = std::unique_ptr<AbstractParserResult<S, U>>(nullptr);
    for (auto it = suffixStates.begin(); it != suffixStates.end();) {
      if (auto v = (*it->second)(); v.has_value()) {
        if (!isError(v)) {
          // suffix matched, terminate
          matched = std::move(asResult(v));
          max = it->first + 1;
          break;
        }
        // suffix not match, remove this state
        it = suffixStates.erase(it);
      } else {
        // suffix not yet determined
        if (max < ++(it->first))
          max = it->first;
        ++it;
      }
    }
    int count = tokens.size() - max;
    for (int i = 0; i < count; ++i) {
      auto &t = tokens.front();
      if (lastFinished)
        parser->reset();
      auto result = (*parser)(t);
      if ((lastFinished = result.has_value())) {
        if (isError(result)) {
          auto error = asError(result);
          error.record(name);
          return result;
        }
        results->push(std::move(asResult(result)));
      }
      tokens.pop();
    }
    if (matched != nullptr) {
      if (!lastFinished) {
        auto result = (*parser)();
        if (!result.has_value())
          return ParsingError::get<S, T>(
              "Insufficient tokens before the terminating sequence", name);
        if (isError(result)) {
          auto error = asError(result);
          error.record(name);
          return result;
        }
        results->push(std::move(asResult(result)));
      }
      return castResult<TakeTillResult, S, T>(std::move(results),
                                              std::move(matched));
    }
    return ParsingError::get<S, T>("Insufficient tokens: not terminated", name);
  }

  const std::string &getName() override { return name; }
};

} // namespace Parser