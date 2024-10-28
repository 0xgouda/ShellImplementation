# Simple Shell 
- (gouda shell aka gsh)

- A beginner shell project in C that supports basic commands, output redirection, command repetition, and batch file processing. 

## Features

- **Built-in Commands**:
  - `cd <path>`: Change the current directory to `<path>`.
  - `exit`: Exit the shell.
  - `path <dir1> <dir2> ...`: Update the shell's `PATH`.
- **Executable Commands**: Run commands located in directories from `PATH` default `/bin/`.
- **Loop Command**: Repeat a command multiple times.
  - `loop <number> <command>`: Run `<command>` `<number>` times.
- **Batch File Mode**: Execute commands from a file provided as an argument.

## Usage Example
```bash
gcc -o gsh gsh.c
```

```bash
gsh> cd /path/to/directory
/path/to/directory
gsh> path /bin/ /usr/bin/
/bin/
/usr/bin/
gsh> loop 3 echo "Hello"
Hello
Hello
Hello
gsh> exit
```
