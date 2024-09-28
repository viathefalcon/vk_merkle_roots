# Vulkan Merkle Roots

## Summary
A program to demonstrate the calculation/resolution of the roots of [Merkle/hash trees](https://en.wikipedia.org/wiki/Merkle_tree) of arbitrary inputs on GPUs via the [Vulkan API](https://en.wikipedia.org/wiki/Vulkan).

## Background

Initially, the sole goal - incorporating a few subquests, such as implementing the target hash function, SHA-256, from scratch for arbitrary inputs - was to investigate the feasibility of using GPUs to accelerate the generation of blockchain block headers. 

Pulling a proof-of-concept together, using OpenCL, really wasn't too difficult, but iterating on it, while remaining within the constraints I had set for myself (most importantly: to not buy any new hardware) proved .. if not impossible then certainly impractical. 

The main sticking point was overcoming the requirement for a synchronous round trip between the host (CPU) and the accelerator (GPU) for each level of the tree. The specs for OpenCL 2.x+ actually provide a mechanism to avoid this: [Device-Side Enqueue](https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_API.html#device-side-enqueue).

But, support for OpenCL across vendors and devices is varied, to put it mildly, and I could only get device-side enqueueing to work on one (1) integrated Intel GPU. (When I tried to code around this, using OpenCL ~1.2 and pushing as much of the conditional logic as I could into compute shaders, I ended up with [this hot mess](https://gist.github.com/viathefalcon/6d82a14214d6e4f7af29b75133ef6c16).)

It was the arrival of a Steam Deck that prompted this reboot of the project, based on Vulkan.

## Components
### `vkmr`
This is the primary program; it reads inputs from `stdin` and then calculates their Merkle root, both on the CPU and on a selected compute-capable GPU reported by Vulkan.

### `strm`
This helper program accepts an arbitrary number of command-line arguments and writes them to a line-separated stream in `stdout`.

### `rndm`
This helper program generates a specified volume of randomly-filled input strings and writes them to a line-separated stream in `stdout`.

## Building, Running
The Visual Studio Code project includes tasks which will build the programs; it assumes that either the Visual C++ compiler (on Windows) or Clang (elsewhere) is on the `PATH`.

### On (Steam) Deck

To build the project on Steam Deck, I run [VS Code in Flatpak](https://flathub.org/apps/com.visualstudio.code). For building, and running, on Steam Deck, the project has an implicit dependency on LLVM 18, which I satisfy with the [LLVM 18 extension for the flatpak Freedesktop SDK](https://github.com/flathub/org.freedesktop.Sdk.Extension.llvm18). I installed VS Code via the Discovery package manager, and the LLVM extension via the command-line:
```
flatpak install org.freedesktop.Sdk.Extension.llvm18
```

#### Launching VS Code
(and enabling the LLVM 18 extension)
```
FLATPAK_ENABLE_SDK_EXT=llvm18 flatpak run --devel com.visualstudio.code
```

#### Launching Vulkan Merkle Roots
The program, once built, also needs to run in a Flatpak container in which the LLVM16 extension is available. So, I open a shell in the VS Code sandbox:
```
FLATPAK_ENABLE_SDK_EXT=llvm18 flatpak run --command=sh --devel com.visualstudio.code
```

And, inside this shell:
```
cd bin
./rndm.app 1712489279 1024 127 | ./vkmr.app
```