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

#### Run Valgrind

```
valgrind --leak-check=full ./your_program
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
