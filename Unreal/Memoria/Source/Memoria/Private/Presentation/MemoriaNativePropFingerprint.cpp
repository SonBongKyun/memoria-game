#include "Presentation/MemoriaNativePropFingerprint.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Misc/SecureHash.h"

namespace
{
FString Bits(float V){uint32 B;FMemory::Memcpy(&B,&V,4);return FString::Printf(TEXT("%08x"),B);}
FString Corner(const FStaticMeshLODResources& L,uint32 I,int32 UVChannels)
{
    const auto P=L.VertexBuffers.PositionVertexBuffer.VertexPosition(I);const auto& V=L.VertexBuffers.StaticMeshVertexBuffer;
    const auto X=V.VertexTangentX(I),Z=V.VertexTangentZ(I);
    FString K=Bits(P.X)+Bits(P.Y)+Bits(P.Z)+Bits(X.X)+Bits(X.Y)+Bits(X.Z)+Bits(Z.X)+Bits(Z.Y)+Bits(Z.Z)+Bits(X.W)+Bits(Z.W);
    const int32 Stride=V.GetTangentSize()/V.GetNumVertices();K+=FString::FromHexBlob(static_cast<const uint8*>(V.GetTangentData())+I*Stride,Stride);
    const auto C=L.VertexBuffers.ColorVertexBuffer.GetNumVertices()?L.VertexBuffers.ColorVertexBuffer.VertexColor(I):FColor::White;
    K+=FString::Printf(TEXT("%08x"),C.DWColor());
    for(int32 U=0;U<UVChannels;++U){const auto UV=V.GetVertexUV(I,U);K+=Bits(UV.X)+Bits(UV.Y);}return K;
}
}
FString FMemoriaNativePropFingerprint::MeshLOD0(UStaticMesh* Mesh,int32 MaterialSlot,int32 UVChannels)
{
    if(!Mesh || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.IsEmpty())return FString();
    const auto& L=Mesh->GetRenderData()->LODResources[0];const auto& V=L.VertexBuffers.StaticMeshVertexBuffer;
    if(!V.GetNumVertices() || !V.GetTangentData() || !V.GetTexCoordData() || !L.VertexBuffers.PositionVertexBuffer.GetVertexData())return FString();
    if(L.VertexBuffers.ColorVertexBuffer.GetNumVertices() && !L.VertexBuffers.ColorVertexBuffer.GetVertexData())return FString();
    if(UVChannels==INDEX_NONE)UVChannels=V.GetNumTexCoords();
    if(UVChannels<1 || UVChannels>int32(V.GetNumTexCoords()))return FString();
    const auto Indices=L.IndexBuffer.GetArrayView();TArray<FString> Records;
    if(Indices.Num()==0)return FString();
    for(const auto& S:L.Sections)
        if(uint64(S.FirstIndex)+uint64(S.NumTriangles)*3u>uint64(Indices.Num()))return FString();
    for(const auto& S:L.Sections)if(MaterialSlot==INDEX_NONE || S.MaterialIndex==uint32(MaterialSlot))
        for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;I+=3)
        {
            FString C[3];for(int32 J=0;J<3;++J)C[J]=Corner(L,Indices[I+J],UVChannels);
            FString K=C[0]+C[1]+C[2];for(int32 J=1;J<3;++J){const FString R=C[J]+C[(J+1)%3]+C[(J+2)%3];if(R<K)K=R;}Records.Add(MoveTemp(K));
        }
    if(Records.IsEmpty())return FString();Records.Sort();FSHA1 Hash;
    for(const auto& R:Records){const FTCHARToUTF8 Bytes(*R);Hash.Update(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());const uint8 Newline=10;Hash.Update(&Newline,1);}
    Hash.Final();uint8 Digest[FSHA1::DigestSize];Hash.GetHash(Digest);return FString::FromHexBlob(Digest,FSHA1::DigestSize);
}
