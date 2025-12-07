# Hydrogen Lang - Concurrent Compiler (x86-64 + Pthreads)

A minimal concurrent programming language (hydrogen) and compiler backend targeting x86-64 NASM with pthreads, designed to explore shared memory concurrency and TSO weak memory behaviors.

## Breakdown

The compiler generates runnable Linux executables that spawn multiple threads and operate on shared global variables, allowing direct observation of weak TSO memory effects. 
```bash
/src
  parser.hpp         → AST definitions and grammar. Construction of program + worker bodies
  tokenization.hpp   → tokenizer 
  generation.hpp     → x86-64 NASM code generator (globals in .bss, pthread_create/join)
  main.cpp           → compiler driver
```

It supports the following core features:
### Global Shared Variables and Memory Assignment
```bash
global let x = 0;
global let y = 10;
y = x;
```
Allocated in .bss (shared across all threads). Initialized in main assembly text before workers start.
### Integer Expressions
Forms include Integer Literal, Variable Reads, and Addition
### Worker Threads
Define concurrent worker blocks as such:
```bash
|| worker_1
    x = 1;
    a = y;
||
//start threads
start_workers();
```
Worker blocks are generated to be C-style functions, where `start_workers` spin up for multithreading. `start_workers` function generates `main` label for pthread calling and pthread_create/pthread_join loop intiated for calling worker blocks as functions.

## Final Thoughts

I included print statements for global variables `a` and `b` so that one could see the final value when executable is run, of course though this is simply for manually observing `TSO Store Buffering` on a well known, simple problem. For manual checking of TSO behavior, use the following command and CTRL F on your terminal to check the executions:
```bash
for i in {1…<N>}; 
do ./out; done
```
Of course each config for variables `a` and `b` should be possible, though it may take you a couple executions of the command above.

## Challenges and Bottlenecks
### Global vs Local Variable Semantics
Originally, hydrogen supported both global and local variable declarations.
However, I found it really difficult to manage locals in a multithreaded environment required stack-frame bookkeeping, offset tracking, and correct teardown of scopes. This just caused unstable behavior when running the generated assembly, so I decided to leave it out for this moment. Thankfully this stabilized shared-memory and all user-defined variables are now treated as shared globals in .bss.
### Incorrect Section Placement
I hit several segmentation faults early on because I didn’t fully understand how NASM sections work. I was accidentally emitting main and worker instructions while still in the .bss section, which is meant only for uninitialized data. This caused NASM warnings and immediate crashes at runtime. I resolved these issues, but why many headaches.
### Calling Conventions and Expression Stack Management
Lastly, calling conventions and register handling gave me trouble at first. Getting pthread_create’s arguments into the right registers and avoiding accidental clobbering took some trial and error. My stack use in the final design is pretty barebones—since I’m not dealing with local stack frames yet, the stack is mostly used for temporary expression handling and reading values for shared memory operations.

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
