# Native document and compatibility kernel

## Boundaries

The WinUI application owns a Kernel and a collection of Documents. All document and kernel operations stay on the UI thread in this prototype. UCI processes run on a background worker with isolated request data; generation and document/node identity checks discard stale results.

Document owns the authoritative OXQF GameModel. The cache contains only derived FENs, notation and layout. Native edit commands change the model; undo/redo stores bounded snapshots (100 states). This first implementation favors correctness over large-library efficiency.

The embedded QuickJS-NG interpreter executes the exact locked xiangqi.js package and copied pure application modules. The bridge exposes a JSON-in/JSON-out dispatcher: initial, position, move, parse, hydrate, layout and stringify. It does not expose eval to user input, host files, networking, DOM, Obsidian or Node APIs. Runtime memory is bounded to 256 MiB with an operation deadline of 30 seconds; this is not a GUI responsiveness guarantee.

The compiled source array is included in the executable. Node and esbuild are build/test tools only.

## Persistent metadata

OXQF GameModel is serialized with its public writer and read with its public reader. Initial position and moves use OXQ square coordinates (rank \* 9 + file, rank 0 at Red's baseline).

Original PGN headers are kept in metadata.extensions under namespace org.xiangqi.native, key pgn_headers. The value is a JSON string, compatible with OXQ's restricted string-valued metadata schema.

Application node metadata is a source_note annotation with:

- author: org.xiangqi.native/v1
- text: a JSON object with version equal to 1 and data containing original node fields.

The data fields include eval, glyph, annotations, shapes, clock, commentAuthors and per-branch result. Human comments remain comment annotations. Unknown unrelated OXQ annotations remain in the model when editing and saving.

This avoids attaching persistent data to NodeId, which the OXQ reader reconstructs from file indices. Moving/reordering branches preserves metadata because it moves with its node.

PGN serialization retains the original application's conventions and their original fidelity limits. OXQ preserves the richer native evaluation record, including depth and MultiPV.

## Saving and imports

Windows saving uses a sibling file created exclusively, complete writes, FlushFileBuffers, then ReplaceFileW for an existing target or MoveFileExW for a new target. A failed replacement removes the temporary file and reports failure. External modification conflict detection, history backups and crash recovery are future work.

Input files have an application-level 256 MiB limit. PGN warnings reject the load rather than silently saving a truncated game. CBL input currently rejects conversion diagnostics; an explicit report/review UI is planned.

An opened multi-game PGN is a collection of independent Documents. The UI saves the active game as OXQ and exports its PGN. A complete multi-game collection export and manifest are planned.

## UCI service

CreateProcessW launches the selected executable directly without a shell or a visible console. Inherited pipe handles are explicitly limited. UCI initialization and isready handshakes precede analysis; exact scores exclude bound values. Cancellation sends stop and drains bestmove before reuse; timeout/process failure retires the session.

Native engine scores are converted to Red's viewpoint before being stored, matching EngineModule.toWhiteView in the original application. Opening another node or game invalidates the request generation, so a late result cannot modify the wrong position.

The current API analyzes a FEN. Preserving move history for engine repetition context, automatic/batch analysis and engine-vs-human play remain follow-up work.
