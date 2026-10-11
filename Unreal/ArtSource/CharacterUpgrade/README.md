# S354 character geometry upgrade

Runtime sources for nine ambient characters and two dedicated field creatures. These are game adaptations of the existing cast/encounter names, not additions to narrative canon.

## Rebuild

1. Run Blender 4.5.3 with `--background --factory-startup -t 2 --python-exit-code 2 --python Unreal/Tools/build_character_geometry_s354.py`. Optional names after `--` restrict authoring.
2. Run `python Unreal/Tools/generate_creature_surfaces_s354.py` with NumPy/Pillow for the four technical data maps. It creates arrays only; it never opens or modifies source art.
3. Generated packages are immutable revisions: the commandlet imports missing packages and preserves existing ones. To regenerate changed FBX, use a scratch checkout with only the S354 generated SK_Detail/creature packages omitted; never reimport them through Interchange. Build MemoriaEditor, then run `UnrealEditor-Cmd.exe <project> -run=MemoriaCharacterGeometry -unattended -nop4 -RenderOffscreen -AllowCommandletRendering`.
4. Run native FieldCharacters, VerdanExploration, FieldFoes and FoeVariety tests, inspect the native captures, then run the related regression suite.

The pigment atlas is an intact built-in imagegen output, copied into this directory. The full prompt, mode and dimensions are in [Surface/PIGMENT_PROMPT.md](Surface/PIGMENT_PROMPT.md). It is a versioned source input, not reproducible by the mathematical data-map generator. Unreal imports it as sRGB and resamples to power-of-two mips. Normal/roughness data use linear sampling and separate compression.

## Geometry and animation contracts

- Traveler, Bureauagent, Guard, Villagerf, Villagerm, Fisherman, Elder, Child, Scholar retain original 77-bone bind transforms, painted head/hair/hands/boots, material and animation assets. Fitted coats, sleeves, split skirts, cuffs, hats, lapels and pouches replace the cylindrical clothing. Heights remain 174/176/178/165/175/172/165/120/172 cm. Each new mesh is `SK_<Name>_Detail`, with runtime fallback to the original mesh.
- RubbleRat has four articulated legs and a six-joint tail (18 bones). Its core is voxel fused into one surface with a weighted neck transition. DustCrawler has six articulated legs, layered carapace, mandibles and antennae (16 bones).
- Both creatures have five clips: Idle, Walk, Bite, HitReact_Front_Lgt_01 and Death_Front_01. Export uses 30 fps, centimetres and explicit linear FBX vertex colours. Default sRGB vertex export would make dark pigments too pale in the legacy Unreal FBX importer.
- Mesh and clips are imported together through the same explicit legacy FBX scene conversion. Generic reimport is deliberately disabled because switching to Interchange changed the bind axes. Live tests compare body orientation/height to bind pose, inspect actual limb motion, clip compatibility, texture binding and held death. Material authoring rejects shader compilation errors.
- Runtime heights are 62/64 cm; existing HP, damage, attack behaviour, windup, collision and status rules remain unchanged. Old creature/character source assets and packages are preserved.

These remain stylized game models. Shared painted NPC faces remain; this task does not resculpt Arrel, Elia, Malet or the four remaining shared-body encounter variants. No Shipping performance claim or new LOD authoring is made.
