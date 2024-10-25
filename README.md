# Simple Shell 
- (gouda shell aka gsh)

- A beginner shell project in C that supports basic commands, output redirection, command repetition, and batch file processing. 

## Features

- **Built-in Commands**:
  - `cd <path>`: Change the current directory to `<path>`.
  - `exit`: Exit the shell.
  - `path <dir1> <dir2> ...`: Update the shell's `PATH`.
- **Executable Commands**: Run commands located in directories from `PATH`.
- **Loop Command**: Repeat a command multiple times.
  - `loop <number> <command>`: Run `<command>` `<number>` times.
- **Batch File Mode**: Execute commands from a file provided as an argument.

## Usage Examples

```bash
gsh> cd /home
gsh> pwd
/home
gsh> some wrong command
An error has occurred
gsh> path /bin/ /usr/bin/
gsh> loop 3 whoami
ahmed
ahmed
ahmed
gsh> echo "Hello" > file.txt
gsh> cat file.txt
Hello
gsh> exit
```
