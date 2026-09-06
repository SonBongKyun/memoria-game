# Foundation validation content

`Tests/Foundation` contains actual UE 5.8.2 editor-authored packages: an isolated map, Enhanced Input actions/contexts, controlled texture/sprite and a UMG Widget Blueprint. They are produced by `MemoriaFoundationAssetsCommandlet` and validated by the `Memoria.Foundation.MapInputAndModal` PIE test. This is test content, not a campaign slice or production-art import.

`Memoria/Generated/Memory/DA_StartingMemoryCatalog.uasset` is the Phase 1C production typed catalog generated from the real Godot starting-memory source. Reimport it through `Unreal/Tools/import_starting_memory.py`; do not hand-edit generated content. See the migration IR README and Phase 1C report for provenance and acceptance evidence.
