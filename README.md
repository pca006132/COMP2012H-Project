# COMP2012H Project - Parser Combinator

Originally we were doing a C preprocessor, however as we were writing the parser by hand, the code becomes very complicated. After asking Desmond, we decided to do it with a parser combiantor. However, it seems that parser combinators are not popular within the C++ community, I can only find [pcomb](https://github.com/grievejia/pcomb) which uses nearly no OOP patterns. We decided to implement our own parser combinator using OOP and template.

The git repo can be found at [https://github.com/pca006132/COMP2012H-Project](https://github.com/pca006132/COMP2012H-Project).

We used c++17 for the variant and optional type, template parameter deduction and some other features to simplify our code. This is possible in older version but it would be more readable and simple using c++17 constructs.

Compile: make. Dependency: clang++, but other compilers can be used with minor changes to the build script.

## Design
> Notations: `A | B` is used to indicate `std::variant<A, B>`, `A?` indicates `std::optional<A>`. Smart pointer types are omitted.

The (abstract) parser would consume tokens of type `S`, and turn it into tokens of type `T`. The token `S` can be applied to the parser, and the parser would return a `ParserResult<S, T>`. 

The parser result is of type `(ParsingError | AbstractParserResult<S, T>)?` If the parser is not finished (waiting for new inputs), it would return nothing. If the input does not match with the parser, it would return `ParsingError`. If the parsing is completed, it would return a `AbstractParserResult<S, T>` and reset itself. `AbstractParserResult<S, T>` provides 2 functions: `get() -> T?` and `getRemaining() -> S?`. The `getRemaining` function enables the parser to look ahead and return the tokens that it should not consume. That also enabled the parser to expand the input, for example when doing macro expansion, it can return extra tokens. The `get` function would return all the tokens that the parser evaluated. We use a stream like interface to allow lazy evaluation of the tokens in most cases. The parsers are greedy, they would consume every tokens they can. In order to indicate the end of the token stream, the program can apply void to the parser (the parser implements `ParserResult<S, T> operator()()`).

The parsers can be reset, as parsers generally have internal states. And parsers can be cloned (without cloning the internal states). 

We implemented the following combinators:
* Predicate: Match the input using a (stateful) predicate function, and a quantifier. For example the predicate parser can be used to match against `a*`, `a+`, `a?` or `a{n}` (where `n` is an integer). As the predicate can be stateful, it can be used to match against specific strings by providing custom predicates. It can also be used to indicate negative lookahead.
* Alternate: Parse the input using multiple parsers, and return the result of the last match or error if none of them matches the input.
* Sequence: Concat the parsers. Return the results of the individual parsers.
* TakeTill: With two parsers `N` and `M`, the parser would check if the input matches `M`, if no it would match it against `N` and repeat the pattern. This is used to apply the parser `N` repeatedly until a terminating sequence `M` is matched. The `many` construct provided by other parser combinator frameworks can be done by providing a parser that would always fail, such as a predicate parser with predicate `(S) -> True` and quantifier `None`.
* Lazy: Every parser contains a unique pointer to its sub-parsers. As a result, using the above parsers alone cannot build a recursive parser (without breaking the rule of unique pointer, of cause). The Lazy combinator is just a wrapper around the parsers. The sub-parsers will be instantiated lazily (when tokens are applied), and would be deleted when reset to save memory. However this parser requires a special condition: the source parser has to outlive the lazy parser and all its instances. Shared pointer cannot be used as this parser is typically used to implement recursion, and shared pointer with such a loop would cause memory leak. Usually the source parser would have a static lifetime.

In the implementation, as we allow the parser to return extra tokens of type `S`, the combinators have to be careful when dealing with the result of the sub-parser.

## Limitation
Regular expression cannot be implemented directly by the above combinators, as we are not doing NFA. It is possible to try match against the parsers and add states if success, but it is not efficient. Instead specific regular expressions should be implemented by transforming the regular expression first.

To simplify the implementation, some combinators would consume all tokens of type `T` returned from the `get()` function of the result of the sub-parser. This would prevent the parser from lazy-evaluating the returned tokens. This can be fixed quite easily.

A branch construct should be provided as a special case of alternate construct, where the parser used depends on the result of another parser. For example to match comment if the pattern `//` or `/*` is encountered. Alternate construct can handle such cases but the performance would not be great in such cases, as additional computation is needed if every parser matches the input.

The predicate parser currently only support fixed quantifier, it would be better to change the stateful predicate to return an enum, telling the parser whether it is matched, satisfied or failed, as the current approach limited the predicate to match a fixed size input.

As I am not very familiar with C++, the implementation may be a bit ugly and not effective. If the type is wrong in the user code, mysterious multi-page template substitution error would be shown to the user which is not very helpful. The only solution I can think of is to use another language with better type system, basically all modern languages other than C++ can do this better.
