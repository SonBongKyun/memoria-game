# Hybrid Depth — GPT Image 2 Environment Motifs

Generated on 2026-08-01 with the built-in GPT Image 2 workflow, then converted
from a flat chroma-key background to alpha with the installed imagegen helper.
These are painterly `Sprite3D` landmarks for `HybridDepthStage`; they are not
character portraits or full-screen CG replacements.

## Shared production prompt

- Use case: `stylized-concept`
- Asset type: MEMORIA real-time `Sprite3D` environmental cutout
- Medium: hand-painted charcoal-and-gouache dark fantasy game art, restrained
  brush texture, grounded painterly 2D RPG asset rather than glossy 3D render
- Composition: one self-contained landmark, full silhouette visible, centered
  three-quarter view, generous padding, no cropping
- Background: perfectly uniform chroma key (`#00ff00` or `#ff00ff`) with no
  shadow, gradient, texture, reflection, floor plane, or lighting variation
- Constraints: crisp boundary; no cast/contact shadow; no text, logo, watermark,
  character, unrelated scenery, neon, or ornate high-fantasy excess

## Asset-specific prompts

- `motif_root_spire_v1.png`: ancient charred root spire made from fused ash-black
  roots, broken-cathedral silhouette, restrained pale memory-blue fissures and
  tiny desaturated moss traces.
- `motif_relay_obelisk_v1.png`: ruined Belt relay signal post on a heavy stone
  base, bent brass crossbar, cracked unlit lens, blank witness-paper strips.
- `motif_memory_lantern_v1.png`: one suspended blackened-iron memory lantern,
  compact cage and hook, low antique-amber flame remembered through smoke.
- `motif_wrecked_mast_v1.png`: salt-scoured broken mast in compact coastal rocks,
  one snapped crossbeam and restrained weathered rope close to the silhouette.
- `motif_void_monolith_v1.png`: fractured obsidian memory stone, uneven crown,
  single restrained violet fracture, connected black shards at the base.

## Runtime contract

- Alpha edges are locally audited and the source PNGs remain under the Codex
  generated-images store; only final transparent assets live in the project.
- Imports are capped at 1024 px and generate mipmaps because these 1536 px source
  paintings are downscaled into a 640x360 3D subviewport.
- `HybridDepthStage` reuses the same textures across instances, preserves the
  authored painted lighting, keeps the battle center clear, and tints the art in
  response to memory burning.
