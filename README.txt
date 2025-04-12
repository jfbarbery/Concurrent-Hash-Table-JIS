# Concurrent-Hash-Table-JIS
A group project for an Operating Systems class at UCF,
where we will implement an concurrent hash table,
that allows multi-threaded access to the hash table.

How to run the program:
After cloning the repo (or downloading source code manually),
simply run "make" and ./chash

Developers note:
If you want to test different commands.txt text files, swap them out in dev/
Running "make test" will compile and run the program with the "commands.txt" file in dev/

Graders note on AI usage:
In this assignment, we used AI as an understanding tool and as a debate tool to make decisions
on how we should create threads, how granular we should be with our reader writer locks, and
to help us debug random problems, like for example getting garbage in our nodes.
Here's a link to the full extent of our AI usage in one conversation on GPT:
https://chatgpt.com/share/67f9d48a-0df4-8008-b7af-c3e15c7ea256