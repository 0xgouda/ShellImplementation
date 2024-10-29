# Simple Shell __gsh__

A beginner-friendly shell project implemented in C. This shell supports fundamental commands, output redirection, command repetition, and batch file processing.

## Features

- **Built-in Commands**:
  - `cd <path>`: Change the current directory to `<path>`.
  - `exit`: Terminate the shell session.
  - `path <dir1> <dir2> ...`: Update the shell's search `PATH` with specified directories.

- **Executable Commands**: Run commands from directories listed in the `PATH`, defaulting to `/bin/`.

- **Loop Command**: Execute a command multiple times.
  - `loop <number> <command>`: Repeat `<command>` a specified `<number>` of times.

- **Background Execution**: Use `&` to run commands in the background.

- **Batch File Mode**: Execute commands from a file provided as an argument.

## Usage Example

To compile and run the shell, use the following commands:

```bash
gcc -o gsh gsh.c
./gsh
```
```bash
gsh> cd /path/to/directory
gsh> path /bin/ /usr/bin/
gsh> loop 3 echo "Hello"
Hello
Hello
Hello
gsh> exit
```