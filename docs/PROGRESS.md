# Migration progress

Date: 2026-10-25. The current checkpoint implements the user's C++-only requirement and reuses original graphical resources. Full original-feature parity remains in progress.

## Implemented

- Independent Git repository and WinUI 3/C++20 application.
- Native C++ rules/FEN, Chinese/WXF/ICCS notation, PGN/comment metadata and Walker layout.
- QuickJS, JS/TS sources, generated bundles, npm manifests and Node CI steps removed.
- Only the flat native board is exposed to users; custom/bamboo rendering stays internal. All 14 OXDF-editor SVG pieces remain unchanged. Wood and old traditional piece assets are removed.
- Native vector pieces, board click-to-move/flip, legal dots, annotations and best-move arrows; no automatic last-move marking.
- WinUI native side toolbar, FEN copy, evaluation strip, compact branch graph and two-column Chinese move list.
- Multiple games; navigation, branch deletion/promotion, comments, undo/redo.
- OXQF authoritative model, fetched directly at 206f807 from pkuimyy/OXQF; rich metadata and unknown annotations preserved.
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

Release and Debug builds each passed all four CTest targets and actual WinUI smoke tests. Native board source/destination clicks, both board styles, flip and OXQ save/reopen passed. Current OXDF-editor piece assets are verified byte-for-byte against the user-selected source directory. Real Pikafish returned depth 6, bestmove h2e2, ponder h9g7 and +0.52-pawn score (52 centipawns). Evidence and previews are under build/ during development. Run scripts/build.ps1 and scripts/smoke.ps1 to reproduce. Committed previews show the actual WinUI visual tree.

## Latest visual update

The user requested removal of wood artwork and selected OXDF-editor/assets/pieces. All 14 SVGs now load directly through WinUI; their source hashes match. The board is flat; the bamboo renderer is now internal only. Release and Debug UI builds and smoke tests passed with all 14 SVG decode events confirmed; the four native CTest targets also passed in Release. The review executable is build/winui/Release-fetchcontent/XiangqiNative.exe because the existing Release exe was running. Build/smoke scripts now accept an independent OutputDirectory.

## Latest dependency and motion update

The local OXQF source copy is removed. FetchContent now obtains the pinned public repository and uses its original CMake targets. Generated NativeCore.props connects WinUI to the actual headers, libraries and MIT license. Release and Debug each passed all four native CTest targets and actual window smoke tests against the fetched commit. The new reader also successfully loaded a seven-node OXQ saved by the previous application build.

Chess moves now update immediately with a thin destination outline. The previous translucent origin/destination fill and default square-button transitions are removed. The custom native button template retains keyboard focus and click/automation behavior.

## Interface simplification (2026-10-25)

Removed the public board-style ComboBox. Normal application startup always displays the plain board; the custom renderer and bamboo asset remain internal.

Selected piece artwork scales by 10% around its centre, without moving the hit target. The previous selection border is removed, the last-move outline is suppressed on the selected square, and cancelling selection restores its normal size. The native smoke flow captures both states, exercises deselection/reselection and a legal move, then calls the hidden custom renderer directly for coverage. Release build, all four native tests and the UI smoke flow passed. Selected/deselected previews confirm a centred enlargement with no selection outline. The updated executable is build/winui/Release/XiangqiNative.exe.

## Standard Release deployment (2026-10-25)

The current application was rebuilt into build/winui/Release/XiangqiNative.exe using the default build command, without an output-directory override. All four native tests and the window smoke test passed from this exact directory, confirming the plain-only UI and 110% selected-piece scale.

## Board flicker repair (2026-10-25)

Replaced full-board Clear/recreation with app/board_view.hpp: five retained layers, 90 persistent input buttons, cached SVG piece visuals and cached grid/background resources. Moving pieces keep the same parent and only change coordinates. Captures hide their visuals; undo reuses them. Selection and legal hints update existing properties.

Four native tests and the same-directory temporary UI verification passed. The UI checks retained button/grid/piece identity, moving-piece parent, constant 32-piece visual allocation, capture and undo, and selection/cancellation. The standard build/winui/Release/XiangqiNative.exe has now been rebuilt and passed the same window checks after the previous window was closed.

## Recently moved selection repair (2026-10-25)

The last-destination outline now belongs to a dedicated noninteractive ellipse in the retained hints layer. Selecting that piece collapses the outline; input buttons keep zero borders and no shadow. The selected SVG still scales in place to 110%, with no visual-tree rebuild.

Added a regression that moves a cannon, gives its destination pointer focus, selects/deselects it twice, and checks marker visibility, zero button border/shadow and retained piece identity. The Release verification executable and the rebuilt standard build/winui/Release/XiangqiNative.exe both passed the actual WinUI smoke flow, including the new recentMoveSelection regression. The captured recent-move-selected-preview.png confirms no surrounding outline.

## SVG board layout (2026-10-25)

Combined the original grid, OXDF-editor river lettering paths and outlined file coordinates into self-contained normal/flipped SVGs. The canvas is 450x548 with 24px top/bottom gutters; the river pairs are centred by their actual vector bounds at (125,274) and (325,274). Coordinate rows at y=12/536 have at least 6.25px clearance from a selected 49.5px piece frame. See BOARD_GEOMETRY.md and scripts/generate-board.ps1.

Removed native river/coordinate TextBlocks and all automatic last-move decoration. Retained grid/piece/button identities and move/capture/undo behavior are unchanged. The UI smoke flow now exercises only the plain board, including flip and repeated selection of the recently moved piece; it no longer enters the bamboo theme.

Release verification passed and both orientation screenshots were inspected. The standard build/winui/Release/XiangqiNative.exe was rebuilt after the running window closed and passed the same plain-board smoke flow (boardStylesTested=1, lastMoveMarker=false). Both SVGs' 36 coordinate and 4 river path bounds were independently checked to within 0.001px.

## Next work

1. Drag/drop moves, arrow/highlight editing, annotation toggles, result controls, branch folding and keyboard interaction coverage.
2. Position editor, alternative original piece sets, player names/clocks and preview boards.
3. Automatic/batch analysis and human-versus-engine play with move-history context.
4. File conflicts, backups, autosave/crash recovery and persisted preferences.
5. Collection export, CBL conversion diagnostics UI, native image/GIF export.
6. Incremental/cancellable computation, virtualization and large-library indexing.
7. Installer/file associations, clean-machine deployment checks and release packaging.

## Current limits

Full-tree projections run synchronously and undo copies models; large-tree performance is unverified. PGN export is per active game. CBL diagnostics reject input pending a review UI. Custom board rendering has no public control or persisted setting. The current piece set uses the user-selected OXDF-editor SVGs. Competition repetition/chase adjudication is outside the inherited rule implementation.
