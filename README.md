# multi-thread-communication
Project for PSiW in ANSI C about communication between processes.

#Installation
```gcc project.c -o project
./project <processName>
```
<processName> is taken from .config file

#Syntax
```<processName> &&& <command> &&& <fifoPath>```

#Example command
```usr1 &&& ls -l &&& ./queue1```
