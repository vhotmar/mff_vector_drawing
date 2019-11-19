# Vykreslování vektorové grafiky

> Specifikace programu pro předmět Programování v C++

## Cíl (do budoucna)
Vytvoření frameworku pro jednoduché vykreslování vektorové grafiky a matematických objektů na tvorbu efektních matematických prezentací (inspirace kanál [3Blue1brown](https://www.youtube.com/channel/UCYO_jab_esuFRV4b17AJtAw) na YouTube, stránky [acko.net](https://acko.net) a většina prezentací v jeho článcích, např. [How to fold a Julia Fractal](https://acko.net/blog/how-to-fold-a-julia-fractal/)).

Autor 3Blue1Brown poskytuje knihovnu [manim](https://github.com/3b1b/manim), jež ale není vhodná pro tvorbu prezentací, pouze pro videa. Knihovna použitá pro [acko.net](https://acko.net) se nazývá [MathBox](https://github.com/unconed/mathbox) ale je již trochu zastaralá a většina věcí se musí vytvářet ručně (napč. přechod mezi obrazci není příliš jednoduchý).

### Proč ne teď?
Nejsem si jistý jak by mělo vypadat finální API (pravděpodobně by mělo mít základ v C++ a pak použito v jiném dynamičtějším jazyce pro snadnější tvorbu prezentací) a tedy ani přesně nevím jak by tento projekt byl ve výsledku složitý a ani jaký by měl být přesně jeho výstup.

## Cíl (aktuální)
Vykreslování vektorové grafiky. Toto je docela obsáhlý problém a proto bych ho chtěl trochu *"osekat"* *"pouze"* na vykreslování subsetu SVG. 

### Subset SVG
Pro zjednodušení specifikace programu se budu odkazovat na [oficiální specifikaci SVG](https://www.w3.org/TR/2018/CR-SVG2-20181004/).

Zpracování SVG formátu bude omezeno na:
- [Secure static mode](https://www.w3.org/TR/2018/CR-SVG2-20181004/conform.html#secure-static-mode) - program nebude podporovat žádné interakce s uživatelem (animace, reakce na to co uživatel dělá).
- [File processing](https://www.w3.org/TR/2018/CR-SVG2-20181004/conform.html#ConformingSVGStandAloneFiles) - program bude podporovat pouze čisté soubory SVG (pro zjednodušení budeme používat [zjednodušené XML](https://dvcs.w3.org/hg/microxml/raw-file/tip/spec/microxml.html#character-references)

A budou povoleny pouze tyto SVG elementy (všechny se budou používat bez namespaces, neboť je zjednodušené XML nepodporuje):
- `svg` - root
- [`g`](https://www.w3.org/TR/2018/CR-SVG2-20181004/struct.html#GElement) - může obsahovat pouze další zde zmíněné *elements* a `presentation` atributy
- graphics elementy (podporují `presentation` atributy)
	- [`circle`](https://www.w3.org/TR/2018/CR-SVG2-20181004/shapes.html#CircleElement) - specifické atributy `cx`, `cy`, `r`
	- `ellipse` - specifické atributy `cx`, `cy`, `rx`, `ry`
	- `line`
	- [`path`](https://www.w3.org/TR/2018/CR-SVG2-20181004/paths.html#PathLengthAttribute) - specifické atributy `d`
	- `polygon`
	- `polyline`
	- `rect` - specifické atributy `height`,`width`, `x`, `y`

Zmíněné `presentation` atributy:
- `display`
- `fill` - povoleno pouze jednolitá barva (možné rozšírení na `gradients`, `patterns`)
- `fill-opacity`
- `opacity`
- `paint-order`
- `stroke`
- `stroke-dasharray`
- `stroke-dashoffset`
- `stroke-linecap`
- `stroke-linejoin` - `miter`, `round`, `bevel` (možné rozšíření s `arc`)
- `stroke-miterlimit`
- `stroke-opacity`
- `stroke-width`

Jednotky vzdálenosti pouze v `px`.

### Vykreslování
Vykreslování proběhne pomocí [Vulkan](https://www.khronos.org/vulkan/) API (důvod je jednoduchý - chci vyzkoušet jak se s tímto low-level API pracuje), popř. později by měla být jednoduchá migrace na [WebGPU](https://en.wikipedia.org/wiki/WebGPU) (a [WebAssembly](https://webassembly.org))

## Nápady na rozšíření
### Standard SVG
- již zmíněné různé typy `fill` atributu
- již zmíněný `arc` pro `stroke-linejoin`
### Layout framework
Do budoucna budu chtít toto vykreslování rozšírit na plnohodnotný prezentační framework, který by (alespoň dle mého názoru) měl obsahovat jednoduchý Layout Framework. Neboť je jednoduší napsat, něco ve smyslu:

```
<HorizontalLayout>
  <Circle radius={15} />
  <Text>Some description for the circle</Text>
</HorizontalLayout>
```
Než:
```
<Circle x={50} y={50} radius={15}>
<Text x={80}>Some description for the circle</Text>
```
### Přechod mezi `path` v SVG
Nechť máme dva různé obrazce definované pomocí SVG `path` objektu a chceme mezi nimi vytvořit hladký přechod. Chtěl bych vytvořit algoritmus který toto umožňuje.

Na tento probém jsem našel mnoho různých prací, které jsem nezkoumal do hloubky a proto si nejsem úplně jistý komplexitou tohoto problémů.