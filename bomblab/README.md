A "binary bomb" is a program provided to students as an object code file. When
run, it prompts the user to type in 6 different strings. If any of these is
incorrect, the bomb "explodes," printing an error message and logging the event
on a grading server. Students must "defuse" their own unique bomb by
disassembling and reverse engineering the program to determine what the 6
strings should be. The lab teaches students to understand assembly language,
and also forces them to learn how to use a debugger. It's also great fun. A
legendary lab among the CMU undergrads.

Here's a [Linux/x86-64 binary bomb](http://csapp.cs.cmu.edu/3e/bomb.tar) that
you can try out for yourself. The feature that notifies the grading server has
been disabled, so feel free to explode this bomb with impunity.
