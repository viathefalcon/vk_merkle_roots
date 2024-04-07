# Vulkan Merkle Roots

## Summary
A program to demonstrate the calculation/resolution of the roots of [Merkle/hash trees](https://en.wikipedia.org/wiki/Merkle_tree) of arbitrary inputs on GPUs via the [Vulkan API](https://en.wikipedia.org/wiki/Vulkan).

## Components
### `vkmr`
This is the primary program; it reads length-delimited inputs from `stdin` and then calculates their Merkle root, both on the CPU and on each compute-capable GPU reported by Vulkan.

### `strm`
This helper program accepts an arbitrary number of command-line arguments and writes them to a length-delimited stream in `stdout`.

### `rndm`
This helper program generates a specified volume of randomly-filled input strings and writes them to a length-delimited stream in `stdout`.

## Building, Running
The Visual Studio Code project includes tasks which will build the programs; it assumes that either the Visual C++ compiler (on Windows) or Clang (elsewhere) is on the `PATH`.

### Steam Deck

To build the project on Steam Deck, I run [VS Code in Flatpak](https://flathub.org/apps/com.visualstudio.code). For building, and running, on Steam Deck, the project has an implicit dependency on [LLVM v16](https://releases.llvm.org/16.0.0/docs/ReleaseNotes.html), which I satisfy with the [LLVM 16 extension for the flatpak Freedesktop SDK](https://github.com/flathub/org.freedesktop.Sdk.Extension.llvm16). I installed both VS Code and the LLVM 16 extension via the Discovery package manager.

#### Launching VS Code
(and enabling the LLVM16 extension)
```
FLATPAK_ENABLE_SDK_EXT=llvm16 flatpak run --devel com.visualstudio.code
```

#### Launching Vulkan Merkle Roots
The program, once built, also needs to run in a Flatpak container in which the LLVM16 extension is available. So, I open a shell in the VS Code sandbox:
```
FLATPAK_ENABLE_SDK_EXT=llvm16 flatpak run --command=sh --devel com.visualstudio.code
```

And, inside this shell:
```
cd bin
./rndm.app 1712489279 1024 127 | ./vkmr.app
```