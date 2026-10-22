# Xiangqi Native

Independent Windows application. The user requires WinUI 3 native controls, C++ throughout, no Node.js or JavaScript runtime. Reuse original visual assets and migrate rules, Chinese notation, PGN and Pikafish behavior.

## Architecture

- app/: programmatic C++/WinRT UI. No WebView host.
- src/core/rules.cpp: native rules, FEN, Chinese/WXF/ICCS notation.
- src/core/pgn.cpp: native PGN parser/stringifier and original comment metadata conventions.
- src/core/layout.cpp: C++ port of the original Walker/Buchheim layout.
- Document owns the authoritative OXQF GameModel; derived FEN/notation/layout are projections.
- external/oxqf/: vendored source at 7d8a554, MIT.
- external/json/: nlohmann/json 3.12.0, MIT.
- assets/: original wood/bamboo textures and traditional SVGs. Piece SVG circle/text definitions are rendered as native WinUI controls.
- licenses/ and docs/DEPENDENCIES.md: retain source attribution.

## Validation

Use PowerShell scripts/build.ps1 -Configuration Debug or Release; -CoreOnly skips WinUI.
All tests are native C++ and run through CTest. Do not introduce Node or JavaScript build steps.
tests/fixtures/upstream-oracle.json records the original implementation before its removal. Do not rewrite expected output to conceal a regression.
The legacy checkmate flag means no legal moves, including stalemate. The UI distinguishes the two using check.
OXQ NodeId is session-local; never use it as a persistent external key.
Node metadata is a versioned source_note authored org.xiangqi.native/v1; retain unknown annotations.
Native app --smoke-test verifies button-driven board moves, OXQ save/reopen, texture selection, flip, and native rendering.
Use scripts/smoke.ps1 and scripts/setup-engine.ps1. Keep engine and network licenses.
Do not claim complete feature parity; record verified work and actual remaining features in docs/PROGRESS.md.
