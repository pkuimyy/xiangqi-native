# Native application architecture

## Calculation and document authority

All application and calculation code is C++20. There is no JavaScript engine, generated JS bundle, Node build step, browser UI or Obsidian dependency.

Kernel retains its existing JSON-in/JSON-out C++ interface to keep document projections stable. Its operations dispatch directly to native Position, PGN and Walker layout functions. Rules and notation were ported from the original locked @weshell/xiangqi.js 1.0.3. PGN, comment metadata and layout preserve the original application's conventions.

Position uses 90 squares, rank * 9 + file, rank 0 at Red's baseline. FEN validation includes palace advisor points, elephant points and facing generals. Legal moves check horse legs, elephant eyes, cannon screens, crossed-river pawns, self-check and flying generals. No-move positions retain the legacy checkmate=true/stalemate=false API; the UI uses check to distinguish checkmate and stalemate. Long-check/long-chase competition adjudication is not implemented by this migration.

Chinese input is resolved against generated legal moves. This also fixes the old permissive parser's retreat-sign ambiguity; generated retreat notation now executes the corresponding retreat. Chinese aliases, fullwidth numbers, WXF and ICCS are supported.

Document owns the authoritative OXQF GameModel. FEN, notation and coordinates are derived caches. Undo/redo holds up to 100 model snapshots. Document operations and full tree projections currently run on the UI thread.

## Upstream OXQF dependency

CMake FetchContent obtains https://github.com/pkuimyy/OXQF.git at 206f807f8083ae0ec9384499a35313cb97c7a199. It configures the unmodified upstream project with dependency tests disabled; no Node tooling is needed by this application. The app links upstream OXQF::format and OXQF::convert. Existing oxq/core include compatibility is provided by upstream.

CMake generates per-configuration NativeCore.props with target-derived headers, static libraries and license paths. WinUI imports these properties instead of assuming a source folder or library filename. The fetched source/build cache is under build/native/_deps and is not committed.

## Native visuals

The user interface provides only the plain board with a flat native brush and neutral SVG grid. There is no board-style selector or setting. The existing bamboo/custom-board rendering path and its calibrated coordinates remain internal, with no duplicate grid overlay; they are deliberately excluded from tests at the user's request.

The 14 OXDF-editor SVG pieces are copied unchanged from the user's selected directory. Their glyphs are paths, so WinUI SvgImageSource renders them directly instead of reconstructing circles/text. A clipped native Canvas crops transparent outer margins from the 96x96 documents. Native load events verify all 14 resources decode successfully in the UI smoke test. Missing resource files fail startup explicitly.

Chess-square Button controls use a static ContentPresenter template with empty control/content/children transitions. Position updates are immediate. There is no automatic last-move marker. Hit buttons always have zero border thickness and no shadow; there is no origin fill, tween, fade or hover/press backdrop. Selected piece artwork uses a centred 1.10 ScaleTransform, without changing the hit area or layout position. Selection has no separate outline, and cancelling selection restores the original size. System keyboard focus visuals are retained. Native Button controls supply focus, input and accessibility names; the compact branch tree and two-column move list also use Buttons. Viewbox scales the board; the left status strip uses a fixed half-height coloured indicator for the side to move. Normal orientation places Red at the bottom and Black at the top; flipping swaps their positions. Red uses #BC3328 and Black uses #333333, matching the piece artwork. The inactive half is neutral. draw_board updates the strip from the current position's turn, covering navigation, moves, undo and both flip controls. Evaluation values remain in the separate text display and do not affect the strip. There is no HTML/CSS layout.

### Retained board rendering

app/board_view.hpp keeps five layers: background/grid, 90 input buttons, piece visuals, legal destinations and annotations. Normal moves never clear the board subtree. The grid sources (normal/flipped), buttons, hint dots and standard game's 32 SVG piece objects are allocated once.

Position differences update only changed square ownership and accessibility names. A moving piece stays attached to the same piece layer; only its Canvas coordinates change. Captures collapse the existing visual and undo reuses it. Selection updates the existing centred scale transform, without replacing the Image or its Source.

