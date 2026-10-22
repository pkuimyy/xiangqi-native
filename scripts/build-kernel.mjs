import * as esbuild from "esbuild";
import { writeFile, mkdir } from "node:fs/promises";
import { createHash } from "node:crypto";
const build = await esbuild.build({
  entryPoints: ["kernel/bridge.ts"],
  bundle: true,
  write: false,
  format: "iife",
  platform: "neutral",
  target: "es2022",
  treeShaking: true,
  alias: { "@weshell/xiangqi.js": "./reference/xiangqi.js/dist/esm/chess.js" },
  plugins: [
    {
      name: "no-dom-board",
      setup(b) {
        b.onResolve({ filter: new RegExp("^@weshell/xiangqiground$") }, () => ({
          path: "unused-board",
          namespace: "native",
        }));
        b.onLoad({ filter: /.*/, namespace: "native" }, () => ({
          contents: "export const Chessground = undefined;",
        }));
      },
    },
  ],
});
const js = build.outputFiles[0].text;
if (js.includes('from "obsidian"') || js.includes("require('obsidian')"))
  throw Error("Obsidian dependency in kernel");
await mkdir("kernel/generated", { recursive: true });
await writeFile("kernel/generated/kernel.js", js);
const bytes = [...Buffer.from(js), 0];
await writeFile(
  "kernel/generated/kernel.inc",
  bytes
    .map(
      (n, i) =>
        String(n) +
        (i < bytes.length - 1 ? "," : "") +
        (i % 32 === 31 ? "\n" : ""),
    )
    .join(""),
);
console.log("Kernel SHA256", createHash("sha256").update(js).digest("hex"));
