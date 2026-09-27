#include "Presentation/MemoriaUiKit.h"
#include "Engine/Texture2D.h"
namespace MemoriaUiKit
{
FLinearColor Srgb(float R, float G, float B, float A)
{
    FLinearColor C = FLinearColor::FromSRGBColor(FColor(uint8(R * 255.f + .5f), uint8(G * 255.f + .5f), uint8(B * 255.f + .5f)));
    C.A = A; return C;
}
UTexture2D* Paint(int32 W, int32 H, TFunctionRef<FLinearColor(const FVector2D&)> Color)
{
    auto* Texture = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
    if (!Texture) return nullptr;
    Texture->SRGB = true; Texture->Filter = TF_Bilinear; Texture->AddressX = TA_Clamp; Texture->AddressY = TA_Clamp;
    FColor* Pixels = static_cast<FColor*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
    auto Byte = [](float V) { return uint8(FMath::Clamp(V, 0.f, 1.f) * 255.f + .5f); };
    for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X)
    {
        const FLinearColor C = Color(FVector2D((X + .5) / W, (Y + .5) / H));
        Pixels[Y * W + X] = FColor(Byte(C.R), Byte(C.G), Byte(C.B), Byte(C.A));
    }
    Texture->GetPlatformData()->Mips[0].BulkData.Unlock(); Texture->UpdateResource(); return Texture;
}
UTexture2D* Gradient(int32 W, int32 H, FVector2D From, FVector2D To, bool bRadial, std::initializer_list<FStop> Stops)
{
    const FVector2D Axis = To - From; const double Length = FMath::Max(Axis.Size(), 1e-6), Length2 = FMath::Max(Axis.SizeSquared(), 1e-9);
    const TArray<FStop> S(Stops);
    return Paint(W, H, [&](const FVector2D& UV)
    {
        const float T = FMath::Clamp(float(bRadial ? (UV - From).Size() / Length : FVector2D::DotProduct(UV - From, Axis) / Length2), 0.f, 1.f);
        if (T <= S[0].T) return S[0].C;
        for (int32 I = 1; I < S.Num(); ++I)
            if (T <= S[I].T) { const float U = (T - S[I - 1].T) / FMath::Max(S[I].T - S[I - 1].T, 1e-6f); return FMath::Lerp(S[I - 1].C, S[I].C, U); }
        return S.Last().C;
    });
}
}
