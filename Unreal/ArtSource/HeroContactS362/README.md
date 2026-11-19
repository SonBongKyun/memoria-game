# S362 weapon/contact finish — R5 native candidate

R5 reads immutable R2 additive source and creates fresh native packages. Native
import, fresh-process reload, cooked fingerprints and actual captures are
required before graphics acceptance. Original packages, paint, garments,77bind
rigs, clips, grip, combat timing and source revisions remain preserved.

## Native construction

The original sword has848authored hilt triangles but its accepted DX11-visible
Nanite fallback has518. Rebuilding Nanite after adding steel changed derived
hilt attributes; R2/R3 were rejected by exact native comparisons.

R5 exports ACCEPTED ORIGINAL native LOD0 to MeshDescription, removes only14old
steel triangles, and appends158new bevel/fullered steel triangles. NEW sword:
518native painted hilt +158steel =676triangles. Only this new asset disables
Nanite/reduction/basis recomputation and uses persistent UE FastBuild. All1554
original hilt corners must match EXACTLY: position, every UV, RAW packed
tangent bytes, sRGB color, winding and multiplicity. Original8bit tangent/halfUV
storage is retained for durable reload. The installed UE5.8 FastBuild TArray
buffer Init ignores its requested UV count and emits all8 storage channels.
R5 explicitly initializes6unused channels to positive zero in NEW in-memory
source copies. Every original UV0/UV1 remains bit-exact; all added source/render
UV scalars must be exactly zero. Extra nonzero channels reject. This avoids
uninitialized buffer contents through fresh reload and cook without an engine
patch. Hilt-prefix fingerprint explicitly compares both original UV channels;
full cooked fingerprints include all8 actual channels. Original848authored hilt data remains
an independently checked unchanged source diagnostic.

Staff retains the validated method: preserve836original native triangles and
all painted attributes, append760fitted collar triangles, and finish original
gold/crystal material regions. Total1596triangles; original20triangle crystal
geometry stays intact. Original wood/leather material remains unchanged.

Four new materials clone original graphs/overrides. Steel keeps authored
colors/textures and uses a cloned parent with actual roughness.26. Staff
gold/crystal retain the original atlas/UVs and neutral fill. Original materials
are never saved.

## Current command and durable proof

Existing R2source is already generated, hash-pinned and immutable:

- ArrelBlade/ArrelBlade.fbx:158new steel triangles only.
- EliaCollars/EliaCollars.fbx:760new collar triangles only.
- EliaRegions/EliaRegions.fbx:836unchanged source reference triangles;
  regions88wood/leather,728gold,20crystal. Classification only.

Roundtrip retains2508/2508staff reference position/allUV/RGBA corners, all zero
origins and original/R1hashes. Do not regenerate R2. The source generator
build_hero_props_native_s362.py documents this finalR5native method.

Run with isolated validation UserDir:

```
UnrealEditor-Cmd Memoria.uproject -run=MemoriaHeroPropsS362 -Revision=R5 -unattended -nop4 -AllowCommandletRendering -RenderOffscreen
```

Fresh output paths (existing targets are refused):

```
/Game/Memoria/Presentation/Field3D/HeroContactS362/R5/Arrel/SM_Arrel_sword_drawn
/Game/Memoria/Presentation/Field3D/HeroContactS362/R5/Elia/SM_Elia_staff
```

Require exit0, no native errors, exact retained checks, and
S362_PROP_IMPORT_COMPLETE revision=R5. The report is
Saved/Validation/HeroContactS362/PropsNative_R5.json. It records actual serialized
bDoFastBuild, Nanite/recompute/reduction/lightmap/scale/storage flags, and
native_LOD0_fingerprint_sha1_v1. bDoFastBuild is a serialized UPROPERTY;
installed UE Cache invokes BuildFromMeshDescription without the simplifier.

Run MemoriaGraphics.S362.PropsR5Reload in a FRESH editor process: require saved
676/1596triangles, persisted FastBuild and exact original native render records.
Runtime FMemoriaNativePropFingerprint::MeshLOD0 computes the same winding/
multiplicity/raw-attribute SHA1 for cooked smoke. Compare cooked full-mesh R5
hashes to the accepted native report. Empty means unavailable and cannot pass.
R1/R2/R3/R4rejected evidence and immutable sources remain preserved separately.

## Mount and actual views

The mounted pair uses4referencecm outward and15degrees outward cant about native
mouth(0,0,-13.5). Original base offset(0,-21,13), drawn grip and BlockHoldAt.38
remain intact. At150cm hero height mouth movement is3.333worldcm.

Contact compares delivered mount to exact original transform on identical posed
vertices. Crossings AND.25cm-near counts must strictly decrease for idle sheathed,
idle drawn and actual held guard. Residual contacts and original-basis scans
remain reported. Walk/drawn cannot regress; sole/support and.25cm steady drift
thresholds remain. Entry motion, bones, weights, clocks and hashes are separate.

Full captures actual front/profile/back/heldguard PNGs after processed frames in
Saved/Validation/HeroContactS362/CapturesR5 and restores the previous camera.
Root owns integration/rendering. Inspect before acceptance; contact improvement
does not certify full cloth physics.
