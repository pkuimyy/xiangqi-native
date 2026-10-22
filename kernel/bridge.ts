import { calculateTreeLayout } from "./upstream/lib/Tree/layout";
import { Chess, DEFAULT_FEN } from "./upstream/chess";
import { PGNParser } from "./upstream/modules/Source/parser";
import { splitPGN } from "./upstream/utils/parse";
import { stringifyPGN } from "./upstream/utils/stringify-pgn";

const metaKeys = [
  "eval",
  "glyph",
  "annotations",
  "shapes",
  "clock",
  "commentAuthors",
  "result",
];
const sq = (s: string) => s.charCodeAt(0) - 97 + Number(s[1]) * 9;
const algebraic = (s: number) =>
  String.fromCharCode(97 + (s % 9)) + Math.floor(s / 9);
function metadata(n: any) {
  const result: any = {};
  for (const k of metaKeys) if (n[k] !== undefined) result[k] = n[k];
  return result;
}
function flatten(root: any) {
  const result: any[] = [];
  const stack = [root];
  while (stack.length) {
    const n = stack.pop();
    result.push({
      id: n.id,
      x: n.x ?? 0,
      y: n.y ?? 0,
      parent: n.parentID ?? null,
      children: n.children.map((x: any) => x.id),
      fen: n.fen,
      move: n.move
        ? {
            from: sq(n.move.from),
            to: sq(n.move.to),
            zh: n.move.zh,
            iccs: n.move.iccs,
            lan: n.move.lan,
          }
        : null,
      comments: n.comments ?? [],
      meta: metadata(n),
    });
    for (let i = n.children.length - 1; i >= 0; i--) stack.push(n.children[i]);
  }
  return result;
}
function hydrate(data: any) {
  const byId = new Map(data.nodes.map((n: any) => [n.id, n]));
  const all = new Map<string, any>();
  const rootData: any = byId.get(data.root);
  if (!rootData) throw Error("Missing root");
  const root: any = {
    id: rootData.id,
    parentID: null,
    fen: new Chess(data.fen).fen(),
    move: null,
    children: [],
    comments: rootData.comments,
    ...rootData.meta,
  };
  all.set(root.id, root);
  const stack = [root];
  while (stack.length) {
    const n = stack.pop();
    const source: any = byId.get(n.id);
    for (const id of source.children) {
      const child: any = byId.get(id);
      if (!child?.move) throw Error("Missing child move");
      const chess = new Chess(n.fen);
      const m = chess.move(
        algebraic(child.move.from) + algebraic(child.move.to),
      );
      if (!m) throw Error("Illegal stored move");
      const node: any = {
        id,
        parentID: n.id,
        fen: chess.fen(),
        move: m,
        color: m.color === "w" ? "white" : "black",
        children: [],
        comments: child.comments,
        ...child.meta,
      };
      n.children.push(node);
      all.set(id, node);
      stack.push(node);
    }
  }
  return { root, all };
}
export function dispatch(request: string): string {
  const { op, data = {} } = JSON.parse(request);
  let result: any;
  switch (op) {
    case "initial":
      result = { fen: new Chess(DEFAULT_FEN).fen() };
      break;
    case "position": {
      const c = new Chess(data.fen);
      result = {
        fen: c.fen(),
        board: c.board(),
        turn: c.turn(),
        check: c.isCheck(),
        checkmate: c.isCheckmate(),
        stalemate: c.isStalemate(),
        moves: c
          .moves({ verbose: true })
          .map((m: any) => ({
            from: sq(m.from),
            to: sq(m.to),
            zh: m.zh,
            iccs: m.iccs,
            lan: m.lan,
          })),
      };
      break;
    }
    case "move": {
      const c = new Chess(data.fen);
      const m = c.move(
        data.notation ?? algebraic(data.from) + algebraic(data.to),
      );
      result = {
        fen: c.fen(),
        move: {
          from: sq(m.from),
          to: sq(m.to),
          zh: m.zh,
          iccs: m.iccs,
          lan: m.lan,
        },
        check: c.isCheck(),
        result:
          c.isCheckmate() || c.isStalemate()
            ? c.turn() === "b"
              ? "1-0"
              : "0-1"
            : null,
      };
      break;
    }
    case "parse":
      result = splitPGN(data.pgn).map((slot: any) => {
        const p = new PGNParser(slot.raw);
        return {
          headers: Object.fromEntries(p.tags),
          fen: p.getRoot().fen,
          root: p.getRoot().id,
          nodes: flatten(p.getRoot()),
          warnings: p.getSkipped(),
        };
      });
      break;
    case "layout": {
      const h = hydrate(data);
      h.all.set("node-root", h.root);
      calculateTreeLayout(h.all, new Set(), 1.0);
      result = { nodes: flatten(h.root) };
      break;
    }
    case "hydrate": {
      const h = hydrate(data);
      result = { nodes: flatten(h.root) };
      break;
    }
    case "stringify": {
      const h = hydrate(data);
      const tags = Object.entries(data.headers ?? {})
        .map(
          ([k, v]) =>
            "[" +
            k +
            ' "' +
            String(v).replace(/\\/g, "\\\\").replace(/"/g, '\\"') +
            '"]',
        )
        .join("\n");
      result =
        tags +
        "\n\n" +
        stringifyPGN(h.root, true, data.annotationFormat ?? "suffix");
      break;
    }
    default:
      throw Error("Unknown operation: " + op);
  }
  return JSON.stringify(result);
}
globalThis.XiangqiKernel = { dispatch };
