# BFC - BrainFuck Compiler

<strong>Coming in with the creative names again, medjed. -r</strong>
Anyways, this is mainly a translator for Brainfuck, that
generates 64-bit NASM code (and compiles it for you if you want). It does
have an interpreter, but it's experimental and doesn't work
with some programs, and I have no idea how to fix it :p. I tested it
using a Sierpinski Triangle program and a Mandelbrot set viewer;
the former worked, the latter didn't.
The compiler works fine though, with every program.<br />

## Compiling
Use any C++14 (or 11?) compatible compiler.
I use g++:
```
g++ -c bfc.cpp -Wall -std=c++14
g++ bfc.o main.cpp -Wall -std=c++14 -o bfc
```
I've included my types header in this repo, the reason why I use it is
that writing "U32" is cleaner than writing "uint32_t" :3<br />

## Usage

Run
```
bfc -h
```
after compiling to get a synopsis; the program follows normal UNIX
argument parsing rules.
