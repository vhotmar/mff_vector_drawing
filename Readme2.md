# Vykreslování vektorové grafiky
## Zprovoznění
### Windows
Vyzkoušeno na čisté instalaci Windows 10 od [PaperSpace](https://www.paperspace.com/) 
- požadovaná verze Visual Studia je 2019 [16.4](https://docs.microsoft.com/en-us/visualstudio/releases/2019/history)
  (zkoušeno i s aktuální 16.5 a i s preview verzí 16.6 ale při kompilaci nastává chyba `C1001 Internal compiler error`)
- nainstalovat [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows)
- nainstalovat [`vcpkg`](https://github.com/Microsoft/vcpkg) na správu závyslostí
  ([tato stránka](https://github.com/Microsoft/vcpkg) obsahuje návod jak `vcpkg` nainstalovat)
- pomocí `vcpkg` nainstalovat závyslosti:
  `.\vcpkg.exe install vulkan-memory-allocator eigen3 tl-expected catch2 fmt glfw3 glm leaf spdlog boost-program-options range-v3`
- použít cmake s argumentem `-DCMAKE_TOOLCHAIN_FILE=[vcpkg root]\scripts\buildsystems\vcpkg.cmake`
  (pozor na použití konfigurace korespondující [tripletu](https://github.com/microsoft/vcpkg/blob/master/docs/users/triplets.md), které `vcpkg` použilo při instalaci závyslostí)
### OSX
Analogické zprovoznění na platformě Windows - bylo vyzkoušeno pomocí `clang version 10.0.0`.
## Použití
Funkce programu se demonstruje pomocí programu `mff_runner` (popř. `mff_runner.exe`, dále budu používat pouze
`mff_runner`). Tento program akceptuje řadu parametrů:

- `-s` - scale (má být zobrazené svg zvětšené / zmenšené)
- `-w` - výška okna
- `-h` - šířka okna
- `--translate_x` - posunutí zobrazeného obrázku podle x souřadnice
- `--translate_y` - posunutí zobrazeného obrázku podle y souřadnice
- a poziční argument je soubor který chcete zobrazit (musí korespondovat se specifikací, tedy pouze zjednodušené SVG
  s podporovanými argumenty, zbytek bude ignorován)

Příklady:
```
./mff_runner ./Ghostscript_Tiger.svg -s 5 -w 700 -h 800 --translate_x 1000
./mff_runner ./sample01.svg -s 7 -w 700 -h 700 --translate_x 700
```

## Popis zvoleného řešení
Původně jsem chtěl založit integraci s Vulkan API na knihovně již existující ["vulkano"](https://github.com/vulkano-rs/vulkano)
a postupem času toto řešení přizpůsobovat mým potřebám. Nakonec se úkazalo, že to nebylo nejlepší rozhodnutí, neboť tato
knihovna není pouze low-level wrapper nad Vulkan API, ale poskytuje i high-level API. Např. pro synchronizaci "Vulkan
Commands" (pomocí nádstavby zavné `GpuFutures`). Samozřejmě by se tato nádstavba dobře používala, ale byla by složitá na
implementaci a již nesplňovala jednu podmínku specifikace - "vyzkoušení low-level API Vulkan". Tedy vytvořil jsem pouze
jednoduché wrappery kolem Vulkan API pro vytváření `Instance`, `Device`, `Queue`, `RenderPass` a dalších objektů.
Pak i wrappery pro `Image` a `Buffer` (pomocí [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)).
Tyto wrappery se nachází v `projects/graphics`.

Samotné zobrazování SVG je rozděleno na část vykreslovací (`projects/runner/renderer` - obsahuje vše související s Vulkan API)
a část která zpracovává samotnou vektorovou grafiku (`projects/runner/canvas`).

Vykreslovací část je jednoduchá nádstavba nad Vulkan API a `projects/graphics`, ve výsledku poskytující příkaz na
vykreslení triangulovaného tvaru ve tvaru `vertices`, `indices` do specifikovaného `Image` bufferu na GPU.

Jelikož SVG renderování funguje tak, že se dané SVG jednou vykreslí a pak nemění, tak nemělo cenu řešit složitou
synchronizaci mezi spouštěním jednotlivých Vulkan "Commands" a zobrazováním na obrazovku počítače. Proto jsem vytvořil
nádstavbu s názvem `VulkanPresenter`. Funkce `Presenter` třídy je pouze zobrazovat uživatelem zvolený `Image` buffer z
GPU na obrazovku počítače. Tímto je "vyřešená" synchronizace mezi během jednotlivých "Commands" a zobrazováním na obrazovku.

Zpracování samotné vektorové grafiky se spočívá ve vytvoření `canvas::Path2D` objektů, které reprezentují SVG `<path />`
elementy (a protože reprezentují `path` elementy, tak mohou reprezentovat i `rect`, `circle`, `ellipse` atd.). Samotný
`canvas::Path2D` je složený z `canvas::Contour` (křivek) a ty jsou složeny ze základních bloků: `canvas::Segment` (úsečka,
kvadratická a kubická křivka). Pomocí těchto bloků se dá do libovolné přesnosti aproximovat libovolný tvar.

Při vykreslování jednotlivých `canvas::Contour` se tyto křivky rozdělí na základní bloky `canvas::Segment` a ty se
převedou na jednoduchou aproximaci pomocí úseček (tedy i oblouk je aproximován úsečkami). Tento vzniklý útvar se pak již
velice jednoduše vyplní (pomocí [triangulace](https://en.wikipedia.org/wiki/Polygon_triangulation)) a nebo převede na
"obtaženou" křivku. Tyto objekty se pak jednoduše vykreslí pomocí `projects/runner/renderer`.
