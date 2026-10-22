# Native application architecture

## Calculation and document authority

All application and calculation code is C++20. There is no JavaScript engine, generated JS bundle, Node build step, browser UI or Obsidian dependency.

Kernel retains its existing JSON-in/JSON-out C++ interface to keep document projections stable. Its operations dispatch directly to native Position, PGN and Walker layout functions. Rules and notation were ported from the original locked @weshell/xiangqi.js 1.0.3. PGN, comment metadata and layout preserve the original application's conventions.

Position uses 90 squares, rank * 9 + file, rank 0 at Red's baseline. FEN validation includes palace advisor points, elephant points and facing generals. Legal moves check horse legs, elephant eyes, cannon screens, crossed-river pawns, self-check and flying generals. No-move positions retain the legacy checkmate=true/stalemate=false API; the UI uses check to distinguish checkmate and stalemate. Long-check/long-chase competition adjudication is not implemented by this migration.

Chinese input is resolved against generated legal moves. This also fixes the old permissive parser's retreat-sign ambiguity; generated retreat notation now executes the corresponding retreat. Chinese aliases, fullwidth numbers, WXF and ICCS are supported.

Document owns the authoritative OXQF GameModel. FEN, notation and coordinates are derived caches. Undo/redo holds up to 100 model snapshots. Document operations and full tree projections currently run on the UI thread.

## Native visuals

The original wood.jpg and bamboo.jpg are copied unchanged. Native BitmapImage/ImageBrush loads them through app resource URIs. The original SVG board paths are retained for wood with an explicit palette; river lettering uses a native TextBlock. Bamboo is an unchanged complete board image, so it uses its own grid/river artwork with calibrated native piece coordinates.

WinUI's SVG image decoder does not render SVG text. Each original traditional piece SVG is therefore read using the Windows XML DOM: its circles, radii, stroke widths, colors, text, font size and bold weight define native Ellipse/TextBlock controls. Rendering remains vector based. Missing piece assets fail startup visibly instead of silently substituting different graphics.

Native Button controls supply focus, input and accessibility names; the compact branch tree and two-column move list also use Buttons. Viewbox scales the board; evaluation-strip proportions reflect the original winning-chance mapping. There is no HTML/CSS layout.

## Persistent metadata

Game headers are JSON text in metadata.extensions under org.xiangqi.native/pgn_headers. Native node metadata uses a source_note annotation with author org.xiangqi.native/v1 and text {"version":1,"data":{...}}.

Fields include eval, glyph, annotations, shapes, clock, commentAuthors and per-branch result. Human comments remain separate comment annotations. Unknown OXQ annotations are retained when editing/saving. Metadata follows its node through branch moves; it does not depend on session-local NodeId values.

PGN follows the original fidelity limits: evaluation depth/MultiPV are retained in OXQ, while PGN preserves score, bestmove and ponder through compatible tags. Header strings are now correctly escaped/unescaped. Recognized semicolon comments and Chinese/WXF PGN input extend the original parser.

## Files and bounds

Windows saves write a sibling temporary file exclusively, flush it, then use ReplaceFileW or MoveFileExW. Failed writes retain the original target. Files have a 256 MiB limit. Input warnings reject a PGN load, preserving the current document.

Native tree hydration rejects duplicates, cycles, shared children and orphans, with a 100,000-node limit. PGN variation nesting is bounded at 512 and layout/export depth at 2048. Layout and export use explicit traversal stacks; a 2048-ply replay/export regression verifies the supported depth without recursive Windows call-stack growth. These bounds do not imply large-tree UI responsiveness: projections are currently complete recomputations.

Opened multi-game PGNs produce independent Documents. OXQ save and PGN export act on the active game. CBL input currently rejects conversion diagnostics; collection export and diagnostics review are future work.

## UCI

CreateProcessW starts the native engine without a shell or visible console. Explicit pipe inheritance, uci/isready handshakes, exact-score filtering, cancellation, timeout and session retirement are native C++.

Scores are converted to Red's viewpoint. A generation token plus document/node identity prevents stale analysis from editing another position. Current analysis is FEN-based; preserving game history for repetition context, batch analysis and play against the engine remain future work.

## Tests

Native CTest covers document edits/save, UCI protocol and deterministic migration fixtures. The golden JSON was generated from the pinned original implementation before removing JS. It compares legal move sets, Chinese notation, resulting FENs, checks, PGN trees/metadata/stringification and Walker coordinates.

The WinUI smoke test invokes board Buttons through their own automation peers, saves/reopens OXQ, switches native texture controls, flips the board and captures its own visual tree. It does not automate unrelated applications.
