#pragma once
#include "CoreMinimal.h"
class UStaticMesh;

/** Stable native render proof usable by editor reload and cooked smoke. */
struct MEMORIA_API FMemoriaNativePropFingerprint
{
    // v1: LOD0 triangles, cyclic start canonicalized with winding retained,
    // sorted with multiplicity. Corner records contain float position/decoded
    // basis bits, RAW packed tangent bytes, RGBA and all actual UV channels.
    // SHA1 hashes each UTF8 record followed by a newline. Slot filter optional.
    // Explicit UVChannels compares an authored prefix (R5: original2UVs).
    // Default includes ALL actual UVs, including checked-zero padding.
    // Empty means CPU render buffers unavailable; never treat it as a match.
    static FString MeshLOD0(UStaticMesh* Mesh,int32 MaterialSlot=INDEX_NONE,int32 UVChannels=INDEX_NONE);
};
