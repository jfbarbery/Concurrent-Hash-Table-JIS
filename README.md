# Concurrent-Hash-Table-JIS
A group project for an Operating Systems class at UCF, where we will implement an concurrent hash table, that allows multi-threaded access to the hash table.

How to run the program:
After cloning the repo (or downloading source code manually), simply type "make" in this directory.

Developers note:
If you want to run test cases with multiple different input files,
you'll need to do one of two things.
1. Change the "commands.txt" open() argument to the name of the desired input file
2. Change each input file to "commands.txt" before testing
Additionally, if you choose option 2, you'll also need to run "make clean", before running "make"