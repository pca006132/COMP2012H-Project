#pragma once
#include "Parser.hpp"
#include <queue>
#include <stack>

namespace Parser {
/**
 * This class aggregate the output of a list of parser results.
 * We are making a few assumptions here:
 * * Each parser may expand its input (or for lookahead). (`getRemaining`)
 *   Consider a sequence of parsers P1 P2 and P3 applied sequentially,
 *   part of the tokens not consumed from P1 may match P2 and P2 may further
 *   expand the tokens. So we need a stack for holding the tokens.
 * * The result of each parser is finite. (`get`)
 *   We simplify the situation by consuming all tokens from the previous
 *   results. Actually this should be a queue.
 * Actually this class can be modified so that we don't have to have
 * the second assumption, by storing the shared_ptr of the results. However I
 * have no time to implement that.
 */
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

/**
 * This class is a dummy result for storing the input. The input may not be
 * consumed if there are other remaining tokens in the stack of parser results.
 */
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