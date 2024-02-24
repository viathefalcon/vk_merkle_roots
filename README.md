# Vulkan Merkle Roots

## Summary
A program to demonstrate the calculation/resolution of the roots of [Merkle/hash trees](https://en.wikipedia.org/wiki/Merkle_tree) of arbitrary inputs on GPUs via the [Vulkan API](https://en.wikipedia.org/wiki/Vulkan).

## Building
The Visual Studio Code project includes tasks which will build the programs; it assumes that either the Visual C++ compiler (on Windows) or Clang (elsewhere) is on the `PATH`.

## Components
### `vkmr`
This is the primary program; it reads length-delimited inputs from `stdin` and then calculates their Merkle root, both on the CPU and on each compute-capable GPU reported by Vulkan.

### `strm`
This helper program accepts an arbitrary number of command-line arguments and writes them to a length-delimited stream in `stdout`.

### `rndm`
This helper program generates a specified volume of randomly-filled input strings and writes them to a length-delimited stream in `stdout`.
