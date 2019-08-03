# BFC - BrainFuck Compiler

<strong>Coming in with the creative names again, medjed. -r</strong><br />
This is a compiler/translator for Brainfuck, that generates 64-bit NASM code
(and assembles it for you using NASM if you want). I compiled these:
[Sierpinski Triangle](http://www.hevanet.com/cristofd/brainfuck/sierpinski.b)
[Mandelbrot set viewer](https://github.com/ErikDubbelboer/brainfuck-jit/blob/master/mandelbrot.bf);
Both work fine, so I'm assuming it'll work with every program.<br /><br />
This is probably insecure to use compared to some implementations,
considering this doesn't have any checks against pointer
decrements/increments or input reads that go out of bounds.<br /><br />
reading input also doesn't conform to normal brainfuck rules; if you
have several consequent IN operators
(commas), all of them are going to be read at once and copied in that
order into consequent cells, beginning at the current address of the
pointer. I think it makes way more sense this way.<br />

## Dependencies
basically, nasm and gcc.<br />

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
