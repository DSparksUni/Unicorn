# [Unicorn](https://github.com/DSparksUni/Unicorn) (`uni`)

Unicorn is a small stack-based (concatenative) programming language using LLVM as a backend.

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
(`swap`/`over`)

### I/O
```c
  42 printi   // 42

  34.9 printf // 34.9 (the 'f' in this case means float)

  "Hello" prints // Hello
```

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

### Functions (`func`)
- `func name(<arg_name>: <arg_type>, ...): <ret_type>, ...` declares a function
  with the specified arguments and return types.
- Since Unicorn is stack-based, there didn't seem to be a reason not to allow
  multiple return values, so return values can be delimited by type names and commas.
- There is no explicit `return` (yet), so values are returned based on what items are
  on the stack at the end of the function. The stack is required to match the return signature.

### Type Checking
- Static, stack-effect based type checker that runs before codegen. 

### CLI
- **CLI usage:** `uni <input_file> [-o <output_file>]` (defaults to `out.exe`).
- **Additional Options:**
  - `-h (--help) Prints a help message`
  - `--print-tokens Prints the generated token stream`
  - `--print-ops Prints the generated AST`
  - `-O (--opt-level) Sets the optimization level (0, 1, 2, 3, s, z)`

## Work in Progress
- Local (non-global) `let` bindings are parsed but rejected by the type
  checker — all variables are currently global.
- Currently, there is no way to declare and initialize a `let` binding in one
  statement. The current method would be `let x: int` and then `42 ->x`, but 
  having `let x: int = 42` would be more convenient.
- If explicitly assigning a `let` binding, the type should be allowed to be deduced.
- An `return` keyword to explicitly return from a function. This won't change
  the stack return at the end of the function, but allowing an early-exit would
  make sense.
- Changing the syntax of `if` to match `while` would be more in-line with other
  programming languages, and would allow for `elif`.

## Building

Requires LLVM, LLD, `cxxopts`, and `fast-float`, CMake, and a C++20 compiler.

## Running

```sh
uni program.uni -o program.exe
./program.exe
```

## Testing

The test suite is driven by ctest, and can be run as:
```sh
ctest --test-dir <DIR>
```
