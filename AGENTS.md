# Xiangqi Native

Independent Windows native application. The user approved WinUI 3, C++20, and OXQF; retain the original rules, Chinese notation, PGN, and Pikafish behavior.

## Architecture

- app/: WinUI 3 programmatic native UI, no WebView or browser UI.
- src/core/ and include/xiangqi/: native document authority, format mapping, atomic file writes, native UCI process.
- kernel/upstream/: copied pure calculation code from obsidian-xiangqi c6f2197; preserve upstream files and prefer adapters.
- kernel/bridge.ts: calculation-only adapter. Embedded QuickJS has no DOM, Obsidian, Node, network or filesystem bindings.
- kernel/generated/: committed deterministic source bundle and byte array, so normal C++ builds need no Node runtime.
- external/oxqf/: vendored source at 7d8a554, with original MIT license.
- external/quickjs/: git submodule pinned to v0.17.0 (6d46d07d04041b40f4f49eaa7fdebe44c314c699).
- reference/xiangqi.js/: original locked npm package 1.0.3 and BSD-2-Clause license.
- reference/obsidian-tests/: original regression tests with import remapping.

## Validation

Use PowerShell scripts/build.ps1 -Configuration Debug (or -CoreOnly).
Run npm ci --ignore-scripts, npm run kernel:build, npm run kernel:test and npm run upstream:test when calculation sources change.
Do not modify upstream algorithms to satisfy an adapter test; inspect original behavior first.
Rules package isCheckmate() means no legal moves, including stalemate; isStalemate() returns false. The UI distinguishes checkmate from stalemate using isCheck().
OXQ NodeId is session-local; never use it as a persistent external key.
Node machine metadata is a versioned source_note authored org.xiangqi.native/v1; retain unknown annotations.
Keep generated kernel.inc synchronized with kernel.js.
Native app --smoke-test writes result.json and native-preview.png under its output directory/smoke and exits.
Use scripts/setup-engine.ps1 to fetch the pinned official native engine and NNUE; retain accompanying licenses.

## Task continuity

Record achieved milestones, validation results and actual missing features in docs/PROGRESS.md.
Do not claim feature parity until the feature matrix and original regression tests pass.
