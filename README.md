# 🐚 Mysh: A Simple Command-Line Shell

**Author:** Grace Giebel


Mysh is a custom Unix-like shell built in C. It allows users to run commands interactively or via batch mode, supporting features like piping, redirection, wildcard expansion, and several built-in commands. It is designed to be robust, error-tolerant, and straightforward for the user.

---

## 📂 Features

- Interactive and Batch Mode
- Built-in Commands: `cd`, `pwd`, `exit`, `which`
- External Command Execution
- Input/Output Redirection: `>`, `<`, `>>`
- Wildcard Expansion: e.g., `*.txt`
- Piping: e.g., `ls | grep mysh | wc -l`
- Graceful Error Handling

---

## ⚙️ How to Build

Run the following command in the project directory:

```
make
```

To clean up build files:

```
make clean
```

---

## 🚀 Usage

### Interactive Mode

```
./mysh
```

You can then enter commands like:

```
ls -l
cd src
pwd
```

### Batch Mode

You can also provide a file with commands:

```
./mysh batchfile.txt
```

---

## 🔍 Design Overview

### Input Handling
- In interactive mode, commands are read from standard input.
- In batch mode, commands are read from a file line by line.

### Built-in Commands

- `cd`: Changes the current directory.
- `pwd`: Prints current directory.
- `exit`: Exits the shell.
- `which`: Locates the full path of a given command.

### External Commands
If a command isn't built-in, `mysh` uses `fork()` and `execvp()` to run it in a new process.

### Redirection

- `>` redirects standard output
- `<` redirects standard input
- Can combine both: `./mysh < input.txt > output.txt`

### Wildcard Expansion

Supports simple globbing like `*.txt`. If no match is found, the literal string is passed to the command.

### Piping

Commands can be chained using the `|` symbol. For example:

```
cat input.txt | grep "search" | sort
```

### Error Handling

All errors like:

- Invalid commands
- Bad redirection syntax
- Missing files  

Are caught and handled gracefully with user-friendly error messages.

---

## 🧪 Testing

Tested cases include:

- Built-in command functionality (`cd`, `pwd`, `which`, `exit`)
- Piping: `ls | wc -l`, etc.
- Wildcards: `*.c`, `*.txt`
- Redirection: `cat < input.txt > output.txt`
- Batch mode using scripts
- Invalid inputs, typos, and edge cases

All scenarios were verified to avoid shell crashes and ensure consistent behavior.

---

## 📁 Repository Structure

```
├── Makefile
├── mysh.c
├── mysh         # compiled executable
├── myscript.sh  # optional batch test script
├── output.txt   # redirected output
├── testfile.txt # test input file (optional)
└── README.md
```

---

## 📜 License & Attribution

This project was developed as part of coursework and is intended for educational purposes only.







