# Explanation
This custom shell only works in Linux System<br>
It runs Linux system call such as execvp(), fork(), etc.<br>
To execute this code in other Operation System such as Windows, Mac OS, you need to change OS dependent codes listed above.<br>

# Compile In Linux
Using gcc to compile the code <br>
codes are written in c
- gcc -o [compile_name] source_code
- Run with ./compile_name

# Commands
- cd
- other commands supported by execvp

Custom Commands<br>

- history : Shows commands history up to HISTORY_SIZE
- ! n : Execute nth history commands
- !! : Execute last commands executed

# Debug Modes
- To investigate flow of the codes, you set constant variable at line 15, DEBUG_MODE to 1 and run, default val : 0

# Todo
- Pipe instruction
