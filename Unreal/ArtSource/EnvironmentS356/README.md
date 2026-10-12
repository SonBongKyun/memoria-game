# S356 original architecture albedo atlas

`ArchitectureAtlas.png` is one original surface atlas generated with the built-in `image_gen` tool. It is copied byte-for-byte from the generator output and retained intact. The prompt requested 2048px or higher; the **actual output is 1254 x 1254 pixels**. No enlargement, cropping, retouching or replacement of earlier source images was performed. Each quadrant is exactly **627 x 627 pixels**.

The four subjects and their boundary at pixel 627 were inspected in the native atlas. Texture origin is top-left; bounds below use left/top inclusive and right/bottom exclusive. The atlas UV transform uses scale `(0.5, 0.5)` with the listed offset; import/material handling may add an inset to avoid sampling a neighbouring quadrant.

| Quadrant | Surface | Pixel bounds (L,T,R,B) | UV offset |
| --- | --- | --- | --- |
| top_left | weathered russet terracotta roof tiles | `[0, 0, 627, 627]` | `[0, 0]` |
| top_right | aged off-white lime plaster | `[627, 0, 1254, 627]` | `[0.5, 0]` |
| bottom_left | rough cool grey fieldstone masonry | `[0, 627, 627, 1254]` | `[0, 0.5]` |
| bottom_right | dark weathered timber planks | `[627, 627, 1254, 1254]` | `[0.5, 0.5]` |

This is painted albedo reference, not measured PBR/normal/roughness data. `generation_prompts.json` preserves the exact prompt and generated source/destination paths. `manifest.json` records PNG and source SHA256 values, dimensions, byte length, quadrant decoded-pixel hashes and mean RGB for distinctness checks. No quadrant files were exported.

Original PNG SHA256: `18e048a2890fcce1179dd6a3faafd8f67263bd923b80ae4a622ce5d4c610d708`.

The original and copied PNG SHA256 values match. Runtime package import, UV binding and native scene verification are recorded separately by the coordinator.
