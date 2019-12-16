#pragma once
#include "Parser.hpp"
#include <queue>
#include <stack>

namespace Parser {

/**
 * This is a combinator that represents the union of parsers.
 */
template <typename S, typename T>
class Alternate : public AbstractParser<S, T> {
private:
  /**
   * This class is to store the parser result of a single parser.
   * As the parsing of other parsers may continue after one success (their
   * length may be different), we have to store the additional tokens in
   * a queue in this object.
   */
  class StateResult : public AbstractParserResult<S, T> {
  private:
    std::queue<S> waitlist;
    AbstractParserResultPtr<S, T> result;

  public:
    StateResult(decltype(result) result) : result(std::move(result)) {}

    StateResult(const StateResult &) = delete;

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
  std::unique_ptr<std::vector<AbstractParserPtr<S, T>>> options;
  std::vector<bool> completed;
  std::unique_ptr<StateResult> result;
  std::optional<ParsingError> error;
  std::string name;

public:
  Alternate(decltype(options) options, const std::string &name)
      : options(std::move(options)), name(name) {
    reset();
  }

  void reset() override {
    result = std::unique_ptr<StateResult>(nullptr);
    completed.clear();
    for (auto &p : *options) {
      p->reset();
      completed.push_back(false);
    }
  }

  AbstractParserPtr<S, T> clone() override {
    auto v = std::make_unique<std::vector<AbstractParserPtr<S, T>>>();
    for (auto &parser : *options)
      v->push_back(std::move(parser->clone()));
    return std::make_unique<Alternate<S, T>>(std::move(v), name);
  }

  ParserResult<S, T> operator()(const S &value) override {
    bool allCompleted = true;
    // as we continue the parsing, we have to store the token in the previous
    // result or they will be lost those undetermined parsers failed.
    if (result != nullptr)
      result->push(value);
    // Iterate through the undetermined parsers and apply the token.
    // If they success, make them our current result. We only keep the latest
    // result as that matches the most tokens. (be greedy)
    for (unsigned int i = 0; i < completed.size(); ++i) {
      if (!completed[i]) {
        auto r = (*options->at(i))(value);
        if (r.has_value()) {
          completed[i] = true;
          if (isError(r)) {
            error = std::optional(asError(r));
          } else {
            auto &v = asResult(r);
            result = std::make_unique<StateResult>(std::move(v));
          }
        } else {
          allCompleted = false;
        }
      }
    }
    // If all parsers are determined, we return a result if we have one,
    // or an error if none of the parsers matches the input.
    // Otherwise, indicate that we are not completed yet.
    if (allCompleted) {
      if (result != nullptr) {
        AbstractParserResultPtr<S, T> p = std::move(result);
        auto parsed = std::make_optional(
            std::variant<ParsingError, decltype(p)>(std::move(p)));
        reset();
        return parsed;
      }
      auto v = error.value();
      v.record(name + " (alt)");
      reset();
      return ParsingError::get<S, T>(v);
    }
    return {};
  }

  ParserResult<S, T> operator()() override {
    // This function is similar to the previous one, the only different
    // is we don't have an input. Return if we have any result, and fail if no
    // result.
    bool allCompleted = true;
    for (unsigned int i = 0; i < completed.size(); ++i) {
      if (!completed[i]) {
        auto r = (*options->at(i))();
        if (r.has_value()) {
          completed[i] = true;
          if (isError(r)) {
            error = std::optional(asError(r));
          } else {
            auto &v = asResult(r);
            result = std::make_unique<StateResult>(std::move(v));
          }
        } else {
          allCompleted = false;
        }
      }
    }
    if (result != nullptr) {
      AbstractParserResultPtr<S, T> p = std::move(result);
      auto parsed = std::make_optional(
          std::variant<ParsingError, decltype(p)>(std::move(p)));
      reset();
      return parsed;
    }
    reset();
    if (error.has_value()) {
      auto v = error.value();
      v.record(name + " (alt)");
      return ParsingError::get<S, T>(v);
    }
    // we have nothing matched nor any error. Actually this should not happen
    // as the parsers should return something when they encounter operator()()
    // which indicates the end of input.
    return ParsingError::get<S, T>("Insufficient Tokens", name);
  }

  const std::string &getName() override { return name; }

  auto &getOptions() { return options; }

  template <typename array>
  static auto get(const std::string &name, array &&args) {
    auto list = std::make_unique<std::vector<AbstractParserPtr<S, T>>>();
    for (auto &i : args)
      list->push_back(std::move(i));
    return std::make_unique<Alternate<S, T>>(std::move(list), name);
  }
};

} // namespace Parser