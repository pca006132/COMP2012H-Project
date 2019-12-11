#pragma once
#include "Parser.hpp"
#include <queue>
#include <stack>

namespace Parser {

template <typename S, typename T>
class StateResult : public AbstractParserResult<S, T> {
private:
  std::queue<S> waitlist;
  std::unique_ptr<AbstractParserResult<S, T>> result;

public:
  StateResult(decltype(result) result) : result(std::move(result)) {}

  StateResult(const StateResult<S, T> &) = delete;

  void push(const S &value) { waitlist.push(value); }

  std::optional<S> getRemaining() override {
    if (auto s = result->getRemaining(); s.has_value())
      return s;
    if (!waitlist.empty()) {
      S s = waitlist.front();
      waitlist.pop();
      return s;
    }
    return {};
  }

  std::optional<T> get() override { return result->get(); }
};

template <typename S, typename T>
class Alternate : public AbstractParser<S, T> {
private:
  std::unique_ptr<std::vector<std::unique_ptr<AbstractParser<S, T>>>> options;
  std::vector<bool> completed;
  std::unique_ptr<StateResult<S, T>> result;
  ParsingError error;
  std::string name;

public:
  Alternate(decltype(options) options, const std::string &name)
      : options(std::move(options)), name(name) {
    reset();
  }

  void reset() override {
    result = std::unique_ptr<StateResult<S, T>>(nullptr);
    completed.clear();
    for (auto &p : *options) {
      p->reset();
      completed.push_back(false);
    }
  }

  std::unique_ptr<AbstractParser<S, T>> clone() override {
    auto v =
        std::make_unique<std::vector<std::unique_ptr<AbstractParser<S, T>>>>();
    for (auto &parser : *options)
      v->push_back(std::move(parser->clone()));
    return std::make_unique<Alternate<S, T>>(std::move(v), name);
  }

  std::optional<
      std::variant<ParsingError, std::unique_ptr<AbstractParserResult<S, T>>>>
  operator()(const S &value) override {
    bool allCompleted = true;
    if (result != nullptr)
      result->push(value);
    for (unsigned int i = 0; i < completed.size(); ++i) {
      if (!completed[i]) {
        auto r = (*options->at(i))(value);
        if (r.has_value()) {
          completed[i] = true;
          if (std::holds_alternative<ParsingError>(r.value())) {
            error = std::get<ParsingError>(r.value());
          } else {
            auto &v = std::get<std::unique_ptr<AbstractParserResult<S, T>>>(
                r.value());
            result = std::make_unique<StateResult<S, T>>(std::move(v));
          }
        } else {
          allCompleted = false;
        }
      }
    }
    if (allCompleted) {
      if (result != nullptr) {
        std::unique_ptr<AbstractParserResult<S, T>> p = std::move(result);
        return std::make_optional(
            std::variant<ParsingError, decltype(p)>(std::move(p)));
      }
      error.record(name + " (alt)");
      return ParsingError::get<S, T>(error);
    }
    return {};
  }

  const std::string &getName() override { return name; }
};

} // namespace Parser