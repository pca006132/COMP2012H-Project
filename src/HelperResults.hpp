#pragma once
#include "Parser.hpp"
#include <queue>
#include <stack>

namespace Parser {
template <typename S, typename T>
class AggregatedParserResult final : public AbstractParserResult<S, T> {
private:
  std::unique_ptr<std::stack<AbstractParserResultPtr<S, T>>> prevResults;
  AbstractParserResultPtr<S, T> result;
  std::queue<T> prev;

public:
  AggregatedParserResult(decltype(prevResults) prevResults,
                         decltype(result) result, decltype(prev) prev)
      : prevResults(std::move(prevResults)), result(std::move(result)),
        prev(prev) {}
  AggregatedParserResult(const AggregatedParserResult &) = delete;
  std::optional<S> getRemaining() override {
    if (auto v = result->getRemaining(); v.has_value())
      return v;
    while (!prevResults->empty()) {
      if (auto v = prevResults->top()->getRemaining(); v.has_value())
        return v;
      prevResults->pop();
    }
    return {};
  }
  std::optional<T> get() override {
    if (!prev.empty()) {
      T top = prev.front();
      prev.pop();
      return std::make_optional(top);
    }
    return result->get();
  }
};

template <typename S, typename T>
class QueueParserResult final : public AbstractParserResult<S, T> {
private:
  std::queue<S> inputs;

public:
  void push(const S &value) { inputs.push(value); }
  std::optional<S> getRemaining() override {
    if (!inputs.empty()) {
      S value = inputs.front();
      inputs.pop();
      return std::make_optional(value);
    }
    return {};
  }
  std::optional<T> get() override { return {}; }
};

} // namespace Parser