# Migration progress

Date: 2026-10-23. The current checkpoint implements the user's C++-only requirement and reuses original graphical resources. Full original-feature parity remains in progress.

## Implemented

- Independent Git repository and WinUI 3/C++20 application.
- Native C++ rules/FEN, Chinese/WXF/ICCS notation, PGN/comment metadata and Walker layout.
- QuickJS, JS/TS sources, generated bundles, npm manifests and Node CI steps removed.
- Original wood/bamboo textures, 14 traditional piece SVG definitions and board paths.
- Native vector pieces, board click-to-move/flip, legal dots, last-move highlights, annotations and best-move arrows.
- WinUI native texture selector, side toolbar, FEN copy, evaluation strip, compact branch graph and two-column Chinese move list.
- Multiple games; navigation, branch deletion/promotion, comments, undo/redo.
- OXQF authoritative model; rich metadata and original unknown annotations preserved.
- PGN import/export; Unicode paths, native file dialogs and atomic OXQ overwrite.
- Native UCI analysis with cancellation, timeout, stale-result protection and Red-view scores.
- Automatic discovery of pinned optional Pikafish and NNUE.
- Actual app startup, save/reopen and rendered visual-tree smoke test.

## Validation

The native CTest targets cover:
- 600 original deterministic positions and 23,063 legal moves, including generated Chinese replay.
- 8 endgame/repeated-piece positions and all their move results.
- 25 PGN inputs, including original parser cases, variations, comments, NAGs, result, authors, shapes, clocks, evaluations and custom FEN numbering.
- Exact original Walker coordinates for a 232-node branching tree.
- 2048-ply native layout, PGN export and re-import with explicit traversal stacks.
- Cannon screens, horse legs, elephant eyes, pinned pieces, river crossing, FEN validation, retreats, traditional input aliases, escaped headers and invalid trees.
- OXQ/PGN document edits, metadata, undo/redo, Unicode atomic overwrite.
- UCI handshakes, score bounds, MultiPV, ponder, orientation, cancellation/reuse and timeout.

The previous JS prototype passed the original 41 TS tests. Those historical results do not claim that all 41 utility behaviors have already been reimplemented in native UI: player-clock presentation and annotation toggle controls remain below. Active verification now runs entirely in C++.

Release and Debug builds each passed all four CTest targets and actual WinUI smoke tests. Native board source/destination clicks, both textures, flip and OXQ save/reopen passed. All 16 copied original image/SVG assets retain their source hashes. Real Pikafish returned depth 6, bestmove h2e2, ponder h9g7 and +0.52-pawn score (52 centipawns). Evidence and previews are under build/ during development. Run scripts/build.ps1 and scripts/smoke.ps1 to reproduce. Committed previews show the actual WinUI visual tree.

## Next work

1. Drag/drop moves, arrow/highlight editing, annotation toggles, result controls, branch folding and keyboard interaction coverage.
2. Position editor, alternative original piece sets, player names/clocks and preview boards.
3. Automatic/batch analysis and human-versus-engine play with move-history context.
4. File conflicts, backups, autosave/crash recovery and persisted preferences.
5. Collection export, CBL conversion diagnostics UI, native image/GIF export.
6. Incremental/cancellable computation, virtualization and large-library indexing.
7. Installer/file associations, clean-machine deployment checks and release packaging.

## Current limits

Full-tree projections run synchronously and undo copies models; large-tree performance is unverified. PGN export is per active game. CBL diagnostics reject input pending a review UI. Theme selection is currently session-only. Only original traditional pieces are implemented. Competition repetition/chase adjudication is outside the inherited rule implementation.
