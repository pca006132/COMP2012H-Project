#include "Alternate.hpp"
#include "Lazy.hpp"
#include "Predicate.hpp"
#include "Sequence.hpp"
#include "TakeTill.hpp"
#include <cassert>
#include <iostream>

// some tests for the parsers.
// some functions for convenience

using CharPredicate =
    Parser::PredicateParser<char, std::string, Utils::fromChar, Utils::fold>;
auto conv(std::optional<std::variant<
              Parser::ParsingError,
              std::unique_ptr<Parser::AbstractParserResult<char, std::string>>>>
              &&v) {
  return std::get<
      std::unique_ptr<Parser::AbstractParserResult<char, std::string>>>(
      std::move(v.value()));
}

static Parser::AbstractParserPtr<char, std::string>
operator"" _c(const char *str, const std::size_t size) {
  std::string st(str, size);
  return Parser::StringPredicate(st, st);
}

void trivialPredicateTest() {
  auto a = CharPredicate('a', Parser::MORE, "Test");
  auto b = CharPredicate('a', Parser::ANY, "Test 2");
  {
    std::cout << "Predicate 1" << std::endl;
    a.reset();
    assert(std::holds_alternative<Parser::ParsingError>(a('b').value()));
  }
  {
    std::cout << "Predicate 2" << std::endl;
    a.reset();
    for (int i = 0; i < 4; ++i) {
      assert(a('a').has_value() == false);
    }
    auto v = conv(a('b'));
    assert(v->get().value() == "aaaa");
    auto remain = v->getRemaining();
    assert(remain.has_value() == true);
    assert(remain.value() == 'b');
    assert(v->getRemaining().has_value() == false);
  }
  {
    std::cout << "Predicate 3" << std::endl;
    a.reset();
    for (int i = 0; i < 4; ++i) {
      assert(a('a').has_value() == false);
    }
    auto v = conv(a());
    assert(v->get().value() == "aaaa");
    assert(v->getRemaining().has_value() == false);
  }
  {
    std::cout << "Predicate 4" << std::endl;
    b.reset();
    auto v = conv(b('b'));
    assert(v->getRemaining().value() == 'b');
    assert(v->getRemaining().has_value() == false);
  }
  {
    std::cout << "Predicate 5" << std::endl;
    a.reset();
    assert(std::holds_alternative<Parser::ParsingError>(a('b').value()));
  }
}

void stringPredicateTest() {
  auto a = *Parser::StringPredicate("abcd", "abcd");
  {
    std::cout << "String 1" << std::endl;
    a.reset();
    for (char c : std::array{'a', 'b', 'c'}) {
      auto v = a(c);
      assert(v.has_value() == false);
    }
    auto v = conv(a('d'));
    assert(v->get().value() == "abcd");
    assert(v->get().has_value() == false);
    assert(v->getRemaining().has_value() == false);
  }
}

void sequenceTest() {
  auto a = std::make_unique<CharPredicate>('a', Parser::OPTIONAL, "Test 1");
  auto b = std::make_unique<CharPredicate>('b', Parser::MORE, "Test 2");
  auto c = std::make_unique<CharPredicate>('c', Parser::OPTIONAL, "Test 3");
  auto d = std::make_unique<CharPredicate>('a', Parser::NONE, "Test 4");
  auto sequence = std::make_unique<std::vector<
      std::unique_ptr<Parser::AbstractParser<char, std::string>>>>();
  sequence->push_back(std::move(a));
  sequence->push_back(std::move(b));
  sequence->push_back(std::move(c));
  sequence->push_back(std::move(d));

  auto parser =
      Parser::Sequence<char, std::string>(std::move(sequence), "Parser");
  {
    std::cout << "Sequence 1" << std::endl;
    parser.reset();
    for (char c : std::array{'a', 'b', 'b', 'b'}) {
      assert(parser(c).has_value() == false);
    }
    auto value = conv(parser('d'));
    assert(value->get().value() == "a");
    assert(value->get().value() == "bbb");
    assert(value->get().has_value() == false);
    assert(value->getRemaining().value() == 'd');
    assert(value->getRemaining().has_value() == false);
  }
  {
    std::cout << "Sequence 2" << std::endl;
    parser.reset();
    for (char c : std::array{'b', 'b', 'b', 'c'}) {
      assert(parser(c).has_value() == false);
    }
    auto value = conv(parser('c'));
    assert(value->get().value() == "bbb");
    assert(value->get().value() == "c");
    assert(value->get().has_value() == false);
    assert(value->getRemaining().value() == 'c');
    assert(value->getRemaining().has_value() == false);
  }
  {
    std::cout << "Sequence 3" << std::endl;
    parser.reset();
    for (char c : std::array{'b', 'b', 'b'}) {
      assert(parser(c).has_value() == false);
    }
    auto value = conv(parser());
    assert(value->get().value() == "bbb");
    assert(value->get().has_value() == false);
    assert(value->getRemaining().has_value() == false);
  }
  {
    std::cout << "Sequence 4" << std::endl;
    parser.reset();
    assert(std::holds_alternative<Parser::ParsingError>(parser('c').value()));
  }
  {
    std::cout << "Sequence 5" << std::endl;
    parser.reset();
    parser('b');
    auto e = std::get<Parser::ParsingError>(parser('a').value());
    assert(e.toString() == "Unexpected a\n  at Test 4\n  at Parser");
  }
}

