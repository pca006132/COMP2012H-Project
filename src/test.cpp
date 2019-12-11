#include "Alternate.hpp"
#include "Predicate.hpp"
#include "Sequence.hpp"
#include <cassert>
#include <iostream>

typedef Parser::PredicateParser<char, std::string, Utils::fromChar, Utils::fold>
    CharPredicate;
auto conv(std::optional<std::variant<
              Parser::ParsingError,
              std::unique_ptr<Parser::AbstractParserResult<char, std::string>>>>
              &&v) {
  return std::get<
      std::unique_ptr<Parser::AbstractParserResult<char, std::string>>>(
      std::move(v.value()));
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
    b.reset();
    auto v = conv(b('b'));
    assert(v->getRemaining().value() == 'b');
    assert(v->getRemaining().has_value() == false);
  }
  {
    std::cout << "Predicate 4" << std::endl;
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
    assert(std::holds_alternative<Parser::ParsingError>(parser('c').value()));
  }
  {
    std::cout << "Sequence 4" << std::endl;
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
  {
    std::cout << "Alternate 2" << std::endl;
    parser.reset();
    for (char c : std::array{'f', 'o', 'o', 'b', 'a'}) {
      auto v = parser(c);
      assert(v.has_value() == false);
    }
    auto v = conv(parser('g'));
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

int main() {
  trivialPredicateTest();
  stringPredicateTest();
  sequenceTest();
  alternateTest();
  return 0;
}