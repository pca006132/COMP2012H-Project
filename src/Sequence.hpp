#pragma once
#include "Parser.hpp"
#include <functional>
#include <queue>
#include <stack>

namespace Parser {

template <typename S, typename T>
class AggregatedParserResult final : public AbstractParserResult<S, T> {
private:
  std::unique_ptr<std::stack<std::unique_ptr<AbstractParserResult<S, T>>>>
      prevResults;
  std::unique_ptr<AbstractParserResult<S, T>> result;
  std::queue<T> prev;

public:
  AggregatedParserResult(decltype(prevResults) prevResults,
                         decltype(result) result, decltype(prev) prev)
      : prevResults(std::move(prevResults)), result(std::move(result)),
        prev(prev) {}
  AggregatedParserResult(const AggregatedParserResult<S, T> &) = delete;
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

template <typename S, typename T> class Sequence : public AbstractParser<S, T> {
private:
  std::unique_ptr<std::vector<std::unique_ptr<AbstractParser<S, T>>>> sequence;
  std::unique_ptr<std::stack<std::unique_ptr<AbstractParserResult<S, T>>>>
      prevResults;
  std::queue<T> content;
  QueueParserResult<S, T> *input;
  std::string name;
  unsigned int i = 0;

public:
  Sequence(std::unique_ptr<std::vector<std::unique_ptr<AbstractParser<S, T>>>>
               sequence,
           const std::string &name)
      : sequence(std::move(sequence)), name(name) {
    reset();
  }
  void reset() override {
    for (auto &parser : *sequence)
      parser->reset();
    prevResults = std::make_unique<
        std::stack<std::unique_ptr<AbstractParserResult<S, T>>>>();
    auto storage = std::make_unique<QueueParserResult<S, T>>();
    input = storage.get();
    prevResults->push(std::move(storage));
    while (!content.empty())
      content.pop();
    i = 0;
  }
  std::optional<
      std::variant<ParsingError, std::unique_ptr<AbstractParserResult<S, T>>>>
  operator()(const S &value) override {
    if (i == sequence->size())
      throw std::out_of_range("The sequence is already finished");
    input->push(value);
    while (true) {
      S v;
      while (true) {
        if (auto t = prevResults->top()->getRemaining(); t.has_value()) {
          v = t.value();
          break;
        }
        if (prevResults->size() == 1) {
          // we don't delete the last one, that is the input queue
          return {};
        }
        prevResults->pop();
      }
      auto opt = (*sequence->at(i))(v);
      if (!opt.has_value())
        continue;
      if (std::holds_alternative<ParsingError>(opt.value())) {
        auto e = std::get<ParsingError>(opt.value());
        e.record(name);
        return ParsingError::get<S, T>(e);
      }
      auto &result =
          std::get<std::unique_ptr<AbstractParserResult<S, T>>>(opt.value());
      if (++i == sequence->size()) {
        return castResult<AggregatedParserResult<S, T>, S, T>(
            std::move(prevResults), std::move(result), content);
      }
      for (auto t = result->get(); t.has_value(); t = result->get()) {
        content.push(t.value());
      }
      prevResults->push(std::move(result));
    }
  }
  const std::string &getName() override { return name; }
};

} // namespace Parser