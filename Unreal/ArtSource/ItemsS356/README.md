# S356 original item paintings

Six original inventory paintings were generated with the built-in `image_gen` tool for MEMORIA's existing field-item kit. All files are **1254 x 1254 RGBA PNGs**, copied byte-for-byte from the generator output. The SHA256 of each destination equals its generated source file. No source image was enlarged, cropped, retouched or used as an editing reference; old HUD icons remain intact.

`generation_prompts.json` retains all seven exact generation prompts, generated source paths and destination paths, including the companion architecture atlas. `manifest.json` records the six item PNG byte lengths, dimensions, source/destination SHA256 values, alpha statistics and runtime bindings.

The five existing quick slots remain Z/X/C/V/B. Potion and Hi-Potion share Z and retain automatic selection from wound depth. The other four categories retain cure, fire, escape and ward behavior. Distinct blue/cyan healing vessels, green botanical antidote, iron firebomb, cloth smoke bundle and violet inkwell silhouettes identify the existing items.

| Item ID | Runtime texture in `/Game/Memoria/Presentation/ItemsS356/` | Alpha <= 2 | Alpha >= 245 | Original PNG SHA256 |
| --- | --- | ---: | ---: | --- |
| potion | T_ItemPotion | 63.31% | 36.10% | `bf9afe172cfabf2224ad897229e58eb10f00279c1ce062735653e538aa3374ca` |
| hi_potion | T_ItemHiPotion | 68.46% | 30.82% | `61ee6fa7f58309779340f03a2874ca7de23b3ea9b9f52bff64e8f3c5828de6a7` |
| antidote | T_ItemAntidote | 76.94% | 22.10% | `0e622a5c2f29487711289d88794d096c9c8a60414b9b57883a43835ee9817655` |
| firebomb | T_ItemFirebomb | 51.49% | 47.21% | `24fe727029d9203907a21d19d3029b5dbb5f9e2b5f901f70ae736367ceea8836` |
| smoke_bomb | T_ItemSmokeBomb | 74.16% | 22.99% | `18af056530fb726c9baf70544b53fb2de5f1ee09d2c451a1f4d8a7143da9324a` |
| witness_ink | T_ItemWitnessInk | 61.82% | 36.93% | `7b8d67bf2b1e075e38e80970a7d236141ae1d045255ed7bc91d6f52459a48084` |

All six cutouts meet the importer/test thresholds: at least 20% transparent pixels, at least 5% opaque subject pixels and alpha <= 2 at all four corners. Alpha verification reads the original PNGs; no pixels are rewritten.

`MemoriaItemArtAssets` imports these files into new immutable packages and refuses existing targets. It retains the complete source painting, uses sRGB alpha-safe UI compression (`TC_EditorIcon`), clamp sampling, no mips and a 512px maximum UI resource. The original on-disk PNG and old texture packages are preserved. Run the commandlet after building MemoriaEditor; successful engine import and native rendering are documented separately by the coordinator.
