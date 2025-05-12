## setup:
(run from project root folder)
```sh
git clone https://github.com/libsdl-org/SDL vendored/SDL
git clone https://github.com/libsdl-org/SDL_ttf vendored/SDL_ttf
git clone https://github.com/ocornut/imgui vendored/imgui
```

and download single files (e.g. with curl):
```sh
curl 'https://github.com/hanickadot/compile-time-regular-expressions/tree/main/single-header/ctre.hpp' > vendored/
curl 'https://github.com/canonical/Ubuntu-Sans-Mono-fonts/blob/main/fonts/ttf/UbuntuSansMono-Regular.ttf' > inputs/
```

building:
```sh
cmake -S . -B out
cmake --build out
```

running the program then MUST be done FROM THE PROJECT ROOT PATH, e.g.
```sh
.../assembly_editor> out/Debut/hello.exe
```
otherwise it will not find the input files.

opening the folder with Visual Studio also builds everything fine (cloning and downloading must still be done manually), 
but then executes somewhere else, the VS_DEBUGGER_WORKING_DIRECTORY line in CMakeLists.txt does not seem to fix this.

