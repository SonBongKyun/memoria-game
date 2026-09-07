#include "MemoriaNarrativeImport.h"
#include "MemoriaStartingCatalogImport.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "HAL/FileManager.h"
#include "AssetRegistry/AssetRegistryModule.h"

namespace MemoriaNarrativeImport
{
namespace
{
using Obj=TSharedPtr<FJsonObject>;
using Val=TSharedPtr<FJsonValue>;
bool Same(const FString& A,const FString& B) { return A.Equals(B,ESearchCase::CaseSensitive); }
bool Has(const Obj& O,const TCHAR* K)
{ if (!O) return false; for (const auto& P:O->Values) if (P.Key.ToView().Equals(K,ESearchCase::CaseSensitive)) return true; return false; }
bool KeysSubset(const Obj& O,std::initializer_list<const TCHAR*> Keys)
{
    if (!O) return false;
    for (const auto& P:O->Values) { bool Found=false; for (const auto* K:Keys) Found |= P.Key.ToView().Equals(K,ESearchCase::CaseSensitive); if (!Found) return false; }
    return true;
}
bool Keys(const Obj& O,std::initializer_list<const TCHAR*> K) { return O && O->Values.Num()==int32(K.size()) && KeysSubset(O,K); }
bool ReadFString(const Obj& O,const TCHAR* K,FString& S)
{
    if (!Has(O,K) || !O->HasTypedField<EJson::String>(K) || !O->TryGetStringField(K,S)) return false;
    for (TCHAR C:S) if (C==0) return false;
    return true;
}
bool Readint32(const Obj& O,const TCHAR* K,int32& I)
{
    double N;
    if (!Has(O,K) || !O->HasTypedField<EJson::Number>(K) || !O->TryGetNumberField(K,N) || !FMath::IsFinite(N) || N<0 || N>MAX_int32 || N!=FMath::FloorToDouble(N)) return false;
    I=int32(N); return true;
}
bool Readbool(const Obj& O,const TCHAR* K,bool& B) { return Has(O,K) && O->HasTypedField<EJson::Boolean>(K) && O->TryGetBoolField(K,B); }
Obj Object(const Obj& O,const TCHAR* K) { const Obj* V=nullptr; return O && Has(O,K) && O->TryGetObjectField(K,V) ? *V : nullptr; }
Val Wrap(const Obj& O) { return MakeShared<FJsonValueObject>(O); }
FString Hash(const Obj& O) { return MemoriaCatalogImport::Sha256(MemoriaCatalogImport::Canonical(Wrap(O))+TEXT("\n")); }
bool Hex(const FString& S,int32 N) { if (S.Len()!=N) return false; for (TCHAR C:S) if (!((C>='0' && C<='9') || (C>='a' && C<='f'))) return false; return true; }
#include "MemoriaNarrativeCodecs.inl"

Obj WriteProvenance(const FMemoriaNarrativeProvenance& P)
{
    auto O=MakeShared<FJsonObject>(); O->SetStringField(TEXT("source_file"),P.SourceFile); O->SetStringField(TEXT("source_hash"),P.SourceHash);
    O->SetStringField(TEXT("source_revision"),P.SourceRevision); O->SetStringField(TEXT("dialect"),P.Dialect);
    O->SetNumberField(TEXT("group_position"),P.GroupPosition); O->SetNumberField(TEXT("original_index"),P.OriginalIndex); O->SetNumberField(TEXT("original_choice_index"),P.OriginalChoiceIndex); return O;
}
template<class R> Obj WriteRecord(const R& R0,bool bProvenance)
{
    auto O=MakeShared<FJsonObject>(); O->SetStringField(TEXT("id"),R0.Id); O->SetNumberField(TEXT("original_index"),R0.OriginalIndex);
    O->SetStringField(TEXT("effect_phase"),R0.EffectPhase);
    if (bProvenance) O->SetObjectField(TEXT("provenance"),WriteProvenance(R0.Provenance));
    O->SetObjectField(TEXT("text"),WriteText(R0.Text)); O->SetObjectField(TEXT("presentation"),WritePresentation(R0.Presentation));
    O->SetObjectField(TEXT("gate"),WriteGate(R0.Gate)); O->SetObjectField(TEXT("effects"),WriteEffects(R0.Effects)); O->SetObjectField(TEXT("action"),WriteAction(R0.Action));
    O->SetField(TEXT("jump"),R0.bHasJump ? Val(MakeShared<FJsonValueNumber>(R0.Jump)) : Val(MakeShared<FJsonValueNull>())); return O;
}
template<class D,class Rows> Obj Semantic(const D& Def,const Rows& Items,bool bVN)
{
    auto O=MakeShared<FJsonObject>(); O->SetNumberField(TEXT("schema_version"),1); O->SetStringField(TEXT("content_kind"),bVN?TEXT("narrative.vn"):TEXT("narrative.field")); O->SetStringField(TEXT("dialect"),bVN?TEXT("vn"):TEXT("field"));
    auto Definition=MakeShared<FJsonObject>(); Definition->SetStringField(TEXT("id"),Def.Id); Definition->SetNumberField(TEXT("index_mapping_version"),Def.IndexMappingVersion); Definition->SetObjectField(TEXT("metadata"),WriteMetadata(Def.Metadata));
    TArray<Val> Entries;
    for (const auto& R:Items)
    {
        auto Row=WriteRecord(R,false); Row->SetNumberField(TEXT("storage_index"),R.StorageIndex); Row->SetBoolField(TEXT("choices_present"),R.bChoicesPresent);
        TArray<Val> Choices; for (const auto& C:R.Choices) Choices.Add(Wrap(WriteRecord(C,false)));
        Row->SetArrayField(TEXT("choices"),Choices); Entries.Add(Wrap(Row));
    }
    Definition->SetArrayField(bVN?TEXT("steps"):TEXT("rows"),Entries); O->SetObjectField(TEXT("definition"),Definition); return O;
}
template<class R> bool MatchesAuthored(const Obj& Source,const R& R0,bool V,bool Choice)
{
    if (!Source) return false;
    auto Expected=MakeShared<FJsonObject>();
    for (const auto& Group:{WriteText(R0.Text),WritePresentation(R0.Presentation),WriteGate(R0.Gate),WriteEffects(R0.Effects),WriteAction(R0.Action)})
        for (const auto& P:Group->Values) Expected->SetField(FString(P.Key.ToView()),P.Value);
    if (Choice && R0.bHasJump) Expected->SetNumberField(V?TEXT("goto"):TEXT("jump_to"),R0.Jump);
    auto Raw=MakeShared<FJsonObject>(); Raw->Values=Source->Values;
    Raw->RemoveField(V?TEXT("choice"):TEXT("choices"));
    if (Has(Raw,TEXT("fade")))
    {
        if (!Raw->HasTypedField<EJson::Number>(TEXT("fade")) || !R0.Presentation.bHasFadeMs || FMath::Abs(Raw->GetNumberField(TEXT("fade"))*1000.0-double(R0.Presentation.FadeMs))>1.e-9) return false;
        Raw->RemoveField(TEXT("fade")); Expected->RemoveField(TEXT("fade_ms"));
    }
    return Same(MemoriaCatalogImport::Canonical(Wrap(Expected)),MemoriaCatalogImport::Canonical(Wrap(Raw)));
}
template<class D,class Rows> bool AttestSource(const FMemoriaNarrativeImportMetadata& M,const D& Def,const Rows& Items,bool V)
{
    FString Text; Obj Raw;
    if (!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("../../")/M.Sources[0].Path)) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Raw)) return false;
    const TArray<Val>* SourceRows=nullptr;
    if (V) { if (!Raw->TryGetArrayField(TEXT("steps"),SourceRows) || !Same(Raw->GetStringField(TEXT("id")),Def.Id)) return false; }
    else { auto Groups0=Object(Raw,TEXT("dialogues")); if (!Groups0 || !Groups0->TryGetArrayField(Def.Id,SourceRows)) return false; }
    if (SourceRows->Num()!=Items.Num()) return false;
    auto Meta=WriteMetadata(Def.Metadata);
    for (const auto& P:Meta->Values) { auto Original=Raw->TryGetField(FString(P.Key.ToView())); if (!Original || !Same(MemoriaCatalogImport::Canonical(P.Value),MemoriaCatalogImport::Canonical(Original))) return false; }
    for (int32 I=0;I<Items.Num();++I)
    {
        const auto& R=Items[I]; auto Original=(*SourceRows)[I]->Type==EJson::Object?(*SourceRows)[I]->AsObject():nullptr;
        if (!MatchesAuthored(Original,R,V,false) || Has(Original,V?TEXT("choice"):TEXT("choices"))!=R.bChoicesPresent) return false;
        if (R.bChoicesPresent)
        {
            const TArray<Val>* C=nullptr; if (!Original->TryGetArrayField(V?TEXT("choice"):TEXT("choices"),C) || C->Num()!=R.Choices.Num()) return false;
            for (int32 J=0;J<C->Num();++J) if ((*C)[J]->Type!=EJson::Object || !MatchesAuthored((*C)[J]->AsObject(),R.Choices[J],V,true)) return false;
        }
    }
    return true;
}
bool Groups(const Obj& O,bool V,bool Choice)
{
    const auto T=Object(O,TEXT("text")), P=Object(O,TEXT("presentation")), G=Object(O,TEXT("gate")), E=Object(O,TEXT("effects")), A=Object(O,TEXT("action"));
    if (!T||!P||!G||!E||!A) return false;
    if (Choice && (!KeysSubset(T,{TEXT("text"),TEXT("text_ko")}) || P->Values.Num() || A->Values.Num())) return false;
    if (!V && !KeysSubset(T,{TEXT("speaker"),TEXT("text"),TEXT("text_ko")})) return false;
    if (V && !KeysSubset(G,Choice ? std::initializer_list<const TCHAR*>{TEXT("requires_flag"),TEXT("requires_not_flag"),TEXT("requires_memory_intact")} : std::initializer_list<const TCHAR*>{TEXT("requires_flag"),TEXT("requires_not_flag")})) return false;
    if (!V && !Choice && (!KeysSubset(E,{TEXT("set_flag"),TEXT("record_ending")}) || A->Values.Num())) return false;
    if (!V && Has(E,TEXT("allow_faded_burn"))) return false;
    if (!Choice && Has(E,TEXT("cost_memory"))) return false;
    for (const auto& Group:{G,E,A}) for (const auto& Pair:Group->Values)
        if (Pair.Value->Type==EJson::String && Pair.Value->AsString().IsEmpty() && !Pair.Key.ToView().Equals(TEXT("path"),ESearchCase::CaseSensitive)) return false;
    return true;
}
// Reviewed Field cohort only; count, source group position and package travel together.
struct FFieldCohort { const TCHAR* Id; int32 Count; int32 Position; const TCHAR* Asset; };
const FFieldCohort* FieldCohort(const FString& Id)
{
    static const FFieldCohort Cases[] = {
        {TEXT("verdan_arrival"),5,0,TEXT("DA_Field_VerdanArrival")},
        {TEXT("malet_taste_burned"),3,16,TEXT("DA_Field_MaletTasteBurned")},
        {TEXT("malet_encounter"),10,1,TEXT("DA_Field_MaletEncounter")},
        {TEXT("malet_refused"),3,4,TEXT("DA_Field_MaletRefused")}
    };
    for (const auto& C : Cases) if (Same(Id,C.Id)) return &C;
    return nullptr;
}
template<class R> bool ReadRecord(const Obj& O,R& Row,const FMemoriaNarrativeImportMetadata& M,const FString& Seq,int32 I,int32 J,int32 Count)
{
    const bool V=Same(M.Dialect,TEXT("vn")), Choice=J>=0;
    if (!Groups(O,V,Choice) || !ReadFString(O,TEXT("id"),Row.Id) || !Readint32(O,TEXT("original_index"),Row.OriginalIndex) || Row.OriginalIndex!=(Choice?J:I)) return false;
    FString Id=M.Dialect+TEXT("/")+Seq+TEXT("/")+FString::FromInt(I); if (Choice) Id+=TEXT("/choice/")+FString::FromInt(J);
    if (!Same(Row.Id,Id) || !ReadFString(O,TEXT("effect_phase"),Row.EffectPhase)) return false;
    const FString Phase=Choice ? (V?TEXT("cost_then_flags_burn_rewards_blocking"):TEXT("flags_burn_cost_then_rewards_nonblocking")) : (V?TEXT("effects_then_gate_then_rewards"):TEXT("gate_then_effects"));
    if (!Same(Row.EffectPhase,Phase)) return false;
    Row.Provenance.SourceFile=M.Sources[0].Path; Row.Provenance.SourceHash=M.Sources[0].Sha256Utf8Lf; Row.Provenance.SourceRevision=M.SourceRevision;
    Row.Provenance.Dialect=M.Dialect; Row.Provenance.GroupPosition=V?0:FieldCohort(Seq)->Position; Row.Provenance.OriginalIndex=I; Row.Provenance.OriginalChoiceIndex=J;
    if (!Object(O,TEXT("provenance")) || !Same(MemoriaCatalogImport::Canonical(Wrap(Object(O,TEXT("provenance")))),MemoriaCatalogImport::Canonical(Wrap(WriteProvenance(Row.Provenance))))) return false;
    if (!ReadText(Object(O,TEXT("text")),Row.Text) || !ReadPresentation(Object(O,TEXT("presentation")),Row.Presentation) || !ReadGate(Object(O,TEXT("gate")),Row.Gate) || !ReadEffects(Object(O,TEXT("effects")),Row.Effects) || !ReadAction(Object(O,TEXT("action")),Row.Action)) return false;
    auto Jump=O->TryGetField(TEXT("jump")); if (!Jump) return false; Row.bHasJump=Jump->Type!=EJson::Null;
    if (Row.bHasJump && (!Choice || !Readint32(O,TEXT("jump"),Row.Jump) || Row.Jump>=Count)) return false;
    const auto& A=Row.Action;
    if (Object(O,TEXT("action"))->Values.Num())
    {
        const auto AO=Object(O,TEXT("action"));
        if (!A.bHasAction) return false;
        if (Same(A.Action,TEXT("goto_map")))
        { if (!KeysSubset(AO,{TEXT("action"),TEXT("path"),TEXT("resume_scene"),TEXT("resume_index")}) || !A.bHasPath || !Same(A.Path,TEXT("res://scenes/maps/verdan_market.tscn"))) return false; }
        else if (Same(A.Action,TEXT("goto_scene")))
        { if (!KeysSubset(AO,{TEXT("action"),TEXT("id"),TEXT("start_index")}) || !A.bHasId || !Same(A.Id,Seq)) return false; }
        else if (!Same(A.Action,TEXT("end")) || !Keys(AO,{TEXT("action")})) return false;
        if ((A.bHasResumeIndex && !A.bHasResumeScene) || (A.bHasResumeScene && !Same(A.ResumeScene,Seq)) || (A.bHasStartIndex && A.StartIndex>=Count) || (A.bHasResumeIndex && A.ResumeIndex>=Count)) return false;
    }
    return true;
}
template<class D,class Rows> bool Read(const FString& Path,FMemoriaNarrativeImportMetadata& M,D& Def,Rows& Items,bool V,FString& Error,bool Verify)
{
    auto Fail=[&](const TCHAR* S){Error=S; return false;}; TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes,*Path) || Bytes.Num()>4*1024*1024) return Fail(TEXT("Cannot read bounded IR"));
    FUTF8ToTCHAR Decoded(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()),Bytes.Num()); FString Text(Decoded.Length(),Decoded.Get()); FTCHARToUTF8 Back(*Text);
    if (Back.Length()!=Bytes.Num() || FMemory::Memcmp(Back.Get(),Bytes.GetData(),Bytes.Num())) return Fail(TEXT("Invalid UTF-8"));
    Obj O; if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O) || !Same(MemoriaCatalogImport::Canonical(Wrap(O))+TEXT("\n"),Text)) return Fail(TEXT("Noncanonical/duplicate-key JSON"));
    if (!Keys(O,{TEXT("schema_version"),TEXT("content_kind"),TEXT("dialect"),TEXT("extractor_version"),TEXT("source_revision"),TEXT("sources"),TEXT("definition"),TEXT("semantic_sha256")})) return Fail(TEXT("Envelope keys differ"));
    if (!Readint32(O,TEXT("schema_version"),M.SchemaVersion) || M.SchemaVersion!=1 || !ReadFString(O,TEXT("content_kind"),M.ContentKind) || !Same(M.ContentKind,V?TEXT("narrative.vn"):TEXT("narrative.field")) || !ReadFString(O,TEXT("dialect"),M.Dialect) || !Same(M.Dialect,V?TEXT("vn"):TEXT("field")) || !ReadFString(O,TEXT("extractor_version"),M.ExtractorVersion) || !Same(M.ExtractorVersion,TEXT("memoria-narrative/1")) || !ReadFString(O,TEXT("source_revision"),M.SourceRevision) || !Hex(M.SourceRevision,40) || !ReadFString(O,TEXT("semantic_sha256"),M.SemanticSha256) || !Hex(M.SemanticSha256,64)) return Fail(TEXT("Unsupported metadata"));
    M.IrSha256=MemoriaCatalogImport::Sha256(Text);
    const TArray<FString> Paths={V?TEXT("data/vn_scenes/ch2_market_arrival.json"):TEXT("data/chapter2_dialogue.json"),TEXT("scripts/systems/dialogue_manager.gd"),TEXT("scripts/systems/scene_flow.gd"),TEXT("scripts/ui/vn_scene.gd"),TEXT("scenes/main/vn_host.gd"),TEXT("scenes/maps/verdan_market.gd"),TEXT("scripts/systems/memory_manager.gd"),TEXT("scripts/utils/journey_oath.gd"),TEXT("scripts/core/game_manager.gd")};
    const TArray<Val>* Sources=nullptr; if (!O->TryGetArrayField(TEXT("sources"),Sources) || Sources->Num()!=Paths.Num()) return Fail(TEXT("Source inventory differs")); M.Sources.Reset();
    for (int32 I=0;I<Paths.Num();++I)
    {
        Obj S=(*Sources)[I]->Type==EJson::Object?(*Sources)[I]->AsObject():nullptr; FMemoriaNarrativeSourceHash P;
        if (!Keys(S,{TEXT("path"),TEXT("sha256_utf8_lf")}) || !ReadFString(S,TEXT("path"),P.Path) || !Same(P.Path,Paths[I]) || !ReadFString(S,TEXT("sha256_utf8_lf"),P.Sha256Utf8Lf) || !Hex(P.Sha256Utf8Lf,64)) return Fail(TEXT("Invalid source provenance"));
        if (Verify) { FString S0; if (!FFileHelper::LoadFileToString(S0,*(FPaths::ProjectDir()/TEXT("../../")/P.Path))) return Fail(TEXT("Source missing")); S0.ReplaceInline(TEXT("\r\n"),TEXT("\n")); if (!Same(MemoriaCatalogImport::Sha256(S0),P.Sha256Utf8Lf)) return Fail(TEXT("Source hash differs")); }
        M.Sources.Add(P);
    }
    auto D0=Object(O,TEXT("definition"));
    if (!Keys(D0,{TEXT("id"),TEXT("index_mapping_version"),TEXT("metadata"),V?TEXT("steps"):TEXT("rows")}) || !ReadFString(D0,TEXT("id"),Def.Id) || !(V?Same(Def.Id,TEXT("ch2_market_arrival")):(FieldCohort(Def.Id)!=nullptr)) || !Readint32(D0,TEXT("index_mapping_version"),Def.IndexMappingVersion) || Def.IndexMappingVersion!=1) return Fail(TEXT("Invalid definition/index map"));
    auto Meta=Object(D0,TEXT("metadata"));
    if (!(V?Keys(Meta,{TEXT("title"),TEXT("title_ko"),TEXT("chapter"),TEXT("bgm")}):Keys(Meta,{TEXT("title"),TEXT("title_ko"),TEXT("chapter")})) || !ReadMetadata(Meta,Def.Metadata) || Def.Metadata.Chapter!=2) return Fail(TEXT("Invalid metadata"));
    const TArray<Val>* Entries=nullptr; if (!D0->TryGetArrayField(V?TEXT("steps"):TEXT("rows"),Entries) || Entries->Num()!=(V?13:FieldCohort(Def.Id)->Count)) return Fail(TEXT("Bounded record count differs")); Items.Reset();
    for (int32 I=0;I<Entries->Num();++I)
    {
        Obj R0=(*Entries)[I]->Type==EJson::Object?(*Entries)[I]->AsObject():nullptr; auto& R=Items.AddDefaulted_GetRef();
        if (!Keys(R0,{TEXT("id"),TEXT("original_index"),TEXT("provenance"),TEXT("effect_phase"),TEXT("text"),TEXT("presentation"),TEXT("gate"),TEXT("effects"),TEXT("action"),TEXT("jump"),TEXT("storage_index"),TEXT("choices_present"),TEXT("choices")}) || !ReadRecord(R0,R,M,Def.Id,I,-1,Entries->Num()) || !Readint32(R0,TEXT("storage_index"),R.StorageIndex) || R.StorageIndex!=I || !Readbool(R0,TEXT("choices_present"),R.bChoicesPresent)) return Fail(TEXT("Invalid row/index/phase/semantic fields"));
        const TArray<Val>* Choices=nullptr; if (!R0->TryGetArrayField(TEXT("choices"),Choices) || (!R.bChoicesPresent && !Choices->IsEmpty())) return Fail(TEXT("Choice representation differs"));
        for (int32 J=0;J<Choices->Num();++J)
        {
            auto C0=(*Choices)[J]->Type==EJson::Object?(*Choices)[J]->AsObject():nullptr; auto& C=R.Choices.AddDefaulted_GetRef();
            if (!Keys(C0,{TEXT("id"),TEXT("original_index"),TEXT("provenance"),TEXT("effect_phase"),TEXT("text"),TEXT("presentation"),TEXT("gate"),TEXT("effects"),TEXT("action"),TEXT("jump")}) || !ReadRecord(C0,C,M,Def.Id,I,J,Entries->Num())) return Fail(TEXT("Invalid choice/order/index"));
        }
    }
    if (!Same(Hash(Semantic(Def,Items,V)),M.SemanticSha256)) return Fail(TEXT("Semantic hash differs"));
    if (Verify && !AttestSource(M,Def,Items,V)) return Fail(TEXT("Typed payload differs from authored source; synthetic/modified IR cannot enter production"));
    return true;
}
template<class A> bool ImportAsset(const FString& Path,bool V,bool Check,Obj& Report,FString& Error)
{
    TStrongObjectPtr<A> Candidate(NewObject<A>()); if (!ReadIr(Path,*Candidate,Error)) return false;
    const auto P=Package(V,Candidate->Definition.Id), OP=ObjectPath(V,Candidate->Definition.Id); auto* Existing=FPackageName::DoesPackageExist(P)?LoadObject<A>(nullptr,*OP,nullptr,LOAD_NoWarn|LOAD_Quiet):nullptr;
    Report=MakeShared<FJsonObject>(); Report->SetStringField(TEXT("asset"),OP); Report->SetStringField(TEXT("semantic_sha256"),Candidate->ImportMetadata.SemanticSha256); Report->SetStringField(TEXT("ir_sha256"),Candidate->ImportMetadata.IrSha256);
    if (Existing)
    {
        if (!Same(Fingerprint(*Existing),Existing->ImportMetadata.SemanticSha256)) { Error=TEXT("Generated asset edited; review required"); return false; }
        bool Equal=true; for (TFieldIterator<FProperty> F(A::StaticClass(),EFieldIterationFlags::None);F;++F) Equal &= F->Identical_InContainer(Existing,Candidate.Get());
        if (Equal) { Report->SetStringField(TEXT("result"),TEXT("UNCHANGED")); Report->SetBoolField(TEXT("saved"),false); return true; }
    }
    if (Check) { Error=TEXT("Check-only asset absent/different"); return false; }
    auto File=FPackageName::LongPackageNameToFilename(P,FPackageName::GetAssetPackageExtension());
    if (!Existing && FPaths::FileExists(File)) { Error=TEXT("Wrong-type/unreadable package; no overwrite"); return false; }
    auto* Outer=Existing?Existing->GetOutermost():CreatePackage(*P); IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
    auto* Asset=Existing?Existing:NewObject<A>(Outer,*FPackageName::GetLongPackageAssetName(P),RF_Public|RF_Standalone);
    for (TFieldIterator<FProperty> F(A::StaticClass(),EFieldIterationFlags::None);F;++F) F->CopyCompleteValue_InContainer(Asset,Candidate.Get());
    if (!Existing) FAssetRegistryModule::AssetCreated(Asset); Asset->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    if (!UPackage::SavePackage(Outer,Asset,*File,Args)) { Error=TEXT("Package save failed"); return false; }
    Report->SetStringField(TEXT("result"),Existing?TEXT("UPDATED"):TEXT("CREATED")); Report->SetBoolField(TEXT("saved"),true); return true;
}
}
FString Package(bool V,const FString& Sequence)
{
    if (V) return TEXT("/Game/Memoria/Generated/Narrative/DA_VN_Ch2MarketArrival");
    const auto* C=FieldCohort(Sequence.IsEmpty()?FString(TEXT("verdan_arrival")):Sequence);
    return C?FString(TEXT("/Game/Memoria/Generated/Narrative/"))+C->Asset:FString();
}
FString ObjectPath(bool V,const FString& Sequence) { auto P=Package(V,Sequence); return P+TEXT(".")+FPackageName::GetLongPackageAssetName(P); }
FString Fingerprint(const UMemoriaFieldAsset& A) { return Hash(Semantic(A.Definition,A.Definition.Rows,false)); }
FString Fingerprint(const UMemoriaVNAsset& A) { return Hash(Semantic(A.Definition,A.Definition.Steps,true)); }
bool ReadIr(const FString& P,UMemoriaFieldAsset& A,FString& E,bool V) { return Read(P,A.ImportMetadata,A.Definition,A.Definition.Rows,false,E,V); }
bool ReadIr(const FString& P,UMemoriaVNAsset& A,FString& E,bool V) { return Read(P,A.ImportMetadata,A.Definition,A.Definition.Steps,true,E,V); }
bool Import(const FString& P,bool V,bool C,Obj& R,FString& E) { return V?ImportAsset<UMemoriaVNAsset>(P,V,C,R,E):ImportAsset<UMemoriaFieldAsset>(P,V,C,R,E); }
}
