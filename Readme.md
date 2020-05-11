# Vykreslování vektorové grafiky
## Zprovoznění
### Windows
- požadovaná verze Visual Studia je 2019 16.4.x a přiložená verze MSVC (zkoušeno s aktuální 16.5 a 16.6 ale při kompilaci nastávala chyba `C1001 Internal compiler error`)
- nainstalovat [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows)
- nainstalovat `vcpkg` na správu závyslostí `https://github.com/Microsoft/vcpkg` (tato stránka obsahuje návod jak `vcpkg` nainstalovat)
- pomocí `vcpkg` nainstalovat závyslosti: `.\vcpkg.exe install vulkan-memory-allocator eigen3 tl-expected catch2 fmt glfw3 glm leaf spdlog shaderc boost-program-options`
- již stačí použít jakékoliv IDE podporující CMake (popř. pouze CMake) s nastavením `-DCMAKE_TOOLCHAIN_FILE=[vcpkg root]\scripts\buildsystems\vcpkg.cmake`
### OSX
- bylo vyzkoušeno pomoc `clang` verze `clang version 10.0.0`
- nainstalovat [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) (bude potřeba nainstalovat Vulkan SDK wrapper MoltenVK)
- nainstalovat `vcpkg` na správu závyslostí `https://github.com/Microsoft/vcpkg` (tato stránka obsahuje návod jak `vcpkg` nainstalovat)
- pomocí `vcpkg` nainstalovat závyslosti: `./vcpkg install vulkan-memory-allocator eigen3 tl-expected catch2 fmt glfw3 glm leaf spdlog shaderc boost-program-options`
- `mkdir build && cd build && cmake ../ -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]\scripts\buildsystems\vcpkg.cmake` (popř. využít jakkékoliv IDE podporující CMake)
## Použití
- využívá se zde programu `mff_runner`, který 

Příklady:
```
./mff_runner ./Ghostscript_Tiger.svg -s 5 -w 700 -h 800 --translate_x 1000
./mff_runner ./sample01.svg -s 7 -w 700 -h 700 --translate_x 700
```

## Popis zvoleného řešení
