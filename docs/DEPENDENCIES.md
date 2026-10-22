# Dependency and asset provenance

| Component | Pinned source | License |
| --- | --- | --- |
| Rules/FEN/Chinese notation C++ port | @weshell/xiangqi.js 1.0.3 from original package lock | BSD-2-Clause, licenses/xiangqi.js-LICENSE |
| PGN/comment metadata/Walker C++ port | obsidian-xiangqi c6f2197 | GPL-3.0, repository LICENSE |
| Bamboo board and grid geometry | obsidian-xiangqi c6f2197 | Original repository GPL-3.0 |
| Piece SVGs | User-selected local OXDF-editor/assets/pieces | User-provided assets; see provenance below |
| OXQF model and codecs | local OXQF 7d8a554, external/oxqf | MIT, external/oxqf/LICENSE |
| JSON for Modern C++ | 3.12.0, vendored header | MIT, external/json/LICENSE.MIT |
| WinUI | Microsoft.WindowsAppSDK.WinUI 1.8.260803003 | NuGet package license |
| C++/WinRT | Microsoft.Windows.CppWinRT 2.0.250303.1 | NuGet package license |
| Optional native Pikafish | Pikafish-2026-09-06 | GPL-3.0 Copying.txt; NNUE-License.md |

Original application: https://github.com/west-shell/obsidian-xiangqi/tree/c6f2197

The 14 assets/pieces/{red,black}_*.svg files are copied byte-for-byte from the user-selected C:\Users\yy_1070\project\OXDF-editor\assets\pieces directory (source repository HEAD 83d0bec at copy time). Their path-based glyphs and artwork are loaded directly by WinUI SvgImageSource. No font substitution or glyph redrawing is used; the native image container only crops transparent outer margins.

The wood image and previous traditional SVG set are removed from source and app outputs. The default board is a flat native brush. board-grid.svg retains the original GRID_SVG paths with a neutral palette and native river labels. Optional bamboo.jpg is copied unchanged and already contains its own grid/river artwork.

Pikafish archive: https://github.com/official-pikafish/Pikafish/releases/download/Pikafish-2026-09-06/Pikafish.2026-09-06.7z

SHA-256: 41952BBFE2520FACEB5902C69E6AB4845CC999841D2B49A95CC1BE7867A25E5B

Engine source: https://github.com/official-pikafish/Pikafish/tree/Pikafish-2026-09-06

All application algorithms, tests and UI are C++. QuickJS, Node/npm, esbuild and Vitest were removed from the source/build pipeline after the native port. The upstream behavior oracle is static JSON, not executable script.

Windows App SDK supplies transitive WebView2 build metadata because the control library defines that control. This app creates no WebView2 instance and does not require a browser UI host.

OXQF is vendored as a native library subset; upstream Node release and wrapper-test tooling is omitted. Library C++ sources and generated tables remain unchanged. See external/oxqf/VENDORED.md.
