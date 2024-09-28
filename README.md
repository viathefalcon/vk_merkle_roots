# Vulkan Merkle Roots

## Summary
A program to demonstrate the calculation/resolution of the roots of [Merkle/hash trees](https://en.wikipedia.org/wiki/Merkle_tree) of arbitrary inputs on GPUs via the [Vulkan API](https://en.wikipedia.org/wiki/Vulkan).

## Components
### `vkmr`
This is the primary program; it reads inputs from `stdin` and then calculates their Merkle root, both on the CPU and on each compute-capable GPU reported by Vulkan.

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