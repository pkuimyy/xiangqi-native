# Migration progress

Date: 2026-10-23. This is an implementation checkpoint, not a claim of complete feature parity.

## Implemented

- Independent Git repository; WinUI 3/C++20 application and source builds.
- Embedded original rules 1.0.3, Chinese/ICCS notation, original PGN parser/stringifier and original Walker tree layout.
- Native click-to-move board, board flip, branch graph selection and legal destinations.
- Native arrows/highlights from existing annotation data and stored best-move arrows.
- Multiple opened games with native selector; navigation, branch deletion/promotion, undo/redo.
- Human comment editing; pending text is committed before navigation/save.
- OXQF authoritative game model, PGN import/export and rich metadata OXQ round-trip.
- Unicode file paths, Windows native file pickers, atomic overwrite.
- UCI native process with depth, skill/MultiPV protocol, cancellation, timeout, Red-view evaluation conversion and stale-result protection.
- Automatic discovery of a pinned optional native Pikafish distribution.
- Actual WinUI startup/render/save smoke test with a generated preview.

## Verified during development

- Original application regression tests: 41 passed.
- Bundled kernel comparison tests: 4 passed, covering all initial moves and deterministic game sequences.
- Native C++ document and engine test targets passed in Debug and Release.
- Native engine process tests cover MultiPV, bound scores, ponder, score orientation, cancellation/reuse and timeout.
- Actual Pikafish 2026-09-06 returned depth-6 analysis with bestmove h2e2 for the standard opening.
- Actual WinUI startup/render/save test passed with a 7-node branched game.
- Development dependencies updated to a clean npm audit result.

Release build and the final screenshot are recorded in the current implementation session; repeat scripts/build.ps1 and scripts/smoke.ps1 to regenerate local evidence.

## Next work, in order

1. Native interaction regression tests: exercise the actual board click handler, dragging, keyboard accelerators, branch navigation/folding and comment workflows.
2. Position editor, native move list, glyph/arrow editing, preview boards and richer evaluation presentation.
3. Automatic/batch analysis and play-against-engine, including move-history context and robust engine scheduling.
4. External-file conflict handling, backups, autosave and crash recovery.
5. CBL conversion diagnostics UI and export; whole-collection PGN/CBL workflows.
6. Native PNG/GIF export, themes/piece sets, speech, sound and settings persistence.
7. Incremental/cancellable calculation, large-tree rendering and library indexing.
8. Installer, file associations, clean-machine deployment checks and release packaging.

## Practical limits

The current UI still computes full tree projections on the UI thread and has not been benchmarked for large trees. Undo stores full model copies. CBL imports with diagnostics are currently rejected. PGN export operates on the active game. There is no autosave or external-edit conflict detection yet. Full original-feature parity is not complete.
