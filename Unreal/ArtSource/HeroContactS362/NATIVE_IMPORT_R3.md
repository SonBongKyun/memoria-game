# Native props R3 correction and contact fixture diagnostics

Measured R2 cause: accepted original sword is Nanite-enabled. It retains848
authored painted hilt triangles but renders518fallback hilt triangles plus14
old steel triangles. R2 preserves all2544authored hilt corners exactly and
still has518fallback hilt triangles; its158authored new steel triangles become
142fallback steel triangles. The unconditional676total assumption incorrectly
compared a derived Nanite fallback against a standalone non-Nanite addition.
Staff is not Nanite:1596rendered triangles and2508/2508retained render corners
already passed exactly. R1/R2materials, source files and native reports remain
untouched.

R3 outputs fresh packages under HeroContactS362/R3, reading immutable R2 source.
Use existing commandlet with -Revision=R3 -AllowCommandletRendering
-RenderOffscreen. Report is HeroContactS362/PropsNative_R3.json.

Preservation remains exact: original native MeshDescription corner attributes,
original hilt fallback triangle winding/multiplicity/attributes, original build
and Nanite settings. New authored additions additionally require exact native
MeshDescription triangle/attribute matches. Staff retains exact triangle totals.

Only the Nanite new-steel test changes from an invalid triangle-count equality
to geometric equivalence. Both authored158-triangle and native fallback
surfaces are sampled at barycentric subdivisions8, including every profile
vertex, in both directions. Maximum surface and outer-envelope errors must
remain below one quarter of the smallest authored fuller depth: thin-axis
span/2 *.8minimum section depth *.22crest/trough difference *.25 =.0198cm.
This is a feature-based geometry test, not permission to lose16arbitrary
triangles. All measurements and original fallback settings are reported.
Root must still render and inspect actual new steel before graphics acceptance.

Full contact fixture diagnosis: installed UE GetCPUSkinnedVertices explicitly
calls RefreshBoneTransforms and changes CPU render state. Comparing two calls
therefore re-evaluated the action, producing0.0002..0.0815cm differences in the
first fraction of seven fresh clips. The fixture now manually evaluates the
skeletal child once, then uses native ComputeSkinnedPositions from its current
ref-to-local matrices for both comparisons. Arrel has no morph/cloth simulation
position layer; this is asserted. The exact0 body comparison is unchanged.
Child automatic ticking is disabled only on the fixture component.

Walk history previously included the idle sample eight frames earlier. It now
starts at the first actual walk sample and is updated only by temporal walk
samples. Existing .25cm drift, sole ground/support and airborne thresholds are
unchanged. Steady intervals require both endpoint locomotion weights>.98;
entry-blend supported motion remains separately reported. New walk_step_geometry
records frame, phase, current/previous weight, foot/ball/pelvis bones, each actual
sole center/ground spread/drift and whether the interval counts as planted.
This will distinguish real authored sliding from fixture timing/entry
transitions. The observed4.47455cm aggregate is not waived or declared fixed.

Each before/after pose read records exact native bone-transform SHA1, component
transform, frame, FApp current/fixed delta, age/phase and action time/weight.
Exact body vertex and bone equality assertions remain. Full also waits16native
processed frames for each front/profile/back/heldguard view, requests fresh PNGs,
waits7more frames to check their timestamps/size, and restores the previous
camera. Captures are under HeroContactS362/CapturesR3. The capture stage asserts
that the actual live blade comes from the R3package.

No engine or build was launched by this agent. Native validation remains root's
next step. Original mount, grip, cloth, rig, clips and combat timing are unchanged.
