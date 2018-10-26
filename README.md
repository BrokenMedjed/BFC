# BFC - BrainFuck Compiler

<strong>Coming in with the creative names again, medjed. -r</strong><br />
Anyways, this is mainly a translator for Brainfuck, that
generates 64-bit NASM code (and compiles it for you if you want). It does
have an interpreter, but it's experimental and doesn't work
with some programs, and I have no idea how to fix it :p. I tested it
using a
[Sierpinski Triangle](http://www.hevanet.com/cristofd/brainfuck/sierpinski.b)
program and a
[Mandelbrot set viewer](https://github.com/ErikDubbelboer/brainfuck-jit/blob/master/mandelbrot.bf);
the former worked, the latter didn't.
The compiler works fine though, with every program.<br /><br />
This thing is probably insecure to use compared to some implementations,
considering the compiler doesn't have any protections against pointer
decrements/increments that go out of bounds, nor input reads that go out
of bounds. The interpreter has the former two, but not the input checks.<br /><br />
oh yeah, I should probably mark that reading input doesn't conform to
normal brainfuck rules; if you have several consequent IN operators
(commas), all of them are going to be read at once and copied in that
order into consequent cells, beginning at the current address of the
pointer.<br />
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
