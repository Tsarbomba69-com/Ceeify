#### Generate the build system

```
cmake . .
```

#### Build the project

```
cmake --build .
```

#### Run the tests

```
ctest
```

#### Run linter

```
clang-tidy $your_code.c --checks=*
```

#### Reformat entire project

```
find $dir -name "*.c" -o -name "*.h" | xargs clang-format -i
```

#### Run linter for the entire dir

```
find $dir -name "*.c" -o -name "*.cpp" | xargs clang-tidy --checks=*
```

#### Run Valgrind

```
valgrind --leak-check=full ./your_program
```

#### Run Valgrind for the entire dir

```
find . -name "*.c" -o -name "*.cpp" | xargs valgrind --leak-check=full
```

#### Run debugger

```
gdb ./$my_program
```

#### Set breakpoint

```
break $my_program.c:10
```

#### Run debugger

```
gdb ./$my_program
```
