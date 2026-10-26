# Migration progress

Date: 2026-10-25. The current checkpoint implements the user's C++-only requirement and reuses original graphical resources. Full original-feature parity remains in progress.

## Implemented

- Independent Git repository and WinUI 3/C++20 application.
- Native C++ rules/FEN, Chinese/WXF/ICCS notation, PGN/comment metadata and Walker layout.
- QuickJS, JS/TS sources, generated bundles, npm manifests and Node CI steps removed.
- Only the flat native board is exposed to users; custom/bamboo rendering stays internal. All 14 OXDF-editor SVG pieces remain unchanged. Wood and old traditional piece assets are removed.
- Native vector pieces, board click-to-move/flip, legal dots, annotations and best-move arrows; no automatic last-move marking.
- WinUI native side toolbar, FEN copy, current-turn strip, compact branch graph and two-column Chinese move list.
- Single-document UI; navigation, branch deletion/promotion, comments, undo/redo. Collection parsing remains in the core.
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

## Current-turn strip (2026-10-25)

Repurposed the left strip from evaluation proportions to the current side to move. A single fixed half-height indicator uses the piece colours (#BC3328 Red, #333333 Black); the other half is neutral. Normal orientation shows Red below/Black above, and both retained flip controls swap the position. draw_board updates it from the position turn, independent of engine metadata; the tooltip/accessibility name identifies the side. Numeric engine evaluations remain available separately.

The UI regression checks both colours in both orientations, fixed row heights despite stored +0.35 evaluation metadata, moves/capture, navigation and undo. The Release verification executable and the rebuilt standard build/winui/Release/XiangqiNative.exe both passed the actual WinUI smoke test (turnIndicator=true). Normal/flipped Red/Black screenshots were inspected.

## Half-screen layout preview (2026-10-26)

Replaced the fixed 520px board column and always-visible study/engine controls with a provisional native WorkspaceLayout. File/Start/Analysis/View group the commands, navigation sits below the board, notation input and comments collapse, and moves/graph share a compact sidebar. Widths at or above 1120 effective pixels show graph in a third column; default startup targets half-screen use. Piece rendering, turn indication, document model and engine behavior are reused.

The standard Release build succeeded. A lightweight --layout-preview startup renders 640x640, 768x720 and 1280x760 client areas and captures their actual WinUI content. The three actual rendered captures were inspected and saved as docs/assets/layout-{compact,half,wide}.png. Per user request, full CTest and functional smoke suites are deferred until the design is approved. This is a layout review checkpoint; panel sizes, spacing, movable dividers, saved preferences and visual polish remain open.

## Fixed board and engine guide (2026-10-26)

Board viewport dimensions are chosen once at startup from the current monitor and client area. Resizing the window, displaying the wide graph column and switching command tabs keep the board/input region fixed; smaller windows use scrollbars. Navigation/flip and notation input share one persistent 32px row. The note editor is always open, with fixed space for the evaluation and note card.

Removed the game-count selector. The UI replaces one current document; new/open prepares its replacement, handles unsaved changes through Save/Discard/Cancel, and rejects multi-game inputs before changing the document. Collection parsing remains available in the core for a future explorer.

Added docs/PIKAFISH_GUIDE.md with setup, a depth-6 first run, Red-view scores, stopping/navigation, save/reopen and troubleshooting. The bundled Release engine/NNUE files are present. No engine behavior was changed.

Release compilation and the lightweight preview passed: the actual board remained 450x548 logical pixels on this monitor across compact, half, wide and Analysis views. Final compact/half/wide/analysis captures were inspected and saved under docs/assets; the full regression suite remains deferred during layout review.

## Centred research layout and live Pikafish (2026-10-26)

Kept fixed board/input dimensions and replaced fixed full-window study cards with a centred group: board, move list and a flexible 300..600px research column on wide windows. Tree, permanent analysis card and larger always-open notes share that column; compact windows keep the same controls in the sidebar. Short move/tree cards shrink to their content.

Replaced manual analysis controls with a persistent switch and live score/depth/Chinese-PV panel. Native go-infinite streaming runs on a cancellable background worker with 250ms debounce, warm engine reuse and 150ms update throttling. One thread/32MB Hash/below-normal priority keep the default resource policy modest. Switch/path preferences persist outside document state.

Live progress is cached by node/FEN and does not modify metadata, dirty state or undo. Explicit save/export includes the current node's latest result. Old requests are rejected by revision and node/FEN checks. Flip does not restart analysis; quick navigation cancels and debounces it.

The native.engine and new native.live-analysis tests passed, covering stream/stop/reuse, latest-position scores, debounce, errors and cancellation during stalled startup. Actual bundled Pikafish returned live results in WinUI, followed navigation, preserved document metadata/dirty state and exported the current evaluation; the board remained 450x548 on this monitor across five previews. Final centred alignment and all five captures were inspected; updated previews are saved under docs/assets. The exact standard Release executable passed these checks. Full rules/PGN suites remain deferred during UI review.

## Portable release packaging (2026-10-26)

Added scripts/package.ps1 and portable-tools.ps1. The normal-user ZIP contains only manifest-listed WinUI runtime files, the executable/PRI, 14 piece SVGs and two board SVGs, licenses/notices, offline user guidance and (by default) Pikafish/NNUE. Debug symbols, intermediate files, scripts, tests, screenshots, personal settings/documents, unused board-generation inputs and the internal bamboo asset are excluded. Runtime PE imports, including delay loads, are checked; optional VC CRT files are collected only if imported.

A portable.flag marker selects application-local Data/settings.json. Internal engine paths are stored relatively with Windows case-insensitive prefix handling; development output without the marker retains its existing profile behavior. Command-line flags are parsed as actual arguments. A dedicated isolated portable self-check validates app-local WinUI, 14 resources, bundled analysis and settings restoration without altering the host profile.

The default packaging command was exercised under Windows PowerShell 5.1: Release built with full tests skipped, then the ZIP was extracted into two Chinese/spaced paths and launched with an unrelated working directory. Copying only Data from the first extraction restored the switch and new-location engine path in the second while the original remained present. The first complete package was about 69.9MiB zipped / 106MiB unpacked with 93 files. The no-engine variant also passed under PowerShell 7 (89 files, about 19.7MiB ZIP). ZIP entries use explicit UTF-8 and standard forward slashes. Complete/source/checksum outputs and inventories are refreshed by the packaging command.

Matching source and SHA-256 files are separate. Source includes the actual unmodified OXQF snapshot and the pinned/hash-checked Pikafish source archive. CI uploads the ZIP/source/checksum artifacts instead of the entire Release directory; headless CI skips interactive verification explicitly. A clean Windows VM test remains outstanding.

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
