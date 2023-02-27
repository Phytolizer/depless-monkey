# Monkey programming language

The [Monkey](https://interpreterbook.com/#the-monkey-programming-language)
programming language implemented in C, with no external dependencies.

## Compiling

You can compile any way you want; I use [Tup](https://gittup.org/tup/) for development
-- the provided Tupfile enables sanitizers and uses Clang
(see [Tuprules.tup](/Tuprules.tup) for exact flags).

If you are writing a build script yourself, the project is pretty simple:
- The compiler is implemented as a library in `/src`, with headers in `/include`.
- The unit tests are in `/test`, and use an additional include directory `/test/include`.
- The driver is in `/app`.
