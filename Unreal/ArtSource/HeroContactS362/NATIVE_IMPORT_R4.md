# R4 native hilt preservation

R3 preserved2544/2544authored hilt corners but only441/1554accepted native
fallback corners: Nanite reprocessed that hilt after adding steel. Staff already
preserved2508/2508exact native render corners.

R4 changes only NEW sword build. ExportStaticMeshLOD copies accepted original
native LOD0; remove14old steel triangles and append158new steel. New source and
render inventory explicitly518native hilt +158steel =676. Nanite, reduction,
lightmap generation and basis recomputation are disabled only on new sword.
Persistent UE FastBuild copies supplied values without normalization or
simplification. Original8bit tangent/halfUV storage is retained. Raw packed
tangent bytes, all position/UV/RGBA, winding and multiplicity remain EXACT.
Original848authored hilt is independently checked unchanged. Staff keeps the
valid836+760native method.

Explicit material slot mapping protects paint. Unsupported native storage/basis
flags reject. Report separates position/UV/RGBA retention and basis-angle
diagnostics. FreshR4packages read immutable R2source; actual README and source
generator docstring document current method/command. No engine/build was run.

Durability: serialized bDoFastBuild is checked via reflection and recorded with
Nanite/reduction/recompute/scale/lightmap/storage flags. Installed UE Cache
rebuilds loaded FastBuild through BuildFromMeshDescription, including cook cache.
Fresh-process PropsR4Reload verifies676/1596saved triangles and exact original
render records. Report native_LOD0_fingerprint_sha1_v1 is computed by runtime
FMemoriaNativePropFingerprint::MeshLOD0; cooked smoke must compare loaded R4
full-mesh hashes against accepted native report values. Empty cannot pass.

Contact Full expects actual R4steel and captures four processed views in
Saved/Validation/HeroContactS362/CapturesR4 while restoring the previous camera.
Native compile/import/reload/cook/captures remain pending root execution.
