#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

using std::optional;
using std::unique_ptr;
using std::string;
using std::variant;
using std::vector;
using std::make_optional;

namespace Parser {

//abstract base classes for parsing result
template <typename S> class AbstractStream {
public:
  virtual ~AbstractStream() = default;
  virtual optional<S> get() = 0;
};

template <typename S, typename T>
class AbstractParserResult : public AbstractStream<T> {
public:
  virtual ~AbstractParserResult() = default;
  virtual optional<S> getRemaining() = 0;
};

//class declarations for alias
class ParsingError;
template <typename S, typename T> class AbstractParser;

//defining alias
template <typename S, typename T>
using AbstractParserResultPtr = unique_ptr<AbstractParserResult<S, T>>;

template <typename S, typename T>
using AbstractParserPtr = unique_ptr<AbstractParser<S, T>>;

template <typename S, typename T>
using ParserResult = optional<
    variant<ParsingError, AbstractParserResultPtr<S,T>>>;



class ParsingError {
private:
  string description;
  vector<string> stack;

public:
  ParsingError() : description("") {}

  ParsingError(const string &description, const string &name)
      : description(description) {
    stack.push_back(name);
  }

  void record(const string &name) { stack.push_back(name); }

  //combine all errors into 1 string
  string toString() const {
    string result = description;
    for (const auto &msg : stack) {
      result += "\n  at " + msg;
    }
    return result;
  }

  //static accessor functions
  template <typename S, typename T>
  static ParserResult<S, T> get(const string &desc,
                                const string &name) {
    return make_optional(
        variant<ParsingError, AbstractParserResultPtr<S,T>>(
            ParsingError(desc, name)));
  }

  template <typename S, typename T>
  static ParserResult<S, T> get(ParsingError &e) {
    return make_optional(
        variant<ParsingError, AbstractParserResultPtr<S,T>>(
            ParsingError(e)));
  }
};


template <typename S, typename T>
bool isError(const ParserResult<S, T> &result) {
  return std::holds_alternative<ParsingError>(result.value());
}

template <typename S, typename T>
AbstractParserResultPtr<S, T> &asResult(ParserResult<S, T> &result) {
  return std::get<AbstractParserResultPtr<S, T>>(result.value());
}

template <typename S, typename T>
ParsingError &asError(ParserResult<S, T> &result) {
  return std::get<ParsingError>(result.value());
}

//abstract base class for different parser combinators
template <typename S, typename T> class AbstractParser {
public:
  virtual ~AbstractParser() = default;

  virtual void reset() = 0;

  virtual ParserResult<S, T> operator()(const S &value) = 0;

  virtual ParserResult<S, T> operator()() = 0;

  virtual const string &getName() = 0;

  virtual AbstractParserPtr<S, T> clone() = 0;
};

template <typename R, typename S, typename T, typename... _Args>
inline ParserResult<S, T> castResult(_Args &&... args) {
  AbstractParserResultPtr<S, T> p =
      std::make_unique<R>(std::forward<_Args>(args)...);
  return make_optional(
      variant<ParsingError, decltype(p)>(std::move(p)));
}

} // namespace Parser
