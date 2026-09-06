# Authored Unreal content

No fabricated `.uasset` or `.umap` files are checked in. Engine 5.7 must create and
validate binary content. The C++ foundation uses no missing game asset references.
Create the first test map in Phase 1B; no production chapter or main-menu map is
configured yet. Enabling Paper2D does not introduce 3D character gameplay.

Future generated packages live under `/Game/Memoria/Generated/`; authored maps/UI
under `/Game/Memoria/Maps/` and `/Game/Memoria/UI/`. Original Godot images stay in
the repository's existing `assets/` directory. Refer to the migration data contract.