Background resources change only when the internal board style changes. Orientation changes update piece/input coordinates and select the cached normal/flipped SVG, which includes outlined river lettering and file labels. The 450x548 canvas adds 24px gutters above/below the original 450x500 grid; see BOARD_GEOMETRY.md. Annotation children rebuild only when shape/best-move metadata or orientation changes; other board layers remain connected.

The window smoke test verifies stable grid/button/unchanged-piece identities, the moving piece's identity and parent, stable allocation counts, capture and undo, selection and cancellation, and repeated pointer-focused selection of the most recently moved piece with no automatic last-move marker. This guards against the original whole-board teardown and asynchronous SVG reload path. Full model projection and sidebar graph rebuilds remain separate performance work.

### Provisional adaptive workspace

app/workspace_layout.hpp composes native command tabs (File/Start/Analysis/View), CommandBars with overflow, a fixed-size board Viewbox, navigation below the board, and a study sidebar. Workspace dispatches the commands to the existing document and engine actions.

At widths below 720 effective pixels the sidebar is 228px; at medium widths it is 252px. At 1120px and above it becomes 280px and the existing graph ScrollViewer moves into a separate 300px third column. Narrow layouts switch between moves and graph. The board subtree remains attached throughout. Board viewport dimensions are computed once from the startup monitor/client area. Window resizing and command tabs do not alter them; the workspace scrolls if it does not fit. Five navigation/flip controls, notation input and its submit button share one permanent 32px row. Notes use an always-open fixed-height card, and evaluation text has a reserved height. These values are provisional until user review; draggable dividers and saved pane preferences are not implemented yet.

Default startup approximates half the current monitor width and stays within its working height. --layout-preview captures compact/half/wide/analysis views and verifies that the actual transformed board dimensions remain identical, without running the document/engine regression suite.

## Persistent metadata

Game headers are JSON text in metadata.extensions under org.xiangqi.native/pgn_headers. Native node metadata uses a source_note annotation with author org.xiangqi.native/v1 and text {"version":1,"data":{...}}.

Fields include eval, glyph, annotations, shapes, clock, commentAuthors and per-branch result. Human comments remain separate comment annotations. Unknown OXQ annotations are retained when editing/saving. Metadata follows its node through branch moves; it does not depend on session-local NodeId values.

PGN follows the original fidelity limits: evaluation depth/MultiPV are retained in OXQ, while PGN preserves score, bestmove and ponder through compatible tags. Header strings are now correctly escaped/unescaped. Recognized semicolon comments and Chinese/WXF PGN input extend the original parser.

## Files and bounds

Windows saves write a sibling temporary file exclusively, flush it, then use ReplaceFileW or MoveFileExW. Failed writes retain the original target. Files have a 256 MiB limit. Input warnings reject a PGN load, preserving the current document.

Native tree hydration rejects duplicates, cycles, shared children and orphans, with a 100,000-node limit. PGN variation nesting is bounded at 512 and layout/export depth at 2048. Layout and export use explicit traversal stacks; a 2048-ply replay/export regression verifies the supported depth without recursive Windows call-stack growth. These bounds do not imply large-tree UI responsiveness: projections are currently complete recomputations.

Core parsing retains collection support. The current UI has one document and no game-count selector. New/open prepares the replacement first, rejects non-single-game input, handles unsaved edits through Save/Discard/Cancel, and then replaces the current document. OXQ save and PGN export act on that document. CBL input currently rejects conversion diagnostics; collection export and diagnostics review are future work.

## UCI

CreateProcessW starts the native engine without a shell or visible console. Explicit pipe inheritance, uci/isready handshakes, exact-score filtering, cancellation, timeout and session retirement are native C++.

Scores are converted to Red's viewpoint. A generation token plus document/node identity prevents stale analysis from editing another position. Current analysis is FEN-based; preserving game history for repetition context, batch analysis and play against the engine remain future work.

## Tests

Native CTest covers document edits/save, UCI protocol and deterministic migration fixtures. The golden JSON was generated from the pinned original implementation before removing JS. It compares legal move sets, Chinese notation, resulting FENs, checks, PGN trees/metadata/stringification and Walker coordinates.

The WinUI smoke test invokes board Buttons through their own automation peers, saves/reopens OXQ, uses only the plain board, captures selected and deselected piece states, flips the board and captures its own visual tree. It does not automate unrelated applications.
