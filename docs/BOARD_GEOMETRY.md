# Plain board SVG geometry

The application renders assets/board-grid.svg and assets/board-grid-flipped.svg at 450 x 548 logical pixels. Both are self-contained SVGs: grid, river lettering and coordinates are paths, with no external fonts or native TextBlock overlays.

## Grid and pieces

The original assets/board-lines.svg has a 450 x 500 viewBox. Its intersections are x = 25 + 50 * file, y = 25 + 50 * row (file 0..8, row 0..9 from the top). The generated boards translate this grid by (0, 24); the resulting intersections are x = 25 + 50 * file, y = 49 + 50 * row.

Each unchanged OXDF piece SVG has a 58px disc in a 96px viewBox. The native 45px frame displays the image at 45 * 96 / 62 px, giving a visible disc diameter of 58 * 45 / 62 = 42.096774px. Selection scales its frame by 1.10 around its centre. For clearance calculations, the larger full-frame extent is used: 45 * 1.10 = 49.5px, radius 24.75px.

The uppermost selected frame starts at y = 49 - 24.75 = 24.25. The lowermost selected frame ends at y = 499 + 24.75 = 523.75.

## River lettering

After translation, the river occupies y = 249..299; its centre is y = 274. The two equal horizontal halves occupy x = 25..225 and 225..425, so their centres are (125, 274) and (325, 274).

assets/board-river.svg retains the original two lettering paths from OXDF-editor/assets/board.svg. Their exact local vector bounds are computed by Geometry.Parse, including quadratic-curve extrema. Each pair is uniformly scaled to an actual path height of 28px and translated by:

    scale = 28 / bounds.height
    tx = centre.x - scale * (bounds.x + bounds.width / 2)
    ty = centre.y - scale * (bounds.y + bounds.height / 2)

Both pairs therefore occupy y = 260..288, leaving 11px above and below inside the river. Positioning does not depend on font baselines, spaces or line heights.

## File coordinates

The nine labels in each row are centred exactly on x = 25 + 50 * file. Upper label centres are y = 12; lower centres are y = 536. Each glyph is outlined at generation time, centred by its actual path bounds, and capped at 12px high and 16px wide.

The top labels end no lower than y = 18, so clearance to the selected frame is at least 24.25 - 18 = 6.25px. Bottom labels start no higher than y = 530, giving 530 - 523.75 = 6.25px. Both rows have at least 6px canvas-edge padding. The Viewbox scales the SVG and pieces together, preserving these proportions.

Normal orientation retains upper 1..9 and lower 九..一. Flipped orientation retains the existing reversed numbering: upper 9..1 and lower 一..九. River lettering stays upright.

## Regeneration and integration

Run scripts/generate-board.ps1 on Windows. It reads the grid and original river paths, converts Segoe UI / Microsoft YaHei coordinate glyphs to outlines, and writes both final SVGs. WPF is used only by this optional asset-authoring script. The application remains entirely C++/WinUI, with no runtime font lookup for board artwork.

BoardView owns the board dimensions and 24px grid offset. It retains the native grid Image and both SvgImageSource objects; orientation selects a cached source. Piece, hit-target, legal-dot and annotation positions share the same shifted grid. Moves never reload board resources.

No automatic previous-move decoration is rendered. Legal destinations and explicit annotations remain. The internal custom/bamboo path is retained, hidden from users, and excluded from tests.
