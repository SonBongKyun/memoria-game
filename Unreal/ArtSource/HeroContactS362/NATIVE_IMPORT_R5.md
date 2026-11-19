# R5 explicit FastBuild UV padding

R4 did not reach attribute matching: native sword rendered8UV channels versus
accepted original2. Both tangent CPU buffers were present and Build Params
already had bAllowCpuAccess=true. Sword counts676/676 and source original1554
corners/addition158triangles were exact. Staff2508native corners were exact.

Installed engine cause: StaticMesh.cpp BuildFromMeshDescription passes the
requested count to StaticMeshVertexBuffer::Init(TArray, count, flags). Installed
StaticMeshVertexBuffer.cpp:113 ignores count, calls MakeConstMeshBuildVertexView;
Components.h:220 adds MAX_STATIC_TEXCOORDS=8 views. FastBuild load/cook repeats
this path. A one-time buffer shrink would not persist.

Approved R5 only: pad NEW in-memory hilt and blade MeshDescriptions from2to8
UVs with explicit positive zeros. Record original attribute signatures before
padding and require every position/basis/color/originalUV bit unchanged. Source
addition matching compares all8 initialized channels, therefore original2exact
plus6exactzero. Every native padded scalar across every vertex must be zero;
any nonzero fails. Original asset/source revisions remain untouched. No engine
patch or tolerance change.

Fresh packages HeroContactS362/R5 read immutable R2. Exact original native
hilt keys include original2UVs, packed tangent bytes, position, RGBA, winding
and multiplicity. Explicit MeshLOD0(Sword,0,2) fingerprint compares all original
channels; default full-mesh fingerprint still includes all8. JSON labels the
prefix independently and records source/render padding, CPU buffers, counts
and distinct reject reasons. Staff construction stays unchanged.

Run -run=MemoriaHeroPropsS362 -Revision=R5 with AllowCommandletRendering, then
fresh-process MemoriaGraphics.S362.PropsR5Reload and cooked full8fingerprint
proof against PropsNative_R5.json. No engine/build/native acceptance run here.
