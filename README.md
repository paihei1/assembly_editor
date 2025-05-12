## setup:
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

