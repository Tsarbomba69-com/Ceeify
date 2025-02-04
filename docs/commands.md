#### Generate the build system

```bash
cmake . .
```

#### Build the project

```bash
cmake --build .
```

#### Run the tests

```bash
ctest
```

#### Run linter

```bash
clang-tidy $your_code.c --checks=*
```

#### Reformat entire project

```bash
find $dir -name "*.c" -o -name "*.h" | xargs clang-format -i
```

#### Run linter for the entire dir

```bash
find $dir -name "*.c" -o -name "*.cpp" | xargs clang-tidy --checks=*
```

#### Run Valgrind

```bash
valgrind --leak-check=full ./your_program
```

#### Run Valgrind for the entire dir

```bash
find . -name "*.c" -o -name "*.cpp" | xargs valgrind --leak-check=full
```

#### Run debugger

```bash
gdb ./$my_program
```

#### Set breakpoint

```bash
break $my_program.c:10
```
