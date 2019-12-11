#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Parser {

template <typename S> class AbstractStream {
public:
  virtual ~AbstractStream() = default;
  virtual std::optional<S> get() = 0;
};

template <typename S, typename T>
class AbstractParserResult : public AbstractStream<T> {
public:
  virtual ~AbstractParserResult() = default;
  virtual std::optional<S> getRemaining() = 0;
};

class ParsingError {
private:
  std::string description;
  std::vector<std::string> stack;

public:
  ParsingError() : description("") {}
  ParsingError(const std::string &description, const std::string &name)
      : description(description) {
    stack.push_back(name);
  }

  void record(const std::string &name) { stack.push_back(name); }

  std::string toString() const {
    std::string result = description;
    for (const auto &msg : stack) {
      result += "\n  at " + msg;
    }
    return result;
  }

  template <typename S, typename T>
  static std::optional<
      std::variant<ParsingError, std::unique_ptr<AbstractParserResult<S, T>>>>
  get(const std::string &desc, const std::string &name) {
    return std::make_optional(
        std::variant<ParsingError, std::unique_ptr<AbstractParserResult<S, T>>>(
            ParsingError(desc, name)));
  }
  template <typename S, typename T>
  static std::optional<
      std::variant<ParsingError, std::unique_ptr<AbstractParserResult<S, T>>>>
  get(ParsingError &e) {
    return std::make_optional(
        std::variant<ParsingError, std::unique_ptr<AbstractParserResult<S, T>>>(
            ParsingError(e)));
  }
};

template <typename S, typename T> class AbstractParser {
public:
  virtual ~AbstractParser() = default;
  virtual void reset() = 0;
  virtual std::optional<
      std::variant<ParsingError, std::unique_ptr<AbstractParserResult<S, T>>>>
  operator()(const S &value) = 0;
  virtual const std::string &getName() = 0;
};

template <typename R, typename S, typename T, typename... _Args>
inline std::optional<
    std::variant<ParsingError, std::unique_ptr<AbstractParserResult<S, T>>>>
castResult(_Args &&... args) {
  std::unique_ptr<AbstractParserResult<S, T>> p =
      std::make_unique<R>(std::forward<_Args>(args)...);
  return std::make_optional(
      std::variant<ParsingError, decltype(p)>(std::move(p)));
}

} // namespace Parser