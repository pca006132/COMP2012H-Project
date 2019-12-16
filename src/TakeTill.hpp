#pragma once
#include "HelperResults.hpp"
#include "Parser.hpp"

namespace Parser {

/**
 * Apply the first parser many times until the terminating parser is matched.
 * Simple concept but there are some requirements on the parsers.
 * * The terminating parser should be independent of the first parser. It would
 *   match against every tokens.
 * * The first parser should not have lookahead, as the remainingTokens from the
 *   first parser would be discarded finally. I cannot think of alternatives as
 *   the terminating paresr is independent of the first parser, it does not make
 *   sense to have lookahead. Maybe the first assumption is wrong, the
 *   terminating parser cannot be independent of the first parser and this
 *   should be a recursive branch construct (with optimization).
 * Regarding the implementation, we match the suffix over all tokens, and store
 * each state independently (Bruteforce NFA). If the suffix failed to match the
 * tokens, the tokens will be applied to the parser. We keep track the tokens
 * using a queue, maintain a maximum number of tokens that is currently hold
 * by the different states of the suffix parser, and apply the remaining tokens
 * that are not held by the suffix parser.
 */
template <typename S, typename T, typename U>
class TakeTill : public AbstractParser<S, T> {
private:
  AbstractParserPtr<S, T> parser;
  AbstractParserPtr<S, U> suffix;
  std::deque<std::pair<int, AbstractParserPtr<S, T>>> suffixStates;
  std::unique_ptr<std::stack<AbstractParserResultPtr<S, T>>> prevResults;
  std::queue<S> tokens;
  std::queue<T> content;
  QueueParserResult<S, T> *input;
  std::string name;
  bool lastFinished = true;

  void consumeResult(AbstractParserResultPtr<S, T> result) {
    for (auto t = result->get(); t.has_value(); t = result->get()) {
      content.push(t.value());
    }
    prevResults->push(std::move(result));
  }

  ParserResult<S, T> consumeTokens(const int keep) {
    // keep is the maximum suffix length (maybe currently undetermined)
    int count = tokens.size() - keep;
    for (int i = 0; i < count; ++i) {
      auto &t = tokens.front();
      input->push(t);
      tokens.pop();
    }
    while (true) {
      // this part is similar to what happened in the sequence parser,
      // as the parser is applied repeatedly in a sequential manner.
      S v;
      bool hasValue = true;
      while (true) {
        if (auto t = prevResults->top()->getRemaining(); t.has_value()) {
          v = t.value();
          break;
        }
        if (prevResults->size() == 1) {
          // we don't delete the last one, that is the input queue
          hasValue = false;
          break;
        }
        prevResults->pop();
      }
      if (!hasValue)
        break;
      auto opt = (*parser)(v);
      if ((lastFinished = opt.has_value())) {
        if (isError(opt)) {
          auto error = asError(opt);
          error.record(name);
          reset();
          return ParsingError::get<S, T>(error);
        }
        consumeResult(std::move(asResult(opt)));
      }
    }
    return {};
  }

public:
  TakeTill(decltype(parser) parser, decltype(suffix) suffix,
           const std::string &name)
      : parser(std::move(parser)), suffix(std::move(suffix)), name(name) {
    reset();
  }

  void reset() override {
    parser->reset();
    suffixStates.clear();
    prevResults = std::make_unique<std::stack<AbstractParserResultPtr<S, T>>>();
    auto storage = std::make_unique<QueueParserResult<S, T>>();
    input = storage.get();
    prevResults->push(std::move(storage));
    while (!content.empty())
      content.pop();
  }

  AbstractParserPtr<S, T> clone() override {
    return std::make_unique<TakeTill<S, T, U>>(
        std::move(parser->clone()), std::move(suffix->clone()), name);
  }

  ParserResult<S, T> operator()(const S &value) override {
    tokens.push(value);
    // add new state
    suffixStates.push_back(std::make_pair(0, std::move(suffix->clone())));
    int max = 0;
    auto matched = std::unique_ptr<AbstractParserResult<S, U>>(nullptr);
    // delete states that does not match with the input, and track the length of
    // the suffix
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
    // if this returned something, it must be the parser failed to match the
    // input
    if (auto v = consumeTokens(max); v.has_value())
      return v;
    if (matched != nullptr) {
      // the suffix matched against the input
      // we must terminate the parser
      if (!lastFinished) {
        auto opt = (*parser)();
        if (!opt.has_value()) {
          reset();
          return ParsingError::get<S, T>("Insufficient Tokens", name);
        }
        if (isError(opt)) {
          auto error = asError(opt);
          error.record(name);
          reset();
          return ParsingError::get<S, T>(error);
        }
        consumeResult(std::move(asResult(opt)));
      }
      // discard the output of the suffix parser, as we don't want its output.
      while (matched->get().has_value())
        ;
      auto parsed = castResult<AggregatedParserResult<S, T>, S, T>(
          std::make_unique<std::stack<AbstractParserResultPtr<S, T>>>(),
          std::move(matched), content);
      reset();
      return parsed;
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
    if (matched == nullptr)
      return ParsingError::get<S, T>("Insufficient Tokens: Not Terminated",
                                     name);

    if (auto v = consumeTokens(max); v.has_value())
      return v;
    if (!lastFinished) {
      auto opt = (*parser)();
      if (!opt.has_value()) {
        reset();
        return ParsingError::get<S, T>("Insufficient Tokens", name);
      }
      if (isError(opt)) {
        auto error = asError(opt);
        error.record(name);
        reset();
        return ParsingError::get<S, T>(error);
      }
      consumeResult(std::move(asResult(opt)));
    }
    while (matched->get().has_value())
      ;
    auto parsed = castResult<AggregatedParserResult<S, T>, S, T>(
        std::make_unique<std::stack<AbstractParserResultPtr<S, T>>>(),
        std::move(matched), content);
    reset();
    return parsed;
  }

  const std::string &getName() override { return name; }

  static AbstractParserPtr<S, T> get(decltype(parser) parser,
                                     decltype(suffix) suffix,
                                     const std::string &name) {
    return std::make_unique<TakeTill<S, T, U>>(std::move(parser),
                                               std::move(suffix), name);
  }
};

} // namespace Parser