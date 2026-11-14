# CBuild

Sane build solution for C/C++

## Overview

CBuild is a drop-in single file static executable to build C/C++ projects using declarative toml recipes and optionally user-defined C++ code. CBuild is self-hosted, meaning that CBuild uses CBuild to build itself.

## How to Build

Because CBuild is a static executable, you need a musl compiler toolchain. You either have it packaged in your distro's repository or you can use [musl-cross-make](https://github.com/richfelker/musl-cross-make) to build it from source.

You need to use C++20 standard or newer. GCC version 11 or newer has good enough support for the most part.

**Project Dependencies**

- musl-GCC

- toml++

- fmt

- seq.hpp

- args

## Context

The problem with C/C++ is that they are basically just a specification and therefore do not provide any official tooling to the end user. Turning your source code into binaries should be an easier task than writing your actual program.

The currently available options are pretty terrible in their own way.

The most frequently suggested option is CMake but only because it is the most widely adopted. The problem with CMake is that it's another syntactically bloated language bolted on top of your source language, and the two languages don't know about each other. CMake also generally feels really slow when used.

You also have Meson. Meson is like a lesser-known stepsibling of CMake. It is a bit better because user-defined functions are intentionally disallowed and the syntax is more readable but fundamentally it uses the same model as CMake.

There are Make and Autotools, which both feel really outdated. They are like a shell script with extras so they aren't really portable. You cannot really use Makefiles on Windows unless you use "hacks" like WSL.

Finally, you have Zig Build as a modern contender. It works quite well and provides both speed and type safety. The problem is that you don't want to force a separate language as a dependency onto your project for no reason.
