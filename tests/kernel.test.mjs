import test from "node:test";
import assert from "node:assert/strict";
import vm from "node:vm";
import { readFile } from "node:fs/promises";
import { Chess } from "../reference/xiangqi.js/dist/esm/chess.js";
const context = vm.createContext({});
vm.runInContext(await readFile("kernel/generated/kernel.js", "utf8"), context);
const call = (op, data = {}) =>
  JSON.parse(context.XiangqiKernel.dispatch(JSON.stringify({ op, data })));
test("original Chinese and ICCS notation survives bundling", () => {
  const fen = call("initial").fen;
  const source = new Chess(fen);
  assert.equal(call("position", { fen }).moves.length, 44);
  for (const m of source.moves({ verbose: true })) {
    const result = call("move", { fen, notation: m.iccs });
    assert.equal(result.move.zh, m.zh);
    assert.equal(result.fen, m.after);
  }
});
test("legal moves stay identical over deterministic games", () => {
  let seed = 17;
  for (let game = 0; game < 4; game++) {
    const chess = new Chess();
    for (let ply = 0; ply < 40; ply++) {
      const moves = chess.moves({ verbose: true });
      if (!moves.length) break;
      const bundled = call("position", { fen: chess.fen() });
      assert.deepEqual(
        bundled.moves.map((m) => m.lan).sort(),
        moves.map((m) => m.lan).sort(),
      );
      seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
      const move = moves[seed % moves.length];
      assert.equal(
        call("move", { fen: chess.fen(), notation: move.lan }).move.zh,
        move.zh,
      );
      chess.move(move.lan);
    }
  }
});
test("original PGN branches, comments, glyphs, arrows and evaluation", () => {
  const games = call("parse", {
    pgn: '[Event "迁移"]\n1. H2-E2! {评论 [%eval 0.35] [%cal Gh2e2]} (H0-G2) H7-E7 *',
  });
  assert.equal(games.length, 1);
  assert.equal(games[0].warnings.length, 0);
  assert.equal(games[0].nodes.length, 4);
  const first = games[0].nodes[1];
  assert.equal(first.meta.eval.score, 35);
  assert.equal(first.meta.glyph.symbol, "!");
  assert.equal(first.meta.shapes[0].orig, "h2");
  assert.equal(first.comments[0].trim(), "评论");
  const text = call("stringify", games[0]);
  const reparsed = call("parse", { pgn: text });
  assert.equal(reparsed[0].nodes.length, 4);
  assert.equal(reparsed[0].warnings.length, 0);
});
test("invalid moves fail", () => {
  assert.throws(() =>
    call("move", { fen: call("initial").fen, notation: "a0i9" }),
  );
  assert.equal(
    call("parse", { pgn: "1. H2-E2 invalid" }).at(0).warnings.length,
    1,
  );
});
