#pragma once
#include "HelperResults.hpp"
#include "Parser.hpp"
#include <functional>

namespace Parser {

template <typename S, typename T> class Sequence : public AbstractParser<S, T> {
private:
  std::unique_ptr<std::vector<AbstractParserPtr<S, T>>> sequence;
  std::unique_ptr<std::stack<AbstractParserResultPtr<S, T>>> prevResults;
  std::queue<T> content;
  QueueParserResult<S, T> *input;
  std::string name;
  unsigned int i = 0;

public:
  Sequence(decltype(sequence) sequence, const std::string &name)
      : sequence(std::move(sequence)), name(name) {
    reset();
  }

  void reset() override {
    for (auto &parser : *sequence)
      parser->reset();
    prevResults = std::make_unique<std::stack<AbstractParserResultPtr<S, T>>>();
    auto storage = std::make_unique<QueueParserResult<S, T>>();
    input = storage.get();
    prevResults->push(std::move(storage));
    while (!content.empty())
      content.pop();
    i = 0;
  }

  AbstractParserPtr<S, T> clone() override {
    auto v = std::make_unique<std::vector<AbstractParserPtr<S, T>>>();
    for (auto &parser : *sequence)
      v->push_back(std::move(parser->clone()));
    return std::make_unique<Sequence<S, T>>(std::move(v), name);
  }

  ParserResult<S, T> operator()(const S &value) override {
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
      if (isError(opt)) {
        auto e = asError(opt);
        e.record(name);
        reset();
        return ParsingError::get<S, T>(e);
      }
      auto &result = asResult(opt);
      if (++i == sequence->size()) {
        auto parsed = castResult<AggregatedParserResult<S, T>, S, T>(
            std::move(prevResults), std::move(result), content);
        reset();
        return parsed;
      }
      for (auto t = result->get(); t.has_value(); t = result->get()) {
        content.push(t.value());
      }
      prevResults->push(std::move(result));
    }
  }

  ParserResult<S, T> operator()() override {
    while (true) {
      S v;
      bool hasValue;
      while (true) {
        if (auto t = prevResults->top()->getRemaining(); t.has_value()) {
          v = t.value();
          hasValue = true;
          break;
        }
        if (prevResults->size() == 1) {
          hasValue = false;
          break;
        }
        prevResults->pop();
      }
      auto opt = hasValue ? (*sequence->at(i))(v) : (*sequence->at(i))();
      if (!opt.has_value()) {
        reset();
        return ParsingError::get<S, T>("Insufficient Tokens", name);
      }
      if (isError(opt)) {
        auto e = asError(opt);
        e.record(name);
        reset();
        return ParsingError::get<S, T>(e);
      }
      auto &result = asResult(opt);
      if (++i == sequence->size()) {
        auto parsed = castResult<AggregatedParserResult<S, T>, S, T>(
            std::move(prevResults), std::move(result), content);
        reset();
        return parsed;
      }
      for (auto t = result->get(); t.has_value(); t = result->get()) {
        content.push(t.value());
      }
      prevResults->push(std::move(result));
    }
  }

  const std::string &getName() override { return name; }

  auto &getSequence() { return sequence; }

  template <typename array>
  static auto get(const std::string &name, array &&args) {
    auto list = std::make_unique<std::vector<AbstractParserPtr<S, T>>>();
    for (auto &i : args)
      list->push_back(std::move(i));
    return std::make_unique<Sequence<S, T>>(std::move(list), name);
  }
};

} // namespace Parser