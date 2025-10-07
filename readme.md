# 🧠 Ceefiy

**Ceefiy** is a **Python → C transpiler** written in C.
It reads Python source files, performs **lexical and syntactic analysis**, builds an **Abstract Syntax Tree (AST)**, and generates equivalent C code.

The project serves as an exploration of **compiler and transpiler design**, featuring modular architecture, memory-safe allocation strategies, and tooling for testing and debugging with **Valgrind**.

---

## 🚀 Features

* 🔤 **Lexer** — Tokenizes Python source into lexical units
* 🌲 **Parser** — Builds an Abstract Syntax Tree (AST)
* ⚙️ **AST Nodes & Linked List** — Efficient internal representation
* 🧠 **Arena & Allocator** — Custom memory management
* 🧩 **Transpilation Core** — Generates C syntax from AST nodes
* 🧪 **Unit Testing** — Lexer and parser test suite included
* 🧰 **Scripts & Tooling** — CMake + Bash build automation
* 🧼 **Code Linting** — Enforced via `clang-format`

---

## ⚙️ Building Ceefiy

All build operations are handled by the `build.sh` script at the scripts folder.

### 🔧 Generate Build Files

```bash
./scripts/build.sh gen
```

This runs CMake to generate project files under the `build/` directory.

### 🏗️ Build the Project

```bash
./scripts/build.sh build
```

This compiles the source and produces an executable (e.g. `ceeify`) under `./build`.

---

## 🧪 Testing & Debugging

### 🧠 Run Unit Tests

```bash
./build/test_ceeify
```

Or, to run tests under **Valgrind** (for memory debugging):

```bash
./build.sh valgrind_test
```

### 🔍 Run Ceefiy Under Valgrind

```bash
./build.sh valgrind_run path/to/script.py
```

Valgrind is configured with:

```bash
--leak-check=full --show-leak-kinds=all --track-origins=yes --num-callers=20 --track-fds=yes
```

---

## 🧼 Linting

To automatically format all `.c` and `.h` files using `clang-format`:

```bash
./build.sh lint src
./build.sh lint includes
```

This enforces your `.clang-format` style across the codebase.

---

## 🧬 Running the Transpiler

After building, you can transpile a Python file into C:

```bash
./build/ceeify tests/mock/if_elif.py -o output.c
```

This generates a valid `output.c` file containing equivalent C code.

---

## 📖 Documentation

* 📘 [`docs/commands.md`](docs/commands.md) — Command reference
* 🧩 `docs/ceefiy.png` — Visual overview of Ceefiy’s architecture

---

## 🧩 Future Roadmap

* ✅ Lexical and syntax analysis
* 🔄 AST-to-C code generation
* 🧮 Static type inference
* 🏎️ Optimized C output
* 🧰 CLI options for debugging and verbosity

---

## 🧑‍💻 Contributing

Contributions are welcome!

1. Fork the repository
2. Create a branch (`feature/your-feature`)
3. Commit and push your changes
4. Open a pull request

---

## 📚 Further Documentation

For a detailed overview of the system's design, architecture, and component interactions, please refer to the [docs](./docs) folder.

## 📝 Work in Progress

To see planned features, enhancements, and development priorities, check out the [TODO](./docs/TODO.md) file.
