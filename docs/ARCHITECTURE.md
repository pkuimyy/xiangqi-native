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

Compact layouts keep a 228..320px study column with moves/tree switching, a permanent 144px live-analysis card and notes. At 1120 effective pixels and above, the fixed board, 260..320px move column and 300..600px research column form a centred workspace. The same tree, toggle/result card and note editor move into the research column; their instances and contents are retained. Cards for short move lists and trees fit content. Research notes grow with the available height. The board/input dimensions remain fixed across window changes; small windows scroll.

Default startup approximates half the current monitor width. --layout-preview captures compact/half/wide/analysis views, verifies fixed board dimensions, and uses bundled Pikafish to check current-position follow, stale-result isolation and transient/export semantics. Automation skips user preferences.

## Persistent metadata

Game headers are JSON text in metadata.extensions under org.xiangqi.native/pgn_headers. Native node metadata uses a source_note annotation with author org.xiangqi.native/v1 and text {"version":1,"data":{...}}.

Fields include eval, glyph, annotations, shapes, clock, commentAuthors and per-branch result. Human comments remain separate comment annotations. Unknown OXQ annotations are retained when editing/saving. Metadata follows its node through branch moves; it does not depend on session-local NodeId values.

PGN follows the original fidelity limits: evaluation depth/MultiPV are retained in OXQ, while PGN preserves score, bestmove and ponder through compatible tags. Header strings are now correctly escaped/unescaped. Recognized semicolon comments and Chinese/WXF PGN input extend the original parser.

## Files and bounds

Windows saves write a sibling temporary file exclusively, flush it, then use ReplaceFileW or MoveFileExW. Failed writes retain the original target. Files have a 256 MiB limit. Input warnings reject a PGN load, preserving the current document.

Native tree hydration rejects duplicates, cycles, shared children and orphans, with a 100,000-node limit. PGN variation nesting is bounded at 512 and layout/export depth at 2048. Layout and export use explicit traversal stacks; a 2048-ply replay/export regression verifies the supported depth without recursive Windows call-stack growth. These bounds do not imply large-tree UI responsiveness: projections are currently complete recomputations.

Core parsing retains collection support. The current UI has one document and no game-count selector. New/open prepares the replacement first, rejects non-single-game input, handles unsaved edits through Save/Discard/Cancel, and then replaces the current document. OXQ save and PGN export act on that document. CBL input currently rejects conversion diagnostics; collection export and diagnostics review are future work.

## UCI and live analysis

CreateProcessW starts the native engine without a shell or console. Explicit pipe inheritance, uci/isready handshakes and exact-score filtering remain native C++. The existing finite analyze API remains available; analyze_live uses go infinite and streams exact primary-PV snapshots. Readiness/startup poll cancellation, and stop has a bounded response wait.

LiveAnalysis owns a single worker, a warm process and a 250ms latest-position debounce. Updates are throttled to 150ms, marshalled through DispatcherQueue, checked against the current request revision and node/FEN, and converted to Red's viewpoint. All engine startup and work occur off the UI thread. Live search uses one thread, 32MB Hash and below-normal process priority. Focus loss does not interrupt analysis.

The UI caches progress by node/FEN without changing OXQF metadata or undo/dirty state. Board arrows receive a temporary JSON projection; up to eight PV moves are displayed in Chinese. Explicit save/export snapshots include the current cached result. Saving commits that same result to the document after the file write succeeds. Other visited positions' transient results are not automatically batch-persisted.

The switch state and engine path are stored under LOCALAPPDATA/XiangqiNative/settings.json, preserving unknown settings. Automation skips these preferences. New positions cancel obsolete work; board flip changes rendering only. A terminal position does not start a search. Errors remain visible until the user selects an engine, toggles again or changes position. History-aware repetition context, user-configurable resource settings and play against the engine remain future work.

## Tests

Native CTest covers document edits/save, UCI protocol and deterministic migration fixtures. The golden JSON was generated from the pinned original implementation before removing JS. It compares legal move sets, Chinese notation, resulting FENs, checks, PGN trees/metadata/stringification and Walker coordinates.

The WinUI smoke test invokes board Buttons through their own automation peers, saves/reopens OXQ, uses only the plain board, captures selected and deselected piece states, flips the board and captures its own visual tree. It does not automate unrelated applications.
