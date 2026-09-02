# [Unicorn](https://github.com/DSparksUni/Unicorn) (`uni`)

A small stack-based (concatenative) programming language that compiles to
native executables via LLVM. Programs are lexed, parsed into an AST,
type-checked, then lowered to LLVM IR and handed off to `clang` for final
codegen and linking.

## Features

### Literals
- **Integers** — `1`, `42`, `-5`
- **Floats** — `1.5`, `-3.14`
- **Strings** — `"hello\n"`, with escapes for `\n`, `\t`, `\r`, `\\`, `\"`, `\0`

### Arithmetic & Comparison
- `+ - * /` — work on `int` or `float`, auto-promoting `int` to `float` when
  either operand is a float (`1 2.5 +` → `3.5`)
- `== != < > <= >=` — same numeric-mixing rules, always produce an `int`
  (`1`/`0`) result

### Stack Manipulation
- `dup` — duplicate the top value
- `drop` — discard the top value
- `swap` — swap the top two values
- `over` — copy the second value to the top

These are generic over any single type (`dup`/`drop`) or pair of types
(`swap`/`over`), enforced via type variables in the type checker.

### I/O
- `printi` — print an `int`
- `printf` — print a `float`
- `prints` — print a `string`

### Control Flow
- `if { ... }` / `if { ... } else { ... }` — branches on a leading `int`.
  An `if` with no `else` must be stack-neutral; `if/else` branches must
  leave the stack at the same depth with matching types (values can be
  produced by an `if/else`, e.g. `1 if { 42 } else { 99 }`).
- `while { <cond> } { <body> }` — the condition block must leave exactly
  one extra `int` on the stack; the loop body must be stack-neutral.

### Word Definitions (`def`)
- `def name { ... }` defines a new, reusable word from a block of code.
- Input/output types are **inferred** from the body, including support for
  polymorphic words (e.g. a `drop drop` word works over any two types).
- Definitions may call previously defined words, but nested definitions
  (a `def` inside a `def`) are rejected.
- Redefining an existing word name is a compile error.

### Global Variables (`let`)
- `let name: <type>` declares an immutable global, zero-initialized.
- `let mut name: <type>` declares a mutable global.
- `->name` stores the top of the stack into a variable (type-checked
  against the variable's declared type; storing into an immutable
  variable is an error).
- Supported type annotations: `int`, `float`, `string`.

### Type Checking
- Static, stack-effect based type checker that runs before codegen.
- Distinguishes `int`, `float`, `string`, a generic numeric type (`num`,
  matched by either `int` or `float`), and type variables (for
  polymorphic/generic words).
- Produces descriptive `[ERROR] (line N) ...` diagnostics for: stack
  underflow, type mismatches, unknown words/variables, unbalanced
  `if`/`while`, duplicate/nested definitions, and unknown type names.

### Compiler Pipeline & CLI
- Lexer → Parser → Type checker → LLVM IR Emitter → `clang` for final
  binary generation.
- CLI usage: `uni <input_file> [-o <output_file>]` (defaults to `out.exe`).

## Work in Progress
- `func` (named, typed functions with explicit argument/return lists) is
  partially parsed but not yet type-checked or code-generated.
- Local (non-global) `let` bindings are parsed but rejected by the type
  checker — all variables are currently global.

## Building

Requires LLVM, `cxxopts`, and `fast-float` (via vcpkg), CMake, and a
C++20 compiler.

## Running

```sh
uni program.uni -o program.exe
./program.exe
```

## Testing

The test suite (driven by CTest) compiles and runs each `.uni` file under
`test/pass` and checks its output against a corresponding `.expected`
file, and compiles each file under `test/fail` expecting a compile error
matching the corresponding `.expected` regex.

```sh
ctest --test-dir build
```