void alternateTest() {
  auto a = Parser::StringPredicate("foo", "foo");
  auto b = Parser::StringPredicate("foobar", "foobar");

  auto list = std::make_unique<std::vector<
      std::unique_ptr<Parser::AbstractParser<char, std::string>>>>();
  list->push_back(std::move(a));
  list->push_back(std::move(b));
  auto parser = Parser::Alternate<char, std::string>(std::move(list), "parser");
  {
    std::cout << "Alternate 1" << std::endl;
    for (char c : std::array{'f', 'o', 'o', 'b', 'a'}) {
      auto v = parser(c);
      assert(v.has_value() == false);
    }
    auto v = conv(parser('r'));
    assert(v->get().value() == "foobar");
    assert(v->get().has_value() == false);
    assert(v->getRemaining().has_value() == false);
  }
  auto parser2 = parser.clone();
  {
    std::cout << "Alternate 2" << std::endl;
    for (char c : std::array{'f', 'o', 'o', 'b', 'a'}) {
      auto v = (*parser2)(c);
      assert(v.has_value() == false);
    }
    auto v = conv((*parser2)('g'));
    assert(v->get().value() == "foo");
    assert(v->get().has_value() == false);
    for (char c : std::array{'b', 'a', 'g'}) {
      assert(v->getRemaining().value() == c);
    }
    assert(v->getRemaining().has_value() == false);
  }
  {
    std::cout << "Alternate 3" << std::endl;
    parser.reset();
    auto v = parser('g');
    assert(std::holds_alternative<Parser::ParsingError>(v.value()));
    assert(std::get<Parser::ParsingError>(v.value()).toString() ==
           "Insufficient tokens\n  at foobar\n  at parser (alt)");
  }
}

void takeTillTest() {
  Parser::AbstractParserPtr<char, std::string> ending =
      Parser::StringPredicate("aa/", "end");
  Parser::AbstractParserPtr<char, std::string> ending2 =
      std::make_unique<CharPredicate>('a', Parser::MORE, "end 2");
  Parser::AbstractParserPtr<char, std::string> pattern =
      std::make_unique<CharPredicate>('a', 2, "aa");
  auto parser = Parser::TakeTill<char, std::string, std::string>(
      pattern->clone(), std::move(ending), "parser");
  auto parser2 = Parser::TakeTill<char, std::string, std::string>(
      pattern->clone(), std::move(ending2), "parser2");
  {
    std::cout << "TakeTill 1" << std::endl;
    for (char c : std::array{'a', 'a', 'a', 'a', 'a', 'a'}) {
      auto result = parser(c);
      assert(result.has_value() == false);
    }
    auto result = conv(parser('/'));
    assert(result->getRemaining().has_value() == false);
    assert(result->get().value() == "aa");
    assert(result->get().value() == "aa");
    assert(result->get().has_value() == false);
  }
  {
    std::cout << "TakeTill 2" << std::endl;
    for (char c : std::array{'a', 'a', 'a', 'a', 'a', 'a'}) {
      auto result = parser2(c);
      assert(result.has_value() == false);
    }
    auto result = conv(parser2());
    assert(result->getRemaining().has_value() == false);
    assert(result->get().has_value() == false);
  }
}

void lazyTest() {
  // This is just to demonstrate that recursive parser is possible through the
  // lazy construct.
  // This would not be possible if we don't use lazy construct as it would need
  // infinite many sub-parsers for recursive parser.
  auto alternatives = std::make_unique<
      std::vector<Parser::AbstractParserPtr<char, std::string>>>();
  auto options =
      Parser::Alternate<char, std::string>(std::move(alternatives), "options");
  Parser::AbstractParserPtr<char, std::string> recursive =
      std::make_unique<Parser::LazyParser<char, std::string>>(&options);

  Parser::AbstractParserPtr<char, std::string> seq =
      Parser::Sequence<char, std::string>::get(
          "SEQ", std::array{"("_c, std::move(recursive), ")"_c});
  options.getOptions()->push_back(std::move(seq));
  options.getOptions()->push_back(CharPredicate::get('a', Parser::MORE, "a"));
  options.reset();

  {
    for (char c : "((aaa)") {
      if (c == '\0')
        break;
      auto v = options(c);
      assert(v.has_value() == false);
    }
    auto v = conv(options(')'));
    for (auto s : std::array{"(", "(", "aaa", ")", ")"})
      assert(v->get().value() == s);
    assert(v->get().has_value() == false);
    assert(v->getRemaining().has_value() == false);
  }
}

int main() {
  trivialPredicateTest();
  stringPredicateTest();
  sequenceTest();
  alternateTest();
  takeTillTest();
  lazyTest();
  return 0;
}