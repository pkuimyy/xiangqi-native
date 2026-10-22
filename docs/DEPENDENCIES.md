# Dependency provenance

| Component                            | Pinned source                                                            | License                                                         |
| ------------------------------------ | ------------------------------------------------------------------------ | --------------------------------------------------------------- |
| Original application pure algorithms | obsidian-xiangqi c6f2197, copied under kernel/upstream                   | GPL-3.0, repository LICENSE                                     |
| Original rules and Chinese notation  | @weshell/xiangqi.js 1.0.3, exact package from original package-lock.json | BSD-2-Clause, reference/xiangqi.js/LICENSE                      |
| OXQF codec and model                 | local OXQF 7d8a554, vendored under external/oxqf                         | MIT, external/oxqf/LICENSE                                      |
| QuickJS-NG                           | v0.17.0, 6d46d07d04041b40f4f49eaa7fdebe44c314c699                        | MIT, external/quickjs/LICENSE                                   |
| JSON for Modern C++                  | v3.12.0, vendored single header                                          | MIT, external/json/LICENSE.MIT                                  |
| WinUI                                | Microsoft.WindowsAppSDK.WinUI 1.8.260803003 with NuGet lock              | Distributed under package license                               |
| C++/WinRT                            | Microsoft.Windows.CppWinRT 2.0.250303.1                                  | Distributed under package license                               |
| Native Pikafish                      | Pikafish-2026-09-06, optional download                                   | GPL-3.0, engine Copying.txt; NNUE-License.md applies to network |

Pikafish archive: https://github.com/official-pikafish/Pikafish/releases/download/Pikafish-2026-09-06/Pikafish.2026-09-06.7z

SHA-256: 41952BBFE2520FACEB5902C69E6AB4845CC999841D2B49A95CC1BE7867A25E5B

Source: https://github.com/official-pikafish/Pikafish/tree/Pikafish-2026-09-06

Build tools (Node/npm, esbuild, Vitest, Prettier) are development-only. Normal program startup does not load Node, Obsidian, a browser, or a system JavaScript runtime.
Windows App SDK brings transitive WebView2 build metadata because its control library defines a WebView2 control; the application never instantiates that control. This metadata is not used as a UI host.
