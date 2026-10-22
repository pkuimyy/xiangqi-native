import { defineConfig } from "vitest/config";
import { fileURLToPath } from "node:url";
const path = (name) => fileURLToPath(new URL(name, import.meta.url));
export default defineConfig({
  test: { include: ["reference/obsidian-tests/**/*.test.ts"] },
  resolve: {
    alias: [
      { find: /^\.\.\/src\//, replacement: path("./kernel/upstream/") },
      {
        find: "@weshell/xiangqi.js",
        replacement: path("./reference/xiangqi.js/dist/esm/chess.js"),
      },
      {
        find: "@weshell/xiangqiground",
        replacement: path("./kernel/unused-board.ts"),
      },
    ],
  },
});
