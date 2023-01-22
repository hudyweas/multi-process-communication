# Multi-Process-Communication
Project for PSiW in ANSI C about communication between processes.

## How it works

Each process separates itself into two processes: core and executing.

The core process takes user input and sends data to execute the command on the second program. After the execution, the result is sent back to the first program and then is shown on the terminal.

#### Caution!
To work you need to run the program at least twice (each time for another process from config). Each process can communicate only with the other one from the config.
## Installation
```
gcc project.c -o project
./project <processName>
```

## Config
Config stores process name and its own communication fifo file path to communicate with it.

```
<processName> <fifoPath>
```

## Syntax

```
<processName> &&& <command> &&& <fifoPath>
```

### Example commands
```
usr1 &&& ls -l &&& ./queue1
foo &&& boo &&& fooboo
exit
```
