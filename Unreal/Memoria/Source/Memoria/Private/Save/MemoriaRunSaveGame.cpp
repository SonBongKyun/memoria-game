#include "Save/MemoriaRunSaveGame.h"

bool UMemoriaRunSaveGame::ValidateHeader(FString& OutError) const
{
    if (SchemaVersion != CurrentSchemaVersion) { OutError = TEXT("Unsupported Unreal save schema"); return false; }
    if (!IsSupportedSlot(Slot)) { OutError = TEXT("Slot must be autosave 0 or manual 1-3"); return false; }
    if (!Run.IsValid()) { OutError = TEXT("Invalid run identity or typed run entries"); return false; }
    if (ContentRevision.IsEmpty() || !ContentRevision.Equals(Run.ContentRevision, ESearchCase::CaseSensitive))
    { OutError = TEXT("Content revision mismatch"); return false; }
    if (SceneFlow.SchemaVersion != 1 || SceneFlow.IndexMappingVersion != 1 || FieldReturn.CoordinateVersion != 1)
    { OutError = TEXT("Unsupported continuation or coordinate schema"); return false; }
    OutError.Reset();
    return true;
}
