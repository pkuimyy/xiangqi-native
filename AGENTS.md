# Xiangqi Native

Independent Windows application. The user requires WinUI 3 native controls, C++ throughout, no Node.js or JavaScript runtime. Reuse original visual assets and migrate rules, Chinese notation, PGN and Pikafish behavior.

## Architecture

- app/: programmatic C++/WinRT UI. No WebView host. app/board_view.hpp owns a retained board visual tree.
- src/core/rules.cpp: native rules, FEN, Chinese/WXF/ICCS notation.
- src/core/pgn.cpp: native PGN parser/stringifier and original comment metadata conventions.
- src/core/layout.cpp: C++ port of the original Walker/Buchheim layout.
- Document owns the authoritative OXQF GameModel; derived FEN/notation/layout are projections.
- OXQF: FetchContent from https://github.com/pkuimyy/OXQF.git at 206f807f8083ae0ec9384499a35313cb97c7a199, MIT. Link upstream OXQF::format and OXQF::convert. Do not vendor or patch its source.
- external/json/: nlohmann/json 3.12.0, MIT.
- assets/: only a plain native board in the public UI; custom/bamboo rendering stays internal, and user-provided OXDF-editor SVG pieces. Load the original SVG paths through WinUI SvgImageSource; do not redraw glyphs using fonts. Wood resources are intentionally removed.
- licenses/ and docs/DEPENDENCIES.md: retain source attribution.

## Validation

Use PowerShell scripts/build.ps1 -Configuration Debug or Release; -CoreOnly skips WinUI.
All future Release deliveries must use C:\Users\yy_1070\project\xiangqi-native\build\winui\Release\XiangqiNative.exe, with no version/feature suffix on the directory. Use scripts/build.ps1 -Configuration Release and scripts/smoke.ps1 -Configuration Release without -OutputDirectory unless the user explicitly requests otherwise. An executable lock is not a reason to create a suffixed Release directory. Preserve running windows and unsaved work.
CMake generates build/native/winui/<Configuration>/NativeCore.props for the WinUI project. Never hardcode FetchContent header/library cache paths. Dependency BUILD_TESTING is OFF, so no upstream Node tooling is required.
BoardView creates its five layers and 90 hit buttons once. Keep SVG piece visuals parented to their persistent piece layer; update position/visibility/scale only. Never clear the board or reload grid/piece sources on a move. Captured piece visuals remain cached for undo. Board square buttons intentionally use a static native template with empty transitions. Moves update immediately, with no automatic last-move marker, origin fill or fade. Selected piece artwork scales to 110% around its centre with no selection border; never add a hit-button border or shadow. River lettering and coordinates belong to the generated board SVGs. scripts/generate-board.ps1 derives exact bounds; see docs/BOARD_GEOMETRY.md. Never expose custom board controls in the user interface.
All tests are native C++ and run through CTest. Do not introduce Node or JavaScript build steps.
tests/fixtures/upstream-oracle.json records the original implementation before its removal. Do not rewrite expected output to conceal a regression.
The legacy checkmate flag means no legal moves, including stalemate. The UI distinguishes the two using check.
OXQ NodeId is session-local; never use it as a persistent external key.
Node metadata is a versioned source_note authored org.xiangqi.native/v1; retain unknown annotations.
Native app --smoke-test verifies button-driven board moves, OXQ save/reopen, piece selection/cancellation, plain-board flip and native rendering. Do not exercise the bamboo/custom board in tests.
Use scripts/smoke.ps1 and scripts/setup-engine.ps1. Keep engine and network licenses.
Do not claim complete feature parity; record verified work and actual remaining features in docs/PROGRESS.md.
