# Assembly Editor
## The Idea
- fearless editing with correctness preserving changes
- direct and visual performance feedback from simulated and real testing execution

## Current State
### basic editing on x86 assembly (in proprietary format)
- only really supports the general purpose registers, no control flow
- isa spec read from extension files
### full undo/redo
### other positions
- support high bytes in change-builder
- moving for xmm-registers
- variable displaying of all registers, flags

## Roadmap
### other positions
- understanding of the stack / memory around a register
### control flow
- jump table, updated with changes
- basic blocks, previous/next instruction(-s)
- mark registers dead at end
- eliminate dead swaps/moves
### file format
- in/out assembly/machine code
- replace regex parsing with custom solution
### performance simulation
- simulated backend
- detailed example project, usecase
- new visualization with results
- new design, workflow, UI
### more elaborate change tree
### higher level language
- export to higher level
- update from higher level changes

## Setup after cloning the Repository
(run from project root folder)
```sh
git clone https://github.com/libsdl-org/SDL vendored/SDL
git clone https://github.com/libsdl-org/SDL_ttf vendored/SDL_ttf
git clone https://github.com/ocornut/imgui vendored/imgui/imgui
```
prepare SDL:
```sh
.../assembly_editor/vendored/SDL> cmake . -DSDL_STATIC=ON
```
prepare SDL_ttf, on Windows:
```sh
vendored/SDL_ttf/external/Get-GitModules.ps1
```
or on linux
```sh
vendored/SDL_ttf/external/download.sh
```

and download single files (e.g. with wget):
```sh
wget -O vendored/ctre.hpp https://raw.githubusercontent.com/hanickadot/compile-time-regular-expressions/main/single-header/ctre.hpp
wget -O 'inputs/UbuntuSansMono-Regular.ttf' 'https://raw.githubusercontent.com/canonical/Ubuntu-Sans-Mono-fonts/main/fonts/ttf/UbuntuSansMono-Regular.ttf'
```

building:
```sh
cmake -S . -B build
cmake --build build
```

running the program then MUST be done FROM THE PROJECT ROOT PATH, e.g.
```sh
.../assembly_editor> build/Debug/hello.exe
```
otherwise it will not find the input files.

opening the folder with Visual Studio also builds everything fine (cloning and downloading must still be done manually), 
but then executes somewhere else, the VS_DEBUGGER_WORKING_DIRECTORY line in CMakeLists.txt does not seem to fix this.

