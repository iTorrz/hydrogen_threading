# Hydrogen Lang - Concurrent Compiler (x86-64 + Pthreads)

A minimal concurrent programming language (Hy) and compiler backend targeting x86-64 NASM with pthreads, designed to explore shared memory concurrency and TSO weak memory behaviors.

## Building

Download `nasm` and `ld` on a Linux operating system for execution.

```bash
git clone git@github.com:iTorrz/hydrogen_threading.git
cd hydrogen_threading
mkdir build
cmake -S . -B build
cmake --build build
./build/hydro test.hy
./out
```

Executable will exist in the `build/` directory under the name `hydro`.

## Inspired by `Pixeled`
YouTube video series "[Creating a Compiler](https://www.youtube.com/playlist?list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT241qs)" 
