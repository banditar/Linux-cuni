# Mysh

A simple shell implemented in C programming language.

Was done for the _Linux/Unix Programming in C_ class at Charles University, in my III. semester of my Bachelor's Degree of Computer Science.\
It can do _almost_ anything a normal bash can.\
Any command that runs in bash, runs here too:\
`$ ls`\
`$ cat text.txt`\
`$ sleep 10`\
...

In addition it is capable of the followings:
- You can use `cd`:\
`$ cd /testDir`
- Has error handling. That is, it's responsive to Ctrl + C:\
`$ sleep 10`\
`^CKilled by signal 2.`
- Usage of piplines: `|`:\
`$ date | cat | cat | cat | cat | cat | cat | sort -n | uniq`
- Three basic redirections: `>file`, `<file`, `>>file`:\
`$ mysh$ cat </etc/passwd </etc/group >output >>output2 >output3`
- Multiple commands in one line with the help of semicolons:\
`$ echo X; date; echo Z`

**Usage**

`$ make`\
`$ ./mysh`\
`$ ls`

Student:
- Jánosi József-Hunor

:8ball:
