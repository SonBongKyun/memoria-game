# Player-memory source oracle, schema 1

The expected results are **executed Godot 4.6.2 outputs**, not a second implementation of its rules in Python. The 51 cases contain 95 commands and 109 synchronous event observations. Each observation includes owned order, state bits, erosion, graph, history order, passives, guards, loan, extraction, preservation fields, carry and availability queries. Overload comparisons allow `1e-9` for Godot JSON decimal formatting; all integer values, IDs, event order and state tokens compare exactly.

| File | Role |
| --- | --- |
| `player_memory_inputs.json` | Hand-authored deterministic command vectors; authored by `Unreal/Tools/create_memory_inputs.py` |
| `player_memory_expected.json` | Actual Godot initial, event-time and post-command snapshots |
| `player_memory_provenance.json` | Godot version, reference HEAD, exact source/input/output SHA-256 values |
| `Unreal/Tests/Generated/MemoryParityFixtures.h` | Mechanical C++ representation of those inputs/outputs; generated, not edited by hand |

The source `memory_manager.gd` and `journey_oath.gd` are copied byte-for-byte into a temporary project under `Unreal/Memoria/Intermediate/GodotOracle`. The fixture memory instance stays outside the SceneTree. The real JourneyOath helper executes against a small flag/party/chapter stub. Audio, toast, log and statistics adapters are inert. This avoids changing memory rules or entering production save/profile paths. All subprocess app-data roots are redirected as well.

The oracle certifies **player-memory domain behavior only**. It does not certify GameManager story reactions, battle skills, shop payments/oath breaking, milestone presentation, achievements, full save import, or UI. Silent burns still emit the shared burn event; “silent” does not mean no event. Calling a residue query twice proves the domain query is nondestructive; it does not port the battle residue skill.

Coverage: all five raw grades and weights, display/storage separation, chapter capacity lower/upper bounds, normal/silent burns with and without Elia, preexisting residue bits, faded opt-in, collateral rejection, insertion and history order, effective power, integer truncation, zero erosion counts, overload cap, separate chapter argument/context, core immunity, guard consumption, Still Hands versus cascade behavior, NPC/prefix connection order, intermediate fading, thresholds 5/10/20/30/50, extracted-count subtraction, false passive key presence, case-sensitive and Korean IDs, preservation-state retention and acquisition-before-graph-refresh timing.

Run from the migration worktree (PowerShell):

```powershell
python Unreal/Tools/create_memory_inputs.py
python Unreal/Tools/export_memory_oracle.py --godot '<Godot-4.6.2-console.exe>' --reference '<reference-Game-root>'
python Unreal/Tools/generate_memory_test_header.py
python Unreal/Tools/validate_native_memory.py
python Unreal/Tools/validate_ue57.py --engine-root '<UE_5.7-root>' --build-and-test
```

Only regenerate expected output when the input vectors or reference revision intentionally change. The converter checks provenance hashes and refuses stale input/output combinations; `--check` checks the generated header without writing it. Keep the reference immutable. Never alter an expected result to fit the C++ implementation. No runtime Unreal dependency on Godot or Python is introduced.
