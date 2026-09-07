# Malet source oracle and offline fixtures

`contract_inputs.v1.json` records seven source-derived NPC configurations/cases.
`contract_expected.v1.json` is executable Godot output, not handwritten dialogue.
Recheck with `export_malet_oracle.py --check` and a fresh evidence directory.
The harness runs original PerceptionFilter and verbatim NPC interact/callback
methods; only facing/presentation and execution beyond the recorded fallback
target are deferred. Paid case executes source VN and Verdan guard.

`modified_semantic.field.v1.json` is a transient mutation probe.
`reject_*.field.v1.json` checks group position, row count and unknown group.
Recheck with `malet_test_fixtures.py --check`. These are never saved UE assets.
Only three authored reaction rows are imported by the production pipeline.
