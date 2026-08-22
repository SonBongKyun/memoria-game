# MEMORIA — 개발 세션 로그

---

## S257 - 2026-08-22 (Memory Gameplay v0.3 Sable combined memory state)

### 기준점
- S256 Malet -> Sable downstream ripple을 공식 Memory World Engine suite 12/12, Story QoL/Combat, VN/한국어 validation으로 재검토한 뒤 `b33c13c feat(world): add malet sable memory ripple`로 로컬 체크포인트 커밋했다. 푸시는 하지 않았다.

### 플레이 경험
- Chapter 7 Seam Outskirts의 기존 Sable Cleaner 고백을 선택 기억 조작 지점으로 연결했다. 관리국이 아이에게서 부모의 기억을 지우라는 명령을 내렸다는 사실과, 그 명령의 구체적 대상인 아이와 부모에 대한 Sable의 기억을 분리했다.
- 새 상태는 `fact.authority.child_memory_erasure_order`와 `memory.sable.child_memory_erasure_target`이다. memory를 remove해도 order knowledge는 유지된다.
- 기존 `memory.malet.bl07_request_source`와 Sable memory의 active/removed 조합을 함께 읽어 A/B/C/D 네 가지 반응을 만든다. Sable 재대화에서 Cleaner 대상 질문과 Malet 출처 질문이 각 memory 상태에 따라 교체된다.
- 원고에 있지만 맵에 연결되지 않았던 12인 추모비 `outskirts_monument`를 비핵심 optional story trigger로 연결했다. 추모비의 Sable 후속 반응도 두 memory 조합에 따라 네 가지로 달라진다.

### 상태와 보호 경계
- `npc.sable`을 actor catalog에 추가했다. Sable 상태는 `ch7_sable_truth`가 성립한 첫 필요 시점에 MemoryEngine API로만 seed되며, 기존 active/removed/restored record와 explicit false knowledge를 덮어쓰지 않는다.
- 조작은 DialogueManager의 기존 `remove_world_memory` / `restore_world_memory` choice 계약을 재사용한다. WorldState dictionary, story_flags, MemoryManager를 직접 쓰지 않는다.
- 기존 `outskirts_arrival -> sable_truth -> sable_trial -> trial_complete`과 Chapter 7 exit, 엔딩, 필수 아이템, Quest/SceneFlow는 변경하지 않았다.

### 검증
- Godot 4.6.2 headless editor import exit 0, fatal scan 0.
- 실제 `seam_outskirts.tscn` Sable NPC `interact()`와 `chapter7_dialogue.json` 추모비를 실행하는 live smoke에서 A -> C -> save/load C -> D -> B -> A 조합, 선택지 교체, 추모비 반응을 검증했다.
- `SABLE_MEMORY_GAMEPLAY_SMOKE_PASS`: combinations 4, round trip 1, deterministic mutation events 8개 각 1회, no-op event 0, story_flags delta 0, MemoryManager delta 0, production slots 0.
- `MEMORY_WORLD_ENGINE_SUITE_PASS cases=13 fatal_scan=enabled save_isolation=guarded export_catalog=verified`. export PCK에서 actors.json과 actor 3명을 재검증했다.
- `STORY_QOL_SMOKE_PASS`, `STORY_COMBAT_SMOKE_PASS`, VN 20 files/504 steps/0 errors/0 warnings, 한국어 31 files/1,650 fields/0 errors 통과.
- sandbox save -> WorldState reset -> load 후 combination C가 유지됐고 load event replay는 0이다. StoryLog/read registry fingerprint는 불변이다.
- headless import가 재생성한 기존 tool `.uid` 20개는 working tree에서 제외했다. 새 Sable smoke script UID만 작업 소유 파일로 유지했다.
- v0.3 변경은 검토를 위해 커밋하지 않고 working tree에 남겼다.

## S256 - 2026-08-22 (Memory Gameplay v0.2 downstream Sable consequence)

### 기준점
- S255 Malet live integration을 공식 Memory World Engine suite 12/12, Story QoL/Combat, VN validation, staged diff check로 재검토한 뒤 `2c59212 feat(world): integrate malet memory gameplay` 체크포인트로 로컬 커밋했다. 푸시는 하지 않았다.

### 플레이 가능한 downstream 흐름
- Chapter 3 이후 베르단 시장의 Malet 재대화에서 `memory.malet.bl07_request_source`를 제거하거나 복원하는 기존 실제 플레이어 행동을 그대로 사용한다.
- Chapter 6 The Seam에서 Sable의 기존 첫 대화 `sable_talk`를 마친 뒤 다시 말을 걸면 `sable_malet_route_followup`이 열린다. 메인 도착/브리핑/BL-07 progression은 변경하지 않았다.
- active 상태에서는 Malet의 쪽지가 Arrel을 요청자로 지목하고, Sable에게 쪽지 전체를 읽어 달라는 선택지와 기존 원고의 `북동쪽 -> 절벽 -> 오래된 감시탑 -> 색을 따라감` 경로 정보가 열린다.
- removed 상태에서는 `fact.arrel.seeks_bl07` knowledge가 남아 누군가 BL-07을 찾았다는 사실과 경로의 실재성은 유지되지만, 발신자 표식이 비어 완전한 경로 선택지 대신 불완전한 쪽지에 남은 내용만 물을 수 있다.
- restored 상태에서는 source mark가 돌아왔다는 전용 대사와 전체 경로 선택지가 복구된다. 새 fact/memory/actor나 story flag는 만들지 않았다.

### 변경 경계
- `the_seam.tscn`의 Sable에 이미 존재하는 범용 NPC `repeat_dialogue_key`만 지정했다. 기존 `dialogue_key=sable_talk`, `seam_welcome`, `sable_briefing`은 그대로다.
- downstream dialogue는 DialogueConditionSystem으로 WorldState를 읽기만 하며 mutation, polling, event consumer, 새 UI/framework를 추가하지 않는다.
- MemoryEngine, MemoryManager, Quest, SceneFlow, WorldRewriteDirector, ActorRegistry, SaveManager production code는 수정하지 않았다.
- 별도 presentation framework는 추가하지 않았다. Malet remove/restore 선택 직후의 authored 텍스트와 기존 DialogueBox 선택 SFX를 player-facing feedback으로 유지했다.

### 검증
- Godot 4.6.2 headless editor import exit 0. 새 Parse/SCRIPT ERROR 없음.
- 확장한 실제 scene-node smoke PASS: `verdan_market.tscn` Malet과 `the_seam.tscn` Sable을 instantiate하고 실제 NPC `interact()`/DialogueManager pipeline으로 active, removed, loaded-removed, restored branch와 선택지를 실행했다.
- `MEMORY_WORLD_ENGINE_SUITE_PASS cases=12 fatal_scan=enabled save_isolation=guarded export_catalog=verified`.
- sandbox save -> WorldState reset -> load 뒤 removed Sable branch/제한 선택지가 유지되고, load event replay는 0, restore event는 정확히 1회였다.
- knowledge 유지, 총 mutation event 4회 각 1회, no-op event 0, story_flags delta 0, MemoryManager delta 0, StoryLog/read_lines fingerprint 불변, production slots 0을 검증했다.
- `STORY_QOL_SMOKE_PASS`, `STORY_COMBAT_SMOKE_PASS`, VN 20 files/504 steps/0 errors/0 warnings, 한국어 localization 31 files/1,615 fields/0 errors 통과.
- `git diff --check` 통과. v0.2 변경은 검토를 위해 working tree에 남겼다.

### 실패와 기존 경고
- 첫 확장 smoke는 Sable 검증이 DialogueManager의 현재 선택지 context를 교체해 뒤이은 Malet restore 선택지를 찾지 못하면서 exit 1로 실패했다. Sable read-only 검증 뒤 Malet removed interaction을 다시 여는 실제 UI 순서로 수정했고 재실행과 공식 suite가 통과했다.
- headless editor import가 이번 기능과 무관한 기존 tool script `.uid` 20개를 cache에서 재생성했다. 모두 working tree에서 제외했다.
- 기존 ShaderV duplicate UID와 ObjectDB/resource cleanup 경고는 유지되지만 fatal scan 대상 오류는 없었다.

## S255 - 2026-08-22 (Malet Memory World live gameplay integration)

### 플레이 가능한 흐름
- Chapter 2 말렛 거래가 완료되면 `npc.malet`에게 `fact.arrel.seeks_bl07`과 `memory.malet.bl07_request_source`를 최초 1회 seed한다. 기존 save의 active/removed/restored record와 explicit false knowledge는 덮어쓰지 않는다.
- Chapter 3 이후 베르단 시장으로 돌아와 말렛에게 다시 말을 걸면 기존 한 줄 repeat 대신 실제 `chapter2_dialogue.json`의 structured dialogue가 열린다. 첫 `malet_encounter`와 거래/보상/전환 플래그는 그대로 유지한다.
- 출처 기억이 active이면 말렛은 BL-07 요청자가 아렐임을 기억하며 `그 요청에서 무엇을 알아냈는지 묻는다` 선택지로 추가 정보를 제공한다.
- 플레이어가 `말렛의 기억에서 출처 표식을 태운다`를 고르면 `MemoryEngine.remove_memory()`가 tombstone을 남긴다. 말렛은 BL-07을 찾는 사람이 있다는 knowledge는 유지하지만 요청자의 얼굴을 잃고, 출처 상세 질문이 사라진다.
- `잔존에서 출처 표식을 다시 엮는다`를 고르면 `MemoryEngine.restore_memory()`가 가장 최근 제거 직전 record를 복원한다. restored 전용 대사와 출처 상세 질문이 돌아온다.

### 연결과 경계
- 범용 NPC는 `repeat_dialogue_key`가 명시된 경우에만 authored repeat dialogue를 사용한다. 다른 NPC의 기존 repeat line 동작은 그대로다.
- DialogueManager의 새 `remove_world_memory` / `restore_world_memory` 선택지 필드는 MemoryEngine 공개 API만 호출한다. WorldState dictionary와 story_flags를 직접 쓰지 않는다.
- 실제 actor identity는 이미 `data/world_state/actors.json`에 등록된 `npc.malet`을 그대로 사용한다. 표시 이름 `Malet`은 identity로 쓰지 않는다.
- SaveManager의 기존 `world_state` payload를 그대로 사용하며 새 save schema나 migration은 추가하지 않았다.

### 검증
- Godot 4.6.2 headless import: exit 0, 새 Parse/SCRIPT ERROR 없음.
- 실제 `verdan_market.tscn`에서 Malet 인스턴스를 로드한 live smoke PASS: active/removed/restored, source-detail choice toggle, lifecycle seed no-op, events once, sandbox save/reset/load, story_flags delta 0, MemoryManager delta 0, production slots 0.
- 공식 `MEMORY_WORLD_ENGINE_SUITE_PASS cases=12`: export actor catalog, production/outside-temp guard, migration, event schema/consumer, 이전 vertical slice, live integration 모두 fatal scan과 함께 통과.
- 기존 `STORY_QOL_SMOKE_PASS`, `STORY_COMBAT_SMOKE_PASS` 통과. VN validation 20 files/504 steps, 전체 data JSON 44개 parse, `git diff --check` 통과.
- 테스트는 StoryLog persistence를 종료까지 suppress하고 `read_lines.json` fingerprint 불변을 검증했다. 이전 세션에서 이미 182개로 보이던 registry count는 이번 테스트에서 증가하지 않았다.

### 경고
- Godot editor import에서 기존 ShaderV duplicate UID 경고가 출력됐다. headless 종료 시 기존 ObjectDB/resource cleanup 경고도 남지만 exit code, PASS marker, fatal scan은 정상이다.
- 전체 베르단 맵 `_ready()` 실행은 AchievementManager의 실제 사용자 파일 쓰기 가능성이 있어 자동 테스트에서 피했다. 대신 실제 map PackedScene의 Malet node, map seed 함수, production dialogue file과 DialogueManager interaction을 함께 실행했다.
- 변경은 검토를 위해 커밋하지 않고 working tree에 남겼다.

## S254 - 2026-08-22 (Malet Memory World dialogue vertical slice)

### 기준점
- S253 infrastructure gate 변경 14개 파일을 export/runtime catalog 검증과 공식 suite 10/10으로 다시 검토한 뒤 `f17646c feat(world): finalize engine gate` 체크포인트로 로컬 커밋했다. 푸시는 하지 않았다.

### DialogueManager opt-in 연결
- 기존 `_condition_met` 경로에 Dictionary-valued `condition`이 있을 때만 DialogueConditionSystem을 추가 평가한다. 기존 `requires_*`와 `requires_memory + burned_text` 데이터는 같은 코드 경로와 결과를 유지한다.
- Memory condition에 optional `restored` Boolean을 추가했다. 새 상태를 만들지 않고 기존 `restored_revision > 0` audit metadata만 읽어 최초 active와 restore 이후 active를 구분한다.
- 실제 chapter/VN JSON은 수정하거나 migration하지 않았다. repository 검색상 structured `condition`은 개발용 Malet JSON에만 존재한다.

### 개발 전용 Malet slice
- progression에서 참조되지 않는 `malet_memory_world_development.tscn`과 전용 JSON을 추가했다. Reset, Remove, Restore, Talk, sandbox Save, sandbox Reload 버튼을 제공한다.
- 초기 상태는 `npc.malet`, `fact.veil.exists=true`, active `memory.malet.veil_revelation_source`, source `player.arrel`이다.
- 실제 DialogueManager/DialogueBox pipeline은 active, removed, restored 세 문구 중 정확히 하나를 표시한다. removed에서도 knowledge는 유지된다.
- 모든 runtime mutation은 MemoryEngine API를 사용한다. 명시적인 reset만 WorldState의 public reset API를 사용하며 dictionary/internal store를 직접 변경하지 않는다.

### Event와 persistence
- Malet 전용 read-only consumer는 schema v1을 먼저 검증하고 actor/target을 좁힌 뒤 `memory.removed`, `memory.restored`, `knowledge.learned`, `knowledge.forgotten`에만 반응한다. 매-frame polling이나 state write는 없다.
- SaveManager의 주입된 smoke root에 일반 save payload를 쓴다. `reload_test_world_state`는 같은 guarded slot에서 WorldState만 복원하며 GameManager, MemoryManager, SceneFlow, scene 전환을 건드리지 않는다.
- save -> reset -> reload는 removed tombstone과 동일 dialogue branch를 복원하고 event replay는 0이다.

### 검증
- Godot 4.6.2 headless editor import: exit 0, Parse/SCRIPT ERROR 없음.
- 개발 scene 자체 headless load: exit 0, isolated test root 활성, development JSON 1 dialogue 로드.
- `MALET_MEMORY_WORLD_VERTICAL_SLICE_SMOKE_PASS`: active/removed/restored, events once 5, no-op events 0, round trip 1, consumer refreshes 4, direct state writes 0, production slots 0.
- `MEMORY_WORLD_ENGINE_SUITE_PASS cases=11 fatal_scan=enabled save_isolation=guarded export_catalog=verified`.
- 기존 `STORY_QOL_SMOKE_PASS`, `STORY_COMBAT_SMOKE_PASS`는 exit/marker/full fatal scan으로 통과했다.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- save migration fixtures와 production/outside-temp guard는 공식 suite에서 재통과했다. `git diff --check` 통과.

### 발견한 부수 효과와 보호
- 최초 vertical smoke에서 실제 DialogueBox가 개발 문구를 StoryLog에 전달해 read registry count가 179에서 182로 증가했다. production save slot은 아니며 실제 progression에서 도달할 수 없는 세 개발 문구지만, 실제 user persistent file이므로 자동 삭제하지 않았다.
- 이후 개발 scene은 StoryLog entries/read keys/dirty/persistence 상태를 snapshot/restore한다. 재실행과 개발 scene load에서 count 182가 더 증가하지 않았고 StoryLog persistence mode도 복원됨을 smoke로 검증했다.
- 실제 Story/NPC/VN/Quest/SceneFlow/MemoryManager/WorldRewriteDirector 콘텐츠는 수정하지 않았다.

## S253 - 2026-08-22 (Memory World Engine final infrastructure gate)

### 기준점
- S252의 save 격리/Event schema/actor catalog 변경 22개 파일을 공식 suite 9/9와 staged diff check로 다시 검토한 뒤 `2ba20dd feat(world): gate engine infrastructure` 체크포인트로 로컬 커밋했다. 푸시는 하지 않았다.

### Event schema v1
- committed event envelope에 `schema_version = 1`을 추가했다. 공통 필드는 `schema_version`, `event_id`, `event_type`, `event_sequence`, `revision`, `actor_id`, `target_id`, `payload`의 정확한 8개다.
- v1 validator는 누락/추가 필드와 지원하지 않는 미래 version을 거부한다. timestamp/random/nonce 금지, sequence 기반 event ID, mutation당 1 event, no-op 0 event, load replay 0 계약은 유지한다.
- 향후 schema 변화는 version 증가와 병렬 validator 또는 명시적 adapter가 필요하며, v1 payload를 새 의미로 암묵 재해석하지 않는다고 문서화했다.

### Actor catalog export gate
- Windows Desktop (Demo) preset은 기존 `export_filter="all_resources"`를 유지하고, runtime string path로 읽는 `data/world_state/actors.json`을 `include_filter`에 명시했다.
- 새 export verifier는 OS temp 하위의 GUID directory에 PCK를 만들고, export된 PCK를 `--smoke-test`로 실행해 `res://data/world_state/actors.json`과 정확히 두 actor를 실제 runtime에서 읽는다. 정규화한 cleanup target이 OS temp 직계 하위가 아니면 삭제를 거부한다.
- pack construction은 exit code와 생성된 PCK의 존재/크기를 검사하고, export된 runtime에는 전체 fatal scan을 적용한다. 기존 plugin/resource의 pack-time engine diagnostics는 별도 error count로 노출한다.

### 공식 smoke entrypoint와 read-only probe
- `run_memory_world_engine_smoke_suite.ps1`을 Memory World Engine의 공식 단일 진입점으로 확정했다. 모든 runtime case에 `--smoke-test`, SaveManager production guard, temp save sandbox, timeout/exit/marker/fatal scan이 적용되며 export/runtime catalog 검증도 선행한다.
- 개발 전용 `world_event_consumer_probe.gd`를 추가했다. 오토로드나 gameplay consumer가 아니며 committed event의 복사본만 받아 schema, sequence, actor/target identity와 payload를 읽고 진단을 기록한다.
- probe smoke는 5개 event type을 각 1회 수신하고 payload 10개 필드를 읽은 뒤, probe 없는 동일 mutation 결과와 WorldState가 정확히 같고 MemoryManager/story_flags도 불변임을 검증한다.

### 검증
- Godot 4.6.2 headless editor import: exit 0, Parse/SCRIPT ERROR 없음.
- 임시 Windows PCK 1,185,303,672 bytes export 후 catalog runtime load 통과. 최종 export의 pack-time `ERROR:` count는 0, runtime fatal scan 활성 상태였다.
- `MEMORY_WORLD_ENGINE_SUITE_PASS cases=10 fatal_scan=enabled save_isolation=guarded export_catalog=verified`.
- `WORLD_EVENT_SCHEMA_SMOKE_PASS`: schema v1, 5종, deterministic IDs, load replay 0.
- `WORLD_EVENT_CONSUMER_PROBE_SMOKE_PASS`: events 5, types once 5, payload fields read 10, state writes 0.
- save migration fixtures, production/outside-temp path guard, isolated crash guard, save/reset/load round-trip, actor catalog/registry 모두 통과했다. production save slot은 읽거나 쓰지 않았다.
- 첫 전체 suite 시도는 잘못 지정한 Godot executable path 때문에 테스트 시작 전에 exit 1이었다. 다음 export 시도는 uncached ShaderV/VFX resource diagnostics가 pack-construction fatal scan과 충돌해 중단됐고, pack 생성 판정과 exported-runtime fatal 판정을 분리한 뒤 최종 suite가 통과했다.
- `git diff --check` 통과. 직접 Godot smoke 종료 시 기존 ObjectDB/resource-in-use cleanup 경고는 계속 발생한다.

### 범위 보호
- 실제 Story/NPC/DialogueManager/SceneFlow/Quest/MemoryManager/WorldRewriteDirector 콘텐츠나 consumer는 수정/연결하지 않았다.

## S252 - 2026-08-22 (Smoke save 완전 격리 + Event/Actor 정적 계약)

### 기준점
- S251 변경 27개 파일을 공통 suite 4/4와 staged diff check로 다시 검토한 뒤 `a41a24f feat(world): strengthen engine contracts` 체크포인트로 로컬 커밋했다. 푸시는 하지 않았다.

### Save 격리
- SaveManager의 production root `user://saves`와 smoke test root를 분리했다. production 실행은 기존 root를 그대로 사용하지만, `--smoke-test` 또는 smoke scene 실행은 usable root 없이 시작하고 autosave도 비활성화한다.
- smoke write는 `user://test_tmp/smoke_saves/<suite>_<pid>` 하위 root를 명시적으로 주입해야 한다. 정규화한 OS 절대 경로가 허용 root 밖이거나 production root이면 target/resolved path를 출력하고 filesystem 접근 전에 exit code 1로 종료한다.
- `smoke_save_sandbox.gd`가 직접 fixture write에도 같은 guard를 적용한다. crash/timeout 후 temp 데이터가 남아도 production slot에는 접근하지 않으며 production 경로를 대상으로 하는 cleanup도 없다.
- `smoke_crash_guards.gd`의 `user://saves/save_3.json` 직접 접근을 제거했다. SaveManager 자체 write/read와 invalid-save transactional load를 모두 주입된 temp slot 3에서만 검증하며 공통 smoke runner를 사용한다.
- PowerShell suite는 47개 `smoke_*.gd` 전체를 정적 검사해 production save literal을 거부하고, 공통 sandbox helper 밖의 직접 FileAccess write가 다시 들어오면 실행 전에 실패한다.

### Event payload schema
- committed event 공통 필드를 `event_id`, `event_type`, `event_sequence`, `revision`, `actor_id`, `target_id`, `payload`로 고정했다.
- `event_id`는 `event_sequence`를 `world.%08d` 형식으로 포맷한 값이며 시간/난수/nonce 및 extra field를 허용하지 않는다. 5개 event type별 payload 필드를 EventBus에서 검증한다.
- 성공 mutation은 정확히 1 event, no-op은 0 event, save/load는 event를 재발행하지 않는 계약을 별도 smoke로 검증했다. 실제 Dialogue/NPC/Quest consumer는 연결하지 않았다.

### Actor catalog
- `player.arrel`과 `npc.malet`을 `data/world_state/actors.json` schema v1로 외부화했다.
- ActorRegistry는 catalog를 임시 dictionary에 전부 검증한 뒤 원자적으로 교체한다. missing/invalid/duplicate/cross-namespace slug collision은 명확한 오류를 남기며 기존 registry를 보존하고 silent fallback하지 않는다.

### 검증
- Godot 4.6.2 headless editor import: exit 0, 새 Parse/SCRIPT ERROR 없음.
- 공통 runner 강제 실패: exit 1, named failure 출력, PASS marker 없음.
- `MEMORY_WORLD_ENGINE_SUITE_PASS cases=9 fatal_scan=enabled save_isolation=guarded`.
  - runner contract, production path expected failure, outside-temp expected failure, isolated crash guards, Memory World Engine, save migration fixtures, actor catalog, actor collision, world event schema 모두 통과.
- save migration fixture 5종과 save/reset/load round-trip 통과. 실제 production save slot은 검증 과정에서 읽거나 쓰지 않았다.
- `git diff --check` 통과.

### 기존 synthetic slot 3
- S251에서 확인한 actual `save_3.json`의 synthetic `crash-guard-smoke` 상태는 이번 세션에서 읽거나 변경하지 않았다. 당시 같은 경로의 `.bak`은 발견되지 않았다.
- 원래 데이터 존재 여부를 확인할 근거가 없으므로 자동 삭제/덮어쓰기는 금지한다. 처리하려면 먼저 파일을 active save 경로 밖으로 보존 복사한 뒤, 사용자 확인을 거쳐 quarantine 또는 휴지통 이동을 별도 작업으로 수행한다.

### 기존 경고
- ShaderV duplicate UID와 headless 종료 ObjectDB/resource cleanup 경고는 기존 환경에서 계속 발생한다.
- Story/NPC/DialogueManager/SceneFlow/Quest/MemoryManager/WorldRewriteDirector 실제 consumer는 수정하거나 연결하지 않았다.

## S251 - 2026-08-22 (Memory World Engine 계약/테스트 인프라 강화)

### 기준점
- S250 MVP 변경 16개 파일을 재검토하고 `MEMORY_WORLD_ENGINE_SMOKE_PASS`와 staged diff check를 다시 통과시킨 뒤 `7bfa3a3 feat(world): add memory world engine MVP`로 로컬 체크포인트 커밋했다. 푸시는 하지 않았다.

### 구현
- 공통 `smoke_test_runner.gd`를 추가했다. suite/test 이름과 실패 원인을 분리해 출력하고, 실패가 하나라도 있으면 PASS marker 없이 process exit code 1로 끝난다.
- `run_memory_world_engine_smoke_suite.ps1`은 각 smoke를 별도 프로세스로 실행해 timeout, exit code, PASS marker, fatal diagnostics를 함께 검증한다.
- `data/test_fixtures/save_migrations`에 legacy 0.3.0, current 0.4.0, WorldState 누락, 손상 payload, 지원하지 않는 schema 5개 JSON fixture를 추가했다. 전부 `res://` read-only이며 실사용 `user://saves/`를 쓰지 않는다.
- SaveManager는 유효한 schema v1 snapshot을 보존하고, 누락/손상/미지원 WorldState는 deterministic defaults로 복구한다. legacy `story_flags`와 기존 `MemoryManager` data는 계속 마이그레이션하지 않는다.
- `ActorRegistry` 오토로드를 추가해 `player.arrel`/`npc.malet`, 표시 이름, ID 문법, exact duplicate, player/npc를 가로지르는 actor slug 충돌을 중앙 관리한다. WorldState는 registry actor의 runtime container만 소유한다.
- `MemoryEngine`에 `learn_fact`, `forget_fact`, `knows_fact`, `restore_memory`를 추가했다. knowledge false record를 삭제하지 않아 explicit forgetting과 missing record를 구분한다.
- restore는 최신 tombstone이 보존한 content/fact/source/created revision을 유지하고, `removed_revision`을 `last_removed_revision`으로 옮긴 뒤 active 상태와 `restored_revision`을 기록한다. 성공 이벤트는 `memory.restored` 정확히 1회이며 재복원은 no-op이다.
- ID, knowledge, restore, save recovery 계약을 `docs/MEMORY_WORLD_ENGINE_CONTRACT.md`에 문서화했다. 실제 스토리, DialogueManager, SceneFlow, Quest, NPC 콘텐츠 소비자는 연결하지 않았다.

### 검증
- Godot 4.6.2 headless editor import: exit 0, 새 Parse/SCRIPT ERROR 없음.
- 공통 runner 강제 실패 probe: 명시적 suite/test/reason 로그, PASS marker 없음, exit code 1.
- `MEMORY_WORLD_ENGINE_SUITE_PASS cases=4 fatal_scan=enabled`.
  - `SMOKE_TEST_RUNNER_CONTRACT_PASS`
  - `MEMORY_WORLD_ENGINE_SMOKE_PASS`: learn/forget, remove/restore, memory/knowledge 독립, 이벤트 단일 발행, save/reset/load 동일 복원.
  - `SAVE_MIGRATION_FIXTURES_SMOKE_PASS`: fixtures=5, user slots touched=0, unsupported direct import non-mutation.
  - `ACTOR_REGISTRY_SMOKE_PASS`: defaults=2, exact/cross-namespace slug collisions, unknown access.
- fixture JSON 5/5 PowerShell parse 통과.
- 기존 `STORY_QOL_SMOKE_PASS`, `QUEST_ILLUSTRATION_SMOKE_PASS` 통과.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- `git diff --check` 통과.

### 남은 기존 경고/오류
- `smoke_burn_directive_stabilization`은 marker를 출력하지만 `Lambda capture at index 1 was freed`를 11회 출력한다. S250과 동일하게 재현되며 새 suite fatal pattern은 이 문구를 실패로 처리한다.
- 기존 `smoke_crash_guards.gd`는 `CRASH_GUARDS_SMOKE_PASS`를 출력했지만 실제 `user://saves/save_3.json`을 직접 덮어쓰고 원본을 복구하지 않는 것을 실행 후 확인했다. 따라서 이 실행 결과는 무효 처리했다. 현재 해당 파일은 synthetic `crash-guard-smoke` 데이터이며 같은 경로의 `.bak`은 발견되지 않았다. 추가 삭제나 덮어쓰기는 하지 않았다.
- ActorRegistry collision smoke의 duplicate/slug/invalid-ID 경고 3개는 거부 계약을 검증하기 위해 의도적으로 발생한다.
- ShaderV duplicate UID와 headless 종료 ObjectDB/resource cleanup 메시지는 기존 환경 경고로 남아 있다.

## S250 - 2026-08-22 (Memory World Engine MVP 1 — 기억과 지식의 분리 저장)

### ID 계약
- 영속 Actor ID는 `player.<slug>` 또는 `npc.<slug>`, Memory ID는 `memory.<actor_slug>.<memory_slug>`, Fact ID는 `fact.<domain_slug>.<fact_slug>`로 제한했다.
- 모든 slug는 영문 소문자로 시작하고, 소문자 ASCII 문자·숫자와 비어 있지 않은 구간 사이의 단일 `_`만 사용한다. 화면 표시명과 씬 노드명은 영속 ID로 사용하지 않는다.
- 저장소의 실제 주인공 표기가 `Arrel/arrel`이므로 정식 플레이어 ID는 권장안의 `player.arell` 대신 `player.arrel`로 확정했다.
- MVP 테스트 actor/fact/memory는 각각 `npc.malet`, `fact.veil.exists`, `memory.malet.veil_revelation_source`다.

### 구현
- 기존 `MemoryManager`와 분리된 오토로드 `EventBus`, `WorldState`, `MemoryEngine`, `DialogueConditionSystem`을 추가했다.
- `WorldState`는 Malet의 Veil 존재 지식과 actor별 memory/knowledge/relationship/emotion/location/quest/flag 컨테이너를 JSON-safe snapshot으로 소유한다.
- `MemoryEngine`에 `add_memory`, `remove_memory`, `check_memory`를 추가했다. 제거는 레코드를 삭제하지 않고 `status="removed"` tombstone과 제거 revision을 남긴다.
- memory 추가/제거는 저장되는 단조 증가 sequence를 가진 `world_event_committed`를 각각 한 번만 발행한다. 실패하거나 이미 제거된 memory에는 이벤트를 발행하지 않는다.
- `DialogueConditionSystem`은 기존 대화에 연결하지 않은 read-only evaluator로 memory/knowledge 및 `all`/`any`/`not` 조건을 지원한다.
- SaveManager v0.4.0 payload에 `world_state`를 추가했다. 누락된 구버전 세이브는 기존 `story_flags`나 `memory`를 옮기지 않고 기본 WorldState만 보강한다.
- 일반 New Game, Part 2 Aftermath Preview, New Game+ 시작 시 WorldState만 기본값으로 reset한다. 기존 스토리/퀘스트/NPC 분기는 새 시스템을 아직 소비하지 않는다.

### 검증
- 첫 MVP smoke는 JSON 왕복 뒤 memory revision 숫자 타입 정규화가 빠져 동일성 비교에 실패했다. 또한 Godot `assert()`가 실행을 중단하지 않아 잘못된 PASS까지 출력하는 테스트 문제를 확인했다.
- memory record import 타입을 정규화했고, 새 smoke는 실패 누적 시 반드시 종료 코드 1을 반환하도록 고쳤다.
- `MEMORY_WORLD_ENGINE_SMOKE_PASS`: Malet knowledge 유지, memory tombstone, memory=false/knowledge=true 조건, 제거 이벤트 정확히 1회와 재제거 no-op, save/reset/load 완전 복원, legacy save 기본값 보강, 기존 시스템 상태 불변을 확인했다.
- `QUEST_ILLUSTRATION_SMOKE_PASS`, `STORY_QOL_SMOKE_PASS` 통과. 당시 `CRASH_GUARDS_SMOKE_PASS` marker도 확인했으나, S251에서 실사용 slot 3 덮어쓰기가 드러나 해당 결과를 소급 무효 처리했다.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- Godot 4.6.2 headless editor import와 `git diff --check` 통과. ShaderV duplicate UID 및 headless 종료 cleanup 경고는 기존 환경에서 계속 발생한다.
- 기존 `smoke_burn_directive_stabilization`은 기능 marker를 출력하지만 `Lambda capture at index 1 was freed` 오류를 11회 출력한다. 단독 실행에서도 재현되며 이번 변경 범위에서는 수정하지 않았다.

### 범위 보호
- `MemoryManager`, `DialogueManager`, SideQuest/quest 데이터, `WorldRewriteDirector`, 실제 NPC/스토리/대화 데이터는 수정하지 않았다.
- AI/LLM, 기존 `story_flags` 마이그레이션, 실제 대화 조건 연결, quest/world event 소비자는 추가하지 않았다.

## S249 - 2026-08-14 (전투가 시작되면 장소가 지워진다 — 계측은 남기고, 고치지는 못했다)

S241에서 전투 화면의 빈 무대를 밝기 0.102로 쟀지만, 그건 적의 대비를 재기 위한 **바닥**으로만 썼다. 그 바닥 자체가 낮은지는 물은 적이 없었다.

### 계측: 진짜 문제는 어둡다는 게 아니라 장소가 사라진다는 것

UI가 없는 가로 띠(y 0.25~0.58)만 표본해 필드 맵 열 곳과 전투 다섯을 같은 잣대로 쟀다.

| | 밝기 | 구조량 | 범위 |
|---|---|---|---|
| 필드 맵 | 0.216 | 0.096 | 0.108 ~ 0.489 (**4.4배**) |
| 전투 무대 | 0.083 | 0.066 | 0.073 ~ 0.092 (**1.3배**) |

전투는 필드의 39% 밝기다. 그러나 더 중요한 것은 폭이다. 맵은 대낮의 림 숲(0.489)과 BL-07 공허(0.108)가 4.4배 차이로 장소의 정체를 담는데, 전투는 어디서 싸우든 전부 0.08이다. **전투가 시작되면 장소가 지워진다.** S236에서 아홉 맵이 단색 안개에 덮여 있던 것과 같은 유형이다.

### 세 레버를 당겼고 셋 다 실패했다

1. **3D 무대 조명을 지역별로.** 프로필에 `light`를 넣고 앰비언트에 곱했다. 띠 밝기 0.0813 → 0.0825. 거의 무변화.
2. **배경 CG 후퇴량을 지역별로.** S209가 균일하게 0.50으로 눌러 둔 값을 지역 광량에 비례시켰다. 0.0833. 여전히 무변화.
3. **후퇴량을 더 크게.** 0.62까지 올렸다. 0.0872 — 후퇴량을 24% 올려 밝기 5%를 얻었다.

그래서 **무엇이 그 띠를 어둡게 하는지** VN에서 통한 방식으로 직접 쟀다. 전체 화면급 오버레이를 하나씩 꺼 봤더니 **지배적으로 어둡히는 층이 없었다.** 오히려 가장 큰 단일 항(`@ColorRect@601`)은 *밝히는* 층이고(끄면 −0.0246), 배경 CG는 띠 밝기의 12%만 기여한다. 전투 화면은 무엇에 덮여서가 아니라 **애초에 어둡게 구성돼** 있다.

지배 항을 못 찾은 채로 값을 바꾸지 않기로 하고 전부 되돌렸다. 게임 코드에 남은 것은 주석 한 덩어리뿐이다.

### 내가 만들어 낸 트레이드오프를 철회한다

중간에 "장소의 정체성과 전투원 분리가 1:1로 맞바꿔진다"고 결론 내렸다. 후퇴량을 올리자 빈 무대 구조량이 0.026 → 0.062로 뛰고 적이 배경 위로 솟는 비가 2.16배에서 1.05배가 되었다는 근거였다.

**그 근거는 무효다.** 0.026은 S241의 기준선인데, 오늘 모든 변경을 되돌린 상태에서도 그 값은 0.0588이었다. 즉 두 세션 사이 어딘가에서 이 표본이 달라졌고 비교할 수 없는 값이었다. 세션 내부 값으로만 보면 후퇴량 0.50 / 0.55 / 0.62에서 적·무대 비는 1.10 / 1.07 / 1.05로 거의 변하지 않는다. **트레이드오프는 실재하지 않았고 내가 만들어 낸 것이다.**

되돌림 자체는 여전히 옳다. 이유가 "분리를 해쳐서"가 아니라 "밝기를 5%밖에 못 올려서"로 바뀔 뿐이다.

### 남긴 것
- `probe_stage_vs_field` — 필드와 전투를 같은 띠에서 비교한다. 열 맵의 실측 밝기 순서가 여기 남는다.
- `probe_battle_overlays` — 전체 화면급 층을 하나씩 꺼서 기여를 잰다.

다음에 이어갈 사람에게: 지배 항은 오버레이가 아니다. 배경(12%)도 아니다. 남은 후보는 3D 무대 자체의 재질/스톤 색과 전투원 판이 차지하는 면적이다. 그쪽을 발자국으로 분해하는 것이 다음 단계다.

### 검증
- 스모크 38/38. VN 20파일 504스텝 0오류. `git diff --check` 통과.
- 게임 코드 변경 없음(주석만).

### 방법론
- **낡은 기준선과 비교하면 없는 문제를 만든다.** 세션을 넘어선 수치 비교는 그 사이에 무엇이 바뀌었는지 모른 채 하는 비교다. 같은 세션 안에서 얻은 값끼리만 비교해야 했다.
- 세 번 당기고 세 번 실패한 뒤에야 "무엇이 원인인가"를 직접 쟀다. **그 순서가 거꾸로였다.** VN에서 이미 배운 것인데(발자국을 먼저 뽑는다) 이번엔 그럴듯한 가설이 있어서 건너뛰었다.

---

## S248 - 2026-08-14 (한 번도 계측된 적 없는 세 축: 난이도·NG+·장비)

S246~247은 기본 조건(보통 난이도, NG+0, 장비 없음)에서만 쟀다. 그런데 난이도는 적 HP·공격력에 0.7/1.0/1.4를 곱하고, NG+는 회당 +30%를 얹으며, 최고 장비 조합은 공격 +20 / 방어 +15를 준다. 어느 것도 효과가 확인된 적이 없었다.

전수 조합(18가지)은 돌리지 않았다. 축을 하나씩 쓸어 기준선과 비교했다.

### 두 축은 잘 작동한다

**난이도**는 실제로 다른 게임이다. 최종 보스 치명도 쉬움 0.84 / 보통 1.39 / 어려움 2.68. 쉬움은 평타만으로 넘을 수 있고, 어려움은 HP 세 통이 필요하다. 이름만 다른 게 아니다.

**장비**도 체감된다. 상점 최고 조합이 치명도를 절반 안팎으로 줄인다(파수꾼 1.24 → 0.54, 최종 보스 1.35 → 0.96). 진행 보상으로 제 몫을 한다.

### NG+는 복리로 뛰고 있었다

맨몸 기준 파수꾼 1.22 → 2.53 → **3.89**, 4장 잡몹조차 NG+2에서 1.88이었다.

다만 NG+ 플레이어는 장비를 이어받으므로(S34) 맨몸 비교는 **실제로 존재하지 않는 상황**을 재는 것이다. 장비를 갖춘 조건으로 다시 쟀더니 그래도 최종 보스가 0.91 → 1.61 → **2.74**였다. 첫 회차 맨몸이 1.36인데, **최고 장비를 갖춘 NG+2가 그 두 배**다.

원인은 둘이었다.

1. **같은 의도를 두 번 셌다.** `start_battle`이 `get_ng_scale()`로 적 HP·공격력에 회당 1.3배를 곱하는데, 몇 줄 아래에서 NG+1 이상이면 `difficulty_bonus += 0.20`을 **또** 얹었다.
2. **플레이어 성장이 평평했다.** NG++ 진입 시 `max_hp`를 120으로 올리지만 챕터 성장식(`100+(ch-1)*15`)이 3장이면 그 위로 올라가 보너스를 흡수한다. NG++ 전용 장비도 일반 최고 조합 대비 **공격 +3 / 방어 +4**뿐이다. 즉 적은 기하급수로 크는데 플레이어는 한 번 오르고 멈춘다.

**고친 것:** 중복 계상을 제거하고(스케일링은 `get_ng_scale()` 한 곳에서만), 챕터 성장식에 회차당 +20을 상시로 얹었다.

| 교전 | ng0+장비 | ng1+장비 | ng2+장비 |
|---|---|---|---|
| void_beast | 0.27 → 0.25 | 0.56 → **0.37** | 0.90 → **0.49** |
| shade_sentinel | 0.61 → 0.61 | 1.18 → **0.85** | 1.72 → **1.04** |
| kairos | 0.91 → 0.92 | 1.61 → **1.28** | 2.74 → **1.68** |

NG+0은 변하지 않았다(대조군). 회차마다 확실히 어려워지되(0.92 → 1.28 → 1.68) 각 단계가 감당할 폭이고, NG+2가 첫 회차 맨몸(1.48)보다 어렵다 — 세 번째 회차로 맞는 자리다. 난이도 축도 회귀 없음(0.88 / 1.48 / 2.52).

### 검증
- 축 계측 전후 각 1회, 전 조건 stalls=0.
- 스모크 38/38. VN 20파일 504스텝 0오류. 한국어 31파일 1592필드 0오류. `git diff --check` 통과.

### 방법론
- **불공정한 비교를 먼저 걷어냈다.** 맨몸 NG+ 수치(3.89)만 보고 고쳤다면 존재하지 않는 상황에 맞춰 조정했을 것이다. "이 조건이 실제로 일어나는가"를 묻는 것이 숫자를 읽는 것보다 앞선다.
- 스크립트가 단언에서 멈춰 **변경이 하나도 적용되지 않은 채** 측정을 돌린 적이 있다. 수치가 기준선과 똑같아서 알아챘다. 자동 치환은 성공 여부를 반드시 확인해야 한다.

---

## S247 - 2026-08-14 (검증 범위를 넓히니 로스터에서 하나가 걸렸다 + 브랜치·잔여 정리)

S246은 프리셋 6종만 재고 그 위에서 다이얼 세 개를 돌렸다. 내가 만든 열린 위험이었다.

### 1. 로스터 28종 전체 계측

맵 스크립트가 인라인으로 정의하는 적이 **28종**이고, 플레이어가 실제로 가장 많이 싸우는 건 그쪽이다. 로스터는 손으로 옮겨 적지 않고 맵 스크립트에서 뽑아 넣었다.

**28종 중 정확히 하나가 치명도 1.0을 넘었다 — 1장 Void Beast, 1.53(10.9턴).** 100 HP짜리 1장 플레이어를 한 번 반 죽일 만큼 때린다. 원인은 보이드 적에게 공격이 30%만 들어가는 것(`base_dmg * 0.3`)인데, 그건 S231이 "연소를 강요하는 벽"으로 의도한 설계다. 문제는 배치였다. 이 적이 **1장 랜덤 인카운터 풀**에 있었다. 스토리에서 만나는 벽과 재방문 중 우연히 밟는 벽은 다르다.

보이드 적을 빼지 않고 1장에 맞는 크기로 바꿨다(Void Wisp 50/12). **1.53 → 0.90**, 다른 보이드 적(0.74~0.83)과 같은 계열이 되었고 벽은 남았다. Void Beast 자체는 프롤로그 시나리오 전투로 그대로 있다.

**덤으로 S241 수정의 사각지대도 드러났다.** 그때 프리셋 표의 낡은 `img`만 고쳤는데, 맵 스크립트에 **인라인 사본**이 따로 있었다. Ash Crawler(비-보이드 1장 잡몹)와 Memory Eater가 전용 컷인을 두고도 `void_beast_confrontation.png`를 달고 있었다. 둘 다 비웠다. 나머지 셋(Coastal Void Beast / Void Fragment / Void Wraith)은 진짜 보이드고 전용 아트가 없어 공용 폴백이 타당하므로 두었다.

구조적 관찰: **보이드 감쇠가 챕터 스케일링을 압도한다.** 보이드 적은 챕터와 무관하게 치명도 상위에, 비-보이드는 하위에 몰린다(9장 Hollow Walker 95HP/22atk가 0.21).

### 2. 중간 등급 곡선: 가설이 틀렸다

`burn_weighty`가 보스를 2턴에 끝내는 걸 보고 "S246에서 넓힌 BREAK 창(×1.8)이 연소까지 증폭해서"라고 보았다. 창의 배율을 연소에만 1.30으로 분리했는데 **아무 변화가 없었다.**

이유가 분명했다. 그 정책은 매 턴 태우므로 평타를 안 치고, 그러면 게이지가 안 차서 **창이 한 번도 열리지 않는다.** 즉 S246에서 설계한 고리를 재는 정책이 애초에 없었다.

`break_loop` 정책을 추가했다 — 평타로 BREAK를 열고 창이 열린 동안에만 무게 있는 기억을 꽂는다.

| 교전 | 공격만 | 고리 | 쓴 기억 |
|---|---|---|---|
| 잡몹 ch1~2 | 0.19 / 0.24 | 0.19 / 0.21 | **0** |
| threshold_shade ch7 | 0.80 | **0.72** | 1 |
| shade_sentinel ch6 | 12.8턴 / 1.28 | **10.0턴 / 1.07** | 1 |
| kairos ch10 | 11.7턴 / 1.41 | **9.2턴 / 1.10** | 1.3 |

설계한 모양 그대로다. 잡몹에서는 창이 안 열려 기억을 **한 개도** 안 쓰고, 보스에서는 잘 맞춘 희생 하나가 판을 바꾼다. **중간 등급 곡선을 깎을 필요가 없었다** — 비현실적 정책("매 턴 태운다")을 재고 있었을 뿐이다. 창 배율 분리는 원리상 맞으므로 남겼다(창을 여는 대가는 평타가 치르므로 몫도 평타가 갖는다).

### 3. 브랜치 분리: 깨끗한 분리는 불가능하다

14개 커밋을 네 주제로 나눠 `main`에서 체리픽했는데 **첫 그룹만 통과하고 셋은 충돌**했다. 커밋들이 순차 의존이라 같은 파일(battle_scene.gd, ui_theme.gd, 맵 스크립트)을 연달아 고치기 때문이다.

강제로 풀지 않았다. 대신 주제 경계마다 브랜치 포인터를 세웠다 — 충돌 위험 0으로 같은 목적(주제별 리뷰 경계)을 이룬다.

- `stack/1-typography-battle-ui-memory` (1커밋)
- `stack/2-field-atmosphere-motion` (6커밋)
- `stack/3-screen-surveys-korean-ui` (5커밋)
- `stack/4-battle-balance` (2커밋)

로컬에만 만들었다. 푸시는 저장소 구조를 바꾸는 일이라 작가 판단에 맡긴다.

### 4. 남은 구멍 둘

**영어 챕터 이름을 정본 하나로 모았다.** 저널의 `CHAPTER_NAMES`, `RICH_PRESENCE_CHAPTERS`, pause_menu의 지역 사전이 서로 달랐다(12장이 "Verdan Underlock"이기도 "The Reader"이기도 했다). 한국어는 S242에서 VN `title_ko`를 정본으로 통일했으므로, 영어도 같은 계열인 `RICH_PRESENCE_CHAPTERS`를 쓰게 했다. 이제 두 언어가 같은 것을 가리킨다.

**저널 번역 누락을 스모크로 막았다.** `_field`가 `_ko`가 없으면 조용히 영어로 되돌아가는 폴백은 유지할 값이 있지만(화면이 비지 않는다) 그 탓에 새 항목의 번역 누락을 아무도 알려 주지 않았다. `smoke_journal_localization`이 65개 항목의 `_ko` 존재를 단언한다. 스모크가 37 → **38**개가 되었다.

### 검증
- 스모크 38/38. VN 20파일 504스텝 0오류. 한국어 31파일 1592필드 0오류. `git diff --check` 통과.
- 로스터 28종 치명도 1.0 초과 1 → **0**.

### 방법론
- **"검증 범위 밖"이 실제로 위험했다.** 6종에서 좋았던 값이 28종 중 하나에서 무너져 있었다. 다만 무너진 것은 내가 바꾼 다이얼이 아니라 원래 있던 배치 문제였고, 넓히지 않았으면 못 봤다.
- **가설이 틀렸을 때 계측기를 탓하지 않았다.** BREAK 창 분리가 아무 변화를 못 낸 것은 계측 오류가 아니라, 그 정책이 창을 열지 않기 때문이었다. 답은 지표를 고치는 게 아니라 **설계한 고리를 재는 정책을 만드는 것**이었다.
- **불가능한 것은 불가능하다고 보고했다.** 브랜치를 깨끗이 쪼갤 수 없다는 것은 충돌을 강제로 푸는 대신 구조를 바꿔 우회할 이유였다.

---

## S246 - 2026-08-13 (전투 밸런스를 처음으로 계측하다: 값싼 기억이 지배 전략이다)

화면에는 서베이 방식이 다섯 번 값을 했다(맵·전투 화면·오버레이·VN·엔딩). 전투 **밸런스**는 한 번도 계측된 적이 없었다. S17이 챕터별 HP/ATK 성장을 손으로 정한 것이 마지막이고, 그 뒤로 스탠스·콤보·속성·브레이크·수정자·연소 체인이 얹혔다.

묻는 것은 하나였다. **기억을 태우지 않고 이길 수 있는가.** 이 게임의 중심 긴장이 거기 있다.

### 계측기를 네 번 고쳤다

계산식을 다시 구현하지 않고 실제 `BattleManager`를 헤드리스로 돌린다. 출시되는 코드가 아니면 잰 의미가 없다. 그 과정에서 네 번 틀렸고, **네 번 모두 게임이 아니라 계측기 문제였다.**

1. **상태 폴링(1차):** void_beast가 20회 중 10회 "멈춤"으로 잡혔다. 가속(`time_scale`) 상태에서는 적 턴 하나가 프레임 경계 사이에 통째로 끝나 `PLAYER_TURN → ENEMY_TURN → PLAYER_TURN` 전이가 관측되지 않는다. "void_beast 전투가 절반은 멈춘다"고 보고할 뻔했다.
2. **턴 카운터(2차):** `_total_turns`는 적이 행동하기 **전에** 증가하므로 받은 피해를 걷기 전에 라운드가 끝난 것으로 잡혔다. ash_crawler 피해가 0으로 나왔다.
3. **신호 구동(3차, 유효):** `player_turn_started`에 연결. 프레임 타이밍과 무관해졌다. 두 번 연속 stalls=0, 값도 재현.
4. **판 간 상태 누수:** `start_battle`은 최대 HP를 챕터 성장치보다 낮을 때만 **올리고 내리지 않는다.** 10장을 돌고 나면 1장 시뮬레이션도 235로 시작했다. 기억도 영구 소모라 첫 교전 뒤 전부 소진되어 연소 정책이 사실상 공격 정책이 되었다(burns=0.0). 판마다 초기화.
5. **정책이 틀렸던 건:** 처음에는 "등급이 가장 높은 기억"을 태우게 했더니 카이로스(450 HP)까지 전부 1턴에 죽었다. 열거형이 `{GRADE_5, GRADE_4, GRADE_3, GRADE_2, GRADE_1}`이라 등급 **값**이 가장 큰 것은 GRADE_1, 곧 연소력 999의 정체성 기억이었다. 그건 게임 전체에 한 번뿐인 최후의 수단(엔딩 "이름을 태우다")이지 매 턴 쓰는 자원이 아니다. **계산식은 정상이었고 내 정책이 틀렸다.** 실제 플레이어처럼 가장 값싼 것부터 쓰게 바꿨다.

### 측정 결과

| 교전 | 챕터 | 공격만 턴 | 공격만 치명도 | 연소 턴 | 연소 치명도 | 쓴 기억 |
|---|---|---|---|---|---|---|
| ash_crawler | 1 | 2.7 | 0.19 | 1.0 | 0.00 | 1 |
| forest_shade | 2 | 2.8 | 0.19 | 1.0 | 0.00 | 1 |
| void_beast | 4 | 8.0 | 0.80 | 2.4 | 0.17 | 2.4 |
| threshold_shade | 7 | 8.3 | 0.96 | 2.3 | 0.20 | 2.3 |
| shade_sentinel | 6 (보스) | 15.4 | **1.65** | 4.0 | 0.40 | 4 |
| kairos | 10 (보스) | 13.4 | **2.10** | 5.0 | 0.76 | 5 |

치명도는 받은 누적 피해 ÷ 최대 HP다. 1.0을 넘으면 회복 없이는 못 버틴다.

### 드러난 것

**1. 가장 값싼 기억을 태우는 것이 모든 상황에서 지배 전략이다.**
연소는 턴을 62~73% 줄이고 받는 피해를 64~79% 줄인다. 예외가 없다. GRADE_5 감각 잔편("비 온 뒤의 숲 냄새")은 서사적으로 거의 아무 대가가 아닌데, 그것만 다섯 개 태우면 최종 보스를 치명도 0.76(생존)으로 넘긴다. 공격만으로는 2.10(두 번 죽을 만큼)이다.

즉 **"소중한 것을 내주고 힘을 얻는다"는 중심 긴장이, 소중하지 않은 것을 쓰는 것으로 우회된다.** 값싼 연소가 평타보다 항상 강하므로 평타를 고를 이유가 없다.

증폭 원인은 연소 체인(S53, 연속 연소 +20%)으로 보인다. 값싼 연소를 연달아 쓰면 배율이 쌓인다.

**2. 6장 보스가 10장 최종 보스보다 긴 전투다** — 공격만으로 15.4턴 대 13.4턴. HP는 180 대 450인데. `shade_sentinel`은 `weakness: void`라 물리 평타에 이점이 없고 `is_void` 감쇠까지 받는 반면, `kairos`는 `weakness: physical`이라 평타가 잘 든다. 다만 연소로 재면 4턴 대 5턴으로 순서가 정상이므로, 이건 "평타로 밀면 안 되는 보스"라는 의도된 설계일 수 있다.

### 고친 것: 고리를 닫았다

세 가지를 한 세트로 바꿨다. 따로 만지면 서로를 상쇄한다.

**1. 값싼 연소의 기본 피해를 내렸다.** GRADE_5 30→12, GRADE_4 60→30. 잡동사니 연소가 1장 평타(약 20)의 두 배였던 것이 이제 비슷하고, 후반에는 평타보다 약하다. 서사적으로 대가가 없는 것에는 화력도 없어야 한다.

**2. 연쇄는 무게가 있는 기억에만 쌓이게 했다.** 예전에는 등급과 무관하게 `_burn_chain`이 올라가 감각 잔편 연타만으로 +20%씩 배율이 붙었다. 이것이 값싼 연소 지배의 증폭원이었다. 이제 GRADE_3 이상만 쌓이고, 더 가벼운 것을 태우면 쌓은 것이 흩어진다.

**3. BREAK를 평타의 보상으로 만들었다.** 중립 획득 10→16, 붕괴 창 ×1.35→×1.80, 지속 1턴→2턴. 예전에는 100을 채우는 데 평타 열 번(보스는 열네 번)이 걸리고 보상은 1턴 +35%였다. 튜토리얼은 "공격은 BREAK를 쌓고, 연소는 화력을 산다"고 가르치는데 실제 플레이에서 그 문장은 거짓이었다. 이제 예닐곱 번이면 두 턴짜리 ×1.8 창이 열려, **평타로 열고 무게 있는 기억을 그 안에 꽂는** 순서가 성립한다.

### 전후 비교 (같은 계측기, 20회 평균)

| 교전 | 공격만 치명도 전→후 | 공격만 턴 전→후 |
|---|---|---|
| ash_crawler | 0.19 → 0.17 | 2.7 → 2.6 |
| forest_shade | 0.19 → 0.20 | 2.8 → 2.9 |
| void_beast | 0.80 → **0.72** | 8.0 → 7.8 |
| threshold_shade | 0.96 → **0.84** | 8.3 → 7.8 |
| shade_sentinel | 1.65 → **1.16** | 15.4 → **12.4** |
| kairos | 2.10 → **1.32** | 13.4 → 11.8 |

값싼 연소의 지배는 끊겼다. 전에는 모든 교전에서 턴을 62~73%, 받는 피해를 64~79% 줄였고 예외가 없었다. 지금은 잡몹에서 2.6→2.0턴으로 미미하고, 최종 보스에서도 치명도 1.32→0.93으로 30% 개선에 그친다(전에는 2.10→0.76).

무게 있는 연소는 여전히 결정적이다(GRADE_3 사용 시 대부분 1턴, 보스 2턴). 그것이 설계 의도다 — **소중한 것을 내주면 판이 뒤집힌다.** 다만 이제 그 힘은 희소하고 영구히 사라지는 자원에만 붙어 있지, 잡동사니 연타로는 나오지 않는다.

6장 보스가 10장 보스보다 길던 문제도 좁혀졌다(15.4 대 13.4 → 12.4 대 11.8).

### 남은 튜닝 여지 (명시)

`burn_weighty` 정책이 중간 교전을 전부 1턴에 끝낸다. GRADE_3 기억(기본 120 + 연소력 50 = 170)이 매우 강하고, 여기에 BREAK 창 ×1.8이 겹치면 더 그렇다. 시작 소지품에 GRADE_3이 둘뿐이라 남용은 제한되지만, 중간 등급 곡선은 한 번 더 볼 값이 있다. 이번에는 손대지 않았다 — 한 세트에서 네 개를 동시에 움직이면 무엇이 무엇을 바꿨는지 알 수 없어진다.

### 검증
- 신호 구동 계측기로 변경 전 두 번, 변경 후 한 번 실행. 세 번 모두 stalls=0.
- 스모크 37/37. VN 20파일 504스텝 0오류. 한국어 31파일 1592필드 0오류. `git diff --check` 통과.

### 방법론
- **네 번의 계측기 오류 중 세 번은 "게임 버그"로 보고할 수 있는 모양이었다.** 멈춤, 피해 0, 원턴킬. 매번 숫자가 상식과 어긋났고, 매번 계측기가 원인이었다. 상식과 어긋나는 값은 발견이 아니라 점검 신호다.
- **다섯 번째는 계측기가 아니라 정책이 틀린 경우였다.** 원턴킬은 코드가 아니라 내가 "가장 좋은 기억"을 잘못 정의한 결과였다. 도구가 맞아도 질문이 틀릴 수 있다(S245에서 배운 것과 같은 형태).

---

## S245 - 2026-08-13 (튜토리얼 힌트 띠를 겹치지 않는 자리로: 지표를 바꾸자 답이 뒤집혔다)

S241에서 힌트 패널의 높이 결함(768x806)을 고치면서 "관찰만 하고 남긴다"고 적어 둔 것. 띠가 4초 동안 "전장 관측" 카드와 적 이름을 덮는다.

### 눈으로 본 것을 면적으로 바꿨다

노드 이름을 추측하지 않고, 힌트 사각형과 겹치는 **보이는 Control 전부**를 찾아 면적순으로 신고하게 했다. 예전 위치(y=10)에서:

- `EnemyReadout` **9,677px**
- `CombatCue`(전장 관측 카드) **9,651px**

### 지표를 바꾸자 답이 정반대가 되었다

어디로 옮길지도 추측하지 않았다. 같은 폭의 띠를 10px 간격으로 화면 위에서 아래로 내리며 겹침을 쟀다.

- **그림까지 세었을 때:** 최소가 **y=0**(36,853)이고 중앙 대역은 오히려 더 나빴다(y=240에서 43,798). 전투 화면에 빈 자리는 없고 중앙은 무대 아트가 차지하고 있다는 뜻이었다.
- **글자와 막대만 세었을 때:** y **180~250** 구간이 유일하게 **겹침 0**이었다.

두 결과가 정반대인 이유는 값이 다르기 때문이다. 4초 동안 무대 그림을 가리는 것과 적의 HP를 가리는 것은 같은 비용이 아니다. 첫 번째 지표는 "무엇이든 덜 덮는 자리"를 물었고, 두 번째는 "읽을 것을 덜 덮는 자리"를 물었다. 후자가 실제 질문이었다.

깨끗한 대역의 한가운데인 y=200을 썼다. 전투에서만 내려가고, 위쪽이 비어 있는 탐색 화면은 예전 자리(y=10)를 그대로 둔다. 장식 배너도 패널과의 관계(상단 -12 ~ +68)를 유지한 채 따라간다.

### 결과
- 옮긴 뒤 겹치는 것은 무대 아트(TextureRect)뿐이다. `EnemyReadout`과 `CombatCue`는 목록에서 사라졌고, 글자·막대·패널은 **하나도 가려지지 않는다**.
- 첫 전투 화면 전체가 한 번에 읽힌다. 목표 카드, 전장 관측, 적 정보(잿빛 크롤러 · 브레이크 · 미판독), 덱 8칸이 모두 드러난다.
- 탐색 힌트는 (256, 10) 54px 그대로.

### 검증
- 스모크 37/37. VN 20파일 504스텝 0오류. 한국어 대화 31파일 1592필드 0오류. `git diff --check` 통과.

### 방법론
- **지표를 하나 더 만들어 본 것이 답을 바꿨다.** 첫 지표만 믿었다면 "옮길 자리가 없다"는 잘못된 결론에 도달했을 것이다. 계측 결과가 직관과 어긋날 때는 계측기가 틀렸을 수도 있지만, **질문이 틀렸을 수도 있다.**
- 이번에도 위치를 손으로 고르지 않았다. 화면을 훑어 얻은 값이 자리를 정했다.

---

## S244 - 2026-08-13 (저널 항목 한국어화: S242가 일부러 남긴 것)

S242에서 "기계적 치환이 아니라 서사 문체를 맞춰야 하는 번역"이라며 정확히 세어서 남긴 항목이다. 절반만 하면 더 나쁘므로 손대지 않았던 것을 이번에 끝냈다.

### 옮긴 것

기존 관례를 따라 원문 옆에 `_ko`를 붙였다(VN의 `text`/`text_ko`, 장소 씬의 `title_en`/`title_ko`). 별도 표를 만들지 않았으므로 번역이 원문 옆에서 읽힌다.

- 사건 46항목: `title_ko` + `desc_ko`
- 인물 5항목: `role_ko` + `desc_ko` (+ 토비아스는 `name_ko`)
- 세계 14항목: `title_ko` + `desc_ko` — 본문이 100자를 넘는 문단들
- 선택 7항목: `title_ko` + `desc_ko`

합계 `title_ko` 67, `desc_ko` 72, `role_ko` 5.

**영어 원문은 파일에서 직접 뽑아 썼다.** 스크립트가 제목으로 항목을 찾아 `desc`를 그대로 읽고, 내가 쓴 것은 한국어뿐이다. 원문을 손으로 옮겨 적지 않았으므로 전사 오류가 생길 수 없고, 번역이 빠진 항목이 있으면 스크립트가 단언에서 멈춘다.

읽는 쪽은 `_field(entry, key)` 하나로 통일했다. 한국어일 때 `<key>_ko`를 먼저 보고 비어 있으면 영어로 되돌아간다. 그래서 나중에 항목을 추가하면서 번역을 빠뜨려도 화면이 비지 않는다.

### 숫자가 0을 가리켰는데 화면에는 영어가 남아 있었다 (세 번째)

`_field` 연결 직후 오버레이 로케일 프로브는 저널 0을 돌려줬다. 그런데 다섯 탭을 렌더해 보니 두 곳이 그대로 영어였다.

- **선택 탭이 통째로 영어**였다. `choice_entries`는 상수 표가 아니라 함수 안의 **지역 배열**이라 앞선 스크립트의 정규식 범위 밖에 있었다.
- **인물 탭의 "Tobias Crane"**만 영어로 남았다. `SPEAKER_NAMES_KO`의 키가 `"Tobias"`라 `"Tobias Crane"`이 통과하지 못했다. 표를 고치는 대신 항목이 스스로 `name_ko`를 들게 했다.
- 상실 탭의 빈 목록 문구도 영어였다.

프로브가 못 잡은 이유는 단순하다. **열려 있는 탭만 본다.** S242는 상시 HUD를 빠뜨렸고, S243은 독립 씬을 못 봤고, 이번엔 닫힌 탭이었다. 세 번 연속 같은 형태다.

그래서 `capture_journal_tabs`를 만들었다. 다섯 탭을 각각 렌더하고, 각 탭에서 이름이 가장 긴 항목을 눌러 상세 패널까지 채운다. 목록만 찍으면 상세는 "항목을 고르세요..." 상태로 남아, 긴 한국어 문단이 실제로 어떻게 흐르는지가 안 보인다.

### 검증
- 세계 탭의 가장 긴 본문(100자 이상)이 상세 패널에서 다섯 줄로 흐르고 여유가 남는다. 넘침·잘림 없음.
- 다섯 탭 재렌더 후 육안 확인. 사건·인물·세계·선택·상실 전부 한국어로 읽힌다.
- 오버레이 로케일 프로브 6 → **3**. 남은 셋은 `x1.0`, `720p (1280x720)`, `HP`로 번역 대상이 아니다.
- 스모크 37/37. VN 20파일 504스텝 0오류. 한국어 대화 31파일 1592필드 0오류. `git diff --check` 통과.

### 방법론
- **원문을 손으로 옮기지 않았다.** 번역 작업에서 가장 흔한 사고는 영어 쪽을 잘못 적어 매칭이 조용히 실패하는 것이다. 스크립트가 파일에서 원문을 읽게 하면 그 실패가 구조적으로 불가능해진다.
- **계측기 범위 문제가 세 세션 연속 나왔다.** 이제 패턴이 분명하다 — 프로브가 0을 돌려주면 그 프로브가 무엇을 안 보는지부터 묻는다. 그림을 다시 보는 것이 매번 답이었다.

---

## S243 - 2026-08-13 (VN·타이틀·엔딩을 한 장에 모아 보다: CG 위의 사각형, 영어로 끝나는 게임)

같은 방식이 세 번 연속 실제 결함을 찾아냈다(맵 S236, 전투 S241, 오버레이 S242). 남은 곳은 VN 504스텝과 게임의 처음·끝 화면이다. 분량은 VN이 가장 큰데 한 번도 나란히 놓고 본 적이 없었다.

### 1. 모든 VN CG 위에 네 변이 뚜렷한 밝은 사각형이 얹혀 있었다

`capture_vn_survey`가 20개 VN에서 "CG가 깔리고 인물이 말하는" 첫 스텝까지 진행시켜 한 장에 모은다. 여러 칸에서 CG를 가로지르는 밝은 띠가 보였다.

**계측기를 두 번 바꿨다.**

- 1차: 띠 안쪽/바깥의 평균 밝기 차이. 여섯 장면 중 넷에서 델타가 **정확히 0**이었고(그 장면들에선 `_cg_detail_top`이 아예 안 그려진다) 나머지도 0.006~0.008로 작았다. 그런데 스크린샷에는 띠가 분명히 있다. 평균값 지표로는 어느 레이어가 그리는지 가려지지 않았다.
- 2차: 레이어를 하나씩 껐다 켜고 화면을 빼서 **그 레이어가 건드린 픽셀을 그대로 그림으로** 남겼다(8배 증폭). 추측이 필요 없어졌다.

범인은 `_cg_focus_glow`였다(내가 처음 의심한 `_cg_detail_top`이 아니다). 발자국이 부드러운 원형 광원이 아니라 **하드 엣지 직사각형**이었다. 경계 날카로움을 숫자로 만들었다 — 자기 사각형 위 변(y=282)에서 세로 도약 **0.0695**, 오른 변(x=1127)에서 가로 도약 0.0287로 화면에서 가장 날카로운 경계였다.

원인: 앵커 x 0.12~0.88 / y 0.42~1.03 사각형에 그리는데, 그라디언트가 사각형 경계에 닿기 전에 투명해지지 않는다. 사각형을 화면 전체로 넓혀 경계를 화면 밖으로 보냈다. 광원의 화면상 위치·크기는 유지하려고 `fill_from`/`fill_to`를 환산했다(중심 (640, 653), 반지름 끝 (1088, 419)).

`_cg_lower_wash`도 같은 병이었다. 평면 `ColorRect`라 위 변(y=389)에서 어둡기가 뚝 끊겼다(도약 0.0275). 위쪽이 투명한 세로 그라디언트로 바꾸고, 기본 필터가 256줄을 331px로 늘리며 만드는 계단을 없애려 선형 보간을 켰다. 도약 0.0275 → **0.0223**, 최대 기여 0.1408 → **0.0734**.

포커스 글로우의 최대 도약은 y=282 → y=60으로 옮겨 갔다. y=60은 레터박스 경계(검은 띠와 CG의 경계)이므로 레이어 이음매가 아니라 화면 구조다. 같은 자리에서 비네트도 0.0347을 보인다.

### 2. 게임이 영어로 끝나고 있었다

`capture_endpoint_survey`가 타이틀·게임 오버·데모 종료·엔딩 분기별 크레딧을 찍는다.

- **크레딧이 통째로 영어였다.** 항목 이름(Created By, Special Thanks, Engine)뿐 아니라 **분기별 에필로그 한 줄까지** — "He kept his name. The seal held." 전체 플레이의 마지막 문장이다. 네 갈래 전부 옮겼다.
- **데모 종료 화면도 통째로 영어였다.** 데모를 끝까지 본 플레이어가 마지막으로 보는 화면이다.

S242의 계측기가 이걸 못 잡은 이유: 그 프로브는 오버레이 싱글턴만 훑는데 크레딧·데모 종료는 독립 씬이다. 도구의 범위가 곧 측정의 한계라는 걸 또 확인했다.

### 3. 데모 종료 배경에 가짜 영어 메뉴가 구워져 있었다

배경이 `game_start.png`, 즉 **타이틀 화면 목업**이었다. NEW GAME / CONTINUE / SETTINGS / EXIT와 PRESS ANY KEY, MEMORIA 워드마크가 삽화에 그려져 있어서 진짜 한국어 문구 뒤로 겹쳐 보였다. 본문이 "벨트를 지나"라고 말하므로 그 다음 목적지의 챕터 스플래시로 바꿨다.

### 캡처 도구에서 먼저 고친 것 둘

첫 VN 캡처는 대사가 문장 중간에서 끊긴 채 찍혔다. 잘림으로 신고할 뻔했으나 박스에 아래로 두 줄 더 들어갈 자리가 있었다 — 고정 1.1초 대기가 타자기 효과보다 짧았을 뿐이다. VN이 스스로 `_typing_done`을 세울 때까지 기다리게 고쳤다. (S241에서 턴 배너로 같은 실수를 할 뻔했다.)

크레딧 세 장은 첫 캡처에서 모두 빈 화면이었다. 화면 아래에서 위로 흐르는 두루마리라 1.6초 시점에는 본문이 아직 화면 밖이다. 두루마리를 본문 한가운데로 옮겨 놓고 찍는다.

### 검증
- VN 콘택트 시트 수정 전후 재렌더 후 육안 확인. 20칸 모두에서 사각형 띠가 사라지고 삽화가 이어져 읽힌다. 대사도 잘림 없이 완결된다.
- 엔드포인트 시트 재렌더. 여섯 화면 모두 한국어로 읽히고, 크레딧 세 갈래가 각각 다른 에필로그를 보여 준다(분기 로직도 함께 확인).
- 스모크 37/37. VN 20파일 504스텝 0오류. 한국어 대화 31파일 1592필드 0오류. 오버레이 로케일 프로브 6(전부 번역 대상 아님). `git diff --check` 통과.

### 방법론
- **평균값 지표가 0을 돌려줬는데 눈에는 보였다.** 이럴 때 답은 지표를 더 정교하게 만드는 게 아니라 **무엇이 어디를 칠하는지 그림으로 뽑는 것**이었다. 발자국 영상 한 장이 두 번의 잘못된 추론(detail_top 의심)을 즉시 끝냈다.
- **계측기 범위가 곧 한계**라는 걸 두 세션 연속 확인했다. S242는 상시 HUD를 빠뜨렸고, S243은 그 프로브가 독립 씬을 못 본다는 걸 드러냈다.
- **잘림처럼 보이는 것을 먼저 의심했다.** 박스에 여유 공간이 있다는 한 가지 관찰이 "게임 결함" 신고를 "내 캡처 타이밍" 수정으로 바꿨다.

---

## S242 - 2026-08-12 (오버레이를 한 장에 모아 보다: 한국어 로케일인데 UI 크롬이 영어였다)

S241에서 찾은 결함(autowrap 라벨 + `Control.position`)이 **유형**인지 일회성인지부터 확인하고, 맵·전투에서 두 번 값을 한 "나란히 놓고 보기"를 오버레이에 적용했다.

### 1. 같은 결함 유형은 재발하지 않았다 (음성 결과)

`probe_overlay_bounds`가 오버레이 12곳을 실제로 열고 트리를 걸어 뷰포트를 넘는 Control을 전부 신고한다. 화면보다 큰 것은 둘뿐이고 **둘 다 ScrollContainer 안쪽**이라 정상이다(도감 618x1505, 옵션 720x1264). `LOOSE` 0건. S241의 806px 패널은 고립된 사례였다.

계측기 자체는 유효하다 — 806 > 720이므로 그때의 결함은 이 검사에 걸렸을 것이다.

### 2. 콘택트 시트가 드러낸 것: 로케일은 ko인데 UI는 영어

`capture_overlay_survey`가 오버레이 12곳을 실제 맵 위에 같은 조건으로 렌더한다. 시트 전체를 관통하는 패턴이 보였다. 대화·기억 이름은 한국어인데 **UI 크롬만 영어로 남아 있었다.**

기존 `validate_korean_localization.py`는 대화 JSON만 본다(31파일 1592필드 0오류). UI 크롬은 어떤 검사도 받은 적이 없어서 지금까지 드러나지 않았다.

`probe_overlay_locale`을 만들었다. 보이는 Label/Button/RichTextLabel 중 ASCII 알파벳이 있고 한글이 하나도 없는 것을 신고한다. 처음 측정 **45개**.

고친 방식은 새로 만드는 게 아니라 **이미 있는데 연결되지 않은 것을 연결하는** 쪽이었다.

- 도감이 적 이름을 영어 원문으로 내보내고 있었다. `GameManager.localized_enemy_name()`과 이름표는 이미 있었고, 같은 파일의 상세 패널은 이미 그걸 쓰고 있었다. 목록만 안 쓰고 있었다.
- 상점의 `0 Grains`. `RUNTIME_TEXT_KO`에 `" Grains" → " 그레인"`이 이미 있었다.
- 챕터 이름의 한국어가 pause_menu 안에만 1~11장까지 지역 사전으로 있었다. 영어 표는 세 곳에 서로 다른 이름으로 흩어져 있었다. 11~24장 한국어는 VN 씬의 `title_ko`에 이미 정본으로 있었다. 둘을 합쳐 `GameManager.CHAPTER_NAMES_KO`(1~24)와 `localized_chapter_name()`을 두고, 저널·일시정지가 그것을 쓰게 했다. **새로 지어낸 챕터 이름은 없다.**

### 3. 로컬라이제이션이 아니었던 결함: 도감이 내부 씬 ID를 노출했다

미발견 항목이 `??? · rim_root_hollow`, `??? · verdan_ledger_cellar`처럼 나왔다. `_map_label`은 `ExplorationHUD.MAP_NAMES_KO`를 찾아보고 없으면 원시 ID로 되돌아가는데, **선택 기억 장소 아홉 곳이 이름표에 등록된 적이 없었다.** S239에서 대기 예산까지 받은 맵들이다.

값은 각 장소 씬이 이미 선언한 `title_en`/`title_ko`를 그대로 옮겼다(기록목 뿌리 공동, 기억 대출 장부실, 신호 야적장, 이정표 성소, 신더 항구, 랜턴 구역, 이름의 골짜기, 회색 캐러밴, 씨앗 금고). 세 이름표 모두에 추가했다.

### 4. 계측기 범위를 넓히고 나서 11개가 더 나왔다

45개를 3개까지 줄인 뒤 콘택트 시트를 다시 보니 아래 세 칸에 `MEMORY COMPASS / THREADS HUM / The needle rests...`, `FIELD FLOW`가 그대로 있었다. 계측기가 **항상 떠 있는 것**(ExplorationHUD, MemoryCompass)을 안 보고 있었고, 저널 항목은 진행 플래그가 없어 비어 있었다. 범위를 넓혀 11개를 더 찾았다.

나침반 13개 지역 프로필의 상태·설명을 전부 한국어로 옮겼다.

### 결과

| | 처음 | 지금 |
|---|---|---|
| 저널 | 10 | 3 (항목 데이터) |
| 도감 | 14 | 0 |
| 상점 | 9 | 0 |
| 퍼즐 | 4 | 0 |
| 일시정지 | 5 | 0 |
| 옵션 | 3 | 2 (`x1.0`, `720p (1280x720)`) |
| 탐색 HUD | (미측정) 3 | 1 (`HP`) |
| 나침반 | (미측정) 3 | 0 |

남은 6개 중 `x1.0`, `720p (1280x720)`, `HP`는 번역 대상이 아니다.

### 캡처 도구에서 먼저 고친 것 둘

첫 캡처는 3번 칸부터 무효였다. StoryLog가 닫히지 않고 이후 아홉 장 위에 그대로 남았다 — 흔한 닫기 이름을 순서대로 시도하는 헬퍼를 썼는데 StoryLog에는 `close_log`만 있어 아무것도 호출되지 않았다. 추측 목록을 버리고 표에 명시했다.

두 번째 캡처는 배경으로 깐 맵이 자기 Ch2 도착 시퀀스를 자동 재생해서 VN 장면이 상점·퍼즐·성좌를 덮었다. 진행 플래그를 미리 세워 정지 상태로 만들었다.

### 남긴 것 (범위 밖으로 명시)

`story_journal.gd`의 저널 항목 **72개**가 영어 `title` + `desc`만 갖고 있고 한국어 필드가 하나도 없다. 이건 기계적 치환이 아니라 게임의 서사 문체를 맞춰야 하는 번역이라 성격이 다르고 분량도 크다(제목 72 + 본문 72). 절반만 하면 오히려 더 나쁘므로 손대지 않고 정확히 세어서 남긴다.

영어 챕터 표가 세 곳에 서로 다른 이름으로 존재하는 것(저널 `CHAPTER_NAMES` ch12 "Verdan Underlock" 대 `RICH_PRESENCE_CHAPTERS` ch12 "The Reader")도 그대로 두었다. 한국어 경로는 정본 하나로 통일했지만 영어 표 통합은 별개의 정리 작업이다.

### 검증
- `probe_overlay_locale`: 45 → 6 (실제 잔여 3). 범위를 넓힌 뒤 기준으로는 56 → 6.
- `probe_overlay_bounds`: LOOSE 0건 유지.
- 콘택트 시트 재렌더 후 육안 확인. 저널·도감·상점·퍼즐·일시정지·필드 HUD가 한국어로 읽히고, 도감 미발견 항목이 `??? · 벨트 중계소`처럼 제대로 된 지명을 보여 준다.
- 스모크 37/37. VN 20파일 504스텝 0오류. 한국어 대화 검증 31파일 1592필드 0오류. `git diff --check` 통과.

### 방법론
- **음성 결과도 결과다.** S241의 결함이 유형인지 확인하는 데 든 비용은 프로브 하나였고, 재발이 없다는 걸 알았기에 41곳의 autowrap 라벨을 하나씩 들여다보지 않아도 됐다.
- **계측기 범위가 곧 측정의 한계다.** 45를 3으로 줄이고도 화면에는 영어가 남아 있었다. 숫자만 봤다면 끝난 줄 알았을 것이다. 그림을 다시 본 것이 범위 누락을 잡았다.
- **없는 것을 만들기 전에 있는 것을 찾는다.** 45개 중 상당수는 한국어가 이미 존재하는데 호출되지 않은 경우였다. 챕터 이름 24개를 새로 쓸 뻔했지만 VN 파일에 정본이 있었다.

---

## S241 - 2026-08-12 (전투 화면을 처음으로 한 장에 모아 보다: 잘못된 적 삽화, 눌린 적 대비, 806px 튜토리얼 패널)

전투 화면은 지금까지 The Seam 한 장면만 보고 판단해 왔다. 맵은 열 곳을 나란히 놓고 나서야 아홉 곳이 단색 안개에 덮여 있다는 걸 알았다(S236). 전투도 같은 방식으로 보지 않으면 잘 나온 한 장면을 전체라고 착각하게 된다. `capture_battle_survey`로 여섯 정규 교전을 같은 조건에 렌더해 한 장에 모았고, 세 가지가 나왔다.

### 1. 애시 크롤러가 보이드수 삽화를 달고 있었다

게임의 첫 정규 전투다. `resolve_enemy_image_by_name("Ash Crawler")`는 `RECURRING_ENEMY_ART`에서 전용 컷인을 이미 올바르게 찾아낸다. 그런데 프리셋에 박힌 `img: void_beast_confrontation.png`가 그 값을 덮어썼다. S204에서 컷인을 그리기 전의 잔재였고, 형제 프리셋 `forest_shade`는 이미 `""`로 비워져 있어 정상 동작하고 있었다. 낡은 항목만 지웠다.

비-보이드 1장 잡몹(`is_void: false`)이 보이드수 그림으로 등장하고 있었다는 뜻이다.

### 2. 적은 파티의 4분의 1만큼만 화면에 남아 있었다

콘택트 시트에서 오른쪽 적이 배경에 잠겨 보였다. 계측기를 두 번 버린 뒤에야 맞는 값을 얻었다.

- **1차(무효):** 판 중앙 35% 반경을 "몸통"으로 잡았다. 아렐에게 그 자리는 얼굴이 아니라 검은 코트라(luma 0.046) *아군이 적보다 14배 안 보인다*는 거꾸로 된 답이 나왔다. 게다가 플레이트는 `STRETCH_KEEP_ASPECT_CENTERED`라 텍스처가 사각형을 채우지도 않아 UV 매핑까지 어긋나 있었다.
- **2차(유효):** "어디가 얼굴인가"를 추측하지 않는 지표로 바꿨다. 실제로 그려지는 영역의 국소 대비(RMS)를 재고, 같은 화면의 빈 무대 한 조각을 바닥으로 삼는다.

| | 평균 밝기 | RMS | 빈 무대(0.0261) 위 |
|---|---|---|---|
| 아렐 | 0.0455 | 0.0785 | +0.0524 |
| 동행자 | 0.0624 | 0.0661 | +0.0400 |
| 적(수정 전) | 0.0368 | 0.0391 | **+0.0130** |

원인을 세 단계로 좁혔다.

- **원화가 아니다.** 적 원화의 구조량(src_rms 0.133)은 아렐(0.135)과 사실상 같다.
- **축소가 아니다.** 원화를 그려지는 크기(350x196 / 296x296)로 리샘플해도 손실은 4~15%뿐이다. 밉맵 가설 기각.
- **손실은 렌더 합성에서 일어난다.** 유지율은 아렐 55%, 동행자 34%, 적 35%. 즉 **적이 아니라 아렐이 예외**였다.

그 이유는 프로필 주석에 이미 적혀 있었다 — 아렐만 `plate_modulate` 1.10 리프트를 받았고("preserves Arrel's face and blade as the first readable player-side silhouette"), 나머지는 1.0 그대로였다. 적에게 같은 처방을 1.50으로 주었다.

RMS 0.0391 → **0.0564**(×1.44, 예측 ×1.5), 여섯 교전 전부 상승. 바닥 위로 +0.0130 → **+0.0303**(2.3배), 아렐 대비 25% → 58%. 밝기는 0.046으로 무대(0.102)보다 여전히 어두워 실루엣 읽기는 유지된다.

**기각한 가설(기록):** `edge_softness`가 적의 구조를 먹는다고 보고 0.30 → 0.14로 내렸다. 역할별 `edge_softness`(0.12/0.24/0.30)와 RMS 유지율(0.58/0.33/0.29)의 순서가 정확히 일치했기 때문이다. 그러나 RMS는 6%만 올랐고(예측은 0.06 근처) 밝기가 24% 떨어졌다 — 판이 불투명해지며 뒤로 비치던 밝은 무대가 사라진 것일 뿐 디테일이 돌아온 게 아니다. 세 역할은 원화 종류도 판 크기도 다르므로 그 상관은 교란이었다. 되돌렸다.

### 3. 튜토리얼 패널이 화면보다 컸다

챕터1 첫 교전에서 어두운 띠가 화면 중앙을 세로로 통째 가로지르며 덱의 1·3번 버튼까지 덮고 있었다. 코드상 높이는 54px여야 한다. 실측은 **768 x 806** — 720px 뷰포트보다 크다.

두 겹이었다.

- `AUTOWRAP_WORD` 라벨은 폭이 정해지기 전에 최소 크기를 물으면 *한 글자 폭으로 접었을 때의 높이*를 돌려준다. 한국어 힌트 43자 × ~18px = 774, 여백 32를 더해 정확히 806. → 앵커가 정한 폭을 라벨에 미리 알려 준다(뷰포트에서 유도해 해상도 변화를 따라간다).
- 그것만으로는 안 바뀌었다. `Control.position` 세터가 `size_cache`를 보존하며 offset을 다시 쓰기 때문에, 낡은 806이 `offset_bottom = 816`으로 이미 굳어 있었다. → `position` 대신 `offset_top`/`offset_bottom`을 짝으로 애니메이션한다.

768x806 → **768x54**, offset 10/816 → 10/64. 가장 긴 한국어 힌트 셋(`first_battle`/`first_approach`/`first_directive`) 모두 한 줄에 잘림 없이 들어간다.

### 접은 것
"당신의 턴" 배너가 거의 안 보인다고 적으려다 접었다. `_show_turn_indicator`는 0.15s 페이드인 / 0.4s 유지 / 0.25s 페이드아웃이고 캡처는 0.7s 지점이었다 — 페이드아웃 도중이다. 출시 결함이 아니라 내 캡처 타이밍이었다.

실루엣 림라이트도 다시 밟지 않았다. 프로필의 `rim`이 네 역할 전부 0.0인 건 배선만 되고 안 쓰인 게 아니라 S214가 측정 후 끈 것이다("원화 알파 가장자리가 흐려 림라이트가 지직거리는 노이즈로 변한다").

### 남은 관찰
힌트 띠(4초 지속)가 "전장 관측" 카드와 적 이름을 잠시 가린다. 전투 화면 중앙에는 큰 빈 대역이 있으므로 힌트를 그쪽으로 내리면 충돌이 없어지지만, 배너 텍스처가 상단용으로 그려져 있어 이번에는 건드리지 않았다.

### 검증
- 여섯 교전 콘택트 시트를 수정 전후로 렌더해 육안 확인. 카이로스는 검은 덩어리에서 법의를 입은 인물로, 파수꾼은 방사형 구조로, 애시 크롤러는 마디 달린 벌레로 바뀌었다. 어느 것도 날아가지 않았다.
- 첫 전투 화면 전체 재확인: 힌트는 상단 띠, 덱 8칸 전부 노출, 적 형태 판독 가능.
- 스모크 37/37 통과. VN 검증 20파일 504스텝 0오류. `git diff --check` 통과.

### 방법론
- 계측기를 두 번 버렸다. 판별 기준은 **같은 화면 안의 내부 대조군**이었다 — 아군이 적보다 14배 안 보인다는 결과가 눈과 정면으로 어긋났기에 계측기를 의심할 수 있었다. 대조군이 없었다면 그 숫자를 그대로 믿었을 것이다.
- 가설마다 **수치 예측을 먼저 적었다**. `edge_softness` 건은 "RMS가 0.06 근처까지 오른다"는 예측이 6%로 빗나가 기각됐고, `plate_modulate` 건은 "배수만큼 오른다"는 예측이 ×1.44로 맞아 채택됐다. 예측을 적지 않았다면 6% 상승도 "개선"으로 포장할 수 있었다.
- 원인을 좁힐 때 **후보를 하나씩 실측으로 죽였다**(원화 → 축소 → 렌더). 추측으로 건너뛰었다면 밉맵 필터를 만지느라 시간을 썼을 것이다.

---

## S240 - 2026-08-10 (The remaining problem did not exist; per-pixel rim tried, matched, reverted)

S239가 남긴 "정공법"(`hint_screen_texture`로 실루엣 뒤쪽을 픽셀 단위로 읽기)을 실제로 구현하고 19개 맵에서 측정했다. **결론: 그 작업은 필요 없었고, 애초에 남은 문제가 없었다.**

### 구현하고 측정한 것
셰이더가 실루엣 바깥 방향으로만 화면을 표본해(그 방향의 텍스처 알파가 비어 있을 때만) 배경 휘도를 넓게 평균 내고, 그 값으로 밝은 테를 켜거나 끈다. 캐릭터는 대기 오버레이(레이어 3~5)보다 먼저 그려지므로 여기서 읽히는 것은 겹칠 이전의 지면이다.

문턱을 화면 휘도 기준으로 다시 잡아야 했다(캔버스 평균 기준 0.22 → 화면 기준 0.38). 그 값에서 코어 10개 맵은 **개선 7 / 잡음 3 / 악화 0**, 2회 재현. 즉 **S238의 전역 방식과 정확히 같은 결과**다.

### 일반화 시험
전역 문턱 0.22가 0.217과 0.229 사이 폭 0.012에 놓여 있어 두 표본에 과적합된 것 아닌가 의심했다. 튜닝에 쓰이지 않은 **선택 기억 장소 아홉 곳**으로 시험했다.
결과: 전역 **9/9 개선, 악화 0** / 픽셀 단위 **9/9 개선, 악화 0**. 차이는 잡음 범위였다(belt_signal_yard 1.31 vs 1.22, waste_grey_caravan 1.00 vs 1.09).
과적합 우려는 현실화되지 않았다. 전역 문턱은 19개 맵 전부에서 잘 일반화된다.

### 그래서 되돌렸다
픽셀 단위 방식은 캔버스 셰이더에서 화면 텍스처를 읽으므로 매 프레임 백버퍼 복사를 강제한다. **19개 맵에서 측정상 이득이 없는데 비용만 는다.** 셰이더 변경을 되돌렸다. 계측기의 맵 목록 확장(10 → 19)만 남긴다.

### 정정: S238과 S239의 서술이 틀렸다
S238 커밋에 "전역 밝기로는 rim_forest와 belt_waystation을 가르지 못해 이득 하나를 포기했다"고 썼는데, **문턱 0.22는 0.217과 0.229 사이에 있어 정확히 갈라낸다.** belt는 테가 켜진 채 1.31x를 유지했고 포기한 이득은 없었다. 같은 항목의 최종 표가 이미 그 사실을 보여 주고 있었는데 서술만 잘못 남았고, S239가 그 잘못된 서술을 "남은 것"으로 이어받았다. 두 항목에 정정 주석을 달았다.

### Verification
- 계측기 맵 목록을 10개에서 19개로 넓혔다. 두 방식을 같은 조건에서 비교한 근거가 여기서 나왔다.
- 스모크 **37/37 통과**. VN 20파일 0에러. `git diff --check` 클린.

### 배운 것
"남은 것"을 로그에 적을 때는 그 근거가 같은 항목의 측정 결과와 일치하는지 확인해야 한다. 이번 건은 표가 맞고 문장이 틀렸는데, 그 문장이 두 세션의 작업 방향을 정했다.

## S239 - 2026-08-10 (Optional sites join the atmosphere budget; local rim sampling fails and is reverted)

S238이 남긴 항목(테가 배경을 픽셀 단위로 알게 하기)과 그래픽 확장을 함께 진행했다. 하나는 실패해서 되돌렸고, 하나는 성공했다.

### 실패: 발밑 지면 표본
rim_forest(지면 0.229)와 belt_waystation(0.217)이 전역 평균으로는 구분되지 않으니, 배우가 **실제로 선 자리**를 표본하면 둘 다 살릴 수 있으리라 보았다. 맵 캔버스를 64x64로 줄여 들고 위치로 표본하는 방식을 구현했다.

**결과는 더 나빴다.** belt_waystation의 테가 꺼지면서, 계측기가 바로 그 위치에서 확인했던 1.31x 이득을 잃었다. 즉 "증명되지 않은" 정도가 아니라 **측정된 지점에서 틀린 판단**을 내렸다.

원인 추정: 캐릭터 실루엣은 발밑 타일이 아니라 화면상 몇 타일 **뒤쪽** 지면을 배경으로 놓고 보인다. 발밑을 표본하면 엉뚱한 영역을 읽는다. 되돌리고 전역 방식을 유지했다.

### 성공: 선택 기억 장소 아홉 곳
`optional_memory_site`는 손으로 맞춘 오버레이만 남아 있던 마지막 맵이었다. 비네트 + 주변광 + 패럴랙스뿐이고 그레이딩도, 안개도, 깊이도, 렌즈도 없었다. 코어 10개 맵이 S236에서 대기 예산을 받는 동안 여기만 빠져 있었다.

각 장소는 이미 `ambient`(바탕색)와 `accent`(빛의 색)를 선언하고 있어서, 새 숫자를 만들지 않고 그대로 정체성으로 넘겼다. mood만 바이옴별로 정했다(씨앗 금고 0.88이 가장 짓눌린 곳). 무색 카라반은 채도 0.15를 유지한다.

**대비도 9/9 개선 (1.61x ~ 2.56x), 휘도는 약 3배.**
bl07_seed_vault 2.56x · forest_name_hollow 2.00x · belt_signal_yard 1.96x · waste_grey_caravan 1.95x · drift_waymarker_shrine 1.92x · verdan_ledger_cellar 1.83x · seam_lantern_ward 1.82x · coast_cinder_harbor 1.69x · rim_root_hollow 1.61x.
휘도 0.032~0.142 → 0.159~0.368. 코어 맵보다도 어두웠던 곳들이다.

### Verification
- 신규 `capture_site_survey`로 아홉 장소 콘택트 시트를 뽑아 육안 확인: 장부 지하실의 따뜻한 서고, 등불 병동, 회색 카라반의 창백한 황무지, 씨앗 금고의 자주색이 각자 선언한 색을 지키면서 형태가 읽힌다.
- 스모크 **37/37 통과**. 코어 맵 서베이 재렌더 이상 없음. VN 20파일 0에러. `git diff --check` 클린.

### 남은 것
(S240에서 확인: 이 문제 자체가 없었다. 아래 S240 항목 참고.)

## S238 - 2026-08-10 (The rim learns what it stands on, and S237's numbers are retracted)

S237의 "다음 작업"으로 시작했다. 결과적으로 **S237의 계측 결과를 철회**하고, 계측기를 다시 만든 뒤, 그 위에서 테를 재조정했다.

### S237 계측기는 캐릭터를 재고 있지 않았다
S237은 "캐릭터 있는 화면 − 없는 화면"의 차이로 마스크를 만들었다. 마스크 진단을 붙여 보니:
외접 상자가 **모든 맵에서 표본 상자 전체(140x220)**, rim_forest는 채움 **1.00**, 마스크 크기 최대 **30,796px**. 캐릭터는 2,000~4,000px다. 렌즈 셰이더 맥동·안개·흔들리는 초목이 전부 "캐릭터"로 잡힌 것이다.
따라서 S237이 보고한 gain(평균 1.22x, forgotten_forest 0.86x 예외)은 신뢰할 수 없다. 해당 로그 항목에 정정 주석을 달았다.

### 새 계측기: 렌더가 아니라 알파에서 마스크를 얻는다
스프라이트의 현재 프레임 텍스처에서 알파를 읽어 경계 텍셀을 찾고, 그 텍셀과 바깥 이웃을 각각 화면 좌표로 옮긴다. 배경이 무엇을 하든 마스크는 변하지 않는다 — **표본 수가 모든 맵에서 정확히 310개**로 고정된다.
여기에 두 가지 통제를 더했다:
- **동일 조건 대조군.** 같은 설정으로 두 장을 찍어 그 편차를 잡음 바닥으로 삼는다 (맵당 1.02~1.13x). 이걸 안 하면 배경 표류를 효과로 오해한다.
- **끼워 찍기.** 켠 화면을 끈 화면 두 장 *사이에* 찍고 앞뒤 평균과 비교한다. 앞의 한 장하고만 비교하면 신호가 대조군보다 두 배 긴 시간 간격을 갖는다.

### 알아낸 것
- 밝은 테는 **어두운 지면에서만** 이득이다. 지면이 가장 밝은 colorless_waste(휘도 0.342)에서는 0.89x로 손해였다.
- **중간 세기가 가장 나쁘다.** belt_waystation은 테를 온전히 주면 1.31x인데 절반(0.24)만 주면 **0.94x**로 나빠진다. 약하게 섞인 테는 그 밝기가 배경 근처에 앉아, 원래 어두운 바깥 테가 갖고 있던 대비까지 지운다. 그래서 세기를 서서히 줄이지 않고 **켜거나 끈다**.
- 전역 지면 밝기는 rim_forest(0.229, 손해)와 belt_waystation(0.217, 이득)을 가르지 못한다. 문턱을 0.22로 잡아 이득 하나 대신 손해 없는 쪽을 택했다.
  > **S240 정정.** 이 문장은 틀렸다. 0.22는 0.217과 0.229 **사이**에 있어 두 맵을 정확히 갈라낸다. belt는 테가 켜진 채 1.31x를 유지했고 포기한 이득은 없다. 같은 항목의 최종 표(belt rim 0.42 / 1.31x, rim_forest rim 0.00 / 1.00x)가 그 사실을 이미 보여 주고 있었는데 서술만 잘못 남았다.

### Done
- `add_map_canvas`가 지면 텍스처의 평균 휘도를 한 번 재서 `FieldActorVisuals.set_ground_luma()`로 알린다. 이미 만들어진 배우들도 함께 갱신된다.
- 밝은 테는 지면 휘도 0.22 미만에서만 켜진다. 중간값은 없다.

### Verification
**개선 7 / 잡음 수준 3 / 악화 0.**
the_seam 3.29x · bl07_void 2.23x · drift_shelter 2.14x · verdan_market 1.65x · seam_outskirts 1.45x · crumbling_coast 1.44x · belt_waystation 1.31x.
rim_forest·colorless_waste는 테가 꺼져 1.00x, forgotten_forest는 1.08x(잡음 바닥 1.13 이내).
스모크 **37/37 통과** (켜짐/꺼짐 이분 계약 추가). VN 20파일 0에러. `git diff --check` 클린.

### 남은 것
rim_forest와 belt_waystation은 지면 밝기가 거의 같은데 반대로 반응한다. 전역 값으로는 여기까지가 한계다. 둘을 모두 살리려면 `hint_screen_texture`로 뒤 배경을 픽셀 단위로 읽어야 한다. 이제 그 판단을 내려 줄 계측기가 있다.
> **S240 정정.** 위 "남은 것"은 존재하지 않는 문제였다. 문턱 0.22가 이미 두 맵을 갈라내고 있었다.

## S237 - 2026-08-10 (Silhouette rim, on a measurement that finally holds)

S236에서 가독성 패스를 시도했다가 계측을 세 번 실패해 되돌렸다. 이번에는 **계측기부터** 다시 만들었다.

### 계측기 (`probe_field_legibility`)
같은 장면을 캐릭터가 **있을 때와 없을 때** 두 번 렌더한다. 두 그림의 차이가 곧 캐릭터가 화면에서 차지한 정확한 마스크다. 그 마스크의 경계 픽셀에서 "캐릭터가 그린 색"과 "그 자리에 원래 있던 배경색"을 같은 좌표로 비교한다. 텍스처 좌표 계산도, 배경 추정도 없다.

첫 시도는 경계 픽셀이 수만 개로 나왔다. 렌즈 셰이더의 TIME 맥동과 흐르는 안개가 마스크에 섞인 것이다. 두 가지를 더했다:
- 마스크를 플레이어 주변 상자로 한정 (경계 픽셀 수만 → 728~6,287).
- 절대값 하나를 믿는 대신, **같은 실행 안에서 테두리를 끈 화면과 켠 화면을 연속 프레임으로 찍어 짝지어 비교**. 배경 흔들림은 두 조건에 똑같이 들어가므로 비교에서 상쇄된다.

**계측기 검증(먼저).** 동일 코드 2회 실행: 짝지은 gain의 흔들림 최대 16.9%. 크기는 참고용이지만 **부호는 10개 맵 전부에서 안정적**이다. 앞선 세 계측기는 이 조건을 만족하지 못했다.

### Done
- **듀얼 림.** 바깥은 어둡게(밝은 지형에서 떼어 냄), 안쪽은 밝게(어두운 지형에서 떠오름). 어두운 한 겹만으로는 어두운 맵에서 실루엣이 사라진다.
- 계약을 셰이더 기본값이 아니라 `FieldActorVisuals.apply_finish`가 직접 싣는다. 기본값에 기대면 조용히 어긋난다.

### Verification
- **짝지은 gain 평균 1.22x.** 개선 7/10: belt 1.42/1.18, drift 1.54/1.49, coast 1.51/1.55, seam 1.38/1.25, void 1.31/1.26, verdan 1.28/1.18. 중립 2 (rim_forest 1.05, outskirts 1.00/1.06).
- **악화 1: forgotten_forest 0.81/0.91.** 두 실행 모두 일관되게 나빠졌다. 이 맵은 지면이 창백해서 밝은 안쪽 테와 싸운다. 고정 밝은 테는 고정 어두운 테와 같은 종류의 한계를 갖는다.
  > **S238 정정.** 이 문단은 틀렸다. 여기 쓰인 계측기는 "캐릭터 있는 화면 - 없는 화면"으로 마스크를 만들었는데, 진단해 보니 그 마스크가 캐릭터가 아니었다(모든 맵에서 외접 상자가 표본 상자 전체, rim_forest는 채움 1.00). 배경 애니메이션이 통째로 차이로 잡힌 것이다. forgotten_forest의 지면 휘도는 0.152로 창백하지 않으며, 알파 마스크 계측기로 다시 재면 1.08~1.18x로 **개선**된다. 실제로 손해를 보던 맵은 지면이 가장 밝은 colorless_waste(0.342)였다. 이 세션의 gain 수치는 신뢰하지 말 것.
- 같은 프레임 확대 A/B 육안 확인: 절제돼 있고 스티커 외곽선처럼 보이지 않는다. drift_shelter에서 특히 잘 떠오른다.
- 스모크 **37/37 통과**. `smoke_ambient_life`에 실루엣 두 겹 계약 추가 (`rim_split=0.84`). VN 20파일 0에러. `git diff --check` 클린.

### 다음
forgotten_forest의 역행은 근본적으로 "테 색이 배경을 모른다"는 문제다. 제대로 고치려면 `hint_screen_texture`로 뒤 배경을 픽셀 단위로 읽어 밝기에 따라 테 색을 뒤집어야 한다. 이번에는 넣지 않았다 — 계측기가 이제 그 판단을 내려 줄 수 있으니 별도로 시도할 것.

## S236 - 2026-08-10 (Atmosphere budget: the map art was buried under its own overlays)

"그래픽, 움직임을 대폭 개선"을 받아, 맵별 이펙트 개수를 세는 대신 전 맵을 같은 조건으로 렌더해 콘택트 시트를 만들었다(`capture_map_survey`). 이펙트는 맵당 13~24개로 이미 고르게 깔려 있었다. 문제는 부족이 아니라 과잉이었다.

### Audit findings
- **rim_forest만 읽히고 나머지 아홉 맵은 단색 안개였다.** 놀이 공간만 잘라 재 보니 대비도가 rim_forest 대비 43~53%, bl07_void는 34%. 평균 휘도 0.11~0.19 대 rim_forest 0.39. 의뢰해서 그린 맵 캔버스가 안 보였다.
- **원인은 premium_lens 셰이더였다.** 이 셰이더는 `tint_color`를 단색으로 칠하고 알파를 `vignette + letterbox + tint_strength + shaft`의 **합**으로 정한다. verdan_market은 그 합이 **0.635** — 화면의 63%가 단색 한 겹이었다. rim_forest만 0.303이라 유일하게 읽혔다.
- **그리고 그 단색 겹칠이 이 맵들의 주 광원이기도 했다.** 맵은 `add_ambient_lighting`(CanvasModulate)으로 0.38~0.66까지 어둡게 곱한 뒤, 렌즈 단색으로 밝기를 되찾고 있었다. 단색을 덮으면 밝아지지만 그만큼 명암 차이가 지워진다.
- 필드 위협은 "사냥한다"고 말하면서 한 발도 움직이지 않았다.

### Done
- **대기 예산(`MapEffects.apply_atmosphere`).** 맵은 이제 정체성만 선언한다: 바탕색(hue), 빛의 색(light), 짓눌림 정도(mood), 채도(saturation). 여섯 겹 오버레이에 얼마를 쓸지는 예산이 한 번만 정한다. 흩어져 있던 수십 개의 매직 넘버가 사라졌다.
- **핵심 규칙: 밝기는 곱셈에서, 분위기는 겹칠에서.** 밝기를 CanvasModulate로 옮겨(명암 관계 보존) 렌즈 단색 겹칠은 총 0.33 이하로 묶었다. 빛의 색은 바탕색과 별개로 맵이 소유한다(보라색 방에 드는 호박색 광선은 정상이다).
- **필드 위협이 움직인다.** 자기 자리 주변을 느린 8자 궤도로 돌고(실측 19~21px), 플레이어를 알아채면 그쪽으로 기운다. 압박이 오를수록 궤도가 좁아진다. 접촉 반경과 인카운터 규칙은 건드리지 않았다.

### Verification
- **대비도 9/10 맵 개선** (1.01x ~ 1.41x). 상세: colorless_waste 1.41x, forgotten_forest 1.40x, belt_waystation 1.32x, rim_forest 1.19x, crumbling_coast 1.17x, verdan_market 1.14x, drift_shelter 1.10x, bl07_void 1.09x, the_seam 1.01x. seam_outskirts만 0.95x로 남았다.
- 각 맵의 색 정체성은 유지됐다(숲 초록, 시장 호박, 셸터 청색, 외곽 보라, 황무지 무채, 보이드 자주). 콘택트 시트를 육안으로 확인: 시장 좌판·등불, 벨트 광장 문양, 더 섬의 계단과 꽃, 잊힌 숲의 거대 뿌리가 모두 다시 보인다.
- **계측기 자체를 먼저 검증했다.** 같은 코드로 두 번 렌더해 흔들림 최대 2.3%를 확인한 뒤에야 10~30% 변화를 판단 근거로 썼다.
- 스모크 **37/37 통과**. `smoke_ambient_life`에 위협 순찰 계약 추가(실제 렌더러 2회 연속 통과). VN 20파일 504스텝 0에러. `git diff --check` 클린.

### 시행착오 기록 (같은 함정을 다시 밟지 않기 위해)
1. 처음에는 오버레이를 "어둡게 만드는 것"으로 가정하고 값을 줄였다. 결과는 **더 어두워졌다**. 셰이더를 읽고 나서야 단색 겹칠이 광원 역할을 하고 있었음을 알았다.
2. 그 사이 두 가지 실수를 스스로 만들었다가 되돌렸다: 원래 `add_fog`를 쓰지 않던 맵에 안개를 새로 얹은 것, 그리고 truncated grep 출력을 보고 the_seam의 색을 보라에서 갈색으로 바꾼 것. 원본 값을 git에서 다시 읽어 복구했다.
3. 위협 기울기 검사를 한 순간의 좌표로 하려다 실패했다. 순찰이 진동이라 위상에 따라 답이 뒤집힌다. S235의 발 미끄러짐 때와 같은 함정이어서, 궤도와 무관한 "이번 프레임의 목표"를 읽는 방식으로 바꿨다.

### 가독성 패스: 시도했고, 수렴하지 못했고, 넣지 않았다

이어서 실루엣 가독성 패스를 시도했다. 진단까지는 확실했지만 **개선을 입증하지 못해 변경을 되돌렸다.**

**측정된 진단 (`probe_field_legibility`).** 몸통과 배경의 휘도 차이:
`rim_forest 0.128` / `verdan 0.096` / `colorless 0.063` / `belt 0.053` / `coast 0.043` / `the_seam 0.043` / `outskirts 0.039` / `drift 0.038` / **`bl07_void 0.0083`** / **`forgotten_forest 0.0014`**.
가장 어두운 두 맵에서 캐릭터는 배경과 사실상 같은 밝기다. 원인은 `field_actor_finish` 셰이더의 외곽선이 거의 검정(0.025, 0.035, 0.060) 고정이라는 것. 밝은 숲길에서만 듣고, 어두운 맵이 대부분인 이 게임에서는 아무 일도 하지 않는다.

**시도한 해법.** 맵 밝기를 셰이더에 알려 주는 대신 실루엣이 스스로 두 방향을 감당하게 했다: 바깥 한 겹은 어둡게, 안쪽 한 겹은 밝게. 배경이 어느 쪽이든 한쪽이 대비를 만든다.

**되돌린 이유.** 계측을 세 번 시도했고 세 번 다 실패했다.
1. 몸통-배경 평균 차이 — 테두리는 1~2px만 바꾸므로 평균이 거의 반응하지 않는다. 측정 대상이 틀렸다.
2. 가장자리 기울기 — 주변 배경 소품까지 훑어서 오염됐다. bl07_void가 최고값(0.46)으로 나오는 등 값이 신뢰할 수 없었다.
3. 맵에서의 확대 A/B — 파티클과 안개가 프레임마다 달라 셰이더 차이를 가렸다. 고정 배경 격리 씬으로 다시 시도했지만 ExplorationHUD 오토로드가 화면을 덮어 못 썼다.

`forgotten_forest`가 0.0014 → 0.0345로 오른 것은 확인했지만, 같은 계측에서 다른 맵들은 무작위로 오르내렸다(belt 0.053→0.033). 계측을 믿을 수 없는 상태에서 시각 변경을 넣는 것은 "개선했다"가 아니라 "바꿨다"일 뿐이므로 셰이더를 되돌렸다. 진단과 계측 도구는 남긴다.

**다음에 할 것.** 캐릭터 알파 마스크를 이용해 실루엣 경계 픽셀만 정확히 표본하는 계측기를 먼저 만들 것. 배경 소품이 섞이지 않아야 판단이 선다.

### Intentionally not done
- seam_outskirts는 아직 기준 대비 0.95x다. mood를 0.52에서 0.40으로 낮췄지만 완전히 회복되지 않았다.

## S235 - 2026-08-10 (Ambient character motion: gait sync, living population)

"캐릭터 움직임을 더 자연스럽게"를 받아, 추측 대신 계측기(`probe_ambient_motion`)로 먼저 재고 시작했다. 플레이어와 동행자의 이동(가속/선회/거리 기반 발소리/빵부스러기 추종)은 S218에서 이미 정교하게 다듬어져 있었다. 문제는 다른 곳에 있었다.

### Audit findings
- **배회 NPC의 발이 지면을 긁고 있었다.** 이동은 SINE 가감속 트윈이라 실제 속도가 0에서 평균의 1.57배까지 오르내리는데, 다리 회전(`speed_scale`)은 평균 한 값으로 고정돼 있었다. 실측: 123프레임 동안 보폭 변화 0회, 평균 미끄러짐 10.2 px/s. 걷기 시작과 끝에서는 몸이 거의 멈춘 채 다리만 돌았다.
- **월드 인구 NPC 전원이 굳어 있었다.** 3초 관측에서 5명 전부 0.00px 이동, ambient 드라이버 0/5, 호흡 없음. 19개 맵의 주민 84명이 단일 정지 삽화였고, 움직이는 것은 베르단 시장의 배회 NPC뿐이었다(`add_npc_wander`는 그 맵에서만 호출된다).
- **드라이버의 진폭이 조용히 무효화되고 있었다.** `AnimatedSprite2D.offset`은 로컬 공간이라 `scale`이 다시 곱해진다. 모든 필드 배우는 표시 높이 50px에 맞춰지므로 스케일이 원본 그림 크기에 따라 0.04~0.34로 벌어진다. 코드의 `0.18`은 누군가에게는 화면상 0.18px, 누군가에게는 0.02px였다. 즉 숫자상으로는 "호흡함"이지만 눈에는 보이지 않았다.

### Done
- **보폭을 해석적으로 맞춘다.** 위치를 미분해 속도를 구하면 구조적으로 한 프레임 늦고, 떨림을 막으려 필터를 걸면 지연이 더 붙는다. 이동 곡선은 우리가 정했으므로 속도는 계산하면 된다. SINE 가감속의 위치가 `D(1-cos(pi t/d))/2`이므로 속도는 `D pi/(2d) sin(pi t/d)`다. 지연도 필터도 없다. 걸음이 끝나면 동기화가 떨어지고 `speed_scale`이 1.0으로 복구된다.
- **서 있는 사람에게 숨을 넣는다.** 월드 인구 NPC가 기존 ambient 드라이버(호흡/무게 이동/접지 보정)를 단다. 걸어 다니게 만드는 것이 아니라 서 있는 자세를 살린다.
- **알아채게 한다.** 플레이어가 96px 안으로 들어오면 몸을 돌리고, 132px 밖으로 나가면 원래 방향으로 돌아온다. 두 반경이 달라 경계에서 깜빡이지 않는다. 감지는 0.2초 주기다. 네 방향이 모두 같은 그림인 단일 삽화 NPC는 없는 자세를 지어내는 대신 좌우 반전으로 돌아본다.
- **진폭을 지면 픽셀 기준으로 되돌린다.** 드라이버가 스프라이트 스케일의 역수를 보정으로 들고, 진폭 상수를 "몸 높이 50px 대비 픽셀"로 읽는다. 이제 스케일이 0.04이든 0.34이든 같은 픽셀만큼 움직인다.

### Verification
- 실측 비교(실제 렌더러 1280x720 및 640x360):
  - 발 미끄러짐 평균 **10.2 px/s → 0.4~1.7 px/s**, 최악 프레임 29 → 2~9. 보폭 변화 0회 → 매 프레임.
  - 서 있는 호흡 **0.02~0.18 렌더 px → 전원 1.32 렌더 px** (스케일 무관).
  - 월드 인구 드라이버 0/5 → 5/5.
- 신규 `smoke_ambient_life`: 보폭 동기화 부착/해제, 정지 시 `speed_scale` 복구, 드라이버 부착, 호흡 폭(지면 픽셀 기준 상·하한), 인지 반경 이력, 단일 삽화 좌우 반전을 검사. 실제 렌더러 3회 연속 통과.
  - 미끄러짐 검사는 실제 프레임 시간에서만 의미가 있다. 헤드리스는 최대 속도로 돌아 delta가 무의미하므로, 조용히 통과시키는 대신 `mean_footslip=not_measured(headless)`로 밝히고 구조 계약만 검사한다.
- **스모크 37/37 통과.** `MOVEMENT_NATURALISM_SMOKE_PASS`와 `VISUAL_CLARITY_SMOKE_PASS`(필드 배우 표시 높이 0.5px 계약 포함) 모두 유지.
- 실제 OpenGL 캡처 재확인: `verdan_field_motion.png`, `rim_forest_first_exploration.png`, `verdan_malet_field.png`(말렛이 플레이어 쪽으로 돌아선 `idle_left`), `hybrid_battle_stage.png`. VN 검증 20파일 504스텝 0에러. `git diff --check` 클린.

### Intentionally not done
- 필드 위협(`FieldThreat`)은 여전히 제자리에서 맥동만 한다. 순찰하게 만드는 것은 움직임 자연스러움이 아니라 인카운터 설계 변경이라 범위 밖으로 두었다.
- 전투 배틀러의 공격 모션(준비 스쿼시 → 돌진 → 임팩트 정지 → 오버슛 복귀)은 이미 애니메이션 원칙대로 구성돼 있어 손대지 않았다.

## S231-234 - 2026-08-10 (Memory economy: preservation build, loan, cascade, Korean memory text)

네 가지 게임성 업그레이드를 순차 진행했다. 전부 새 시스템을 얹은 것이 아니라, 이미 있던 것들 사이에 뚫려 있던 구멍을 메운 작업이다.

### S231 — 보존 플레이에 진행축을 준다

**발견.** 연소 패시브의 첫 문턱은 5회(`PASSIVE_THRESHOLDS`), The Weave(제3의 길)는 총 연소 4회 미만(`WEAVE_MAX_BURNS`)을 요구했다. 두 숫자가 겹치지 않아서 **보존을 택한 플레이어는 게임 전체에서 패시브를 하나도 얻지 못했다.** 연소는 동시에 주 딜 수단이고 적 HP/ATK는 챕터마다 성장하며, 침식은 대항 수단 없이 매 챕터 깎았다. 주제적으로 가장 풍부한 선택이 기계적으로는 모든 면에서 열등한 빌드였다.

**한 일.**
- **기억 파수(Anchor Vigil).** 연소 카운터의 거울. 챕터를 넘길 때 온전한 앵커 하나당 2점, 이름(`core_name_origin`)이 온전하면 +3점이 쌓인다. 문턱 8/20/36/56/80에서 패시브 5종이 열린다.
- 다섯 패시브 모두 실제 수치에 연결했다. 선언만 있는 패시브는 두지 않았다: 고요한 집중(WITNESS 요구 -1), 흔들리지 않는 손(비연소 공격의 BREAK 압력 ×1.3), 무뎌지지 않는 날(보이드수 상대 일반 공격 감쇠 0.3 → 0.55), 나눠 진 무게(엘리아 기술 쿨다운 -1), 깊은 닻(온전한 앵커당 받는 피해 -3%).
- **기억 고정(Erosion Guard).** 챕터마다 슬롯 2개, 그레인으로 지불. 침식을 되돌리고 다음 한 번을 막는다. 전부는 못 지킨다는 점이 요점이다. 서고에 버튼과 남은 슬롯, 못 누르는 이유까지 표시한다.
- 침식/파수 적립을 챕터당 한 번으로 고정했다. `add_chapter_memories`는 맵 진입에서도 불려서, 같은 챕터 맵을 다시 밟으면 침식이 또 적용되던 잠재 버그가 있었다.
- 서고에 두 진행축을 나란히 표시한다. 연소 패시브조차 해금 토스트로만 존재해 플레이어가 자기 빌드를 볼 수 없었다.

### S232 — 기억 대출 (GDD 2.4, 미구현이던 설계)

**발견.** GDD 2.4의 세 거래 중 판매/추출만 구현돼 있고 대출은 코드 어디에도 없었다. 연소 결정에는 "태운다 / 안 태운다" 둘뿐이라 시간 축이 없었다.

**한 일.** 기억을 담보로 잡고 지금 그레인을 받는다. 기한 2챕터, 이자 40%. 갚으면 담보가 풀린다. **기한을 넘기면 관리국이 강제 추출한다.** 이때 기억은 사라지지만 연소 위력도, 패시브 적립도 없다 (`check_unlock_passives`가 `get_voluntary_burn_count`를 쓴다). 스스로 태우는 것과 빼앗기는 것의 차이가 이 시스템의 요점이다. 담보로 잡힌 기억은 연소/판매 후보에서 빠지고 두 연소 경로 모두 거부한다. 상점에 담보 대출 탭을 추가했다.

### S233 — 기억 별자리를 실제 구조로

**발견.** 기억 그래프(`connections`, `burned_neighbor_count`, MemoryConstellation)는 S62부터 있었지만 실제 메카닉 용도는 연쇄 연소 +20% 하나뿐이었다. 그래서 "무엇을 태울까"는 사실상 "제일 싼 걸 태운다"였다.

**한 일.** 태우면 이어진 기억이 함께 침식된다(등급별 2/4/7/11). 엘리아 관련 기억은 서로 묶여 있으므로 그 묶음을 건드리면 묶음 전체가 상하고, 외딴 기억은 싸다. 강제 추출은 1.5배로 번진다. 고정해 둔 기억은 연쇄도 한 번 막고 보호를 소모한다. 핵심 기억은 침식과 같은 규칙으로 면역이다. **연소 확인창이 커밋 전에 무엇이 함께 상하는지 이름으로 예보한다** (S226의 "숨은 대가를 만들지 않는다" 원칙).

### S234 — 기억 텍스트 한국어화

**발견.** S226이 "데이터 모델 변경이라 범위 밖"으로 명시적으로 미뤄 둔 항목. 이 게임의 중심 결정은 "무엇을 태울 것인가"인데, 한국어판에서도 기억의 제목과 설명만 영어였다. 연소 확인창, 전투 로그, 서고, 상점, 관리국 감지 로그까지 전부. 무엇을 잃는지 읽지 못하면 선택의 무게도 없다.

**한 일.** 40개 기억 전부에 한국어 제목/설명/스토리 효과를 썼다(`MEMORY_TEXT_KO`). Memory 클래스에 필드를 늘리는 대신 id로 찾는 표를 둬, 기존 40개 정의를 건드리지 않고 새 기억은 한 줄만 더하면 되게 했다. 표시 함수 세 개를 통해 서고/상점/확인창/연소 목록/전투 로그/관리국 로그/도감/저널/세계 재기록이 모두 같은 경로를 지난다. 서고 UI 문구(제목, 등급 탭, 상태 표시, 요약, 하단 바)도 함께 한국어화했다. 표가 없는 기억은 영어로 떨어질 뿐 빈 문자열이 되지 않는다.

### 그 외 고친 것
- `_play_power_milestone`이 CanvasLayer의 `modulate`를 트윈하려 했다. CanvasLayer에는 그 속성이 없어 `tween_property`가 null을 돌려주고 페이드아웃과 `queue_free`가 통째로 사라졌다. 결과적으로 5/10/20/30/50번째 연소마다 "POWER AWAKENED" 오버레이가 화면에 영구히 남았다. 페이드 대상 Control을 두고 그 아래에 전부 매달았다.
- `SystemLog`에 임의 문장을 올리는 `show_log()`를 추가했다. 담보 등록과 강제 추출이 연소 감지와 같은 창을 쓴다.

### Verification
- **스모크 36/36 통과** (기존 32 + 신규 4). 신규: `BATTLE_ANCHOR_TRACK_SMOKE_PASS preservation_vigil=90 preservation_passives=5/5 burns=0 weave=1`(예전에는 패시브 0개), `MEMORY_LOAN_SMOKE_PASS term=2 interest=0.40 extracted_gives_power=0 collateral_burnable=0`, `MEMORY_CASCADE_SMOKE_PASS grades=5 extraction_mult=1.5 preview=named guard=absorbs core=immune`, `MEMORY_LOCALIZATION_SMOKE_PASS memories=25 translated=40 synthesis_ko=4`.
- 실제 OpenGL 1280x720 캡처 육안 확인: `burn_preview_stakes_ko.png`(한국어 기억 텍스트 + 이름이 적힌 연쇄 예보), 신규 `memory_archive_ko.png`(두 진행축, 고정 버튼, 한국어 서고). `capture_hybrid_battle` 회귀 없음.
- VN 검증 20파일 504스텝 0에러. 한국어 로컬라이제이션 31파일 1,592필드 0에러. `git diff --check` 클린. 커밋/푸시 없음.

## S230 - 2026-08-10 (Typography system and battle-screen interface pass)

### Audit findings
- **Every glyph in the game was rendering from the thinnest master.** Both bundled faces are variable fonts whose default instance is the lightest weight (`NotoSansKR-VF` wght 100 Thin, `NotoSerifKR-VF` wght 200 ExtraLight). `UITheme` and `theme.tres` compensated with `variation_embolden` (synthetic bold), which pushes glyph outlines outward uniformly and fills in the tight counters of Hangul. Korean UI text looked smeared rather than bold.
- Documentation and code disagreed: `assets/fonts/README.md` claimed prose used the serif, while `BODY_FONT_PATH` pointed at the sans. There was no serif/sans role split in practice.
- Text sizes were ad hoc (12/13/14/15/17/20 chosen per call site) with 105 labels below 13px.
- A real-render measurement of the battle screen (new `probe_battle_layout`) found **ten overlapping HUD regions, three of which clipped text**: the combat cue covered the enemy name's first glyph (`그림자 파수꾼` → `림자 파수꾼`), the objective plate art covered the Sable row's label (`세이블:` → `이블:`), and the player status row sat on the command-deck art, cutting the party tag.
- Elia's technique rail and Tobias's command rail were anchored to **exactly the same band** (0.225–0.27), so with the full party they drew on top of each other. Four party rails, the combat cue, the turn strip, and the combo readout all competed for the same top-centre space.
- The enemy readout's declared height (70px) was less than its content (~118px), so it silently overflowed and the status chips positioned "below" it landed on the BREAK gauge instead.
- Party order buttons were English-only in Korean builds, and their selection highlight compared `btn.text.to_lower()` to the action id — that would have broken the moment the labels were localized.
- Status chips were hard-coded English (`SABLE`, `GUARD`, `BROKEN`, `POISON`, `Weak: VOID`).
- Two field-HUD readability defects surfaced in the same render pass: the minimap legend was bare text on the forest canvas, and the Field Flow panel dropped its whole `modulate.a` to 0.62 when quiet — the stylebox assertion passed while the copy became unreadable.

### Done
- **Typography.** `UITheme` now requests real variable-font instances via `variation_opentype = {"wght": N}` (UI sans 600, meta sans 500, body serif 500, title serif 600) and never uses synthetic embolden. Added a shared type scale (`SIZE_DISPLAY` 40 → `SIZE_META` 13) plus `style_label` / `style_ui_label` / `style_meta_label` helpers, raised the floors to 20/14/13, and lifted 105 sub-floor labels across 25 files. `theme.tres` carries the same weight axes so controls without explicit overrides match. Story copy is now serif and interface copy sans, as the README always claimed.
- **Battle HUD layout contract.** Added named columns and bands (`HUD_*` constants) with a documented rationale. Cards that used to be fixed bands now size to their content and grow in one declared direction, so no card can silently overflow onto another.
- **Enemy readout.** Rebuilt as one card (y 8–110, clearing the enemy plate at 115.5): name with ellipsis + HP on one row, BREAK label + gauge + read-state on one row, wrapping status chips inside the card. Added a delayed damage-ghost bar so a hit is legible even against an unread `HP ? / ?`, and a `판독됨 / 미판독` chip that explains *why* the numbers are hidden.
- **Arrel's status cluster.** Portrait, name, HP, Limit gauge (now with a numeric `%` / `준비` readout) and status chips are one card at y 438–540 — below the plate's foot (432.6) and above the deck art (545.8).
- **Party orders.** Stance, Elia, Sable, and Tobias rows now stack inside a single card that folds away when empty. Rows are wrapping containers so wider English labels push down rather than sideways into Tobias's plate. All labels localized; selection now keys on a `party_action` meta value. Locked stances render as greyed slots (they previously disappeared: no `disabled` stylebox plus `modulate.a = 0.4`).
- **Transient cues.** The turn banner gained a reading surface tinted by the event colour and moved to the clear mid-stage band; the combo readout moved out from under the combat cue; the auto-battle indicator became a chip beside the speed chip instead of overlapping the turn strip. HP bars no longer sweep up from zero on the first paint.
- **Field HUD.** The minimap legend gained an opaque plate under the map; Field Flow keeps a readable floor (`FLOW_PANEL_MIN_ALPHA` 0.88) and expresses calm through border colour and pulse instead of erasing its own text. `NO FIELD LINK` localized.
- **Tests.** New `smoke_battle_interface` asserts, in both locales and in the most crowded state, that no two HUD cards overlap, that no card covers a battler plate, that every label and command clears the type-scale floor with an opaque disabled surface, and that no Korean string stayed in English. `smoke_text_readability` now checks the *effective* panel alpha (stylebox × modulate) and that weights come from the wght axis rather than embolden. New `probe_battle_layout` prints every HUD rect plus a full-element contact sheet.
- Fixed three stale assertions in `smoke_hybrid_coupling`, which had been failing silently since S228: it hard-coded Arrel's pre-S228 foot position (226 vs 246), compared a live camera projection against a reference-pose value, and demanded `rim_strength > 0` after S228 deliberately set every painterly plate's rim to 0. Each now reads the value from the role contract instead.

### Verification
- Godot 4.6.2 headless parse/import: exit 0, no `SCRIPT ERROR` / `Parse Error`.
- **All 32 smoke scenes pass** (31 existing + the new `smoke_battle_interface`), including `BATTLE_INTERFACE_SMOKE_PASS locales=2 cards=9 battlers=4 type_scale=enforced` and the restored `HYBRID_COUPLING_SMOKE_PASS anchors=4 projected=(246, 424)`.
- `probe_battle_layout` measured overlaps: **10 harmful → 0.** Everything still reported is parent↔child containment or intentional battler depth layering.
- Real OpenGL 1280x720 captures inspected: `battle_layout_probe.png` (all elements on at once), `hybrid_battle_stage.png`, `battle_moments.png` (6 states), `dialogue_interface_ko.png` (`font=NotoSerifKR-VF.ttf`), `title_grandeur.png`, `rim_forest_first_exploration.png`. Crops at 3x confirm open Hangul counters and even strokes — the smeared synthetic bold is gone.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. `git diff --check` clean (only the usual CRLF notices). Nothing staged, committed, or pushed.

## S229 - 2026-08-09 (Recurring-enemy painterly battle art expansion)

### Audit findings
- The archive already had broad Memory Compass, Loss Records, World Rewrite, Part III story-CG, and boss-cut-in coverage. Generating another isolated narrative batch would have duplicated existing beats instead of improving the live game.
- The S228 cinematic battle stage exposed the clearest remaining graphics gap: several repeatable field threats entered the painterly stage through landscape field art, so they appeared smaller and less cohesive than Arrel and the authored bosses.
- A first live OpenGL pass confirmed that one 16:9 image could not serve both the persistent portrait-shaped battler slot and a wide action beat without sacrificing scale or composition.

### Done
- Generated four matched, identity-preserving enemy art pairs from the existing field designs and Memory Eater battle finish: Ash Hound, Signal Wisp, Rootbound Echo, and Void Fragment. Each pair contains a 1024x1536 stage character and a 1672x941 action cut-in, for eight new RGB illustrations total.
- Connected the stage characters to the canonical enemy resolver for all regional aliases (`Ash Bone Hound`, Archive/Seam/Threshold Wisps, and `Hollow Fragment` included). Field-provided landscape art is now intentionally overridden only for these authored S229 combat identities.
- Connected the wide companions to the live hostile action-cut-in resolver while preserving all existing S204 and boss routes. Added all eight images to the existing Artbook and documented their distinct stage/action roles in `ILLUSTRATION_CATALOG.md`.
- Added a focused 2x2 real-render capture tool. It enters each battle through `BattleManager`, waits for the real encounter-intro blackout to clear, asserts the canonical portrait plate and separate wide action cut-in, and records the persistent combat tableau.

### Verification
- Godot 4.6.2 import and headless editor parse: exit 0; fatal diagnostic scan 0. All eight PNG resources import successfully.
- `VISUAL_CLARITY_SMOKE_PASS` and `CINEMATIC_BATTLE_STAGE_SMOKE_PASS` passed with fatal diagnostic scan 0; the former covers the new canonical resolver aliases and the latter preserves the full S228 painterly/accessibility contract.
- Real NVIDIA OpenGL 3.3 capture passed: `RECURRING_ENEMY_BATTLE_STAGE_CAPTURE_PASS path=res://tmp/visual_audit/recurring_enemy_battle_stage_v1.png encounters=4 resolver=canonical texture=TextureRect action_cutins=wide`.
- The final 2560x1440 contact sheet was visually inspected after the intro transition: all four enemies fill their intended stage slot, remain distinct against their biome backgrounds, keep the command deck and tactical panels clear, and preserve restrained ash/violet focal light. Known ShaderV duplicate-UID and shutdown ObjectDB/resource notices remain non-fatal.

## S228 - 2026-08-09 (Cinematic Battle Stage 2.0)

### Audit findings
- Arrel alone still used an enlarged chibi `AnimatedSprite2D` in the real battle path while allies, support, and enemies had painterly plate paths. That made the protagonist the least cohesive battler on his own stage.
- Battler containers, flat shadows, and HybridDepthStage contact anchors had independent coordinate lists. A visual fit change could leave a plate, its 2D grounding, and its 3D contact shadow out of agreement.
- The existing action vocabulary only animated `AnimatedSprite2D` nodes. Painterly plates had no semantic attack, cast, hurt, or persistent down state, and accessibility modes did not fully settle battler drift back to their authored base positions.
- Follow-up review found two live-path accessibility/cohesion leaks: animated fallback builders could start an idle loop before Reduce Motion took effect, and turn focus used warm/cool RGB shifts instead of neutral brightness-only emphasis.
- The shared ally slot did not record which companion it displayed, so an Elia action could animate Sable when both were present. Hit/status cleanup also restored plain white instead of a role's authored plate modulation, and Reduced Motion still advanced fallback SpriteFrames.

### Done
- Replaced the normal Arrel battle path with the existing `arrel_battle_v3.png` canonical painterly `TextureRect`; the existing animated Arrel sheet remains an explicit fallback if that resource is unavailable. Ally, Tobias, and enemy painterly paths now share the same fit/filter/metadata/grounding/blend restoration construction, while their procedural or animated fallbacks remain available.
- Added one `BATTLE_ROLE_PROFILES` source for player, ally, support, and enemy canvas feet, local feet, container/plate fits, ordering, 3D projection seed, contact-shadow values, and role-specific brightness. Containers and `_sync_battler_anchors_to_stage()` now derive from those profiles.
- Added painterly semantic states with restrained scale, tint, and tilt; the old lunge/knockback containers remain authoritative. Player/enemy visual emphasis now layers onto `HybridDepthStage.set_battle_focus()` without new positional focus tweens. Temporary materials restore the actor's stage blend material.
- Reduce Motion now holds static attack/cast/hurt cues briefly, then generation-guards their restoration to the authored idle scale/rotation; down remains persistent. Clean Gameplay Visuals also returns battlers to their profile base while keeping the static hierarchy readable. Command deck, directive card, HP/readout, item/burn, WITNESS, party, and battle mechanics remain intact.
- The shared ally plate now records and resolves its displayed identity, so only that companion can animate it; Tobias remains independently mapped. Each battler records its authored base modulation, restored after hit/no-status/status-shader cleanup without changing focus/semantic `self_modulate` or blend material. Animated fallbacks now use static semantic frames plus the same generation-guarded Reduce Motion hold/idle lifecycle while normal SpriteFrames playback remains unchanged.
- Animated fallback registration is now the sole initialization path: Reduce Motion immediately selects and pauses the authored idle frame, while normal mode retains looping idle playback. Focus tinting now uses equal RGB brightness factors for active, inactive, and rear actors without moving battler containers or overwriting semantic tint.
- Added the focused `smoke_cinematic_battle_stage` scene (and Godot-generated UID) for canonical art, fallback contracts, role/anchor coupling, solo/max-party bounds, UI clearance, focus/material restoration, accessibility, and action semantics. Updated the existing clarity/filter/story/deck checks and both OpenGL capture scripts for the canonical path and six full-frame moment states.

### Verification
- Godot 4.6.2 headless editor parse: exit 0; no `Infinite loop`, `SCRIPT ERROR`, `Parse Error`, assertion failure, invalid call, or invalid access. Existing ShaderV duplicate-UID and shutdown ObjectDB/resource notices remain non-fatal.
- `CINEMATIC_BATTLE_STAGE_SMOKE_PASS canonical=TextureRect profiles=4 anchors=4 solo=visible max_party=visible ui_clear=1 focus=player_enemy neutral_brightness=1 material_restore=1 displayed_ally=guarded modulation_restore=1 reduce_motion=static_settle stale_guard=1 normal_mechanics=1 clean_view=1 semantics=attack_cast_hurt_down animated_fallback=static fallback_init=static fallbacks=explicit`; it verifies Sable/Elia/Tobias identity routing, exact player/enemy modulation restoration, static fallback initialization and semantic frames, normal idle playback recovery, neutral focus brightness, real action/damage callbacks, and stale cleanup protection through newer cast/down states.
- Direct fatal-aware checks passed: `VISUAL_CLARITY_SMOKE_PASS`, `TEXTURE_FILTERING_SMOKE_PASS`, `STORY_QOL_SMOKE_PASS`, and `BATTLE_COMMAND_DECK_SMOKE_PASS` (`actions=8`, `player_plate=(248.0, 248.0)`).
- `S227_SMOKE_SUITE_PASS cases=6 fatal_scan=enabled`: Burn/Directive stabilization, Early Loop, Tactical Directives, Story Combat, Visual Clarity, and Crash Guards all passed.
- Real non-headless OpenGL 3.3 captures passed at 1280x720: `HYBRID_BATTLE_CAPTURE_PASS` (`profile=the_seam`, player focus, canonical TextureRect) and `BATTLE_MOMENTS_CAPTURE_PASS` (`shots=6`, `idle_player_enemy_action_hurt_down`). Both final files were visually inspected for hierarchy, grounding, semantic readability, and UI clearance.
- `git diff --check` passed; the worktree contains only the S228-owned sources plus the Godot-generated UID for the new smoke script. Nothing was staged, committed, or pushed.

## S227 - 2026-08-02 (Burn/Directive stabilization)

### Audit findings
- The burn preview rebuilt labels and buttons while four independent Tweens were still alive: show/hide, directive-failure pulse, high-grade warning pulse, and the confirm-button loop. The label loops were not bound to their dynamic targets, and delayed unlock callbacks could outlive the selection that created them.
- BattleScene painted its own `MemoryBurnAfterglow` before calling `BattleManager.player_burn()`, while the resulting `MemoryManager.memory_burned` signal also asked `WorldRewriteDirector` to paint `MemoryAbsenceAfterglow`. A real battle burn could therefore stack two washes owned by two systems.
- `swift_finish` forecasted from the current action count instead of the command being considered, so the fourth action remained a generic risk and the fifth action did not preview immediate failure.
- The previous smoke workflow accepted a PASS marker even when Godot printed `Infinite loop detected` after it. Re-running the S226 early-loop smoke reproduced exactly that false green from an orphaned preview pulse.

### Done
- Added one explicit preview-motion lifetime: display/hide and confirm pulse Tweens are tracked, replaced, and killed on reselection, cancellation, or scene exit; the two dynamic label pulses are bound to their labels; delayed button unlocks carry a generation token and can only unlock the current selection.
- Removed BattleScene's Afterglow implementation. `WorldRewriteDirector` is now the only owner, exposes the active non-blocking wash for validation/capture, kills and detaches the prior wash before replacement, and keeps exactly one grouped Afterglow across battle and field.
- Corrected `swift_finish` command forecasting to the projected action count: third action `advance`, fourth action `risk` with a finish-now warning, fifth action `fail` before input is committed.
- Added `smoke_burn_directive_stabilization` with six open/cancel/reselect cycles, stale Tween/timer assertions, the full Swift Finish boundary, and two real `MemoryManager.burn_memory()` calls while a battle scene is alive. The second burn must replace the first WorldRewriteDirector wash without creating a battle-owned duplicate.
- Added `run_s227_smoke_suite.ps1`. A case now passes only when its marker is present, the process exits successfully, and output contains none of `Infinite loop`, `SCRIPT ERROR`, `Parse Error`, assertion failure, invalid call, or invalid access. Each case also has a hard timeout.

### Verification
- `S227_SMOKE_SUITE_PASS cases=6 fatal_scan=enabled`: Burn/Directive stabilization, Early Loop, Tactical Directives, Story Combat, Visual Clarity, and Crash Guards all supplied their PASS markers with no fatal diagnostic.
- `BURN_DIRECTIVE_STABILIZATION_SMOKE_PASS previews=6 swift=advance-risk-fail actual_burns=2 afterglow=1`.
- Godot 4.6.2 headless editor parse: exit 0 and no `Infinite loop`, `SCRIPT ERROR`, `Parse Error`, assertion failure, invalid call, or invalid access.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization: 31 files, 1,592 fields, 19 speakers, 0 errors.
- `git diff --check` passed.

## S226 - 2026-08-02 (Early-loop sharpening: burn stakes, directive clarity, lingering absence)

### Audit findings
- The five-beat core loop (threat -> objective -> burn temptation -> payoff and cost -> lingering consequence) was already implemented across FieldFlow, BattleManager, and WorldRewriteDirector, but three of the five beats were invisible at the moment of decision.
- The burn preview showed damage, grade, and afterimage, yet replaced the actual authored world consequence with the generic line "This will change the world around you", never named the person the memory belonged to, and never mentioned that the active directive forbade burning.
- Approach bonuses (BREAK pre-fill, Resonance, Limit) were applied silently, so an ambush entry felt identical to a plain one.
- World rewrites fired one toast at burn time and then only lived in the archive; nothing on the early route showed the absence a second time.
- The forbidden em dash returned in one shipped battle string (`COST — LOST FOREVER`), which the real-render capture exposed.

### Done
- **Burn stakes.** The preview now leads with the directive conflict, names the tied character, prints the authored world-rewrite line, states "This cannot be restored" on every grade, and adds an Elia line for memories that matter (relationship grade or an authored rewrite). The frame is now sized by its content instead of a fixed rectangle, so it neither clips nor leaves half the screen empty.
- **Clean Hands pressure.** `BattleManager.get_objective_action_relation()` is one source of truth for advance/risk/fail. The burn command wears a red border and the failure text before it is pressed, the burn list repeats the warning, and losing a directive now fires its own cue, sound, and shake instead of one log line.
- **Forecast priority.** Every command forecast leads with its relation to the objective, then the immediate effect, then the permanent cost.
- **Approach payoff.** Ambush/Guarded/Witness entries state the opening they bought (`BREAK 38 open · Resonance +37 · Limit +12`) in the battle banner and log. Field Flow HUD stays quiet at rest and gains border, pulse, and a thicker pressure bar as a threat closes. A Phase Skim now returns half the Phase Step cost, so evasion is a play rather than a skipped fight.
- **Lingering absence.** Twelve rewrite rules gained Korean titles and lines. Five early memories gained authored map-revisit lines; returning to the map spends one and states it once. Three one-time NPC reactions (Elia on the campfire song and the first sword, Malet on the market taste) reuse the existing `PerceptionFilter` metadata convention through a new `take_burn_reaction()` helper that validates the dialogue exists before consuming its flag. A short desaturation wash holds after a burn in both battle and field, scaled by grade, never blocking input.
- **Victory closes on cost.** One aftermath line above the loot rows states what the fight took, what it preserved, or what it released.
- **Early tutorial focus.** The first three hints now teach only burn cost, directive, and approach; a new `first_approach` hint fires on the first non-neutral entry.

### Intentionally not done
- No new mechanic, gauge, status, attribute, map, or autoload. No new UI hub. No chapter or VN expansion. The one-card battle opening from S224 was kept; the obsolete directive briefing overlay stays hidden.
- Memory titles and descriptions remain English-only in Korean play; `MemoryManager.Memory` has no localized fields, which is a data-model change beyond this pass.
- Two pre-existing em dashes remain in `exploration_hud.gd:711` and `story_log.gd:267`; they predate this session and were left rather than widening scope.

### Verification
- Godot 4.6.2 headless project parse: no `Parse Error` or `SCRIPT ERROR`.
- New `EARLY_LOOP_SMOKE_PASS preview=1 conflict=1 forecast=1 approach=1 bypass=1 revisit=3 npc_reaction=1 aftermath=1`.
- 25 existing smoke scenes rerun green, including `TACTICAL_DIRECTIVES_SMOKE_PASS`, `STORY_COMBAT_SMOKE_PASS`, `FIELD_FLOW_SMOKE_PASS`, `CRASH_GUARDS_SMOKE_PASS`, `VISUAL_CLARITY_SMOKE_PASS`, and `TEXT_READABILITY_SMOKE_PASS`.
- Real OpenGL 1280x720 captures inspected: `tmp/visual_audit/burn_preview_stakes.png`, `burn_preview_stakes_ko.png`, `approach_entry_banner.png`, `burn_afterglow.png`. Preview overflow measured at -40px in both locales; afterglow desaturation measured at 0.44.
- `rim_forest.tscn` and `verdan_market.tscn` booted 400 frames each with no script, parse, or null-access error.
- Korean localization: 31 files, 1,592 fields, 19 speakers, 0 errors. VN validation: 20 files, 504 steps, 0 errors. Runtime em-dash scan of new strings: 0.

## S225 - 2026-08-02 (Cinematic title-screen grandeur pass)

### Audit findings
- The live premium title painting already had the strongest available composition: Arrel's silhouette on the left, the memory rupture in the centre, and dark negative space for commands on the right. Replacing it would have weakened continuity with the existing painterly CG archive.
- The flat full-screen image, small wordmark, and unframed command stack were the limiting factors; the start screen needed depth, scale, and entrance rhythm rather than another menu or a disconnected new illustration.

### Done
- Rebuilt the live title composition around the existing key art with a slow oversized parallax drift, an additive memory-rift pulse, sparse drifting ash, a shaped vignette, and restrained cinematic letterbox framing.
- Raised the MEMORIA wordmark to the primary focal tier with the bundled Noto Serif KR font, an eyebrow, gold rail, subtitle, divider ornament, and more legible tagline hierarchy.
- Framed all five existing destinations inside a dark archival command panel with ordered labels, clearer focus/hover states, keyboard guidance, and staged right-side entrance animation. New Game, Continue, Part 2 aftermath preview, Options, and Quit behavior remain unchanged.
- Made all ambient title motion respond live to the existing Reduce Motion option; particles, parallax, pulse, and breathing scale settle into a static composition when enabled.
- Added a focused title smoke scene and a real-render capture scene for future visual regression checks.

### Verification
- `TITLE_GRANDEUR_SMOKE_PASS layers=7 commands=5 reduce_motion=1 title_size=78` under Godot 4.6.2 headless.
- Real OpenGL 1280x720 capture passed at `tmp/visual_audit/title_grandeur.png`; the title, Arrel silhouette, central rupture, and five-command panel remain visually separated.
- `VISUAL_CLARITY_SMOKE_PASS`, `TEXT_READABILITY_SMOKE_PASS`, and `CRASH_GUARDS_SMOKE_PASS` all remained green.
- `git diff --check` passed. Existing forced-shutdown ObjectDB/resource cleanup notices remain; no title `Parse Error` or `SCRIPT ERROR` occurred.

## S224 - 2026-08-01 (Core-loop compression: objective, burn cost, and field tension)

### Audit findings
- The requested gameplay loop already existed in pieces: Field Flow carried three encounter approaches into battle, BattleManager tracked combo/BREAK/WITNESS/burn and tactical objectives, and memory burn already had strong audiovisual feedback plus persistent world rewrites.
- The main rhythm break was the full-screen 2-3 directive selection shown at every battle start. High-grade burn stated its permanent story cost but did not create a short, readable combat-information cost. Visible FieldThreat contact had no rewarded evasion outcome.

### Done
- Replaced the battle-opening directive selection with one deterministic objective that activates immediately on the existing compact card. The card now keeps objective progress and payoff visible; Field Focus strengthens that single payoff instead of adding a third menu choice.
- Added grade-scaled Burn Afterimage to Grade 3-1 memories. It obscures enemy intent and special-ability telegraphs for 1-3 enemy responses; Elia's presence anchors one response for Grade 2-1 without erasing the cost. Burn lists and confirmation previews now state the lost memory, correct display grade, damage, and afterimage duration before confirmation.
- Added a Phase Skim route to visible FieldThreat contact. Crossing the contact line during an active Phase Step resolves the threat without battle and grants 3 Grains; ordinary contact still carries the existing neutral/guarded/witness/ambush approach into combat.
- Prevented synthetic smoke/capture scenes from overwriting the player's autosave when Codex.suppress_recording is active. The temporary capture autosave created during validation was restored from its pre-run backup; manual slots were untouched.
- Updated tactical-directive, Field Focus, Field Flow, and capture contracts for the one-card battle opening.

### Verification
- Godot 4.6.2 headless editor parse completed with no project Parse Error or SCRIPT ERROR.
- TACTICAL_DIRECTIVES_SMOKE_PASS objective=1 modal=0 payoff=1 aftershock=2 elia_anchor=1 grade=S score=100 chain=3 focus=1 save=1 victory_ui=1.
- FIELD_FLOW_SMOKE_PASS routes=3 phase_step=active phase_bypass=rewarded pursuit_break=active battle_handoff=active.
- FIELD_FOCUS_SMOKE_PASS maps=10 deep=10 count=1 resonance=25 limit=20 objective=1 payoff_boost=1.
- STORY_COMBAT_SMOKE_PASS witness=2 release=1 choice_echo=1 preservation_bonus=8 focus=1.
- WORLD_POPULATION_SMOKE_PASS maps=19 voices=84 visible_threats=50 caches=11 curios=10 atlas_gates=7 generated_field_assets=54.
- VISUAL_CLARITY_SMOKE_PASS passed with the existing hybrid stage, objective card, character shots, world records, and boss cut-ins.
- TACTICAL_DIRECTIVE_CAPTURE_SKIP renderer=headless objective=1 modal=0; AUTOSAVE_GUARD_PASS unchanged=True. Real screenshot saving remains reserved for a non-headless renderer.
- git diff --check passed. Existing forced-shutdown ObjectDB/resource and ShaderV duplicate-UID notices remain.

## S223 - 2026-08-01 (Occult villain and high-tier monster battle art)

### Audit findings
- The existing boss art already had strong character silhouettes: Kairos's black ceremonial coat, the Shade Sentinel's faceless iron armor, and the Void Beast's shard-and-bone anatomy.
- The new occult direction was added as an in-world extension of those identities: bureaucratic memory rites for Kairos, a condemned guardian seal for the Sentinel, and a bone-circle rite for the Void Beast.

### Done
- Generated three new canonical boss battle illustrations using the existing boss shots as direct identity and style references:
  - `kairos_occult_editor_v1.png` — sealed memory pages, antique-gold ledger sigils, and a redaction circle.
  - `shade_sentinel_ritual_seal_v1.png` — chained ritual mandala, engraved ward marks, and a violet helm fissure.
  - `void_beast_occult_rite_v1.png` — bone-circle rite, reversed antler frame, and a violet memory core.
- Connected the new illustrations to the live battle image resolver and boss phase/action cut-ins for Kairos, Shade Sentinel, and Void Beast. Previous images remain in the project as preserved fallbacks/archive art.
- Added the three images to a dedicated `Occult Boss Archive` Artbook manifest so the existing 25-image cinematic illustration manifest keeps its original framing contract.
- Extended visual smoke coverage to assert file existence, live battle resolution, and Artbook discoverability for all three occult boss images.

### Verification
- Inspected all three generated images; the silhouettes, restrained charcoal/navy/antique-gold palette, violet void accents, and occult motifs remain coherent with the existing boss archive.
- `STORY_COMBAT_SMOKE_PASS witness=2 release=1 choice_echo=1 preservation_bonus=8 focus=1`.
- `VISUAL_CLARITY_SMOKE_PASS` passed with live boss image resolution and action-cutin checks.
- `ILLUSTRATION_EXPANSION_SMOKE_PASS total=25 categories=5 live_consumers=25`; the separate Occult Boss Archive manifest also resolves all three new images.
- `git diff --check` passed. Only the existing Godot forced-headless ObjectDB cleanup and ShaderV duplicate-UID notices remain.

## S222 - 2026-08-01 (Style-matched population illustration expansion)

### Audit findings
- The existing field population art already established a consistent low-noise painterly language: charcoal and navy silhouettes, worn practical props, restrained antique-gold warmth, and selective memory-blue/violet accents.
- Twelve additional generated images were therefore made against four existing field-art references rather than as isolated new concepts. Existing v1 files were preserved; the new images are v2 field variants for named population records.

### Done
- Generated and alpha-cleaned six dedicated NPC illustrations:
  - `verdan_sealed_note_seller_field_v2.png`
  - `drift_rain_ledger_scribe_field_v2.png`
  - `coast_ash_netter_field_v2.png`
  - `seam_quiet_healer_field_v2.png`
  - `outskirts_threshold_warden_field_v2.png`
  - `forest_root_listener_field_v2.png`
- Generated and alpha-cleaned six dedicated monster illustrations:
  - `belt_scavenger_field_v2.png`
  - `signal_wisp_field_v2.png`
  - `ash_hound_field_v2.png`
  - `rootbound_echo_field_v2.png`
  - `colorless_wraith_field_v2.png`
  - `void_fragment_field_v2.png`
- Wired all twelve assets through `FIELD_ART_OVERRIDES`, including every relevant regional variant ID, so the field sprite and authored hostile art sent into battle use the same generated source.
- Updated the World Population Artbook manifest from ten to twenty-two field illustrations. Existing files were not overwritten.

### Verification
- Inspected all twelve final transparent PNGs after chroma-key removal; no visible green fringe was found in the reviewed silhouettes.
- Godot 4.6.2 headless import reimported all twelve new PNGs successfully.
- `WORLD_POPULATION_SMOKE_PASS maps=19 voices=84 visible_threats=50 caches=11 curios=10 atlas_gates=7 generated_field_assets=54`.
- `VISUAL_CLARITY_SMOKE_PASS` passed with the twenty-two-entry population Artbook manifest.
- `git diff --check` passed. Existing ShaderV duplicate-UID and forced-headless ObjectDB/resource cleanup notices remain; no gameplay `SCRIPT ERROR` or `Parse Error` occurred.

## S221 - 2026-08-01 (Dedicated NPC and monster field illustrations)

### Audit findings
- The expanded population records already had dedicated art for many named roles, but several newly added records reused four broad archetypes: Lantern Cartographer, Root Tender, Cinder Antler, and Ledger Moth Swarm.
- The user asked for images for the added NPCs and monsters themselves, so the missing role-specific variants were generated without overwriting the existing dedicated field art.

### Done
- Generated six additional full-body field cutouts with the built-in image-generation workflow, then removed the flat green chroma key with soft matte/despill processing:
  - `verdan_route_map_seller_field_v1.png`
  - `belt_rail_root_tender_field_v1.png`
  - `outskirts_edge_cartographer_field_v1.png`
  - `bl07_void_root_tender_field_v1.png`
  - `cinder_antler_field_v2.png`
  - `ledger_moth_swarm_field_v2.png`
- Added ID-based `FIELD_ART_OVERRIDES` so the new assets are used by the Verdan, Belt, Seam, Outskirts, Forgotten Forest, Colorless Waste, BL-07, and optional seed-vault population entries while preserving their existing story data.
- Extended the World Population Artbook manifest from four to ten field illustrations.
- Extended population smoke coverage to assert all six new asset paths are live; the generated roster now reports 54 unique live field assets.

### Verification
- Inspected all six final alpha PNGs; silhouettes, props, transparent corners, and chroma-edge cleanup passed.
- Godot 4.6.2 headless import completed and reimported all six final assets.
- `WORLD_POPULATION_SMOKE_PASS maps=19 voices=84 visible_threats=50 caches=11 curios=10 atlas_gates=7 generated_field_assets=54`.
- `VISUAL_CLARITY_SMOKE_PASS` passed with the ten-entry population Artbook manifest.
- Only the existing ShaderV duplicate-UID and forced-headless ObjectDB/resource cleanup notices remain; no `SCRIPT ERROR` or `Parse Error` occurred.

## S219 - 2026-08-01 (GPT Image 2 illustrated landmarks inside the live 3D stage)

### Audit findings
- The post-Claude checkout was clean at `6e85c58` and already contained a real `SubViewport` / `Camera3D` / `MeshInstance3D` hybrid arena, camera coupling, foreground parallax, and memory-burn reactions. The missing piece was not another 3D foundation: the distant biome landmarks were still mostly single-colour boxes, so their silhouettes read as debug geometry over the painted battle plates.
- The project already carried 424 generated CG files. Adding more full-screen story art would have duplicated broad existing coverage, while the live 3D consumer had no dedicated painterly environment cutouts at all.
- Early capture after integration showed that authored dark paintings became nearly invisible when stage lighting was multiplied over them. The generated `Sprite3D` art therefore preserves its own painted light and shares only biome tint, camera depth, and memory-burn tint.

### Done
- Generated five new canonical-environment motifs with the built-in GPT Image 2 workflow: charred memory-root spire, Belt relay obelisk, suspended memory lantern, wrecked coastal mast, and fractured void monolith.
- Converted the flat chroma-key sources to audited alpha PNGs with the installed imagegen helper. The final assets live in `assets/environment/hybrid_depth/`; prompts and the runtime contract are recorded beside them in `README.md`.
- Integrated the art into `HybridDepthStage` as real billboarded `Sprite3D` landmarks rather than screen overlays. Battle stages now select profile-specific silhouettes, the foreground layer uses the same camera rig for close parallax, the world atlas gains five illustrated route anchors, and Curio relics gain a profile-matched painterly centerpiece inside the existing 3D orbit.
- Kept the battle centre clear for actors and commands. The Seam uses exactly seven repeated lantern instances; other profiles use symmetric side landmarks. Shared textures are reused, shadows are disabled, and all imports are capped at 1024 px with mipmaps for the 640x360 subviewports.
- Connected generated landmarks to the game mechanic: memory burning warms the floor, embers, and painted `Sprite3D` landmarks together; Reduce Motion still removes landmark drift.
- Extended `smoke_hybrid_depth` to guard all five asset paths, alpha-art lighting, mipmapped imports, battle/atlas/relic instance counts, and the memory-burn tint response.

### Verification
- Inspected all five generated alpha PNGs locally; silhouettes, padding, chroma removal, story-safe motifs, and edge cleanup passed.
- Godot 4.6.2 headless import completed and respected `mipmaps/generate=true` plus `process/size_limit=1024` on all five assets.
- `HYBRID_DEPTH_SMOKE_PASS battle=151 atlas=96 relic=47 route_markers=10 illustrated=2/5/1 burn_tint=1`.
- All 27 smoke scenes passed with no `SCRIPT ERROR`, `Parse Error`, invalid access, or assertion failure.
- Real OpenGL 3.3 captures were regenerated and inspected: `arena_biomes.png`, `hybrid_depth_board.png`, `hybrid_relic_choice.png`, and `hybrid_battle_stage.png`.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization: 31 files, 1,583 fields, 19 speakers, 0 errors.
- A 300-frame `verdan_market.tscn` boot completed through population, companion spawn, and autosave. Remaining ObjectDB/resource notices occur only during forced test shutdown and contained no gameplay script failure.

## S192 - 2026-07-15 (Meaningful resonance choices, quest dossiers, and HUD repair)

### Gameplay and UX pass
- Reframed Memory Resonance around the game's central promise: crossing an echo no longer silently spends a memory. The player now sees the cost, the immediate reward, and the preservation route before committing.
- Kept the pressure meaningful without making exploration punitive. Preserving an echo gives a capped Field Focus charge for the next battle; kindling it grants the original permanent/local reward; leaving it consumes nothing and lets the player return when ready.

### Done
- Generated nine cohesive GPT Image 2 illustrations in the established low-noise charcoal, navy, and antique-gold art direction: three Memory Resonance choice cards (Bind, Kindle, Leave) and six unique side-quest dossier plates.
- Rebuilt the Memory Resonance interaction as a full keyboard/mouse choice layer with Korean and English copy, readable cost/reward descriptions, focus handling, backdrop, hover feedback, and a short illustrative outcome for Bind or Kindle.
- Made Leave non-destructive: it restores exploration state, re-enables the echo trigger, and does not set the resonance flag. Bind and Kindle intentionally consume the site only after the player chooses.
- Added all six quest plates to `SideQuest` data and the Story Journal quest detail panel, so the active/available/completed quest entry opens with a distinct in-world image instead of plain text alone.
- Repaired the exploration HUD's active side-quest tracker: it now reads the actual `title` field rather than a nonexistent `name`, so active quests no longer resolve to an empty objective.
- Added dedicated smoke scenes for the resonance choice lifecycle and for all six quest-art contracts; Field Focus smoke now also verifies all three choice images resolve.

### Character continuity correction
- Audited the generated art against the current canonical portrait sheets before keeping it. The first Kindle/Leave cards used a generic dark-haired traveller, the Sable quest plate used a young black-haired fighter, and the compass plate gave Elia the wrong hair silhouette.
- Regenerated five replacement plates with the canonical portrait sheets supplied as identity references: object-only Bind, silver-haired blue-eyed Arrel for Kindle and Leave, silver-gray hooded Sable for Sable's Vigil, and short-haired blonde Elia for Compass Calibration.
- Updated live references to the v2 paths and deleted all five mismatched v1 image/import pairs. The other four object/anonymous-echo quest plates remain unchanged because they do not depict a named cast member.

### Verification
- Godot 4.6.2 imported all nine PNGs and completed a headless editor boot without project script or parse errors.
- `smoke_resonance_choice.tscn`: **RESONANCE_CHOICE_SMOKE_PASS** (`options=3`, `leave_persists=true`).
- `smoke_quest_illustrations.tscn`: **QUEST_ILLUSTRATION_SMOKE_PASS** (`quests=6`, `unique_art=6`); `smoke_field_focus.tscn`: **FIELD_FOCUS_SMOKE_PASS**.
- `smoke_visual_clarity.tscn`: **VISUAL_CLARITY_SMOKE_PASS**; `smoke_story_combat.tscn`: **STORY_COMBAT_SMOKE_PASS**; VN validation: **20 files, 504 steps, 0 errors, 0 warnings**; `git diff --check` passed.
- Existing VFX Library/ShaderV headless cleanup notices remain external plugin shutdown noise only; validation reported no gameplay script error.

## S191 - 2026-07-15 (Battle rhythm, tactical reads, and illustrated action beats)

### Combat presentation pass
- Shifted the battle screen away from a repeated command-log loop. Each turn now follows a readable cadence: the field is read, a threat is telegraphed, the player or companion answers, and a visible consequence opens or closes the next decision.
- Kept the story mechanic at the center: ordinary pressure can be answered with swordplay, guard, and items, while WITNESS remains the explicit non-burning route and memory burn continues to state its lasting cost.

### Done
- Generated seven cohesive GPT Image 2, low-noise charcoal-line battle cut-ins: Arrel's blade arc, guard, and WITNESS reach; Elia's anchoring knot; Tobias's fault-line reading; Sable's lantern-thread strike; and the universal BREAK rupture.
- Replaced the generic player attack, defend, WITNESS, Elia anchor, Tobias support, and Sable support images with their corresponding action art. BREAK now gets its own rupture cut-in instead of reading as a small bar-state change.
- Added a central **Combat Beat** card that briefly pairs illustrative art with actionable information: `READ THE FIELD` on player turns, explicit threat/response guidance for enemy abilities, a preservation reminder for WITNESS, defensive timing for Guard, and the opening created by BREAK.
- Added localized response hints for charge, shields/reflection, recovery abilities, status pressure, and aggressive special attacks. The card remains available in Clean Gameplay View because it communicates gameplay rather than decoration.
- Extended visual smoke to verify all seven generated cut-ins and prove the Combat Beat card displays its assigned authored art in a constructed battle scene.

### Verification
- Godot 4.6.2 headless editor imported all seven PNGs without project parse errors.
- `smoke_visual_clarity.tscn`: **VISUAL_CLARITY_SMOKE_PASS** with Combat Beat construction and image-resolution checks.
- `smoke_story_combat.tscn`: **STORY_COMBAT_SMOKE_PASS**; `smoke_world_population.tscn`: **WORLD_POPULATION_SMOKE_PASS** (`maps=19`, `voices=64`, `visible_threats=32`, `caches=11`).
- VN validation passed: **20 files, 504 steps, 0 errors, 0 warnings**. `git diff --check` passed. Editor-only VFX Library/ShaderV shutdown notices remain pre-existing plugin cleanup noise; no gameplay script or parse error was reported.

## S190 - 2026-07-15 (Memory-rewrite illustrations and tactical relic discoveries)

### Story and gameplay pass
- Treated memory loss as a playable, visible cost rather than a separate gallery. The four new images belong to real Chapter 1, 8, 9, and 10 memories, so burning them now changes the world-rewrite flash and the Losses Journal entry that follows.
- Kept exploration rewards tied to a local pressure: the Rim gives an aggressive ash tool after the Void Beast, Verdan gives a record-reading tool after Malet's trade, Drift gives a defensive anchor after its stability arc, and the Colorless Waste gives a WITNESS/recovery knot once the route is reached.

### Done
- Generated and integrated four cohesive GPT Image 2 memory-rewrite CGs: **Forest Scent Thinned**, **Ghost Words Silenced**, **Compass Identity Unmapped**, and **Void Walker Blurred**. They use the established low-noise charcoal-line/painterly dark-fantasy direction and Arrel's current character-shot identity.
- Added the four memory IDs to `WorldRewriteDirector`, including distinct flags, loss text, compass fallout, colors, fullscreen art, and automatic Losses Journal support.
- Generated four alpha-clean illustrated tactical relics: **Cinder Vial**, **Ledger Chalk**, **Anchor Lantern**, and **Witness Knot**. They are live GameManager items, appear in the normal shop/inventory pipeline, and add separate combat decisions: immediate three-turn burn pressure, scan plus BREAK pressure, a guarded Limit window, and a WITNESS/recovery/Limit bridge.
- Placed the relics as one-time, chapter-gated physical caches on the Rim trail, in Verdan Market, at Drift Shelter, and in the Colorless Waste. The Drift cache was moved onto a verified walkable tile during placement validation.
- Expanded visual, combat, and world-population smoke coverage so the new CG paths, item icon paths, combat effects, cache coordinates, and shipped world totals are checked automatically.

### Verification
- Godot 4.6.2 headless visual smoke: **VISUAL_CLARITY_SMOKE_PASS** (`map_canvases=19`, `character_shots=24`, `dialogue_variants=11`, `boss_action_cutins=3`).
- `smoke_story_combat.tscn`: **STORY_COMBAT_SMOKE_PASS**; `smoke_world_population.tscn`: **WORLD_POPULATION_SMOKE_PASS** (`maps=19`, `voices=64`, `visible_threats=32`, `caches=11`).
- VN validation passed: **20 files, 504 steps, 0 errors, 0 warnings**. `git diff --check` passed. The remaining Godot shutdown resource notices are the pre-existing external-plugin cleanup noise only; no parse or script error was reported.

## S189 - 2026-07-15 (Character-expression pass: GPT Image 2 poses, costumes, and live placement)

### Story and placement pass
- Reused the three S188 master character sheets as visual identity anchors instead of producing a disconnected art direction. Each new pose serves an authored story pressure: Arrel's unarmored resolve, Elia's anchoring ritual, Tobias's field-ledger focus, Sable's lantern-watcher duty, the Bureau's different modes of control, and the four boss attack identities.
- Kept baseline portraits and canonical boss shots intact for readable conversation and Bestiary staging. Variant art appears only when the dialogue emotional state, journal record, or battle action earns the escalation.

### Done
- Generated and integrated twelve cohesive GPT Image 2 variants: eight 627px character portraits with distinct travel/formal/field outfits, expressions, props, and action poses, plus four boss action cut-ins.
- Mapped eleven high-tension DialogueBox portrait routes to their corresponding variants. The same selection now drives the dim speaker-stage image, so the portrait and conversation background no longer contradict each other during a reveal, confrontation, or ritual.
- Added the new action variants to five gated Story Journal event records and four key NPC records, giving the player a second contextual place to revisit the character work after a scene.
- Updated Void Beast, Shade Sentinel, Kairos Ascendant, and Echo Shell battle-action cut-ins while preserving their calmer v2 shots for ordinary battle staging and Codex previews.
- Extended visual smoke coverage for all 24 current character/boss shots, the eleven dialogue-stage variant contracts, Journal art resolution, and the three dynamic boss action-cutin paths.

### Verification
- Godot 4.6.2 headless editor import completed for all twelve v3 PNGs without project parse errors.
- `smoke_visual_clarity.tscn`: **VISUAL_CLARITY_SMOKE_PASS** (`character_shots=24`, `dialogue_variants=11`, `boss_action_cutins=3`, `world_records=10`).
- `smoke_story_combat.tscn`: **STORY_COMBAT_SMOKE_PASS**; VN validation passed: **20 files, 504 steps, 0 errors, 0 warnings**. `git diff --check` passed.
- Existing VFX Library/ShaderV shutdown resource notices remain external-plugin cleanup noise only; the validation runs reported no script or parse error.

## S188 - 2026-07-15 (Part I world atlas and refined character shots)

### Story and art pass
- Re-anchored the Part I expansion in the existing manuscript rules: memory can be traded but only lived memories burn; combustion is a tracked signature; Ash Rain is broken-memory residue; record-tree paper is off-grid but physically fragile; and refuge survives through mutual witness.
- Kept the ten-chapter route spoiler-safe. Each in-game world record unlocks only after its corresponding story discovery, so the journal clarifies a present pressure without pre-explaining later answers.

### Done
- Added `docs/PART1_STORY_WORLD_ATLAS.md`: a practical canon atlas covering memory rules, all ten route identities, character stakes, boss narrative contracts, and presentation rules. It links the expanded atlas sites back to the original Rim → BL-07 story rather than treating them as disconnected content.
- Added a chapter-gated **World** tab to Story Journal with ten illustrated records: Ash Rain, memory debt, Belt surveillance, record-tree paper, anchoring, the Seam refuge pact, Void returns, the Listening Wood, paired witnesses in the Colorless Waste, and the Seal's burden.
- Generated and integrated **twelve GPT Image 2 character shots** in one unified, low-noise charcoal-line/painterly dark-fantasy direction. Dialogue and speaker-stage defaults now use the refined Arrel, Elia, Tobias, Sable, Kairos, Nera, Seric, and Veil portraits.
- Added four readable boss shots — Void Beast, Shade Sentinel, Kairos Ascendant, and Echo Shell. Canonical boss encounters now prefer these shots for battle-stage visibility and Bestiary previews while retaining existing wide cinematic CGs for action cut-ins.
- Extended visual smoke coverage to verify all eight portrait routes, all twelve generated character/boss shot assets, Story Journal World-tab construction, all ten gated records and art paths, and canonical boss image resolution.

### Verification
- Godot 4.6.2 headless editor import completed for all 12 new PNGs with no project parse errors.
- `smoke_visual_clarity.tscn`: **VISUAL_CLARITY_SMOKE_PASS** (`character_shots=12`, `world_records=10`) including a real two-record World-tab construction check.
- `smoke_story_combat.tscn`: **STORY_COMBAT_SMOKE_PASS** (WITNESS release, preservation bonus, tactical-item flow unchanged).
- `smoke_world_population.tscn`: **WORLD_POPULATION_SMOKE_PASS** (`maps=19`, `voices=64`, `visible_threats=32`, `caches=7`).
- VN validation passed: **20 files, 504 steps, 0 errors, 0 warnings**. `git diff --check` passed. Existing VFX Library/ShaderV shutdown cleanup notices remain external plugin noise only.

## S187 - 2026-07-14 (Atlas expansion: seven maps, settlements, field roster, and reward caches)

### Story and scale pass
- Re-read the Belt signal-network danger, Drift's failing route literacy, the coast's returned dead, Seam refuge culture, the forest's mimetic paths, the Colorless Waste's travelling witness economy, and BL-07's record-tree seed truth before placing new content.
- Kept the ten-chapter route intact. Each expansion map unlocks only after its source chapter's completion, enters from an explicit field gateway, and returns to the source map. This grows exploration without breaking story pacing or creating a second mandatory quest chain.

### Done
- Added **seven returnable atlas sites**: Belt Signal Yard, Drift Waymarker Shrine, Cinder Harbor, Lantern Ward, Name Hollow, Grey Caravan, and BL-07 Seed Vault. The playable world grows from **12 to 19 maps/sites**.
- Added a dedicated GPT Image 2 low-noise top-down canvas for every new site. The new coast harbor, Seam ward, and waste caravan are actual settlement-scale destinations rather than a re-tinted existing combat field.
- Added **28 localized ambient NPC voices**, **14 visible optional threats**, and **7 one-time physical item caches** across the new maps. Total playable field population is now **64 voices, 32 visible threats, and 48 distinct generated field sprites**.
- Added twelve new civilian/survivor field assets, twelve new hostile field assets, and six alpha-clean tactical item icons. Every asset is used by live population data; no generated roster file remains decorative-only.
- Added six usable rewards to `GameManager.ITEMS`: Root Balm, Signal Jammer, Lantern Salve, Name Thread, Compass Shard, and Seed Capsule. They extend existing cure, escape, recovery, and WITNESS systems rather than introducing dead inventory types.
- Added `WorldAtlas` for chapter-gated main-map gateways and `WorldCache` for persistent, interactable, one-time field rewards. Expanded `WorldPopulation` and its smoke to cover map bounds, item registration, icon hookup, and exact totals.

### Verification
- Godot 4.6.2 editor import completed after all new PNGs were added. No new project parse errors were introduced; the existing VFX Library and ShaderV shutdown notices remain plugin-only noise.
- `smoke_world_population.tscn`: **WORLD_POPULATION_SMOKE_PASS** (`maps=19`, `voices=64`, `visible_threats=32`, `caches=7`, `atlas_gates=7`, `generated_field_assets=48`).
- Headless construction passed for all seven new scenes: Belt Signal Yard, Drift Waymarker Shrine, Cinder Harbor, Lantern Ward, Name Hollow, Grey Caravan, and BL-07 Seed Vault.
- Alpha audit passed for all 61 currently audited field/UI assets with no blank image outputs. Visual contact-sheet review confirmed that new maps, residents, threats, and item icons retain the existing muted charcoal-line dark-fantasy language.

## S186 - 2026-07-14 (World population, visible threats, and social ecology)

### Story and placement pass
- Re-read the Part 1 manuscript beats together with the worldbuilding rules: Rim's near-absence of surveillance, Verdan's memory-loan economy and mine labor, Belt signal-tower scrutiny, Drift's failing literacy, the Seam's refuge function, the parasitic forest, the Colorless Waste compass motif, and BL-07's record-tree/echo-shell truth.
- The goal was not to fill routes with anonymous sprites. Each populated map now answers a local question: who survives here, what memory-system pressure shapes them, and which visible threat belongs to that biome.

### Done
- Added `WorldPopulation`, a chapter-aware placement system used by the ten chapter maps and two optional story sites. It creates **36 localized field NPC voices** with individual English/Korean names and short repeatable story observations, plus **18 visible optional threat encounters** that persist as defeated through story flags.
- Kept Verdan free of roaming monsters so its crowd, debt, and surveillance remain the gameplay pressure. Rim's Ash Hound is held until Chapter 1 is complete, while later routes gain appropriate Ash Hounds, Belt Scavengers, Signal Wisps, Rootbound Echoes, Colorless Wraiths, and Void Fragments.
- Generated twenty-four GPT Image 2 field assets in the established miniature dark-fantasy language: twelve civilians/survivors (including a Rim herbalist, Verdan note-runner, Belt mechanic, Drift archivist, Seam medic, and Waste compass guide) and twelve hostile silhouettes. Sources were chroma-keyed, despilled, cropped, normalized to 128x160 RGBA field canvases, and integrated as project assets.
- Extended NPC support for localized ambient names/lines and static generated field art while retaining all four facing animations. New civilian nodes remain interactable; visible hostiles use non-blocking Area2D contact battles instead of hidden random-trigger rectangles.
- Added **Root Hollow** (unlocked after Chapter 1) and **Verdan Ledger Cellar** (unlocked after Malet's Chapter 2 transaction): small, returnable side sites that turn the Root Bark and memory-loan economy into explorable spaces rather than background lore. Each has a dedicated generated map canvas, contextual inhabitants, and optional visible battles.
- Added `smoke_world_population.tscn`, which checks all twelve map hookups, map-bounds and collision-tile placement for chapter maps, 36/18 total counts, generated NPC/hostile asset resolution, and the NPC facing contract.

### Verification
- Godot 4.6.2 imported all twenty-six new art assets and registered `WorldPopulation`/`MapGateway` with no new project parse errors. Existing VFX Library popup/autoload and ShaderV duplicate-UID shutdown messages remain external plugin noise.
- `smoke_world_population.tscn`: **WORLD_POPULATION_SMOKE_PASS** (`maps=12`, `voices=36`, `visible_threats=18`, `generated_field_assets=24`). The smoke additionally proves that all six specialist civilians and six rare hostile variants are the assets actually used in play.
- `smoke_visual_clarity.tscn`: **VISUAL_CLARITY_SMOKE_PASS** with the expanded `map_canvases=12` audit. The alpha audit also passed for all 24 field files (`128x160`, transparent corners).
- Headless live construction checks: Rim Forest reports `3 voices, 0 visible threats` before its post-Chapter-1 hunt unlock; BL-07 reports `3 voices, 2 visible threats`. Both optional side-site scenes parse and construct with their return gateways.

## S185 - 2026-07-14 (Map identity, exploration HUD, and field-cast refinement)

### Audit
- The first six GPT Image 2 map canvases established the intended gameplay-map quality, but Drift Shelter, Forgotten Forest, Colorless Waste, and Seam Outskirts still reused nearby locations with palette tints.
- Exploration HUD information was structurally correct but its chapter, HP, and objective hierarchy read as a stack of unrelated rows. Companion silhouettes also needed a quieter grounding treatment, while ambient NPC breathing needed to preserve each authored field sprite's scale.

### Done
- Added four dedicated, text-free, top-down GPT Image 2 environment canvases: rain-darkened **Drift Shelter**, root-bound **Forgotten Forest**, achromatic **Colorless Waste**, and violet threshold **Seam Outskirts**. All ten playable maps now have their own environmental identity instead of using a tinted adjacent-map canvas.
- Reframed the exploration HUD into a compact identity header: Arrel's journey, chapter/location context, a clearer HP rail, divider, then the story-thread card. Clean Gameplay View still hides archive economy noise and location-art overlays.
- Added an understated companion ground shadow and resonance line so Elia/Sable follow as embodied field characters rather than floating art. Static NPCs now join the NPC group and receive a micro idle breath that preserves their authored 0.36 field-cast scale.
- Replaced the per-map absolute-scale idle loops with `MapEffects.update_npc_idle_motion()`, preventing NPC sheets from being stretched or shrunk by ambience updates.
- Extended the visual clarity smoke to check HUD hierarchy, companion grounding, preserved NPC scale, and the complete ten-canvas map set.

### Verification
- Godot 4.6.2 imported the four new canvases with no project parse failures (only known VFX Library and ShaderV editor shutdown warnings).
- `smoke_visual_clarity.tscn`: **VISUAL_CLARITY_SMOKE_PASS** with `ui_header=1`, `companion_presence=1`, `npc_scale=preserved`, and `map_canvases=10`.
- Headless construction audit loaded all ten map scenes with no `Parse Error` or `SCRIPT ERROR`.

## S184 - 2026-07-14 (Gameplay balance, tactical items, and presentation cohesion)

### Design pass
- The field-art overhaul fixed the readability and character identity of exploration, but consumables still read as a flat counter and the preservation route had no tactical bridge between a normal WITNESS action and burning a memory.
- Antidote and Firebomb were too narrow to be satisfying shop choices: one could waste a turn with no immediate recovery, while the other deferred all value to a delayed tick.

### Done
- Added **Witness Ink**, a rare illustrated GPT Image 2 consumable with clean alpha: it advances WITNESS by one step, guards the next blow, adds Limit, still obeys boss full-reading rules, and can be purchased, found from void/boss encounters, or used from the opening kit and Boss Rush kit.
- Rebalanced existing consumables around clear tactical roles: Antidote now purges poison/burn and restores 12 HP; Firebomb now has a 12-damage impact before its two-turn burn.
- Reframed the battle item selector into category-colored, two-line action cards (RECOVER, PURGE, IGNITE, ESCAPE, WITNESS) so the effect type, illustration, and player decision scan together.
- Replaced the exploration HUD's generic item total with a live tactical-kit summary: Recovery, Tools, and Witness stock. The Witness count shifts the line to violet, while Clear Gameplay View remains uncluttered.
- Extended the real combat smoke with antidote recovery, firebomb impact-plus-DoT, Witness Ink progress, turn-flow, and shipped-icon assertions. The visual smoke now expects all six shop icons.

### Verification
- Built-in GPT Image 2 Witness Ink source was chroma-keyed to RGBA and passed alpha audit (1199x1312, full 0..255 alpha range).
- Godot 4.6.2 editor import completed for witness_ink.png; only the known VFX Library popup/autoload and ShaderV duplicate-UID shutdown warnings appeared.
- smoke_story_combat: **STORY_COMBAT_SMOKE_PASS** after exercising Antidote, Firebomb, Witness Ink, normal WITNESS release, preservation reward, and Field Focus.
- smoke_visual_clarity: **VISUAL_CLARITY_SMOKE_PASS** (item_icons=2, shop_icons=6, unified field cast, clear-view and battle checks).
- smoke_gameplay_qol and smoke_field_focus both passed; VN validation passed (20 files / 504 steps) and Korean localization passed (31 files / 1,583 fields).
- git diff --check passed apart from normal CRLF notices.

## S183 - 2026-07-13 (GPT Image 2 playable map canvas overhaul)

### Audit
- Verified that the first field view and Verdan Market still read as code-generated tile grids even after the field-cast cleanup. Existing story CGs carried the intended dark-fantasy identity, but the gameplay maps only received them as faint atmosphere overlays.

### Done
- Generated six top-down, low-noise 32-bit environment canvases with GPT Image 2: Rim Forest, Verdan Market, Belt Waystation, Crumbling Coast, The Seam, and BL-07 Void.
- Added a `MapEffects.add_map_canvas()` layer beneath the live TileMap. It uses the generated art for actual terrain values while retaining the original TileMap collision, interactables, minimap data, and script-controlled map flow.
- Connected the canvas layer to all ten playable maps. Shared canvases are deliberately palette-tinted for their adjacent narrative variants: Forgotten Forest, Drift Shelter, Seam Outskirts, and Colorless Waste.
- Suppressed Rim Forest's redundant procedural edge seams and disabled runtime additive point lights when a baked map canvas is present, preventing translucent square patches and preserving the new maps' authored lantern values.
- Kept the field cast, HUD, NPCs, interaction prompts, and objective markers on top of the canvas. Clean Gameplay View now keeps the environmental artwork instead of falling back to a flat code tile layer.

### Verification
- Headless construction smoke passed for all 10 map scenes with no `Parse Error` or `SCRIPT ERROR`.
- Live captures passed for Rim Forest exploration and Verdan Market/Malet interaction after the new layers were attached.
- Canvas reference audit passed (`maps=10`, `canvases=6`), VN validation passed (20 files / 504 steps), Korean localization passed (31 files / 1,583 fields), and `git diff --check` passed.

## S182 - 2026-07-13 (CG consistency correction and unified field cast)

### Audit
- Compared the four most recent CG additions against the established Chapter 3, 5, 7, and 8 plates at full screen. The replacements used a flatter cell-rendered finish, an artificial lower dialogue band, and character proportions that split from the existing charcoal-line, muted-painterly direction.
- Captured the live Rim Forest, Verdan Market, and full directional gallery. The old exploration boards were high-detail AI figures reduced at runtime; at field scale their white halos and fabricated rear head treatment read as noise and did not belong with the 32px map tiles.

### Done
- Removed the four mismatched S181 CG files and their runtime/Artbook references. Rebuilt the same narrative beats with direct established-plate references and no forced dialogue band:
  - `story_ch3_tobias_record_spill_v2.png`
  - `story_ch5_threaded_horizon_v2.png`
  - `story_ch7_residue_witness_v2.png`
  - `story_ch8_anchor_under_roots_v2.png`
- Generated a unified GPT Image 2 field cast for Arrel, Elia, Malet, Tobias, Kairos, Nera, Veil, and Sable. Every cast member now has a clean alpha PNG for down/front, up/back, left, and right under `assets/sprites/field/<name>/`.
- Made `PixelSprite` prefer those purpose-built field directions non-destructively, including real rear-facing art instead of a synthesized face mask. Player, companion, and NPC setup select nearest-neighbour field art with one shared scale and foot baseline; legacy character sheets remain untouched fallbacks.
- Replaced Verdan Market's procedural ambient citizens with scaled field-cast variants, so background people no longer use the incompatible square-face placeholder style.
- Adjusted Clear Gameplay View terrain routing so clean tiles no longer use per-cell color variation or large patch blocks. Also widened the clean-view camera slightly to provide more map context without shrinking the readable field cast.
- Updated the visual-clarity smoke and live capture harnesses to verify the field paths and every directional frame, including Sable.

### Verification
- Godot 4.6.2 headless editor import completed for 32 field frames and the four replacement CGs without script or parse failures.
- `smoke_visual_clarity.tscn` passed after the new cast integration.
- Normal-render captures passed for Rim Forest, Verdan Market, and the six-character directional gallery; the captures show the new field assets resolving from `assets/sprites/field/`.


## S01 — 2026-04-05 (프로젝트 초기 세팅)

### 완료
- [x] 프로젝트 폴더 구조 생성 (scenes, scripts, assets, data)
- [x] project.godot 설정 파일 (1280x720, 입력 매핑, 오토로드, 픽셀아트 필터링)
- [x] GameManager (오토로드) — 게임 상태, 스토리 플래그, 플레이어 데이터
- [x] MemoryManager (오토로드) — 기억 데이터 클래스, 연소/잔존 시스템, 초기 기억 6개
- [x] DialogueManager (오토로드) — 대화 진행, 선택지, 기억 연소 연동
- [x] SceneTransition (오토로드) — 페이드 인/아웃
- [x] Player 스크립트 + 씬 — 4방향 이동, 상호작용 레이캐스트, 카메라
- [x] Main 테스트 씬 — 시스템 확인용 (대화/기억/연소 테스트)
- [x] Chapter 1 대화 데이터 (JSON)
- [x] 프로젝트 아이콘 (SVG 플레이스홀더)

### Godot 설치 필요
- https://godotengine.org/download 에서 Godot 4.x 다운로드
- zip 풀면 바로 실행 가능
- 실행 후 "Import" → Game 폴더의 project.godot 선택

### 다음 세션 (S02) 할 일
- [ ] Godot에서 프로젝트 열기 확인
- [ ] 플레이스홀더 스프라이트 생성 (색깔 사각형)
- [ ] AnimatedSprite2D에 걷기 애니메이션 추가 (4방향 x 4프레임)
- [ ] 첫 번째 맵(림 외곽 숲) 기본 타일맵 레이아웃
- [ ] 실제 실행 테스트

### 메모
- .tscn 파일의 서브리소스는 Godot에서 열 때 자동 생성될 수 있음
- 첫 실행 시 에러 뜨면 씬 파일을 Godot 에디터에서 재생성하면 됨
- 기억 시스템의 초기 데이터는 Chapter 1 기준. 챕터 진행에 따라 추가

---

## S02 — 2026-04-05 (플레이스홀더 스프라이트 + 첫 맵 + 버그 수정)

### 완료
- [x] **버그 수정:** player.tscn SubResource 순서 수정 (정의가 사용보다 앞에 오도록)
- [x] **버그 수정:** main.tscn 비표준 unique_id 속성 제거
- [x] **버그 수정:** chapter1_dialogue.json ██████░░░░ 게이지 → 간접 표현 교체 (개발규칙 4번)
- [x] **개선:** MemoryManager enum 역순 비교에 대한 주석 추가
- [x] **플레이스홀더 스프라이트:** player.gd에서 SpriteFrames 동적 생성 (다크 블루 사각형 + 방향별 눈/머리카락)
- [x] **애니메이션:** 8개 애니메이션 구현 (idle/walk x 4방향, walk은 4프레임 발 움직임)
- [x] **첫 맵:** 림 외곽 숲(rim_forest) — 25x18 타일, 풀/길/나무/덤불/물 5종
- [x] **맵 충돌:** 나무/물 타일에 StaticBody2D 충돌 자동 생성
- [x] **나무/덤불 시각적 디테일:** 줄기+수관, 덤불 레이어 표현
- [x] **main_scene 변경:** project.godot → rim_forest.tscn으로 시작 씬 변경

### 변경된 파일
- `scenes/player/player.tscn` — SubResource 순서 수정
- `scenes/main/main.tscn` — unique_id 제거
- `data/chapter1_dialogue.json` — 게이지 표현 수정
- `scripts/systems/memory_manager.gd` — enum 주석 추가
- `scripts/core/player.gd` — 플레이스홀더 스프라이트 + 애니메이션 전면 재작성
- `scenes/maps/rim_forest.tscn` — **신규** 첫 맵 씬
- `scenes/maps/rim_forest.gd` — **신규** 맵 빌드 스크립트
- `project.godot` — main_scene 경로 변경

### 다음 세션 (S03) 할 일
- [x] NPC 기본 씬 + 상호작용 시스템 (대화 트리거) → S03에서 완료
- [x] 대화 UI (DialogueBox 씬) — 화면 하단 텍스트 박스 + 포트레이트 → S03에서 완료
- [x] DialogueManager에 JSON 파일 로더 연결 → S03에서 완료
- [ ] Godot 4.6에서 프로젝트 열고 실행 테스트 → S04로 이월
- [ ] 맵 레이아웃 조정 (실제 플레이 느낌 확인 후) → S04로 이월

### 메모
- 스프라이트는 _ready()에서 코드로 생성됨. 실제 에셋으로 교체 시 _setup_placeholder_sprites() 함수만 제거하면 됨.
- 맵도 ColorRect + StaticBody2D로 코드 생성. 나중에 TileMap으로 전환 가능.
- 기존 main.tscn (테스트 씬)은 삭제하지 않고 유지. 시스템 디버그용으로 사용 가능.

---

## S03 — 2026-04-05 (NPC 시스템 + 대화 UI + JSON 로더)

### 완료
- [x] **NPC 기본 씬:** StaticBody2D 기반, 플레이스홀더 스프라이트(코드 생성), interact() 인터페이스
- [x] **NPC export vars:** npc_name, dialogue_file, dialogue_key, npc_color (에디터에서 설정 가능)
- [x] **엘리아 NPC 배치:** 림 외곽 숲 맵에 엘리아(청회색) 배치, dialogue_key="elia_appears"
- [x] **DialogueManager JSON 로더:** load_dialogue_file(), load_and_start() 추가. 캐싱 지원.
- [x] **DialogueBox (오토로드):** 하단 대화 박스 UI 전체 코드 생성
  - 어두운 반투명 패널 (서고 모티프)
  - 좌측 포트레이트 (캐릭터별 색상 + 이니셜)
  - 타자기 효과 (글자당 0.03초, Space로 즉시 완료)
  - 선택지 버튼 (VBoxContainer, 키보드 포커스 지원)
  - ▼ 다음 대사 표시기
- [x] **Player 입력 정리:** 대화 진행 입력을 DialogueBox._unhandled_input()으로 이전 (충돌 방지)

### 변경된 파일
- `scripts/core/npc.gd` — **신규** NPC 스크립트
- `scenes/npc/npc.tscn` — **신규** NPC 기본 씬
- `scripts/ui/dialogue_box.gd` — **신규** 대화 UI 스크립트
- `scripts/systems/dialogue_manager.gd` — JSON 로더 추가 (load_dialogue_file, load_and_start)
- `scenes/maps/rim_forest.tscn` — 엘리아 NPC 인스턴스 추가
- `scripts/core/player.gd` — 대화 중 입력 처리 제거 (DialogueBox로 이전)
- `project.godot` — DialogueBox 오토로드 추가

### 다음 세션 (S04) 할 일
- [x] 기억 UI (아렐의 서고) → S04에서 완료
- [x] 시스템 로그 팝업 UI → S04에서 완료
- [ ] Godot 4.6에서 프로젝트 열고 실행 테스트 (전체 흐름) → S05로 이월
- [ ] 맵 레이아웃 조정 (실제 플레이 느낌 확인 후) → S05로 이월

### 메모
- DialogueBox는 CanvasLayer 오토로드 (layer 50). 모든 씬에서 자동 사용 가능.
- NPC collision_layer = 12 (NPCs + Interactables). Player의 InteractionRay mask=4 에 대응.
- 포트레이트는 현재 ColorRect + 이니셜. 실제 이미지 적용 시 _update_portrait() 수정.
- JSON 대화 파일은 첫 로드 시 캐싱됨. 같은 파일 재요청 시 파싱 생략.

---

## S04 — 2026-04-05 (기억 UI + 시스템 로그 팝업)

### 완료
- [x] **아렐의 서고 (MemoryUI):** 기억 인벤토리 전체 화면 UI
  - 서고 모티프 — 어두운 나무색 패널, 등급별 색상 코딩
  - 좌측 등급 필터 탭 (All / Grade 5~1)
  - 중앙 기억 카드 스크롤 목록 (등급 좌측 컬러 바, 연소/잔존 상태 표시)
  - 우측 상세 정보 패널 (이름, 등급, 설명, 연소력, 관련 NPC, 스토리 효과, 상태)
  - Tab/M 키로 토글, ESC로 닫기
  - MENU 상태 전환 (열기/닫기 시 GameManager 연동)
- [x] **시스템 로그 (SystemLog):** 관리국 감지 로그 팝업
  - 화면 상단 청록색 모노스페이스 스타일 팝업
  - MemoryManager.memory_burned 시그널 자동 연결
  - ���이드 인 → 3.5초 유지 → 페이드 아웃
  - 연속 연소 시 대기열(queue) 처리
  - BBCode로 등급별 색상 차이 표현
- [x] **입력 매핑:** memory_menu 액션 추가 (Tab + M)
- [x] **오토로드:** MemoryUI (layer 40), SystemLog (layer 60) 등록

### 변경된 파일
- `scripts/ui/memory_ui.gd` — **신규** 기억 서고 UI
- `scripts/ui/system_log.gd` — **신규** 시스템 로그 팝업
- `project.godot` — 오토로드 2개 + memory_menu 입력 매핑 추가

### 오���로드 레이어 정리
| Layer | 오토로드 | 용도 |
|-------|----------|------|
| 40 | MemoryUI | 기억 서고 (전체화면) |
| 50 | DialogueBox | 대화 UI (하단) |
| 60 | SystemLog | 관리국 로그 (상단 팝업) |
| 100 | SceneTransition | 씬 전환 페이드 |

### 다음 세션 (S05) 할 일
- [x] 전투 시스템 기초 → S05에서 완료
- [x] 전투 중 기억 연소 스킬 선택 → S05에서 완료
- [ ] Godot 4.6에서 전체 프로젝트 실행 테스트 → S06로 이월

### 메모
- MemoryUI는 MemoryManager.memories를 직접 읽어서 카드 생성. 기억 추가/연소 시 _refresh_cards() 호출로 동기화.
- SystemLog는 queue 방식이라 연속 3번 연소해도 순차적으로 표시됨.
- memory_menu 키: Tab(4194306) + M(77)

---

## S05 — 2026-04-05 (포트레이트 이미지 + 전투 시스템)

### 완료
- [x] **포트레이트 이미지 적용:** 아렐 3장(neutral/side/wounded) + 엘리아 2장(neutral/concern) 복사
- [x] **DialogueBox 이미지 지원:** TextureRect 기반 포트레이트 표시, portrait 키→파일 매핑, fallback 유지
- [x] **커버 이미지 복사:** Cover.png → assets/cg/cover.png
- [x] **BattleManager (오토로드):** 턴제 전투 로직
  - Enemy 클래스 (이름, HP, 공격력, 공허수 여부)
  - 플레이어 행동: Attack / Burn / Defend / Flee
  - 기억 연소 스킬 5등급 (Ember → Zero Burn)
  - 공허수 = 일반 공격 불가, 기억 연소만 유효
  - 도주 확률 70%, 공허수 도주 불가
- [x] **전투 씬 (battle_scene):** 코드 생성 UI
  - 적 HP바 (우상단) + 플레이어 HP바 (좌하단)
  - 적 플레이스홀더 스프라이트 (붉은 사각형 + 눈)
  - 행동 버튼 4개 (ATTACK / BURN / DEFEND / FLEE)
  - BURN 선택 → 사용 가능한 기억 목록 팝업
  - 전투 로그 (중앙 텍스트)
- [x] **전투 트리거:** 림 외곽 숲 맵에 2개 배치
  - 남쪽 길: Ash Crawler (일반 몬스터, HP 40, ATK 8)
  - 북쪽 숲: Void Beast (공허수, HP 80, ATK 15, 일반 공격 불가)
  - Area2D + body_entered로 자동 전투 진입

### 변경된 파일
- `assets/portraits/arrel_neutral.jpg` — **신규** 아렐 포트레이트
- `assets/portraits/arrel_side.jpg` — **신규**
- `assets/portraits/arrel_wounded.jpg` — **신규**
- `assets/portraits/elia_neutral.jpg` — **신규** 엘리아 포트레이트
- `assets/portraits/elia_concern.jpg` — **신규**
- `assets/cg/cover.png` — **신규** 커버 이미지
- `scripts/ui/dialogue_box.gd` — TextureRect 포트레이트 + 이미지 매핑 추가
- `scripts/systems/battle_manager.gd` — **신규** 전투 로직
- `scenes/battle/battle_scene.tscn` — **신규** 전투 씬
- `scenes/battle/battle_scene.gd` — **신규** 전투 씬 UI
- `scenes/maps/rim_forest.gd` — 전투 트리거 영역 2개 추가
- `project.godot` — BattleManager 오토로드 추가

### 다음 세션 (S06) 할 일
- [x] Godot 실행 테스트 → 완료 (Color alpha 누락 버그 수정 후 실행 성공)
- [x] 전투 후 HP 회복 → S06에서 완료
- [x] 세이브/로드 → S06에서 완료
- [x] NPC 상호작용 버그 → S06에서 수정

### 메모
- 포트레이트 매핑: PORTRAIT_MAP dict에 키→경로. DEFAULT_PORTRAITS로 화자별 기본값 설정.
- 전투 씬은 SceneTransition으로 전환. 전투 종료 후 return_scene으로 자동 복귀.
- 기억 연소 스킬 데미지 = BURN_SKILLS[grade].base_damage + memory.burn_power
- 공허수(Void Beast)는 is_void_beast=true → player_attack()에서 0 데미지 처리.

---

## S06 — 2026-04-05 (NPC 버그 수정 + HP 회복 + 세이브/로드)

### 완료
- [x] **버그 수정: NPC 상호작용 안 됨**
  - InteractionRay에 `force_raycast_update()` 추가 (입력 시점 동기화)
  - ray 길이 20px → 32px 증가 (NPC 감지 범위 확대)
- [x] **전투 후 HP 회복:** 승리 시 max_hp의 20% 회복
- [x] **세이브/로드 시스템 (SaveManager 오토로드)**
  - 3슬롯 JSON 저장 (user://saves/)
  - F6 = 퀵세이브, F7 = 퀵로드
  - 저장: player_data, story_flags, chapter, memory 상태, 현재 씬
  - GameManager/MemoryManager에 import_data() 추가
- [x] **tscn uid 제거:** 모든 씬에서 가짜 uid 제거 (Godot 자동 생성에 맡김)
- [x] **tscn Color alpha 수정:** Color(r,g,b) → Color(r,g,b,a)

### 변경된 파일
- `scripts/core/player.gd` — force_raycast_update + ray 32px
- `scenes/player/player.tscn` — InteractionRay target_position 32
- `scripts/systems/battle_manager.gd` — 승리 시 HP 20% 회복
- `scripts/systems/save_manager.gd` — **신규** 세이브/로드
- `scripts/core/game_manager.gd` — export_data/import_data 추가
- `scripts/systems/memory_manager.gd` — import_data 추가
- `project.godot` — SaveManager 오토로드
- 모든 .tscn — uid 제거, Color alpha 수정

### 다음 세션 (S07) 할 일
- [ ] Godot 재실행 + NPC 대화 테스트
- [ ] 타이틀 화면 (Cover.png, 새 게임/이어하기)
- [ ] CG 이벤트 시스템 (풀스크린 CG 표시)

### 메모
- SaveManager는 F6/F7 키를 _unhandled_input에서 직접 처리 (input map 불필요).
- 세이브 파일 경로: user://saves/save_1.json ~ save_3.json
- get_save_info()로 슬롯 선택 UI에서 저장 시간/챕터/연소 수 표시 가능.

---

## S07 — 2026-04-05 (타이틀 화면 + CG 이벤트 시스템)

### 완료
- [x] **타이틀 화면:** main.tscn을 Cover.png 배경 타이틀로 전환
  - New Game / Continue / Quit 버튼
  - Continue는 세이브 없으면 비활성
  - New Game 시 기억/플래그 초기화
  - main_scene을 main.tscn으로 복원
- [x] **CG 이벤트 시스템 (CgViewer 오토로드)**
  - show_cg(path, text, auto_close, callback) — 풀스크린 CG 표시
  - 페이드 인/아웃, 텍스트 오버레이, Space로 닫기
  - DialogueManager 연동: 대화 라인에 "cg" 키 있으면 자동 CG 표시
- [x] **CG 이미지 2장 복사:** ch1_forest.jpg, ch1_ash_rain.jpg

### 변경된 파일
- `scenes/main/main.tscn` — 타이틀 화면으로 전면 재작성
- `scenes/main/main.gd` — 타이틀 메뉴 로직
- `scripts/ui/cg_viewer.gd` — **신규** CG 이벤트 시스템
- `assets/cg/ch1_forest.jpg` — **신규** Chapter 1 숲 CG
- `assets/cg/ch1_ash_rain.jpg` — **신규** Chapter 1 재비 CG
- `project.godot` — CgViewer 오토로드 + main_scene 복원

### 다음 세션 (S08) 할 일
- [x] Chapter 1 스토리 흐름 연결 → S08에서 완료
- [x] 재비 파티클 이펙트 → S08에서 완료

### 메모
- CgViewer는 layer 45. MemoryUI(40) < CgViewer(45) < DialogueBox(50) < SystemLog(60) < SceneTransition(100).
- 대화 JSON에서 CG 사용: {"speaker": "", "text": "...", "cg": "res://assets/cg/ch1_forest.jpg", "cg_text": "The forest stretched..."}
- 오토로드 총 10개: GameManager, MemoryManager, DialogueManager, SceneTransition, DialogueBox, MemoryUI, SystemLog, BattleManager, SaveManager, CgViewer

---

## S08 — 2026-04-05 (Chapter 1 스토리 흐름 + 재비 파티클)

### 완료
- [x] **재비(Ash Rain) 파티클:** GPUParticles2D 기반, 회색 플레이크 하강, Player에 부착
  - ParticleProcessMaterial: 느린 하강 + turbulence 좌우 흔들림
  - GradientTexture로 서서히 투명해지는 효과
  - set_intensity()로 강도 조절 가능
- [x] **Chapter 1 스토리 시퀀스:** flag 기반 자동 진행
  - 1. 맵 진입 → opening_void_beast 대화 + 숲 CG (자동)
  - 2. opening 종료 → elia_appears 대화 (자동)
  - 3. elia 종료 → 재비 시작 + ash_rain 대화 + 재비 CG (자동)
  - 4. ash_rain 종료 → 자유 탐색 (전투, NPC 재대화 가능)
  - 5. 남쪽 끝 도달 → camp_night 대화 + 선택지
  - 6. camp 종료 → ch1_complete 플래그, chapter=2
- [x] **대화 데이터 확장:** CG 키 추가, 엘리아 재대화(elia_talk) 추가
- [x] **전투 트리거 위치 조정:** 스토리 동선에 안 겹치도록 이동
- [x] **야영 트리거:** 남쪽 길 끝에 Area2D (ash_rain 본 후만 활성)

### 변경된 파일
- `scripts/effects/ash_rain.gd` — **신규** 재비 파티클
- `scenes/maps/rim_forest.gd` — 스토리 시퀀스 + 재비 + 야영 트리거
- `data/chapter1_dialogue.json` — CG 키 + 대화 확장

### 다음 세션 (S09) 할 일
- [x] Ch2 맵 + 스토리 → S09에서 완료
- [x] 엘리아 동행 시스템 → S09에서 완료

### 메모
- 스토리 시퀀스는 CONNECT_ONE_SHOT으로 연결. 대화 종료 시 다음 단계 자동 진행.
- 전투 트리거를 스토리 동선(중앙 길)에서 벗어나도록 좌상/우상으로 이동.

---

## S09 — 2026-04-05 (엘리아 동행 + Ch2 베르단 시장)

### 완료
- [x] **엘리아 동행 시스템 (Companion)**
  - CharacterBody2D 기반, 플레이어를 따라다님
  - MIN_DISTANCE(40px) 이내 멈춤, MAX_DISTANCE(200px) 초과 시 텔레포트
  - 플레이스홀더 스프라이트 (은발 + 파란 눈)
  - interact() 지원 (대화 가능)
  - Player에 "player" 그룹 추가 → get_first_node_in_group()으로 탐색
- [x] **Ch2 베르단 시장 맵 (30x20 타일)**
  - 돌바닥/벽/노점/골목 5종 타일
  - 말렛 NPC 배치 (The Sump 골목 안쪽)
  - 엘리아 Companion 배치
  - 도착 대화 자동 시작
- [x] **Ch2 대화 데이터**
  - verdan_arrival: 베르단 도착 묘사
  - malet_encounter: 말렛 등장 + 거래 제안 (Grade 2 기억 요구)
  - malet_deal: 선택지 (수락/거절) + 기억 추출 장면
  - malet_reward: BL-07 경로, 세이블 이름, 카이로스 경고
  - elia_ch2_talk: 엘리아 재대화
- [x] **Ch1→Ch2 자동 전환:** camp_night 종료 → verdan_market 씬 전환

### 변경된 파일
- `scripts/core/companion.gd` — **신규** 동행 NPC
- `scenes/npc/companion.tscn` — **신규** 동행 NPC 씬
- `scenes/maps/verdan_market.gd` — **신규** Ch2 맵
- `scenes/maps/verdan_market.tscn` — **신규** Ch2 맵 씬
- `data/chapter2_dialogue.json` — **신규** Ch2 대화
- `scenes/maps/rim_forest.gd` — Ch2 전환 추가
- `scripts/core/player.gd` — "player" 그룹 추가

### 다음 세션 (S10) 할 일
- [x] 시각적 품질 개선 → S10에서 완료

### 메모
- Companion은 collision_layer=4(Interactables), Player의 InteractionRay mask가 감지 가능.
- 말렛 대화에서 "Accept the deal" 선택 시 identity_first_sword 기억 자동 연소.
- Ch2 맵의 골목(ALLEY) 타일은 The Sump 입구를 표현.

---

## S10 — 2026-04-05 (시각적 품질 개선)

### 완료
- [x] **전투 배경 이미지:** BattleManager에 bg_image/enemy_image 경로 지원, 전투 씬에서 TextureRect로 표시
- [x] **공허수 이미지:** void_beast.jpg → 전투 시 실제 크리처 이미지 표시 (ColorRect 대체)
- [x] **전투 배경:** 림 외곽 숲 전투 시 ch1_forest.jpg 배경
- [x] **타이틀 버튼 스타일링:** 다크 판타지 스타일 (어두운 배경, 골드 테두리, 호버 강조, 18px 폰트)
- [x] **대화 나레이션 스타일 분리:** 나레이션=어두운 크림색, 시스템 로그=청록색, 일반 대사=밝은 크림색
- [x] **Ch2 도착 CG:** verdan_arrival 첫 대사에 ch2_verdan.jpg CG 연결
- [x] **CG 이미지 3장 추가:** void_beast.jpg, ch2_verdan.jpg, village_seam.jpg

### 변경된 파일
- `assets/cg/void_beast.jpg` — **신규**
- `assets/cg/ch2_verdan.jpg` — **신규**
- `assets/cg/village_seam.jpg` — **신규** (Ch3 대비)
- `scripts/systems/battle_manager.gd` — bg_image/enemy_image 지원
- `scenes/battle/battle_scene.gd` — 배경 이미지 + 적 이미지 TextureRect
- `scenes/maps/rim_forest.gd` — 전투 트리거에 이미지 경로 전달
- `scenes/main/main.gd` — 버튼 스타일링
- `scripts/ui/dialogue_box.gd` — 나레이션/시스템 로그 색상 분리
- `data/chapter2_dialogue.json` — 도착 CG 추가

### 다음 세션 (S11) 할 일
- [x] 새 이미지 배치 + Ch2 완료 → S11에서 완료

---

## S11 — 2026-04-05 (새 이미지 배치 + Ch2 거래 흐름 완성)

### 완료
- [x] **포트레이트 6장 추가:** 말렛, 카이로스, 세이블(2장), 아렐 angry/pain
- [x] **CG 4장 추가:** 카이로스 경고, 기억 추출, Ch1 녹색 나무, Ash Crawler
- [x] **PORTRAIT_MAP 확장:** 5캐릭터 11장 포트레이트, DEFAULT_PORTRAITS 5명
- [x] **Ch2 말렛 거래 흐름 완성:**
  - malet_encounter → malet_deal(선택지) → malet_deal 추출 CG → malet_reward + 카이로스 경고 CG
  - 거절 시 malet_refused 대화 → 재대화로 재시도 가능
  - 모든 대사에 포트레이트 키 연결
- [x] **Ch1 녹색 나무 히든 CG:** camp 종료 → 3초간 녹색 나무 CG → Ch2 전환
- [x] **Ash Crawler 전투 이미지:** 절지류 크리처 이미지 전투 씬에 표시

### 변경된 파일
- `assets/portraits/` — 6장 추가 (malet, kairos, sable x2, arrel angry/pain)
- `assets/cg/` — 4장 추가 (kairos_warning, extraction, green_tree, ash_crawler)
- `scripts/ui/dialogue_box.gd` — PORTRAIT_MAP 11개 + DEFAULT_PORTRAITS 5명
- `data/chapter2_dialogue.json` — 포트레이트 키 + CG + malet_refused 추가
- `scenes/maps/verdan_market.gd` — 말렛 거래 자동 연결 흐름
- `scenes/maps/rim_forest.gd` — 녹색 나무 CG + Ash Crawler 이미지

### 에셋 현황
- 포트레이트: 11장 (아렐 5, 엘리아 2, 말렛 1, 카이로스 1, 세이블 2)
- CG: 9장 (cover, ch1 x3, ch2 x3, void_beast, ash_crawler)

### 다음 세션 (S12) 할 일
- [x] Ch3 콘텐츠 → S12에서 완료

---

## S12 — 2026-04-05 (Ch2→Ch3 전환 + 크럼블링 코스트 맵)

### 완료
- [x] **Ch2 → Ch3 전환:** 말렛 보상 대화 후 자동 크럼블링 코스트 이동
- [x] **크럼블링 코스트 맵 (25x18):** 바위/모래/절벽/물/길 5종 타일, 물 파도 디테일
- [x] **Ch3 스토리 시퀀스:**
  - 도착 대화 + 크럼블링 코스트 CG
  - 카이로스 목격 이벤트 (2초 딜레이 후 자동 + CG)
  - 북쪽 도달 → The Seam 도착 + 세이블 첫 만남 + village CG
- [x] **Ch3 대화 데이터:** coast_arrival, kairos_sighting, seam_arrival, elia_coast_talk
- [x] **Coastal Void Beast 전투:** HP 100, ATK 18 (강화 공허수)
- [x] **크럼블링 코스트 BGM:** dialogue_tense.mp3 (Morning Light)

### 변경된 파일
- `scenes/maps/verdan_market.gd` — Ch3 전환 + chapter=3
- `scenes/maps/crumbling_coast.tscn` — **신규** Ch3 맵
- `scenes/maps/crumbling_coast.gd` — **신규** Ch3 맵 스크립트
- `data/chapter3_dialogue.json` — **신규** Ch3 대화
- `scripts/systems/audio_manager.gd` — 크럼블링 코스트 BGM 매핑

### 전체 플레이 흐름 (Ch1~Ch3)
타이틀 → Ch1 림 외곽 숲 (오프닝→엘리아→재비→전투→야영→녹색나무) → Ch2 베르단 시장 (도착→말렛 거래→추출→보상+카이로스 경고) → Ch3 크럼블링 코스트 (도착→카이로스 목격→전투→The Seam 도착+세이블)

### 다음 세션 (S13) 할 일
- [x] The Seam 맵 (Ch4 세이블 거점) → S13에서 완료
- [x] 전투 시스템 확장 (적 다양화, 보스전) → S13에서 완료

---

## S13 — 2026-04-05 (The Seam 맵 + Ch4 + 전투 확장)

### 완료
- [x] **The Seam 맵 (25x18):** 절벽/돌/오두막/정원/길/물/랜턴 7종 타일
  - 랜턴: 따뜻한 빛 효과 (중심점 + 은은한 후광)
  - 정원: 랜덤 색상 꽃 디테일 (붉은/노란/보라 — 세상에서 사라져가는 색들)
  - 물: 작은 개울 + 파도 디테일
- [x] **Ch4 스토리 시퀀스:**
  - seam_welcome: The Seam 도착 + village CG
  - sable_briefing: BL-07 보이드 홀 조사 미션 + BL-07 내부 CG
  - bl07_entrance: 보이드 홀 앞 + void_portal CG → Shade Sentinel 보스전
  - bl07_aftermath: 보스전 후 에필로그 → Ch5 전환
- [x] **Ch4 대화 데이터:** seam_welcome, sable_briefing, bl07_entrance, bl07_aftermath, sable_talk, elia_seam_talk
- [x] **Ch3→Ch4 전환:** crumbling_coast.gd _on_seam_ended()에서 The Seam으로 자동 전환
- [x] **전투 시스템 확장:**
  - Enemy 클래스에 is_boss, phase, abilities 추가
  - 보스 페이즈 시스템: HP 50% 이하 → 페이즈 2 전환 + 알림
  - 적 특수 능력 3종: drain(HP 흡수), shield(방어 배리어), multi_hit(2연타)
  - 보스/공허수 도주 불가
  - enemy_shielded: 일반 공격 50% 감소, 번 스킬 30% 감소
- [x] **Shade Sentinel 보스전:** HP 180, ATK 24, 공허수+보스, 능력 3종 모두 보유
- [x] **Coastal Void Beast 능력 추가:** drain 능력
- [x] **The Seam BGM:** exploration.mp3 (Sunrise Over Waves) 매핑

### 변경된 파일
- `scenes/maps/the_seam.tscn` — **신규** Ch4 맵
- `scenes/maps/the_seam.gd` — **신규** Ch4 맵 스크립트
- `data/chapter4_dialogue.json` — **신규** Ch4 대화
- `scenes/maps/crumbling_coast.gd` — Ch4 전환 + Void Beast drain 능력
- `scripts/systems/battle_manager.gd` — 보스/페이즈/특수능력 시스템
- `scripts/systems/audio_manager.gd` — The Seam BGM 매핑

### 전체 플레이 흐름 (Ch1~Ch4)
타이틀 → Ch1 림 외곽 숲 (오프닝→엘리아→재비→전투→야영→녹색나무) → Ch2 베르단 시장 (도착→말렛 거래→추출→보상+카이로스 경고) → Ch3 크럼블링 코스트 (도착→카이로스 목격→전투→The Seam 도착+세이블) → Ch4 The Seam (도착→세이블 브리핑→BL-07 보스전→에필로그)

### 에셋 현황
- 포트레이트: 11장 (아렐 5, 엘리아 2, 말렛 1, 카이로스 1, 세이블 2)
- CG: 18장 (cover, ch1 x4, ch2 x4, ch3 x1, village_seam, bl07_interior, void_portal, void_beast, ash_crawler, arrel_combat, item x2)
- BGM: 7트랙 (title, ch1_forest, ch1_camp, ch2_verdan, battle, dialogue_tense, exploration)

### 다음 세션 (S14) 할 일
- [x] Ch5 콘텐츠 → S14에서 완료
- [x] 기억 시스템 확장 → S14에서 완료

---

## S14 — 2026-04-05 (Ch5 The Seal + 기억 시스템 확장)

### 완료
- [x] **S13 버그 수정:** the_seam.gd _setup_battle_triggers() 호출 누락 수정
- [x] **챕터별 기억 추가 시스템:** MemoryManager.add_chapter_memories(chapter)
  - Ch3: "Salt Wind on the Cliffs" (G5), "Walking With Someone" (G4, Elia 관련)
  - Ch4: "The Woman Who Came Back" (G3, Sable 관련), "Colors That Shouldn't Exist" (G5), "Flowers From Every Season" (G4)
  - Ch5: "What You Saw Inside BL-07" (G2, 보이드 경험)
- [x] **BL-07 보이드 내부 맵 (20x20):** 허공/부유파편/길/균열/핵심부 5종 타일
  - 핵심부 맥동 효과 (_process에서 sin 기반 색상 변화)
  - 부유 파편 시각 디테일
  - 2개 전투 트리거: Void Fragment (HP 70), Memory Eater (HP 90, drain+multi_hit)
- [x] **Ch5 스토리 시퀀스 (The Seal 결정):**
  - void_entry: BL-07 진입 + void_portal CG
  - void_core: 핵심부 도달 + 아렐의 깨달음
  - seal_decision: **플레이어 선택** — "이름을 태워 봉인" vs "이름을 지키고 다른 방법을 찾기"
  - seal_complete: Zero Burn 경로 (이름 상실, 엘리아를 알아보지 못함)
  - seal_refused: 보존 경로 (BL-07 미봉인, 시간과의 싸움)
- [x] **Ch5 대화 데이터:** void_entry, void_core, seal_decision, seal_complete, seal_refused, elia_void_talk
- [x] **Ch4→Ch5 전환:** the_seam.gd 보스전 에필로그 → BL-07 내부 자동 전환
- [x] **BL-07 BGM 매핑:** dialogue_tense.mp3
- [x] **선택지 flag 키 수정:** chapter5에서 "flag" → "set_flag" (DialogueManager 호환)

### 변경된 파일
- `scenes/maps/bl07_void.tscn` — **신규** Ch5 맵
- `scenes/maps/bl07_void.gd` — **신규** Ch5 맵 스크립트 (The Seal 메카닉)
- `data/chapter5_dialogue.json` — **신규** Ch5 대화 (분기 엔딩)
- `scripts/systems/memory_manager.gd` — 챕터별 기억 추가 + _has_memory
- `scenes/maps/crumbling_coast.gd` — Ch3 기억 자동 추가
- `scenes/maps/the_seam.gd` — Ch4 기억 추가 + _setup_battle_triggers 호출 + Ch5 전환
- `scripts/systems/audio_manager.gd` — BL-07 BGM 매핑

### 전체 플레이 흐름 (Ch1~Ch5)
타이틀 → Ch1 림 외곽 숲 → Ch2 베르단 시장 → Ch3 크럼블링 코스트 → Ch4 The Seam (세이블 브리핑→보스전) → Ch5 BL-07 내부 (탐색→핵심부→**The Seal 선택**)
- **Zero Burn 경로:** 이름 연소 → BL-07 봉인 → 아렐의 정체성 상실
- **보존 경로:** 이름 보존 → BL-07 미봉인 → 다른 방법 탐색

### 기억 총 현황 (최대)
| 등급 | 초기 | Ch3 추가 | Ch4 추가 | Ch5 추가 | 계 |
|------|------|----------|----------|----------|-----|
| G5 | 2 | 1 | 1 | 0 | 4 |
| G4 | 2 | 1 | 1 | 0 | 4 |
| G3 | 1 | 0 | 1 | 0 | 2 |
| G2 | 1 | 0 | 0 | 1 | 2 |
| G1 | 1 | 0 | 0 | 0 | 1 |
| **계** | **7** | **2** | **3** | **1** | **13** |

### 다음 세션 (S15) 할 일
- [x] 그래픽 업그레이드 → S15에서 완료

---

## S15 — 2026-04-05 (그래픽 대규모 업그레이드)

### 완료
- [x] **PixelSprite 유틸리티 (pixel_sprite.gd):**
  - Image.set_pixel() 기반 상세 픽��아트 캐릭터 생성
  - 32x32 프레임, 4방향 x (idle + 4 walk) = 20프레임 완전 애니메이션
  - 머리카락 텍스처/하이라이트, 눈(홍채+하이라이트), 코/입 힌트
  - 코트 주름/밝은면, 팔 스윙, 발 스텝 애니메이션
  - 프리셋: arrel_config (은청 머리+남색 코트+검), elia_config (은발+갈색 망토+브로치), sable_config (짧��� 검은 머리+실용복+흉터)
  - npc_config(color) — npc_color 기반 자동 생성
- [x] **TilePainter 유틸리티 (tile_painter.gd):**
  - TileMapLayer + TileSetAtlasSource 기반 맵 렌더링
  - 20종 타일 디테일: grass(풀잎+꽃), tree(줄기+수관), bush, water(파도라인), path(자갈), stone(줄눈), wall(벽돌패턴), stall(천막+물건), door, alley(물웅덩이), sand(바람자국), cliff(균열선), rock, hut(지붕+문+창), garden(색색의 꽃), lantern(빛+후광), void(에너지점), fragment(부유파편), crack(보라 균열), core(맥동)
  - create_tilemap() → TileMapLayer 반환
  - add_collisions() → 벽 충돌 StaticBody2D 일괄 생성
- [x] **캐릭터 스프라이트 교체:**
  - player.gd → PixelSprite.arrel_config() (120줄 삭제)
  - companion.gd → Sprite2D→AnimatedSprite2D 전환 + PixelSprite (방향 애니메이션 추가)
  - npc.gd → PixelSprite.npc_config(npc_color), 이름별 전용 config (Sable, Malet)
- [x] **맵 5개 TileMap 전환:**
  - rim_forest.gd — ColorRect 50줄 → TilePainter 10줄
  - verdan_market.gd — ColorRect 35줄 → TilePainter 10줄
  - crumbling_coast.gd — ColorRect 30줄 → TilePainter 8줄
  - the_seam.gd — ColorRect 60줄 (랜턴/정원 포함) → TilePainter 10줄
  - bl07_void.gd — ColorRect + 맥동 → TilePainter + 코어 오버레이

### 변경된 파일
- `scripts/utils/pixel_sprite.gd` — **신규** 캐릭터 스프라이트 유틸리티
- `scripts/utils/tile_painter.gd` — **신규** 타일맵 생성 유틸리티
- `scripts/core/player.gd` — 스프라이트 코드 120줄 ��� 2줄
- `scripts/core/companion.gd` — AnimatedSprite2D + 방향 애니메이션
- `scripts/core/npc.gd` — PixelSprite 기반 생성
- `scenes/maps/rim_forest.gd` — TileMap 전환
- `scenes/maps/verdan_market.gd` — TileMap 전환
- `scenes/maps/crumbling_coast.gd` — TileMap 전환
- `scenes/maps/the_seam.gd` — TileMap 전환
- `scenes/maps/bl07_void.gd` — TileMap 전환

### 그래픽 개선 요약
| Before | After |
|--------|-------|
| 플랫 ColorRect 타일 | 텍스처 있는 TileMap (풀잎, 벽돌, 파도, 꽃 등) |
| 사각형 블록 캐릭터 | 픽���아트 캐릭터 (머리/눈/코/입/옷 디테일) |
| 정적 엘리아 (Sprite2D 1장) | 4방향 걷기 애니메이션 엘리아 |
| 맵당 수백 개 ColorRect 노드 | TileMapLayer 1개 (성능 대폭 개선) |

### 다음 세션 (S16) 할 일
- [ ] Godot 전체 테스트 (그래픽 확인 + Ch1→Ch5)
- [ ] Ch6 콘텐츠 (분기 후 에필로그)
- [ ] UI 테마 개선

---

## S16 — 2026-04-05 (에필로그 + UI 테마 개선)

### 완료
- [x] **전투씬 BGM 제거:**
  - audio_manager.gd: SCENE_BGM에서 battle_scene 매핑 제거
  - 전투씬 진입 시 stop_bgm() 호출 (무음 전투)
- [x] **Ch6 에필로그 대화 데이터:**
  - `data/chapter6_dialogue.json` — 4개 대화 키
  - epilogue_zero_burn: 이름을 잃은 아렐 (34줄, Elia/Sable 대화)
  - epilogue_preservation: 이름을 지킨 아렐 (24줄, 보이드 홀 미해결)
  - elia_epilogue_talk: 보이드 홀의 패턴 발견 (기억 연소가 원인?)
  - sable_epilogue_talk: 동쪽 정착촌 기억 유실 → 후속작 암시
- [x] **Ch6 에필로그 트리거 연결 (the_seam.gd):**
  - Ch5 완료 후 The Seam 복귀 시 자동 에필로그 시작
  - zero_burn_path / seal_refused 플래그에 따라 분기
  - 에필로그 후 Elia/Sable NPC 개별 대화 트리거
  - 에필로그 시 플레이어 위치를 마을 중앙으로 변경
- [x] **UI 테마 개선:**
  - `scripts/utils/ui_theme.gd` — 공통 UI 색상/스타일 상수 유틸리티
  - 캐릭터별 화자 이름 색상 (Arrel=은청, Elia=라벤더, Sable=보라, Malet=앰버, Kairos=청록)
  - DialogueBox에 UITheme 적용 (화자별 이름 색상 + 나레이션/시스템 색상)
  - 전투 HP바 애니메이션 (데미지 시 Tween 부드러운 감소)
  - HP 25% 이하 시 바 색상 빨간색 전환

### 변경된 파일
- `scripts/systems/audio_manager.gd` — 전투 BGM 제거
- `data/chapter6_dialogue.json` — **신규** 에필로그 대화 데이터
- `scenes/maps/the_seam.gd` — Ch6 에필로그 트리거 + NPC 대화
- `scripts/utils/ui_theme.gd` — **신규** 공통 UI 테마 유틸리티
- `scripts/ui/dialogue_box.gd` — 화자별 이름 색상 적용
- `scenes/battle/battle_scene.gd` — HP바 트위닝 + 저HP 색상 변환

### 스토리 완결 구조
```
Ch1 림 외곽 숲 → Ch2 베르단 시장 → Ch3 크럼블링 코스트
→ Ch4 The Seam → Ch5 BL-07 보이드
→ Ch6 에필로그 (Zero Burn / Preservation 분기)
→ NPC 후일담 대화 (Elia: 보이드 홀 원인 발견, Sable: 후속 탐사 암시)
```

### 다음 세션 (S17) 할 일
- [ ] Godot 전체 테스트 (Ch1→Ch6, 양 분기 엔딩 확인)
- [ ] 전투 밸런스 조정 (필요 시)
- [ ] 추가 CG/이미지 배치 (에필로그용)

---

## S17 — 2026-04-05 (전체 테스트 + 버그 수정 + 밸런스)

### 발견된 버그 및 수정
- [x] **전투 재트리거 (Critical):**
  - 문제: 전투 영역에 다시 들어가면 무한 전투 발생
  - 수정: 4개 맵 모두 `_battle_counter` + 1회성 플래그(`battle_rim_1` 등) 추가
  - 영향: rim_forest.gd, crumbling_coast.gd, the_seam.gd, bl07_void.gd
- [x] **게임 오버 소프트 록 (Critical):**
  - 문제: 패배 시 HP 0으로 맵 복귀 → 이후 전투 불가
  - 수정: 패배 시 HP 30% 회복 + "Something pulls you back..." 메시지
- [x] **보스전 시그널 크래시 (Critical):**
  - 문제: the_seam.gd에서 `battle_ended.connect(_on_boss_defeated)` 후 씬 전환 → 노드 해제 → 콜백 크래시
  - 수정: 시그널 대신 플래그 기반 감지 (맵 재진입 시 `ch4_bl07_entered && !ch4_complete` 체크)
  - 추가: 패배 시 `ch4_bl07_entered` 플래그 리셋으로 재도전 가능
- [x] **대화 CG 입력 충돌 (Medium):**
  - 문제: CG 표시 시 입력 대기(waiting_for_input) + 대화 입력 처리 동시 발생
  - 수정: 대화 중 CG는 `_show_cg_background()` (non-blocking) 사용, 대화 끝나면 자동 닫기
- [x] **동행 캐릭터 애니메이션 (Minor):**
  - 문제: 엘리아가 멈추면 마지막 걷기 프레임에서 정지 (idle 미전환)
  - 수정: `dist < MIN_DISTANCE` 시 idle 애니메이션 재생
- [x] **챕터 기억 누락 (Minor):**
  - 문제: the_seam.gd에서 Ch4 기억이 `_start_ch4_sequence()`에서만 추가 → 세이브 로드 시 누락
  - 수정: `_ready()`에서 `add_chapter_memories(4)` 호출 (중복 방지는 `_has_memory()`)

### 밸런스 조정
- [x] **보이드 적 일반 공격 가능:**
  - 변경 전: 보이드 적 = 일반 공격 완전 무효 → 기억 소진 시 진행 불가
  - 변경 후: 보이드 적에게 30% 감쇠 데미지 ("Your blade struggles against the void...")
- [x] **챕터별 플레이어 성장:**
  - 기본 공격력: 15 + (chapter-1) × 3 (Ch1=15, Ch5=27)
  - 최대 HP: 100 + (chapter-1) × 15 (Ch1=100, Ch5=160)
  - 전투 시작 시 자동 적용 (HP 성장 시 15 회복)

### 밸런스 테이블
| 챕터 | 플레이어 HP | 공격력 | 적 HP | 적 ATK | 특수 |
|------|-----------|--------|-------|--------|------|
| Ch1  | 100       | 15-25  | 40/80 | 8/15   | Void Beast |
| Ch3  | 130       | 21-31  | 100   | 18     | Void+Drain |
| Ch4  | 145       | 24-34  | 60/180| 14/24  | Boss: 3 abilities |
| Ch5  | 160       | 27-37  | 70/90 | 16/20  | Drain+Multi |

### 변경된 파일
- `scenes/maps/rim_forest.gd` — 전투 1회성 플래그
- `scenes/maps/crumbling_coast.gd` — 전투 1회성 플래그
- `scenes/maps/the_seam.gd` — 전투 플래그 + 보스전 플래그 기반 감지 + 기억 추가 위치 변경
- `scenes/maps/bl07_void.gd` — 전투 1회성 플래그
- `scripts/systems/battle_manager.gd` — 패배 HP 회복, 보이드 감쇠, 챕터별 성장
- `scripts/ui/cg_viewer.gd` — 대화 중 CG non-blocking 모드
- `scripts/core/companion.gd` — idle 애니메이션 수정

### 다음 세션 (S18) 할 일
- [ ] Godot 실행 테스트 (Ch1→Ch6, 양 분기 실제 플레이)
- [ ] 추가 CG/이미지 (에필로그용)
- [ ] 사운드 추가 (전투 효과음 확장)

---

## S18 — 2026-04-05 (CG 보완 + SFX 확장 + 연출 강화)

### 완료
- [x] **CG 이미지 연결 보완:**
  - `item_memory_ampoule.jpg` → Ch5 seal_decision (보이드 홀 맥동 장면), seal_complete (백색 불꽃 장면)
  - `item_extractor.jpg` → Ch2 malet_encounter (말렛의 가격 제시 장면)
  - 전체 17장 CG 100% 연결 완료
- [x] **SFX 6종 추가 (총 12종):**
  - `shield` — 적 방어막 (저음 울림)
  - `drain` — 생명력 흡수 (역방향 스윕)
  - `phase_change` — 보스 페이즈 전환 (깊은 공명)
  - `defeat` — 플레이어 패배 (하강 톤)
  - `flee` — 도주 (빠른 상승음)
  - `memory_add` — 기억 획득 (맑은 C-E-G 화음)
  - `void_pulse` — 보이드 맥동 (불안한 저주파)
- [x] **SFX 연결:**
  - battle_manager.gd: drain, shield, phase_change, defeat, flee
  - memory_manager.gd: memory_add (add_memory 시 자동 재생)
  - bl07_void.gd: void_pulse (Seal 결정 장면)
- [x] **Ch5 Seal 연출 강화:**
  - Zero Burn: void_pulse SFX → 화면 백색 플래시 (1.2초) → 서서히 복귀 → seal_complete 대화
  - Preservation: void_pulse SFX → 짧은 딜레이 → seal_refused 대화

### 변경된 파일
- `data/chapter2_dialogue.json` — item_extractor.jpg CG 추가
- `data/chapter5_dialogue.json` — item_memory_ampoule.jpg CG 2곳 추가
- `scripts/systems/audio_manager.gd` — SFX 6종 추가 (shield, drain, phase_change, defeat, flee, memory_add, void_pulse)
- `scripts/systems/battle_manager.gd` — 새 SFX 연결 (drain, shield, phase_change, defeat, flee)
- `scripts/systems/memory_manager.gd` — memory_add SFX (기억 추가 시)
- `scenes/maps/bl07_void.gd` — Seal 장면 연출 강화 (void_pulse + 백색 플래시)

### 전체 에셋 현황
| 카테고리 | 수량 | 상태 |
|---------|------|------|
| CG 이미지 | 17장 | 100% 연결 |
| 포트레이트 | 11장 | 100% 연결 |
| SFX | 12종 | 코드 생성 (외부 파일 불필요) |
| BGM | 5곡 | 씬별 매핑 (전투=무음) |

### 다음 세션 (S19) 할 일
- [x] BGM 확장 → S19에서 완료
- [x] 타이틀 화면 흐름 정리 → S19에서 완료
- [x] 코드 검증 → S19에서 완료

---

## S19 — 2026-04-06 (BGM 확장 + 타이틀 수정 + 코드 검증)

### 완료
- [x] **BGM 3트랙 추가 (총 10트랙):**
  - `epilogue.mp3` (Quiet Gravity) → Ch6 에필로그 전용 BGM
  - `ch5_void.mp3` (Invisible Room) → BL-07 보이드 내부 전용 BGM (dialogue_tense에서 교체)
  - `battle_theme.mp3` (Raindrops in D Minor) → 전투 BGM 복원 (무음→BGM)
- [x] **AudioManager BGM 매핑 확장:**
  - BL-07: dialogue_tense → ch5_void.mp3 전환 (보이드 분위기 강화)
  - 전투씬: stop_bgm() → play_bgm("battle_theme.mp3") (전투 BGM 복원)
  - The Seam 에필로그: 진입 시 epilogue.mp3 수동 재생
- [x] **타이틀 New Game 초기화 버그 수정:**
  - 문제: max_hp, grains, elia_with_party가 리셋되지 않음 (이전 플레이 데이터 잔존)
  - 수정: player_data를 완전히 새 Dictionary로 교체
- [x] **코드 레벨 전체 검증:**
  - 전 스크립트 null 접근, 시그널 크래시, 정수 나눗셈 점검
  - 심각한 버그 없음 확인 (S17 수정이 잘 적용됨)

### 변경된 파일
- `assets/audio/bgm/epilogue.mp3` — **신규** (mugic/Quiet Gravity)
- `assets/audio/bgm/ch5_void.mp3` — **신규** (mugic/Invisible Room)
- `assets/audio/bgm/battle_theme.mp3` — **신규** (mugic/Raindrops in D Minor)
- `scripts/systems/audio_manager.gd` — BL-07 BGM 교체 + 전투 BGM 복원
- `scenes/maps/the_seam.gd` — 에필로그 BGM 재생
- `scenes/main/main.gd` — New Game player_data 완전 초기화

### 전체 BGM 현황
| BGM | 파일 | 사용처 |
|-----|------|--------|
| title.mp3 | 타이틀 | 타이틀 화면 |
| ch1_forest.mp3 | 림 외곽 숲 | Ch1 맵 |
| ch1_camp.mp3 | 야영 | Ch1 야영 장면 |
| ch2_verdan.mp3 | 베르단 시장 | Ch2 맵 |
| dialogue_tense.mp3 | 긴장 대화 | Ch3 크럼블링 코스트 |
| exploration.mp3 | 탐색 | Ch4 The Seam |
| ch5_void.mp3 | 보이드 내부 | Ch5 BL-07 |
| epilogue.mp3 | 에필로그 | Ch6 에필로그 |
| battle_theme.mp3 | 전투 | 모든 전투 씬 |
| battle.mp3 | (미사용) | 예비 |

### 다음 세션 (S20) 할 일
- [x] 폴리싱 → S20에서 완료

---

## S20 — 2026-04-06 (폴리싱 — 일시정지 메뉴 + 전투 피드백 + 환경 효과)

### 완료
- [x] **일시정지 메뉴 (PauseMenu 오토로드):**
  - ESC 키로 토글 (EXPLORATION 상태에서만)
  - Resume / Save (Slot 1) / Load (Slot 1) / Return to Title / Quit
  - 현재 챕터, HP, 기억 상태, 세이브 슬롯 정보 표시
  - 세이브 성공 피드백 ("SAVED!" 텍스트 전환)
  - CanvasLayer 55 (DialogueBox↔SystemLog 사이)
  - `process_mode = ALWAYS` — pause 중에도 작동
  - F6/F7 퀵세이브 힌트 표시
- [x] **전투 시각 피드백 (battle_scene.gd):**
  - 데미지 숫자: 피격 시 떠오르며 사라지는 텍스트 (적=노란, 플레이어=빨간)
  - 히트 플래시: 화면 전체 빨간/흰색 번쩍임 (0.2초 페이드)
  - 스크린 셰이크: 4프레임 랜덤 흔들림 (데미지마다 발동)
- [x] **맵 환경 효과 (MapEffects 유틸리티):**
  - `MapEffects.add_water_shimmer()` — 물 타일 위 반짝이는 반투명 라인 (sin 파동)
  - `MapEffects.add_lantern_lights()` — 랜턴 타일 주변 따뜻한 빛 (촛불 깜빡임)
  - `MapEffects.add_void_particles()` — GPUParticles2D 보라색 떠다니는 입자
  - The Seam: 물 반짝임 + 랜턴 빛
  - Crumbling Coast: 물 반짝임
  - BL-07 Void: 보이드 파티클

### 변경/생성된 파일
- `scripts/ui/pause_menu.gd` — **신규** 일시정지 메뉴 오토로드
- `scripts/utils/map_effects.gd` — **신규** 맵 환경 효과 유틸리티
- `project.godot` — PauseMenu 오토로드 추가
- `scenes/battle/battle_scene.gd` — 데미지 숫자 + 히트 플래시 + 스크린 셰이크
- `scenes/maps/the_seam.gd` — 물 반짝임 + 랜턴 빛 효과
- `scenes/maps/crumbling_coast.gd` — 물 반짝임 효과
- `scenes/maps/bl07_void.gd` — 보이드 파티클 효과

### 오토로드 현황 (12개)
| 이름 | 레이어 | 역할 |
|------|--------|------|
| GameManager | - | 게임 상태, 플래그, 플레이어 데이터 |
| MemoryManager | - | 기억 연소 시스템 |
| DialogueManager | - | 대화 진행 |
| SceneTransition | 100 | 페이드 인/아웃 |
| DialogueBox | 50 | 대화 UI |
| MemoryUI | 40 | 기억 서고 |
| SystemLog | 60 | 관리국 로그 팝업 |
| BattleManager | - | 전투 로직 |
| SaveManager | - | 세이브/로드 |
| CgViewer | 45 | CG 표시 |
| AudioManager | - | BGM/SFX |
| PauseMenu | 55 | **신규** 일시정지 메뉴 |

### 다음 세션 (S21) 할 일
- [x] 게임 오버 화면 → S21에서 완료
- [x] 히든 이벤트 → S21에서 완료
- [x] 코드 검증 → S21에서 완료

---

## S21 — 2026-04-06 (게임 오버 화면 + 히든 이벤트 + 플레이 검증)

### 완료
- [x] **게임 오버 화면 (game_over.tscn + .gd):**
  - 패배 시 자동 HP 복귀 대신 게임 오버 화면으로 전환
  - "You fell." + "Something pulls you back from the edge..."
  - 3개 선택지: Stagger On (HP 30%) / Load Save / Return to Title
  - BattleManager 상태 리셋 (IDLE + enemy null)
  - BattleManager._cleanup() 수정: defeat 시 game_over.tscn으로 전환
- [x] **히든 이벤트 2개:**
  - Ch1 림 외곽 숲 — "나무 그루터기" (A.E. 이니셜 각인, 아렐+엘리아 암시)
    - 맵 우측 하단, 1회성 트리거 (hidden_ch1_stump 플래그)
  - Ch4 The Seam — "숨겨진 정원" (금빛 꽃, 아이의 웃음소리 잔상)
    - 좌상단 정원 타일, 1회성 트리거 (hidden_ch4_garden 플래그)
- [x] **코드 레벨 전체 플레이 검증:**
  - Ch1→Ch6 양 분기 흐름 추적 완료
  - 게임 오버 → 재시도/로드/타이틀 흐름 검증
  - 보스전 패배 → 게임 오버 → 재시도 → 보스 재도전 흐름 검증
  - 히든 이벤트 트리거 논리 검증

### 변경/생성된 파일
- `scenes/ui/game_over.tscn` — **신규** 게임 오버 씬
- `scenes/ui/game_over.gd` — **신규** 게임 오버 로직
- `scripts/systems/battle_manager.gd` — 패배 시 game_over 씬 전환
- `data/chapter1_dialogue.json` — hidden_stump 대화 추가
- `data/chapter4_dialogue.json` — hidden_garden 대화 추가
- `scenes/maps/rim_forest.gd` — 히든 이벤트 트리거
- `scenes/maps/the_seam.gd` — 히든 이벤트 트리거

### 전체 플레이 흐름 (최종)
```
타이틀 (New Game / Continue / Quit)
  ↓
Ch1 림 외곽 숲 — 오프닝 → 엘리아 → 재비 → 전투 → [히든: 그루터기] → 야영 → 녹색 나무 CG
  ↓
Ch2 베르단 시장 — 도착 → 말렛 거래(수락/거절) → 추출 → 보상 + 카이로스 경고
  ↓
Ch3 크럼블링 코스트 — 도착 → 카이로스 목격 → 전투 → The Seam 도착
  ↓
Ch4 The Seam — 도착 → 세이블 브리핑 → [히든: 정원] → BL-07 보스전
  ↓
Ch5 BL-07 내부 — 진입 → 탐색+전투 → 핵심부 → **The Seal 결정**
  ├── Zero Burn: 이름 연소 → 백색 플래시 → seal_complete
  └── Preservation: 이름 보존 → seal_refused
  ↓
Ch6 에필로그 — The Seam 복귀 → 분기별 에필로그 → NPC 후일담
  ↓
패배 시: 게임 오버 화면 → Stagger On / Load / Title
```

### 다음 세션 (S22) 할 일
- [x] 그래픽 개선 + 전투씬 오버홀 → S22에서 완료
- [x] 사운드 폴리싱 (UI 조작음) → S22에서 완료

---

## S22 — 2026-04-06 (그래픽 대폭 개선 + 전투씬 오버홀 + UI SFX)

### 완료
- [x] **전투씬 대폭 개선 (battle_scene.gd 완전 재작성):**
  - 전투 인트로 연출: 검은 화면 → 적 이름/부제 표시 → 구분선 애니메이션 → 페이드인
  - 보스/공허수 인트로 차별화 (빨강/보라 + "BOSS"/"Void Beast" 부제)
  - 턴 표시: "— YOUR TURN —" / "— ENEMY TURN —" (컬러 + 페이드 애니메이션)
  - 공격 VFX: 물리 공격 → 대각선 슬래시 이펙트 (2중 라인)
  - 기억 연소 VFX: 불꽃 파티클 12개 (주황~빨강~노랑, 떠오르며 사라짐) + 중앙 플래시
  - 적 아이들 모션: sin파 기반 부드러운 상하 흔들림 (1.5Hz)
  - 상태 아이콘: SHIELD / PHASE / VOID 표시 (적 HP 패널 아래)
  - 데미지 숫자 개선: 크기 스케일링 (50+/100+), 드롭 섀도우, 스케일 펀치
  - 적 스프라이트 깜빡임: 피격 시 밝게 번쩍 → 복귀
  - 스크린 셰이크 개선: 5프레임 감쇠 흔들림 (intensity 파라미터)
  - 배경 비네트 오버레이 (상단/하단 어두운 바)
  - 버튼 pressed 스타일 추가, 전투 UI SFX 연동
- [x] **타일 깊이감 개선 (tile_painter.gd):**
  - `_paint_edge_shading()` 추가: 상단 하이라이트 + 하단/우측 그림자
  - 타일 종류별 셰이딩 강도 차등 (나무/벽 강, 바닥류 약, 물/보이드 최약)
- [x] **씬 전환 다양화 (scene_transition.gd):**
  - `change_scene_battle()` 추가: 다이아몬드 와이프 효과
  - 16x9 그리드, 중앙에서 퍼져나가는 검은 타일 → 페이드인
  - 모든 전투 진입 5곳에 적용 (림 숲, 크럼블링 코스트, The Seam, BL-07)
- [x] **UI SFX 5종 추가 (총 17종):**
  - `ui_hover` — 짧은 고음 틱 (0.04초)
  - `ui_select` — 명확한 확인음 (0.08초, 이중 사인파)
  - `ui_open` — 메뉴 열기 상승 스윕 (0.12초)
  - `ui_close` — 메뉴 닫기 하강 스윕 (0.1초)
  - `battle_intro` — 전투 진입 긴장 저음 (0.6초)
  - 전투 버튼 hover/pressed에 SFX 연동
- [x] **맵 비네트 오버레이:**
  - `MapEffects.add_vignette()` 추가 (상/하/좌/우 어두운 오버레이)
  - 5개 맵 전체 적용, BL-07은 강한 비네트 (0.6)
- [x] **안개 효과:**
  - `MapEffects.add_fog()` / `update_fog()` 추가
  - 림 외곽 숲에 숲 안개 적용 (3개 반투명 레이어, 천천히 이동)
- [x] **Ch2 말렛 거래 버그 수정:**
  - 선택지가 별도 대화키에 있어 도달 불가능했던 문제
  - 선택지를 `malet_encounter` 끝에 이동
  - 거절 후 재대화 시 플래그 초기화

### 변경/생성된 파일
- `scenes/battle/battle_scene.gd` — **완전 재작성** 전투씬 (인트로, VFX, 아이들, 턴표시, 상태)
- `scripts/utils/tile_painter.gd` — 엣지 셰이딩 추가
- `scripts/core/scene_transition.gd` — 다이아몬드 와이프 전환 추가
- `scripts/systems/audio_manager.gd` — UI SFX 5종 추가
- `scripts/utils/map_effects.gd` — 비네트 + 안개 효과 추가
- `scenes/maps/rim_forest.gd` — 비네트 + 안개
- `scenes/maps/crumbling_coast.gd` — 비네트 + 전투 전환 변경
- `scenes/maps/the_seam.gd` — 비네트 + 전투 전환 변경
- `scenes/maps/bl07_void.gd` — 강한 비네트 + 전투 전환 변경
- `scenes/maps/verdan_market.gd` — 비네트 + 말렛 거래 버그 수정
- `data/chapter2_dialogue.json` — 말렛 선택지 위치 수정

### 전체 SFX 현황 (17종)
| SFX | 설명 | 사용처 |
|-----|------|--------|
| confirm | UI 확인음 | 대화/메뉴 |
| cancel | UI 취소음 | 대화/메뉴 |
| burn | 기억 연소 | 전투 스킬 |
| hit | 타격 | 전투 공격 |
| heal | 회복 | 전투 승리 |
| step | 발걸음 | 이동 |
| shield | 적 방어막 | 전투 |
| drain | 생명력 흡수 | 전투 |
| phase_change | 보스 페이즈 | 전투 |
| defeat | 패배 | 전투 |
| flee | 도주 | 전투 |
| memory_add | 기억 획득 | 탐색 |
| void_pulse | 보이드 맥동 | Ch5 |
| ui_hover | 버튼 호버 | 전투/메뉴 |
| ui_select | 버튼 선택 | 전투/메뉴 |
| ui_open | 메뉴 열기 | UI |
| ui_close | 메뉴 닫기 | UI |

### 다음 세션 (S23) 할 일
- [x] 엔딩 크레딧 화면 → S23에서 완료
- [x] PauseMenu에 UI SFX 연동 → S23에서 완료
- [x] 비네트/안개 CanvasLayer 수정 → S23에서 완료

---

## S23 — 2026-04-06 (엔딩 크레딧 + UI SFX 전체 연동 + 비네트 수정 + 코드 검증)

### 완료
- [x] **비네트/안개 CanvasLayer 수정:**
  - 문제: Node2D 자식이라 카메라 이동 시 화면을 벗어남
  - 수정: CanvasLayer 기반으로 변경 (layer 3 비네트, layer 2 안개)
  - Control 앵커 프리셋 사용 (TOP_WIDE, BOTTOM_WIDE 등)
- [x] **엔딩 크레딧 화면 (credits.tscn + .gd):**
  - 스크롤 크레딧 (40px/sec 상승)
  - 분기별 에필로그 한 줄 (Zero Burn / Preservation)
  - SPACE/ENTER 스킵 지원
  - 에필로그 BGM 재사용
  - 에필로그 NPC 둘 다 대화 후 자동 크레딧 진입
  - the_seam.gd에 _check_credits_trigger() 추가
- [x] **UI SFX 전체 연동 (6곳):**
  - PauseMenu: ui_open(열기), ui_close(닫기), ui_hover(버튼 호버)
  - DialogueBox: ui_select(선택지 선택), ui_hover(선택지 호버)
  - GameOver: ui_hover(버튼 호버)
  - Title: ui_hover(버튼 호버)
- [x] **코드 검증 — 4개 버그 수정:**
  - main.gd: 이중 시그널 연결 제거 (tscn + 코드 중복)
  - the_seam.gd: 크레딧 시그널을 NPC 셋업에서 한 번만 연결 + 완료 후 disconnect
  - credits.gd: 스크롤 종료 감지를 캐시된 _total_height로 교체
  - credits.gd: _input → _unhandled_input 변경 (일관성)

### 변경/생성된 파일
- `scripts/utils/map_effects.gd` — 비네트/안개 CanvasLayer 기반 전환
- `scenes/ui/credits.tscn` — **신규** 크레딧 씬
- `scenes/ui/credits.gd` — **신규** 크레딧 스크립트
- `scenes/maps/the_seam.gd` — 에필로그→크레딧 연결
- `scripts/ui/pause_menu.gd` — UI SFX (open/close/hover)
- `scripts/ui/dialogue_box.gd` — 선택지 SFX (hover/select)
- `scenes/ui/game_over.gd` — 버튼 hover SFX
- `scenes/main/main.gd` — 버튼 hover SFX

### 전체 플레이 흐름 (최종)
```
타이틀 (New Game / Continue / Quit)
  ↓
Ch1 림 외곽 숲 — 오프닝 → 엘리아 → 재비 → 전투 → [히든: 그루터기] → 야영 → 녹색 나무 CG
  ↓
Ch2 베르단 시장 — 도착 → 말렛 거래(수락/거절) → 추출 → 보상 + 카이로스 경고
  ↓
Ch3 크럼블링 코스트 — 도착 → 카이로스 목격 → 전투 → The Seam 도착
  ↓
Ch4 The Seam — 도착 → 세이블 브리핑 → [히든: 정원] → BL-07 보스전
  ↓
Ch5 BL-07 내부 — 진입 → 탐색+전투 → 핵심부 → **The Seal 결정**
  ├── Zero Burn: 이름 연소 → 백색 플래시 → seal_complete
  └── Preservation: 이름 보존 → seal_refused
  ↓
Ch6 에필로그 — The Seam 복귀 → 분기별 에필로그 → 엘리아+세이블 대화
  ↓
**엔딩 크레딧** — 스크롤 크레딧 → 분기별 에필로그 한 줄 → 타이틀 복귀
  ↓
패배 시: 게임 오버 화면 → Stagger On / Load / Title
```

### 다음 세션 (S24) 할 일
- [x] 전체 코드 정적 분석 → S24에서 완료
- [x] 최종 폴리싱 → S25에서 완료

---

## S24 — 2026-04-06 (전체 코드 정적 분석 + 버그 수정)

### 완료
- [x] **5개 병렬 코드 감사 에이전트 실행:**
  - 코어 오토로드 (GameManager, MemoryManager, DialogueManager, SceneTransition, SaveManager, BattleManager, AudioManager)
  - UI 스크립트 (DialogueBox, MemoryUI, SystemLog, PauseMenu, CgViewer, Credits, GameOver)
  - 맵 스크립트 (RimForest, VerdanMarket, CrumblingCoast, TheSeam, BL07Void, BattleScene, Main)
  - 유틸/플레이어 (TilePainter, MapEffects, PixelSprite, Player, UITheme, project.godot)
  - 리소스 무결성 검증 (load/preload, change_scene, play_bgm, CG, 포트레이트, 대화 JSON)
- [x] **BUG 7개 수정:**
  1. `crumbling_coast.gd` — `_process()` 안 `await` 제거. 도착 시퀀스를 `_ready()`로 이동
  2. `battle_scene.gd` — `_exit_tree()`에서 오토로드 시그널 5개 disconnect (freed 객체 참조 방지)
  3. `battle_manager.gd` — `_enemy_turn()`에 VICTORY/DEFEAT/FLED 상태 가드 추가 (0.8s 타이머 레이스)
  4. `audio_manager.gd` — `_on_tree_changed()` 씬 경로 캐싱 (프레임당 수백 회 호출 방지)
  5. `the_seam.gd` — `_check_credits_trigger` 이중 연결 방지 (`is_connected` 가드)
  6. `pause_menu.gd` — `_on_save()` await 후 `is_open` 가드 (메뉴 닫힌 후 UI 접근 방지)
  7. `cg_viewer.gd` — `cg_shown` 시그널을 auto-close 전에 emit (시그널 타이밍 수정)
- [x] **WARN 수정:**
  - `battle_manager.gd` — float/int 나눗셈 경고 6곳 수정 (`/ 2.0` → `/ 2` 또는 `* 2 <=`)
  - `audio_manager.gd` — 죽은 코드 제거 (`FORMAT_IMA_ADPCM` 할당)
  - `game_over.gd` — 하드코딩 자식 인덱스 → 직접 버튼 참조 + `is_instance_valid` 가드

### 변경된 파일
- `scenes/maps/crumbling_coast.gd` — _process await 버그 수정
- `scenes/battle/battle_scene.gd` — _exit_tree 시그널 정리
- `scripts/systems/battle_manager.gd` — 상태 가드 + float/int 경고 수정
- `scripts/systems/audio_manager.gd` — 씬 캐싱 + 죽은 코드 제거
- `scenes/maps/the_seam.gd` — 이중 연결 방지 + Void Wraith 밸런스
- `scripts/ui/pause_menu.gd` — await 가드 + MemoryUI 열림 체크
- `scripts/ui/cg_viewer.gd` — cg_shown 시그널 타이밍
- `scenes/ui/game_over.gd` — 버튼 포커스 안전화

---

## S25 — 2026-04-06 (최종 폴리싱 — 밸런스, UX, 정리)

### 완료
- [x] **ESC 이중 바인딩 해결:**
  - PauseMenu에 `MemoryUI.is_open` 체크 추가 (이중 안전)
  - MemoryUI가 MENU 상태로 전환하므로 기본적으로 안전하지만 방어적 코딩
- [x] **엘리아 대화 키 수정:**
  - `rim_forest.tscn` — `elia_appears` → `elia_talk` (스토리 대화 대신 탐색 대화)
- [x] **보이드 비스트 메시지 수정:**
  - "normal attacks won't work" → "normal attacks are weakened" (30% 감쇠와 일치)
- [x] **세이브/로드 안전성:**
  - F6/F7 퀵세이브/로드를 EXPLORATION 상태에서만 허용 (전투/대화 중 저장 방지)
- [x] **Ch4 Void Wraith 밸런스 조정:**
  - HP 60→90, ATK 14→18 (Ch3 Coastal Void Beast와 동등 이상)
- [x] **플레이어 null guard:**
  - `player.gd` `_ready()`에 sprite null 체크 추가
- [x] **미사용 에셋 정리:**
  - `assets/audio/bgm/battle.mp3` 삭제 (battle_theme.mp3과 중복)
  - `assets/audio/bgm/ch1_camp.mp3` 삭제 (미참조)

### 변경/삭제된 파일
- `scripts/core/player.gd` — sprite null guard
- `scripts/ui/pause_menu.gd` — MemoryUI.is_open 체크
- `scenes/maps/rim_forest.tscn` — Elia 대화 키 수정
- `scripts/systems/battle_manager.gd` — Void Beast 메시지 수정
- `scripts/systems/save_manager.gd` — 퀵세이브 상태 가드
- `scenes/maps/the_seam.gd` — Void Wraith 밸런스
- `assets/audio/bgm/battle.mp3` — **삭제** (미사용)
- `assets/audio/bgm/ch1_camp.mp3` — **삭제** (미사용)

### 리소스 검증 결과
- load()/preload() 참조: **전부 유효**
- change_scene() 대상 .tscn: **전부 존재**
- play_bgm() 오디오: **전부 존재**
- CG/포트레이트: **전부 존재**
- 대화 JSON 키: **전부 매칭**

### 다음
- [x] 추가 기능/콘텐츠 → S26에서 완료

---

## S26 — 2026-04-06 (옵션 메뉴 + 위치 세이브 + 엔딩 2종 + 기억 월드 반응)

### 완료
- [x] **옵션 메뉴 (OptionsMenu 오토로드):**
  - CanvasLayer (layer 56, PauseMenu 위)
  - Master/BGM/SFX 볼륨 슬라이더 (0-100, `linear_to_db()`)
  - BGM/SFX는 AudioManager 플레이어 직접 제어
  - 전체화면 토글 (`DisplayServer.window_set_mode`)
  - `user://settings.json`에 설정 저장/로드
  - PauseMenu에 "Options" 버튼 추가
  - 타이틀에 "Options" 버튼 추가

- [x] **플레이어 위치 세이브/로드:**
  - SaveManager에 `loaded_player_pos` 변수 + `player_pos` 세이브 데이터
  - `get_tree().get_nodes_in_group("player")`로 위치 수집
  - 5개 맵 `_position_player()`에 위치 복원 코드 추가
  - 엘리아 동행 위치도 플레이어 기준으로 복원

- [x] **추가 엔딩 2종 (총 4종):**
  - **Ash 엔딩** — `seal_refused` + 기억 4개 이상 연소: 껍데기만 남은 아렐
  - **Seam 비밀 엔딩** — `seal_refused` + 히든 이벤트 2개 발견: 작은 아름다움 속 희망
  - `the_seam.gd` 에필로그 분기 로직 확장
  - `credits.gd` 분기별 에필로그 텍스트 4종
  - `chapter6_dialogue.json`에 epilogue_ash/epilogue_seam 대화 추가

- [x] **기억 연소 월드 반응 시스템:**
  - DialogueManager의 기존 `requires_memory`/`burned_text` 활용
  - Ch1 `elia_talk`: 숲 냄새/첫 검술 기억 반응
  - Ch2 `elia_ch2_talk`: 시장 음식/모닥불 노래 기억 반응
  - Ch3 `elia_coast_talk`: 모닥불 노래 기억 반응
  - Ch4 `elia_seam_talk`: 관계 기억 반응
  - Ch4 `sable_talk`: 보이드 워커 기억 반응
  - 기억을 태우면 NPC 대화가 실제로 변하는 핵심 메카닉 구현

### 변경/생성된 파일
- `scenes/ui/options_menu.gd` — **신규** 옵션 메뉴 오토로드
- `project.godot` — OptionsMenu 오토로드 등록
- `scenes/main/main.gd` — 타이틀 Options 버튼
- `scenes/main/main.tscn` — OptionsButton 노드 추가
- `scripts/ui/pause_menu.gd` — Options 버튼 + 콜백
- `scripts/systems/save_manager.gd` — 플레이어 위치 세이브/로드
- `scenes/maps/rim_forest.gd` — 위치 복원
- `scenes/maps/verdan_market.gd` — 위치 복원
- `scenes/maps/crumbling_coast.gd` — 위치 복원
- `scenes/maps/the_seam.gd` — 위치 복원 + 에필로그 4분기
- `scenes/maps/bl07_void.gd` — 위치 복원
- `scenes/ui/credits.gd` — 4종 에필로그 텍스트
- `data/chapter1_dialogue.json` — 기억 반응 대사
- `data/chapter2_dialogue.json` — 기억 반응 대사
- `data/chapter3_dialogue.json` — 기억 반응 대사
- `data/chapter4_dialogue.json` — 기억 반응 대사
- `data/chapter6_dialogue.json` — Ash/Seam 에필로그

### 엔딩 분기 (최종)
```
Ch5 The Seal 결정:
  ├── Zero Burn: 이름 연소 → epilogue_zero_burn → "He burned everything."
  └── Preservation: 이름 보존 →
       ├── 기억 4+ 연소 → epilogue_ash → "Just ash, drifting."
       ├── 히든 2개 발견 → epilogue_seam → "Something green still grows."
       └── 기본 → epilogue_preservation → "He kept his name."
```

### 다음
- [x] 추가 폴리싱 → S27에서 완료

---

## S27 — 2026-04-06 (챕터 타이틀 + 텍스트 속도 + 발걸음 + UI 폴리싱)

### 완료
- [x] **OptionsMenu ESC 닫기:**
  - `_unhandled_input`에서 "cancel" 액션으로 닫기
  - 입력 소비(`set_input_as_handled`)로 PauseMenu 전파 방지

- [x] **텍스트 속도 옵션:**
  - OptionsMenu에 Text Speed 슬라이더 추가 (1-5단계)
  - 1=Slow(0.06s), 2=Slow+(0.045s), 3=Normal(0.03s), 4=Fast(0.015s), 5=Instant(0s)
  - `settings.json`에 저장/로드
  - DialogueBox에서 런타임으로 OptionsMenu 설정 참조

- [x] **챕터 타이틀 카드:**
  - `MapEffects.show_chapter_title()` — CanvasLayer 기반 오버레이
  - 페이드인(0.5s) → 홀드(2.0s) → 페이드아웃(0.8s) → 자동 제거
  - "CHAPTER X" + 타이틀 + 서브타이틀 3줄 구성
  - 5개 맵 전부 적용:
    - Ch1 "Rim Forest" — "The edge of what remains"
    - Ch2 "Verdan Market" — "Where memories are currency"
    - Ch3 "Crumbling Coast" — "The ground gives way"
    - Ch4 "The Seam" — "Between what was and what will be"
    - Ch5 "BL-07" — "The Void stares back"
  - `await`로 타이틀 카드 완료 후 스토리 시퀀스 시작

- [x] **발걸음 SFX:**
  - AudioManager에 `step_player` 전용 플레이어 추가 (-12dB, SFX와 독립)
  - `play_step()` 메서드 — 다른 SFX와 겹치지 않음
  - Player에서 0.25초 간격으로 이동 중 발소리 재생
  - EXPLORATION 상태에서만 동작

### 변경된 파일
- `scenes/ui/options_menu.gd` — ESC 닫기 + 텍스트 속도 슬라이더
- `scripts/ui/dialogue_box.gd` — 동적 타자기 속도 (OptionsMenu 연동)
- `scripts/utils/map_effects.gd` — `show_chapter_title()` 정적 함수
- `scenes/maps/rim_forest.gd` — Ch1 타이틀 카드
- `scenes/maps/verdan_market.gd` — Ch2 타이틀 카드
- `scenes/maps/crumbling_coast.gd` — Ch3 타이틀 카드
- `scenes/maps/the_seam.gd` — Ch4 타이틀 카드
- `scenes/maps/bl07_void.gd` — Ch5 타이틀 카드
- `scripts/core/player.gd` — 발걸음 타이머 + SFX
- `scripts/systems/audio_manager.gd` — step_player + play_step()

### 다음
- [x] UX 개선 → S28에서 완료

---

## S28 — 2026-04-06 (탐색 HUD + 알림 토스트 시스템)

### 완료
- [x] **탐색 HUD (ExplorationHUD 오토로드):**
  - CanvasLayer (layer 10), 좌상단 고정
  - HP 바 (100px 프로그레스바 + 수치, HP≤25%면 빨간색)
  - 챕터/지역 표시 ("Ch.3 — Crumbling Coast")
  - 기억 카운터 ("Memories: 6 held, 2 burned")
  - 0.5초 타이머 기반 업데이트 (성능 최적화)
  - HP 바 트윈 애니메이션 (0.4s ease-out)
  - GameState.EXPLORATION에서만 표시 (전투/대화/메뉴 중 숨김)
  - UITheme 색상 체계 활용

- [x] **알림 토스트 시스템 (NotificationToast 오토로드):**
  - CanvasLayer (layer 35), 하단 중앙
  - 슬라이드 업 + 페이드인(0.3s) → 홀드(2.0s) → 페이드아웃(0.5s)
  - 3가지 타입: INFO(ℹ 앰버), SUCCESS(✓ 녹색), WARNING(⚠ 오렌지)
  - 대기열 시스템 (연속 알림 순차 처리)
  - 자동 시그널 연결:
    - MemoryManager.memory_added → "✓ Memory acquired: {title}"
    - MemoryManager.memory_burned → "⚠ Memory burned: {title}"
    - SaveManager.save_completed → "✓ Game saved — Slot {n}"
    - SaveManager.load_completed → "ℹ Game loaded — Slot {n}"

### 변경/생성된 파일
- `scripts/ui/exploration_hud.gd` — **신규** 탐색 HUD 오토로드
- `scripts/ui/notification_toast.gd` — **신규** 알림 토스트 오토로드
- `project.godot` — ExplorationHUD + NotificationToast 오토로드 등록

### 오토로드 목록 (최종 14개)
```
GameManager, MemoryManager, DialogueManager, SceneTransition,
DialogueBox, MemoryUI, SystemLog, BattleManager, SaveManager,
CgViewer, PauseMenu, OptionsMenu, ExplorationHUD, NotificationToast
```

### 다음
- [x] 기억 거래 상점 → S29에서 완료
- [x] 엘리아 분리 메카닉 → S30에서 완료

---

## S29 — 2026-04-08 (기억 거래 상점 + Grains 경제 시스템)

### 완료
- [x] **MemoryShop 오토로드 (layer 42):**
  - 풀스크린 상점 UI (오버레이 + 패널)
  - Sell/Buy 탭 전환, 아이템 목록 + 상세 패널
  - 판매: 플레이어 보유 미연소 기억 → Grains 획득 (Grade 1 핵심 기억 판매 불가)
  - 구매: 상인 인벤토리 기억 → Grains 소모
  - 등급별 가격 체계: G5=5/10, G4=15/25, G3=30/50, G2=60/100, G1=150/300
  - ESC 닫기, UI SFX (hover/select/open/close), NotificationToast 연동
  - `shop_closed` / `grains_changed` 시그널

- [x] **Grains 경제 시스템:**
  - 전투 승리 시 Grains 자동 보상 (일반 3+HP/20, 보이드 8+HP/20, 보스 20+HP/20)
  - NotificationToast로 획득량 표시
  - DialogueManager에 `add_grains` 선택지 지원 추가

- [x] **Verdan Market 말렛 상점 연동:**
  - malet_reward 대화 후 상점 자동 오픈
  - 상점 재고: "The Taste of Copper" (G5, 8G), "A Deal in the Dark" (G4, 20G)
  - 상점 닫으면 Ch3 전환

- [x] **ExplorationHUD Grains 표시:**
  - 하단에 "Grains: N" 표시 (금색, 0.5초 갱신)

### 변경/생성된 파일
- `scripts/ui/memory_shop.gd` — **신규** 기억 거래 상점 오토로드
- `project.godot` — MemoryShop 오토로드 등록 (총 15개)
- `scripts/systems/battle_manager.gd` — Grains 전투 보상 + NotificationToast
- `scripts/systems/dialogue_manager.gd` — `add_grains` 선택지 지원
- `scripts/ui/exploration_hud.gd` — Grains 표시 추가
- `scenes/maps/verdan_market.gd` — 말렛 상점 연동

---

## S30 — 2026-04-08 (엘리아 분리 메카닉 + 앵커링 시스템 실체화)

### 완료
- [x] **엘리아 분리 선택 (Ch3 크럼블링 코스트):**
  - 카이로스 목격 후 분리 선택지 자동 발생
  - "Go together" → `elia_stays` 플래그, 동행 유지
  - "Split up" → `elia_separates` 플래그, `elia_with_party=false`
  - 분리 시 엘리아 씬에서 비활성화 (visible=false, physics 중지)
  - 세이브/로드 시에도 분리 상태 유지

- [x] **잔존(Residue) 메카닉 차별화:**
  - 엘리아 동행 시: Grade 3+ 기억 연소 → 잔존(희미한 흔적) 상태로 남음
  - 엘리아 분리 시: 모든 기억 연소 → 완전 소실 (잔존 없음)
  - 기존 MemoryManager의 `elia_with_party` 체크가 자동으로 동작

- [x] **분리 인지 대화 변형:**
  - Ch3 분리/동행 선택 대화 3종 (choice, stays_response, separates_response)
  - Ch4 솔로 도착 대화 (seam_welcome_solo) — 혼자 온 아렐에 대한 세이블 반응
  - Ch3 재합류 대화 (elia_reunion) — 해안길 경유 후 재합류

- [x] **재합류 메카닉 (Ch4 The Seam):**
  - 분리 상태로 The Seam 도착 시 솔로 대사 → 재합류 이벤트 자동 발생
  - `elia_reunited` 플래그 + `elia_with_party=true` 복원
  - 엘리아 다시 표시 + 물리 활성화 + 플레이어 근처 위치
  - 재합류 후 기존 Ch4 시퀀스 정상 진행

### 변경/생성된 파일
- `data/chapter3_dialogue.json` — 분리 선택 대화 4종 추가
- `data/chapter4_dialogue.json` — 솔로 도착 대화 추가
- `scenes/maps/crumbling_coast.gd` — 분리 선택 이벤트 + 상태 반영
- `scenes/maps/the_seam.gd` — 재합류 이벤트 + 솔로 대사 분기

### 엘리아 앵커링 효과 (최종)
```
엘리아 동행 (elia_with_party = true):
  └── Grade 3+ 기억 연소 시 → 잔존(Residue) 상태로 남음 (희미한 효과 유지)

엘리아 분리 (elia_with_party = false):
  └── 모든 기억 연소 시 → 완전 소실 (되돌릴 수 없음)
  └── Ch3 크럼블링 코스트에서만 분리 가능
  └── Ch4 The Seam에서 자동 재합류
```

### 오토로드 목록 (최종 15개)
```
GameManager, MemoryManager, DialogueManager, SceneTransition,
DialogueBox, MemoryUI, SystemLog, BattleManager, SaveManager,
CgViewer, AudioManager, PauseMenu, OptionsMenu, ExplorationHUD,
NotificationToast, MemoryShop
```

### 다음
- [x] 미니맵 + 저널 → S31에서 완료

---

## S31 — 2026-04-08 (미니맵 + 스토리 저널 시스템)

### 완료
- [x] **미니맵 시스템 (Minimap 유틸리티 클래스):**
  - CanvasLayer(9) 기반, 우상단 140x100px 미니맵
  - 맵 타일을 4px 단위로 렌더링 (20+ 타일 종류 색상 매핑)
  - 플레이어 마커 (밝은 파란 6px 점, 실시간 위치 추적)
  - 엘리아 마커 (은빛 4px 점, 분리 시 숨김)
  - GameState.EXPLORATION에서만 표시
  - 5개 맵 전체 통합 (_build_map에서 생성, _process에서 업데이트)
  - 반투명 배경 + 앰버 테두리 (UITheme 일관)

- [x] **스토리 저널 / 코덱스 (StoryJournal 오토로드, layer 57):**
  - 3개 탭: Events(이벤트) / People(NPC) / Choices(선택)
  - Events: 21개 이벤트 엔트리 (Ch1~Ch6, 히든 포함), 챕터별 헤더 구분
  - People: 4명 NPC (Elia/Malet/Sable/Kairos), 캐릭터 색상 반영
  - Choices: 6개 주요 분기 선택 기록
  - story_flags 기반 자동 언락 (직접 기록 불필요)
  - 좌측 스크롤 목록 + 우측 상세 패널
  - ESC 닫기, UI SFX 연동

- [x] **PauseMenu 통합:**
  - "Journal" 버튼 추가 (Resume 바로 아래)
  - 저널 오픈 시 PauseMenu 위에 표시

### 변경/생성된 파일
- `scripts/ui/minimap.gd` — **신규** 미니맵 유틸리티 (class_name Minimap)
- `scripts/ui/story_journal.gd` — **신규** 스토리 저널 오토로드
- `project.godot` — StoryJournal 오토로드 등록 (총 16개)
- `scripts/ui/pause_menu.gd` — Journal 버튼 추가
- `scenes/maps/rim_forest.gd` — 미니맵 통합
- `scenes/maps/verdan_market.gd` — 미니맵 통합
- `scenes/maps/crumbling_coast.gd` — 미니맵 통합
- `scenes/maps/the_seam.gd` — 미니맵 통합
- `scenes/maps/bl07_void.gd` — 미니맵 통합

### 오토로드 목록 (최종 16개)
```
GameManager, MemoryManager, DialogueManager, SceneTransition,
DialogueBox, MemoryUI, SystemLog, BattleManager, SaveManager,
CgViewer, AudioManager, PauseMenu, OptionsMenu, ExplorationHUD,
NotificationToast, MemoryShop, StoryJournal
```

### 다음
- [x] 랜덤 인카운터 + 상태이상 → S32에서 완료

---

## S32 — 2026-04-08 (랜덤 인카운터 + 전투 상태이상)

### 완료
- [x] **전투 상태이상 시스템 (BattleManager 확장):**
  - 3종 상태이상: Poison(독 DoT), Weaken(공격력 30% 감소), Burn(화상 DoT)
  - StatusEffect enum + StatusEntry 클래스 (효과/지속턴/위력)
  - `apply_status()`: 대상(player/enemy)에 상태이상 부여, 중복 시 강한 쪽 갱신
  - `_process_statuses()`: 턴 시작 시 DoT 처리 + 지속턴 감소 + 만료 알림
  - `_get_weaken_multiplier()`: 약화 상태 시 공격력 계수 반환
  - 플레이어 공격/적 공격 모두 약화 적용
  - Grade 2+ 기억 연소 시 적에게 화상 DoT 자동 부여
  - `status_changed` 시그널 → UI 실시간 갱신
  - 독/화상으로 사망 시 정상 승리/패배 처리

- [x] **적 상태이상 능력 4종 추가:**
  - `poison`: 3턴 DoT (공격력 30% + 2~5 랜덤)
  - `burn_attack`: 데미지 + 2턴 화상 DoT
  - `weaken`: 플레이어 공격력 30% 감소 3턴
  - 기존 drain/shield/multi_hit과 병합

- [x] **전투 UI 상태이상 표시 (battle_scene.gd):**
  - 적 상태 컨테이너: 기존 SHIELD/PHASE/VOID + 새 상태이상 아이콘
  - 플레이어 상태 컨테이너: 플레이어 HP 패널 아래 상태이상 표시
  - 색상 코드: POISON(녹색), WEAK(주황), BURN(오렌지)
  - 남은 턴 수 표시 (예: "POISON 2")
  - `status_changed` 시그널 연결 + _exit_tree에서 정리

- [x] **랜덤 인카운터 시스템 (RandomEncounter 유틸리티):**
  - `class_name RandomEncounter` (static utility, Minimap과 동일 패턴)
  - 이동 거리 기반 인카운터 (타일 단위 step 카운터)
  - 맵별 설정: 적 풀, 최소/최대 걸음 수, 배경/적 이미지
  - 챕터 완료 후 재방문 시에만 활성화
  - `setup()` → `update()` 패턴 (맵 _ready에서 초기화, _process에서 체크)

- [x] **5개 맵 랜덤 인카운터 통합:**
  - Rim Forest (Ch1 완료 후): Ash Crawler, Forest Shade(독), Void Beast — 50~90 걸음
  - Verdan Market (Ch2 완료 후): Alley Rat(독), Market Thief(약화) — 60~100 걸음
  - Crumbling Coast (Ch3 완료 후): Coastal Void Beast, Cliff Stalker(독+연타), Shore Wraith(화상+약화) — 40~70 걸음
  - The Seam (Ch4 완료 후): Void Wraith(흡수+약화), Seam Lurker(독+방어) — 45~80 걸음
  - BL-07 Void (Ch5 진입 후): Void Fragment(화상), Memory Eater(흡수+연타+약화), Null Wisp(독+화상) — 30~55 걸음

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — 상태이상 시스템 (StatusEffect, apply/process/weaken)
- `scenes/battle/battle_scene.gd` — 상태이상 UI (플레이어+적 상태 컨테이너)
- `scripts/utils/random_encounter.gd` — **신규** 랜덤 인카운터 유틸리티
- `scenes/maps/rim_forest.gd` — 랜덤 인카운터 통합
- `scenes/maps/verdan_market.gd` — 랜덤 인카운터 통합
- `scenes/maps/crumbling_coast.gd` — 랜덤 인카운터 통합
- `scenes/maps/the_seam.gd` — 랜덤 인카운터 통합
- `scenes/maps/bl07_void.gd` — 랜덤 인카운터 통합

### 상태이상 효과 정리
```
Poison (독): 매 턴 고정 데미지, 3턴 지속
Weaken (약화): 공격력 30% 감소, 3턴 지속
Burn (화상): 매 턴 고정 데미지, 2턴 지속
- Grade 2+ 기억 연소 시 적에게 자동 화상 부여
- 같은 효과 중복 시 강한 쪽으로 갱신
```

### 다음
- 전투 아이템 시스템 (포션/해독제) 추천
- New Game+ 모드
- 업적 시스템

---

## S33 — 2026-04-08 (전투 소모 아이템 시스템)

### 완료
- [x] **아이템 정의 + 인벤토리 (GameManager):**
  - `ITEMS` 상수: potion(40HP), hi_potion(80HP), antidote(독/화상 해제), firebomb(2턴 화상), smoke_bomb(확정 도주)
  - `player_data.items` Dictionary (`{id: count}`)
  - `add_item()`, `remove_item()`, `get_item_count()` — NotificationToast 연동

- [x] **전투 중 아이템 사용 (BattleManager):**
  - `player_use_item(item_id)`: heal/cure/burn/flee 4종 처리
  - heal: HP 회복 (max_hp 클램프)
  - cure: 독+화상 상태이상 해제
  - burn: 적에게 화상 DoT 부여
  - flee: 확정 도주 (battle_fled 시그널)
  - 아이템 사용 = 1턴 소모 → 적 반격

- [x] **전투 UI ITEM 버튼 + 아이템 목록 (battle_scene.gd):**
  - 행동 버튼에 ITEM 추가 (ATK/BURN/ITEM/FLEE)
  - 초록색 테마 아이템 리스트 패널 (토글 표시)
  - 보유 아이템만 버튼으로 표시 (이름 + 수량)
  - 클릭 시 사용 + 목록 닫기

- [x] **전투 승리 아이템 드롭:**
  - 30% 확률 아이템 드롭 (적 유형별 가중치)
  - void 적: firebomb/antidote 위주, 일반 적: potion/antidote 위주

- [x] **스타터 아이템:**
  - Ch1 캠프 완료: potion ×1
  - Ch2 말렛 보상: potion ×2, antidote ×1, firebomb ×1

- [x] **ExplorationHUD 아이템 카운터:**
  - HUD에 "Items: N" 행 추가 (초록색, Grains 아래)
  - 0.5초 주기 자동 갱신

### 변경/생성된 파일
- `scripts/core/game_manager.gd` — ITEMS 상수, add/remove/get_item, player_data.items
- `scripts/systems/battle_manager.gd` — player_use_item(), _try_item_drop()
- `scenes/battle/battle_scene.gd` — ITEM 버튼, 아이템 목록 UI
- `scenes/maps/rim_forest.gd` — Ch1 스타터 아이템
- `scenes/maps/verdan_market.gd` — Ch2 말렛 보상 아이템, 상점 아이템
- `scripts/ui/exploration_hud.gd` — items_label 추가

### 다음
- MemoryShop에 아이템 상점 탭 추가
- New Game+ 모드
- 업적 시스템

---

## S34 — 2026-04-08 (MemoryShop 아이템 탭 + New Game+ + 업적 시스템)

### 완료
- [x] **MemoryShop 아이템 탭:**
  - 3번째 탭 "Items" 추가 (Sell Memories / Buy Memories / Items)
  - 아이템 구매: GameManager.ITEMS 전체 목록, Grains로 구매
  - 아이템 판매: 보유 아이템 60% 가격에 Grains로 판매
  - 상세 패널 (이름/설명/가격) + 구매/판매 버튼

- [x] **New Game+ 모드:**
  - 게임 클리어 시 `user://ng_plus.json` 영구 파일 생성
  - 타이틀 화면에 "New Game+" 버튼 동적 추가 (클리어 후에만)
  - NG+ 시작: Grains + 아이템 유지, 스토리/기억 초기화, 회차 증가
  - 적 스케일링: HP/ATK × (1 + 0.3 × 회차) — NG+1 = 1.3배, NG+2 = 1.6배
  - HUD/PauseMenu에 "NG+N" 회차 표시
  - GameManager export/import에 ng_plus_cycle 포함 (세이브 호환)

- [x] **업적 시스템 (AchievementManager 오토로드):**
  - 25종 업적 정의 (전투 6 / 기억 4 / 탐색 3 / 스토리 9 / 경제 2 / NG+ 1)
  - `user://achievements.json` 영구 저장 (세이브 슬롯과 독립)
  - 전투: first_blood, void_slayer, boss_hunter, battle_veteran(10승), survivor(10HP이하 승리), item_master(10회 사용)
  - 기억: first_burn, pyromaniac(5회 연소), identity_crisis(Grade 2), zero_burn(핵심 기억)
  - 탐색: hidden_stump, hidden_garden, explorer(5맵 방문)
  - 스토리: Ch1~5 완료, 4종 엔딩, all_endings(4종 수집)
  - 경제: merchant(말렛 거래), wealthy(100 Grains)
  - PauseMenu "Achievements" 버튼 + 뷰어 패널 (아이콘/제목/설명, 미해금=???)
  - 달성 시 NotificationToast 자동 알림

### 변경/생성된 파일
- `scripts/ui/memory_shop.gd` — Items 탭 (buy_item/sell_item 모드)
- `scripts/core/game_manager.gd` — ng_plus_cycle, NG+ 관련 함수, export/import
- `scripts/systems/battle_manager.gd` — NG+ 적 스케일링, 아이템 사용 업적 추적, Grains 업적
- `scripts/ui/achievement_manager.gd` — **신규** 업적 오토로드
- `scripts/ui/pause_menu.gd` — Achievements 버튼 + 뷰어 패널, NG+ 표시
- `scripts/ui/exploration_hud.gd` — NG+ 회차 표시
- `scenes/main/main.gd` — NG+ 버튼, New Game 아이템 초기화
- `scenes/ui/credits.gd` — 게임 완료 기록, 엔딩 업적
- `scenes/maps/rim_forest.gd` — 맵 방문/챕터 완료/히든 이벤트 업적
- `scenes/maps/verdan_market.gd` — 맵 방문/챕터 완료/거래 업적
- `scenes/maps/crumbling_coast.gd` — 맵 방문/챕터 완료 업적
- `scenes/maps/the_seam.gd` — 맵 방문/히든 정원 업적
- `scenes/maps/bl07_void.gd` — 맵 방문/챕터 완료 업적
- `project.godot` — AchievementManager 오토로드 추가

### 다음
- 도감 시스템 (적/기억 수집도)
- 난이도 선택 (Easy/Normal/Hard)
- 미니게임 (시장 미니 퀴즈)

---

## S35 — 2026-04-08 (도감 + 난이도 + 기억 매칭 퍼즐)

### 완료
- [x] **도감 시스템 (Codex 오토로드):**
  - Bestiary 탭: 만난 적 기록 (이름/타입/HP/ATK/조우 횟수/격파 횟수)
  - Memory Archive 탭: 수집한 기억 카탈로그 (제목/등급/설명/연소 여부)
  - `user://codex.json` 영구 저장 (세이브와 독립)
  - BattleManager.battle_started/ended → 적 자동 기록
  - MemoryManager.memory_added/burned → 기억 자동 기록
  - 2탭 UI: 좌측 리스트 + 우측 상세 패널
  - PauseMenu "Codex" 버튼 추가

- [x] **난이도 선택 (OptionsMenu 확장):**
  - 3단계: Easy (적 0.7배) / Normal (1.0배) / Hard (1.4배)
  - OptionsMenu에 Difficulty 토글 버튼 추가 (순환식)
  - `settings.json`에 저장 → 재시작 시 유지
  - BattleManager.start_battle에서 적 HP/ATK에 난이도 계수 적용
  - NG+ 스케일링과 중첩 (NG+1 Hard = 1.3 × 1.4 = 1.82배)

- [x] **기억 매칭 퍼즐 (MemoryPuzzle 오토로드):**
  - 카드 뒤집기 미니게임 (현재 보유 기억에서 랜덤 쌍 생성)
  - 3~8쌍 설정 가능, 그리드 자동 조절
  - 매칭 성공/실패 시각 피드백 + SFX
  - 클리어 시 Grains 보상 (기본 + 시도 횟수 보너스)
  - 베르단 시장 (Ch2 완료 후): 4쌍, 15G 보상
  - The Seam (Ch4 완료 후): 5쌍, 20G 보상
  - 녹색/보라색 인디케이터로 퍼즐 영역 표시

### 변경/생성된 파일
- `scripts/ui/codex.gd` — **신규** 도감 오토로드
- `scripts/ui/memory_puzzle.gd` — **신규** 기억 매칭 퍼즐 오토로드
- `scenes/ui/options_menu.gd` — 난이도 토글 (difficulty 설정)
- `scripts/systems/battle_manager.gd` — 난이도 스케일링 적용
- `scripts/ui/pause_menu.gd` — Codex 버튼 추가
- `scenes/maps/verdan_market.gd` — 퍼즐 트리거 영역
- `scenes/maps/the_seam.gd` — 퍼즐 트리거 영역
- `project.godot` — Codex, MemoryPuzzle 오토로드 추가

### 다음
- 파티 시스템 (세이블 동행)
- 맵 날씨 효과 (비/눈/안개 변화)
- 전투 콤보 시스템

---

## S36 — 2026-04-08 (파티 시스템 + 날씨 효과 + 전투 콤보)

### 완료
- [x] **파티 시스템 (세이블 전투 동행):**
  - Ch4(The Seam) 브리핑 완료 시 `sable_joined` 플래그 설정
  - BattleManager: `sable_in_party` — Ch4+ & sable_joined일 때 활성
  - 세이블 지원 행동 (40% 확률): 힐(10~20HP) / 타격(8~18dmg) / 약화(20% 2턴)
  - `ally_action` 시그널 → 전투 로그에 세이블 행동 표시
  - 전투 UI에 "SABLE" 동행 아이콘 표시

- [x] **맵 날씨 효과:**
  - MapEffects: `add_rain()`, `add_snow()`, `add_heavy_fog()`, `update_heavy_fog()` 추가
  - 크럼블링 코스트(Ch3): 비 (GPUParticles2D, 빗방울 파티클)
  - The Seam(Ch4): 눈 (느린 낙하 파티클)
  - BL-07 보이드(Ch5): 짙은 안개 (5개 ColorRect 드리프트)

- [x] **전투 콤보 시스템:**
  - 연속 공격 시 콤보 카운터 누적 (2=+15%, 3=+30%, 4+=+50% 데미지)
  - 방어/연소/아이템 사용 시 콤보 리셋
  - `combo_changed` 시그널 → 전투 UI "COMBO x%d" 표시
  - 전투 로그에 콤보 배율 메시지

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — 콤보 변수/시그널, 세이블 지원 행동, 콤보 배율
- `scripts/utils/map_effects.gd` — rain/snow/heavy_fog 파티클 함수
- `scenes/battle/battle_scene.gd` — 콤보/세이블 상태 아이콘 표시
- `scenes/maps/crumbling_coast.gd` — 비 효과 추가
- `scenes/maps/the_seam.gd` — 눈 효과, sable_joined 플래그
- `scenes/maps/bl07_void.gd` — 짙은 안개 효과

### 다음
- 맵 인터랙티브 오브젝트 (숨겨진 상자/단서)
- 전투 약점/저항 시스템 (적 속성)
- 기억 조합 시스템 (2개 기억 합성)

---

## S37 — 2026-04-08 (인터랙티브 오브젝트 + 속성 시스템 + 기억 합성)

### 완료
- [x] **맵 인터랙티브 오브젝트:**
  - 5개 맵 전체에 숨겨진 상자(금색 인디케이터) + 단서(청색 인디케이터)
  - 상자: 아이템 + Grains 보상, 1회 획득 (플래그 기반)
  - 단서: 세계관 텍스트 토스트 표시, 탐색 보상 강화
  - 림 포레스트: 포션x2+10G, 돌무더기 단서
  - 베르단 마켓: 화염탄+15G, 썸프 입구 단서
  - 크럼블링 코스트: Hi-Potion+12G, 해독제x2+8G, 카이로스 단서
  - 더 씸: Hi-Potion+연막탄+20G, 개울 단서
  - BL-07: Hi-Potionx2+화염탄+25G, 보이드 속삭임 단서

- [x] **전투 약점/저항 시스템:**
  - 3속성: PHYSICAL(일반공격), FIRE(Grade 5~3 연소), VOID(Grade 2~1 연소)
  - 약점 적중 = +50% 데미지, 저항 적중 = -30% 데미지
  - Enemy 클래스에 `weakness`/`resistance` 속성 추가
  - 기본: 보이드 수 = VOID 약점+PHYSICAL 저항, 일반 적 = FIRE 약점
  - Memory Eater: FIRE 약점, VOID 저항 (역전된 상성)
  - Shade Sentinel 보스: VOID 약점, FIRE 저항
  - 전투 UI에 WEAK/RESIST 아이콘 표시
  - 연소 메뉴에 속성(FIRE/VOID) 표시 → 전략적 기억 선택 유도
  - "It's super effective!" / "It's not very effective..." 로그 메시지

- [x] **기억 조합 시스템 (Synthesis):**
  - 동일 등급 미연소 기억 2개 → 상위 등급 1개로 합성
  - 원본 소실 (연소와 다른 방식의 상실 — 테마 강화)
  - 합성 결과: burn_power = (A + B) * 0.7 + 10 보너스
  - Grade 5→4→3→2 합성 가능, Grade 1(최고)은 합성 불가
  - MemoryUI에 SYNTHESIZE 버튼 (조건 충족 시 표시)
  - 합성 모드: 첫 기억 선택 → 두 번째 기억 선택 → 즉시 합성
  - 등급별 합성 결과 이름: Blended Sensation / Woven Routine / Bound Connection / Forged Identity
  - `memory_synthesized` 시그널 + 토스트 알림

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — 속성 시스템 (ELEMENT_BONUS/RESIST, _get_element_multiplier)
- `scripts/systems/memory_manager.gd` — synthesize(), has_synthesizable_pair(), SYNTHESIS_NAMES
- `scripts/ui/memory_ui.gd` — 합성 모드 UI (synth_btn, synthesis_mode, _on_synth_pressed)
- `scenes/battle/battle_scene.gd` — WEAK/RESIST 아이콘, 연소 메뉴 속성 표시
- `scenes/maps/rim_forest.gd` — 인터랙티브 오브젝트 (상자+단서)
- `scenes/maps/verdan_market.gd` — 인터랙티브 오브젝트 (상자+단서)
- `scenes/maps/crumbling_coast.gd` — 인터랙티브 오브젝트 (상자x2+단서)
- `scenes/maps/the_seam.gd` — 인터랙티브 오브젝트 (상자+단서), 보스 약점/저항
- `scenes/maps/bl07_void.gd` — 인터랙티브 오브젝트 (상자+단서), Memory Eater 약점/저항

### 다음
- 맵 간 자유 이동 (월드맵 / 빠른 이동)
- 전투 궁극기 시스템 (게이지 축적 → 강력 일격)
- 기억 잔존 활용 (잔존 기억으로 약한 스킬 재사용)

---

## S38 — 2026-04-08 (Fast Travel + Limit Break + Residue 재사용 + 버그 수정)

### 버그 수정
- [x] **Burn DoT 등급 반전:** `memory.grade <= 2`가 Grade 5~3(낮은 등급)을 선택하던 문제 → `>= MemoryGrade.GRADE_2`로 수정 (Grade 2=3, Grade 1=4만 DoT 부여)
- [x] **보이드 수 이중 감쇠:** resistance="physical" + is_void_beast 0.3배가 중첩되어 0.21배 → resistance="" 로 변경 (is_void_beast 감쇠만 적용)

### 완료
- [x] **Fast Travel 시스템:**
  - PauseMenu에 "Travel" 버튼 추가 (Journal과 Codex 사이)
  - 맵 선택 오버레이: 5개 맵 (림 포레스트~BL-07)
  - 챕터 진행도에 따른 맵 해금 (현재 챕터 이상만 이동 가능)
  - 미해금 맵은 "???" + 비활성화
  - 선택 즉시 SceneTransition으로 이동, ESC로 닫기

- [x] **Limit Break 궁극기:**
  - limit_gauge: 0~100 게이지, 전투 시작 시 리셋
  - 게이지 축적: 공격(+8), 연소(+12), 피격(+15), 방어(+5)
  - 게이지 100% 시 LIMIT 버튼 활성화 (보라색 강조)
  - Memory Cascade: 300 + 챕터보너스(40/ch) + 연소보너스(15/burn) 데미지
  - VOID 속성 공격 + 적 약화 2턴 부여
  - 전투 UI: 게이지 바 (플레이어 HP 우측), 꽉 찬 상태 색상 변경

- [x] **기억 잔존 재사용 (Residue Burn):**
  - 연소된 기억 중 is_residue=true인 기억을 전투에서 재사용
  - 50% 데미지, 기억 소멸 없음 (반복 사용 가능)
  - 연소 메뉴 하단에 [RESIDUE] 섹션으로 표시 (보라색 테마)
  - MemoryManager에 get_residue_memories(), get_residue_memory() 추가
  - BattleManager에 player_burn_residue() 추가

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — Limit Break 시스템, Residue Burn, 버그 수정 2건
- `scripts/systems/memory_manager.gd` — get_residue_memories(), get_residue_memory()
- `scenes/battle/battle_scene.gd` — Limit 게이지 UI, LIMIT 버튼, Residue 번 메뉴
- `scripts/ui/pause_menu.gd` — Travel 버튼, 맵 선택 오버레이

### 다음
- 전투 AI 패턴 강화 (적 행동 다양화)
- 맵 시각 다양성 (맵별 고유 오브젝트/디테일)
- 기억 관련 사이드퀘스트

---

## S39 — 2026-04-08 (전술적 AI + 사이드 퀘스트 + 맵 데코레이션)

### 버그 수정
- [x] **SideQuest null 체크:** `_find_quest()`가 `{}`를 반환하지만 호출부에서 `== null`로 비교 → `.is_empty()`로 변경 (4개소)
- [x] **Limit 게이지 비-데미지 능력 축적:** shield/summon/weaken 등 피해 없는 능력에도 `_add_limit(LIMIT_GAIN_HIT)` 호출 → 데미지 능력(drain/multi_hit/burn_attack)에만 호출하도록 이동
- [x] **보스 턴 카운터 드리프트:** `_boss_turn_counter += 1`이 확률 체크 전에 실행되어 능력 미사용 시에도 증가 → 확률 체크 후로 이동

### 완료
- [x] **전투 AI 강화:**
  - 랜덤 능력 선택 → 전술적 `_select_ability()` 도입
  - HP 비율/플레이어 상태/쉴드 유무에 따른 능력 우선순위
  - "summon" 능력 추가 (최대HP 15% 회복 + 플레이어 약화)
  - 보스 페이즈2 분노 패턴: 매 3턴 1.3배 강화 + multi_hit 3연타
  - `_boss_turn_counter` 페이즈2 전용 카운터

- [x] **사이드 퀘스트 시스템:**
  - `SideQuest` 유틸리티 클래스 (class_name, 비-오토로드)
  - 3개 퀘스트: Echoes in the Ash (림), The Sump Ledger (시장), Sable's Vigil (심)
  - 플래그 기반 단계 진행 (GameManager.story_flags)
  - 보상: Grains + 아이템 + 고유 기억
  - 맵별 NPC/단서 트리거 배치 (림/시장/심)
  - 대화 데이터 추가 (chapter1/2/4_dialogue.json)
  - StoryJournal "Quests" 탭 추가 (상태별 색상 표시)
  - "all_quests" 업적 (Memory Hunter) + check_quest_complete()

- [x] **맵 시각 다양성 (5개 맵):**
  - 림 포레스트: 발광 버섯 + 쓰러진 통나무
  - 베르단 시장: 걸린 랜턴 + 연기 효과
  - 크럼블링 코스트: 조수 웅덩이 + 표류목
  - The Seam: 크리스탈 포메이션 + 덩굴
  - BL-07: 기억 파편 + 보이드 균열

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — 전술적 AI, summon 능력, 분노 패턴, 버그 2건 수정
- `scripts/utils/side_quest.gd` — **신규** 사이드 퀘스트 유틸리티
- `scripts/ui/story_journal.gd` — Quests 탭 추가
- `scripts/ui/achievement_manager.gd` — all_quests 업적, check_quest_complete()
- `scenes/maps/rim_forest.gd` — 퀘스트 트리거 + 발광 버섯/통나무 데코
- `scenes/maps/verdan_market.gd` — 퀘스트 트리거 + 랜턴/연기 데코
- `scenes/maps/crumbling_coast.gd` — 조수 웅덩이/표류목 데코
- `scenes/maps/the_seam.gd` — 퀘스트 트리거 + 크리스탈/덩굴 데코
- `scenes/maps/bl07_void.gd` — 기억 파편/보이드 균열 데코
- `data/chapter1_dialogue.json` — 퀘스트 대화 3개
- `data/chapter2_dialogue.json` — 퀘스트 대화 4개
- `data/chapter4_dialogue.json` — 퀘스트 대화 2개

### 다음
- 추가 폴리싱 및 밸런스 조정
- 사운드/VFX 보강

---

## S40 — 2026-04-08 (셰이더 시스템 + 그래픽 대폭 개선)

### 완료
- [x] **커스텀 셰이더 5종 신규:**
  - `water_distortion.gdshader` — 이중 사인파 왜곡 + 수면 반짝임
  - `vignette.gdshader` — 부드러운 원형 비네트 (직사각형 → 래디얼)
  - `dissolve.gdshader` — 노이즈 기반 디졸브 + 엣지 글로우 (적 사망)
  - `chromatic_aberration.gdshader` — 색수차 펄스 (Limit Break 연출)
  - `glow_pulse.gdshader` — 맥동 글로우 (랜턴/크리스탈/보이드)

- [x] **맵 비네트 셰이더 전환:**
  - map_effects.gd: 직사각형 4면 비네트 → 셰이더 기반 원형 비네트
  - battle_scene.gd: 전투 비네트도 셰이더 적용
  - 폴백: 셰이더 로드 실패 시 기존 방식 유지

- [x] **물 왜곡 셰이더:**
  - 물 타일 영역에 실시간 웨이브 왜곡 오버레이 자동 배치
  - row 단위 연속 물 구간 그룹핑 (성능 최적화)
  - 기존 ColorRect 반짝임과 셰이더 오버레이 공존

- [x] **전투 VFX 대폭 강화:**
  - 데미지 숫자 색상 분류: 화염(주황)/보이드(보라)/드레인(연초록)/독(녹색)/콤보(금색)/회복(초록)
  - 슬래시 VFX: 길이 확장 애니메이션 + 크로스 슬래시 + 충격 파편 입자
  - 보이드 전용 VFX: 보라색 방사형 파티클 폭발 (기억 연소/Limit Break)
  - 적 사망 디졸브: 셰이더 기반 노이즈 디졸브 + 보라색 엣지 글로우
  - Limit Break 색수차: 전체 화면 크로마틱 펄스 + 강한 셰이크

- [x] **원형 와이프 전환 (아이리스):**
  - scene_transition.gd에 change_scene_iris() 추가
  - 셰이더 기반 원형 닫힘/열림 (부드러운 엣지)
  - CG/보스전 전환에 활용 가능

- [x] **랜턴/보이드 글로우 셰이더:**
  - 랜턴: glow_pulse 셰이더로 자연스러운 촛불 맥동
  - 보이드 파티클: 환경 글로우 오버레이 추가

### 변경/생성된 파일
- `assets/shaders/water_distortion.gdshader` — **신규**
- `assets/shaders/vignette.gdshader` — **신규**
- `assets/shaders/dissolve.gdshader` — **신규**
- `assets/shaders/chromatic_aberration.gdshader` — **신규**
- `assets/shaders/glow_pulse.gdshader` — **신규**
- `scripts/utils/map_effects.gd` — 셰이더 비네트/물 왜곡/랜턴 글로우/보이드 글로우
- `scenes/battle/battle_scene.gd` — 셰이더 비네트, 데미지 색상, 디졸브, 색수차, 보이드 VFX, 슬래시 개선
- `scripts/core/scene_transition.gd` — 원형 와이프 전환

### 다음
- 밸런스 조정 및 추가 폴리싱

---

## S41 — 2026-04-08 (대규모 업그레이드: 플러그인+전투VFX+장비+UX+성능)

### 완료

- [x] **플러그인 다운로드 (3종):**
  - Dialogic 2 — 비주얼 대화 에디터
  - ShaderV — 2D 비주얼 셰이더 노드 라이브러리
  - GODOT-VFX-LIBRARY — 전투 VFX 씬 컬렉션

- [x] **전투 VFX 강화:**
  - 상태이상 비주얼 (적 스프라이트 틴트: 독=초록 맥동, 화상=주황 깜빡, 약화=파란 톤)
  - 콤보 버스트 VFX (금색 텍스트 스케일 펀치 + 방사형 파티클)
  - 턴 순서 미리보기 (상단 PLAYER/ENEMY 턴 큐 3턴 표시)

- [x] **장비 시스템:**
  - 10종 장비 (무기 4, 방어구 3, 액세서리 3)
  - ATK/DEF 스탯 보너스, 전투에 자동 적용
  - 특수 효과: burn_boost(+20% 연소), void_resist(-25% 보이드 피해)
  - MemoryShop "Equip" 탭으로 구매/장착
  - 세이브/로드에 장비 데이터 포함

- [x] **보스 전투 패턴 확장:**
  - void_pulse: 데미지 + 콤보 초기화
  - despair: 독 + 약화 동시 부여
  - 장비 방어력 적 공격 시 피해 감소

- [x] **UX 개선:**
  - 퀘스트 트래커 HUD (ExplorationHUD에 활성 퀘스트 표시)
  - 장비 상태 HUD 표시
  - 세이브 슬롯 정보 확장 (위치/HP/Grains 표시)
  - 지형별 발걸음 SFX (풀/모래/돌/물 4종)

- [x] **성능 최적화:**
  - 셰이더 캐시 시스템 (MapEffects._shader_cache)
  - 물 반짝임 간격 3→5로 줄여 ColorRect 40% 감소
  - 맵 비네트/글로우/물 왜곡 모두 캐시 적용

- [x] **버그 수정 (이전 세션):**
  - 전투 승리 후 화면 프리즈 (SceneTransition await 누락)
  - 미니맵 시그널 누수 (freed 노드 참조)
  - 세이블 적 처치 미감지

### 변경/생성된 파일
- `addons/dialogic/` — **신규** (플러그인)
- `addons/shaderV/` — **신규** (플러그인)
- `addons/vfx_lib/` — **신규** (플러그인)
- `scripts/core/game_manager.gd` — 장비 시스템 (EQUIPMENT, equip/export/import)
- `scripts/systems/battle_manager.gd` — 장비 방어력, 연소 부스트, 보스 능력 2종, 세이블 킬 감지, 전투 후 await
- `scenes/battle/battle_scene.gd` — 상태이상 비주얼, 콤보 버스트, 턴 미리보기
- `scripts/ui/exploration_hud.gd` — 퀘스트 트래커, 장비 표시
- `scripts/ui/memory_shop.gd` — 장비 탭 (구매/장착)
- `scripts/ui/minimap.gd` — freed 노드 가드
- `scripts/core/player.gd` — 지형별 SFX
- `scripts/systems/audio_manager.gd` — 지형 SFX 4종 (sand/stone/water)
- `scripts/systems/save_manager.gd` — 세이브 정보 확장
- `scripts/utils/map_effects.gd` — 셰이더 캐시, 물 반짝임 최적화

### 다음
- 플러그인 Godot 에디터에서 활성화 (Project > Project Settings > Plugins)
- 장비 밸런스 테스트
- Dialogic 2로 대화 시스템 마이그레이션 검토

---

## S42 — 2026-04-08 (그래픽 대규모 업그레이드)

### 목표
캐릭터, 맵, 전투 전반의 시각적 품질 대폭 개선.

### 완료

#### 1. 2D 조명 시스템 (PointLight2D + CanvasModulate)
- `MapEffects`에 `add_ambient_lighting()`, `add_point_light()`, `add_tile_lights()`, `update_point_lights()` 추가
- 프로시저럴 원형 라이트 텍스처 생성 (`_create_light_texture()`)
- 5개 맵 전체 적용:
  - rim_forest: 어두운 숲 분위기 (0.55 ambient) + 버섯 초록 라이트
  - verdan_market: 따뜻한 시장 (0.5 amber ambient) + 노점 라이트
  - crumbling_coast: 폭풍 해안 (0.5 cool ambient)
  - the_seam: 은신처 (0.4 dim ambient) + 랜턴 PointLight2D 자동 배치
  - bl07_void: 보이드 (0.3 purple ambient) + 코어/파편 보라 라이트

#### 2. 패럴랙스 배경
- `MapEffects.add_parallax_background()` — ParallaxBackground + 3레이어 (하늘/산/중경)
- 바이옴별 중경 요소: 나무/바위/건물/크리스탈 실루엣 프로시저럴 생성
- 하늘 그라디언트 (위 밝음→아래 어둠)
- 5개 맵에 바이옴별 색상+요소 적용

#### 3. 캐릭터 스프라이트 48x48 업그레이드
- `PixelSprite` SIZE 32→48으로 확대 (2.25배 픽셀 밀도)
- 눈 2x2 + 하이라이트 + 눈동자 + 눈썹 추가
- 입, 코, 볼 하이라이트, 귀 디테일 강화
- 코트 라펠, 주름, 벨트+버클, 하단 테두리
- 부츠 하이라이트+상단 테두리
- 팔 스윙 진폭 ±2px로 확대
- player.gd, companion.gd, npc.gd — SPRITE_SIZE 48로 업데이트

#### 4. 전투 파티클 VFX (GPUParticles2D)
- `_play_gpu_slash_particles()` — 물리공격 방사형 스파크 (24 particles)
- `_play_gpu_burn_particles()` — 화염 상승 (40 particles + 열기 오버레이)
- `_play_gpu_void_particles()` — 보이드 방사형 폭발 (50 particles + 에너지 링)
- `_play_heal_vfx()` — 힐 상승 초록 파티클 (25 particles)
- `_play_limit_burst_vfx()` — 리밋 브레이크 80 particles + 백색 플래시
- 기존 ColorRect VFX에 GPU 파티클 오버레이로 동시 재생

#### 5. 전투씬 연출 강화
- `_add_battle_atmosphere()` — 배경 먼지 파티클 20개 상시 + 컬러 그레이딩 오버레이
- 적 이름 기반 파티클/그레이딩 색상 (보이드=보라, 일반=재)
- 스크린 셰이크 강화: 프레임 수 = 6+intensity×2, 진폭 ±7px

### 수정/생성 파일
- `scripts/utils/pixel_sprite.gd` — 48x48 전면 리라이트
- `scripts/utils/map_effects.gd` — 조명+패럴랙스 시스템 추가
- `scenes/battle/battle_scene.gd` — GPU 파티클 VFX 6종 + 분위기 시스템
- `scenes/maps/rim_forest.gd` — 조명+패럴랙스 적용
- `scenes/maps/verdan_market.gd` — 조명+패럴랙스 적용
- `scenes/maps/crumbling_coast.gd` — 조명+패럴랙스 적용
- `scenes/maps/the_seam.gd` — 조명+패럴랙스 적용
- `scenes/maps/bl07_void.gd` — 조명+패럴랙스 적용
- `scripts/core/player.gd` — SPRITE_SIZE 48
- `scripts/core/companion.gd` — SPRITE_SIZE 48
- `scripts/core/npc.gd` — SPRITE_SIZE 48

### 다음
- S43 그래픽 심화 개선

---

## S43 — 그래픽 심화 개선: 캐릭터 아웃라인, 타일 텍스처, 애니메이션 타일, 적 스프라이트, 색상 팔레트 (2026-04-08)

### 목표
S42 이후에도 아쉬운 그래픽 품질을 한 단계 더 끌어올리기 위한 5가지 심화 개선.

### 구현 내용

#### 1. 캐릭터 아웃라인 (1px 외곽선)
- `PixelSprite._add_outline()` — 48x48 스프라이트 8방향 이웃 체크, 투명 픽셀에 아웃라인 배치
- `_add_outline_64()` — 64x64 적 스프라이트용
- 모든 캐릭터(아렐/엘리아/세이블/NPC) 자동 적용

#### 2. 타일 텍스처 강화
- `TilePainter` 전면 리라이트: 풀(다층 노이즈+18풀잎+꽃4색), 물(듀얼웨이브+4줄파도), 돌(사인파텍스처+균열+이끼), 나무(수피텍스처+3층캐노피+엣지잎), 길(10자갈+발자국), 모래(듀얼웨이브+조개), 보이드(맥동에너지링)

#### 3. 애니메이션 타일
- `MapEffects.add_grass_sway()` — 풀 타일에 흔들리는 풀잎 배치 (4타일당 1개)
- `MapEffects.update_grass_sway()` — sin() 기반 회전 애니메이션
- `MapEffects.add_fire_particles()` — 랜턴 타일에 GPUParticles2D 불꽃 (6파티클, 주황→빨강)
- 림 외곽 숲: 풀 흔들림, The Seam: 불꽃 파티클

#### 4. 적 스프라이트 생성 (64x64)
- `PixelSprite.create_enemy_sprite()` — 적 종류별 전용 스프라이트
- 5종 전용 드로어: Void Beast(보라 갑각+빛나는 눈), Shadow Wisp(유령형), Memory Eater(턱+빛나는 코어), Shade Sentinel(갑옷+검), Void Stalker(가시+꼬리)
- `_draw_generic_enemy()` — 미등록 적은 이름 해시 기반 색상
- 전투씬 `_build_enemy_sprite()`에서 ColorRect 대신 적용

#### 5. 색상 팔레트 갱신
- 캐릭터: 더 채도 높은 색상 (아렐 파랑 강화, 엘리아 금빛, 세이블 은색)
- 림 외곽 숲: 풀 0.15,0.32,0.12 (진한 녹색), 물 0.08,0.18,0.32 (깊은 파랑)
- 베르단 시장: 돌 0.32,0.3,0.28, 가판대 0.42,0.3,0.18 (따뜻한 톤)

### 버그 수정
- `battle_scene.gd` — `BattleManager.enemy_name` → `BattleManager.current_enemy.name` (크래시 수정)
- `exploration_hud.gd` — `SideQuest.has_method()` 비정적 호출 → `SideQuest.get_all_quests()` 정적 호출로 변경

### 수정/생성 파일
- `scripts/utils/pixel_sprite.gd` — 아웃라인 + 적 스프라이트 + 색상 갱신
- `scripts/utils/tile_painter.gd` — 7종 타일 텍스처 전면 강화
- `scripts/utils/map_effects.gd` — 풀 흔들림 + 불꽃 파티클
- `scenes/battle/battle_scene.gd` — 적 스프라이트 생성 적용
- `scenes/maps/rim_forest.gd` — 풀 흔들림 + 색상 팔레트
- `scenes/maps/verdan_market.gd` — 색상 팔레트
- `scenes/maps/the_seam.gd` — 불꽃 파티클

### 다음
- S44 전투씬 비주얼 오버홀

---

## S44 — 전투씬 사이드뷰 오버홀: 캐릭터/적 128x128, 전투 애니메이션, 삽화 느낌 연출 (2026-04-08)

### 목표
전투씬에 캐릭터와 몬스터가 실제로 표시되는 사이드뷰 레이아웃으로 전면 개편. 삽화에 가까운 비주얼 연출.

### 구현 내용

#### 1. 전투 전용 128x128 대형 스프라이트 (PixelSprite 확장)
- `create_battle_sprite(who)` — 아렐/엘리아/세이블 전투 포즈 (사이드뷰)
  - 아렐: 검을 든 전투 자세, 사이드뷰 (오른쪽 바라봄)
  - 엘리아: 손 모은 기도 자세, 브로치 강조
  - 세이블: 주먹 쥔 전투 자세, 흉터 디테일
- `create_battle_enemy(enemy_type)` — 5종 전투용 대형 적 스프라이트
  - Void Beast (128x128): 갑각 패턴, 4다리, 보이드 연기, 이빨
  - Shadow Wisp: 유령형, 코어 빛, 떠다니는 입자
  - Memory Eater: 곤충형, 등딱지 무늬, 더듬이
  - Shade Sentinel: 보스 갑옷, 검, 보이드 에너지 코어
  - Void Stalker: 3눈 인간형, 뿔/가시, 긴 팔
- `_add_outline_n()` — N사이즈 범용 아웃라인
- `_bellipse()` — 타원 채우기 헬퍼

#### 2. 사이드뷰 전투 레이아웃
- 왼쪽(x=120): 아렐 스프라이트 (200x200 영역, 포트레이트 또는 픽셀)
- 왼쪽 뒤(x=20): 동행자 스프라이트 (엘리아/세이블, 160x160)
- 오른쪽(x=820): 적 스프라이트 (260x260 영역)
- 하단 58%: 전투 지면 (그라운드 플랫폼, 경계선, 그라데이션)
- 각 캐릭터 아래 그림자 (ColorRect, 투명도)
- 각 캐릭터 발밑 광원 (색상별 은은한 빛)

#### 3. 전투 애니메이션
- 아이들 호흡: 모든 캐릭터/적에 sin() 기반 상하 미세 움직임 (각기 다른 위상)
- 공격 돌진: `_player_attack_rush()` — 아렐이 적 방향으로 빠르게 이동 후 복귀 (BACK ease)
- 피격 밀림: 맞은 대상이 반대 방향으로 살짝 밀렸다 돌아옴
- 피격 깜빡임: 플레이어 피격 시 빨간 틴트, 적 피격 시 흰색 플래시

#### 4. 삽화 느낌 연출
- 속도선: `_play_speed_lines()` — 공격 시 화면 가로 빗금 6줄 (빠르게 날아감)
- 임팩트 버스트: `_play_impact_burst()` — 타격점에 원형 플래시 + 방사선 4개
- 발밑 광원: 캐릭터별 색상 (파랑/금색/보라)
- HP 옆 미니 포트레이트: 아렐 초상화 (52x52)
- VFX 좌표 전부 사이드뷰 기반으로 이동 (적 위치 920,310)
- 로그 패널 중앙 하단 재배치

### 수정/생성 파일
- `scripts/utils/pixel_sprite.gd` — 전투 128x128 스프라이트 시스템 전체 추가
- `scenes/battle/battle_scene.gd` — 사이드뷰 레이아웃 + 애니메이션 전면 개편

### 다음
- F5 실행 테스트: 사이드뷰 레이아웃, 캐릭터/적 표시, 공격 돌진 확인

---

## S45 — 2026-04-09 (삽화 56장 통합 + CG/포트레이트 대규모 업그레이드)

### 완료

#### 1. 이미지 식별 및 분류
- `../이미지/` 폴더 64개 파일 전수 조사 (Read 도구로 시각 확인)
- 포트레이트 16장 + CG 40장 = 56장 매핑 완료

#### 2. 포트레이트 16장 추가 (총 27장)
- 아렐: determined, sad, cold, rage, pensive, battle (기존 5 + 신규 6 = 11장)
- 엘리아: hopeful, sad, determined, calm, side, void (기존 2 + 신규 6 = 8장)
- 말렛: desk (기존 1 + 신규 1 = 2장)
- 네라: neutral (신규 NPC)
- 세릭: neutral (신규 NPC)
- 토비아스: neutral (신규 NPC)

#### 3. CG 40장 추가/교체 (총 71장)
- 아렐 전투: combat(업그레이드), combat2, combat3, wounded(업그레이드)
- 엘리아: reading, healing
- 풍경: ch1_forest2, ch1_ash_forest, ch1_ash_rain2, ch1_green_tree2/3
- 베르단: verdan_city, ch2_verdan2/3/4, ch2_verdan_overlook, ch2_sump_interior2
- 추출: ch2_extraction2/3
- 보이드: void_portal2, void_swirl, void_beast2/3, crumbling_coast2
- 세계관: bureau_tower/2, bureau_hall, frozen_city, seam_forest, village_seam2/3
- 아이템: item_sword, item_memory_vial, item_extractor2, authority_pills, memory_artifacts
- 기타: wasteland, ash_crawler2, arrel_combat3
- Cover2.png → cover.png (타이틀 화면 업그레이드)

#### 4. DialogueBox PORTRAIT_MAP 확장
- 16개 신규 포트레이트 키 등록
- DEFAULT_PORTRAITS에 Nera, Seric, Tobias 추가

#### 5. CG 참조 업그레이드 (6개 JSON + 2개 GDScript)
- Ch1: ash_rain → ash_rain2 (아렐+엘리아 동행 장면)
- Ch1: green_tree → green_tree2 (선명한 녹색 나무)
- Ch2: verdan → verdan2, sump_interior → sump_interior2, extraction → extraction2
- Ch3: seam_arrival → village_seam2
- Ch4: village_seam → village_seam2 (2곳), void_portal → void_portal2
- Ch5: void_portal → void_portal2, item_memory_ampoule → item_memory_vial (2곳)
- Ch6: village_seam → village_seam2 (4곳)
- the_seam.gd: village_seam → village_seam2 (전투 배경 3곳)

### 수정/생성 파일
- `assets/portraits/` — 16장 신규 추가
- `assets/cg/` — 40장 신규 추가/교체
- `scripts/ui/dialogue_box.gd` — PORTRAIT_MAP + DEFAULT_PORTRAITS 확장
- `data/chapter1_dialogue.json` — CG 1곳 업그레이드
- `data/chapter2_dialogue.json` — CG 3곳 업그레이드
- `data/chapter3_dialogue.json` — CG 1곳 업그레이드
- `data/chapter4_dialogue.json` — CG 3곳 업그레이드
- `data/chapter5_dialogue.json` — CG 3곳 업그레이드
- `data/chapter6_dialogue.json` — CG 4곳 업그레이드
- `scenes/maps/rim_forest.gd` — green_tree CG 교체
- `scenes/maps/the_seam.gd` — 전투 배경 village_seam2로 교체

### 다음
- F5 테스트: 새 CG/포트레이트 표시 확인
- 추가 포트레이트를 대화 JSON에 적용 (감정별 전환)

---

## S46 — 2026-04-09 (게임성/그래픽 개선 — 전투 타격감, VFX, 아군 조작, 기억 연소 반응, 맵 비주얼)

### 완료

#### 1. 전투 타격감 강화
- 히트스톱: 피해량 비례 0.04~0.12초 일시정지 (process_always 타이머)
- 화면 셰이크 스케일링: 데미지 비례 0.5~3.0x 강도
- 콤보 배율 확장: 2연속 1.15x → 6+ 연속 2.0x (기존 3단계→5단계)
- 콤보 마일스톤 보상: 3/5/7 연속 시 Limit 게이지 보너스 (+5/+10/+20)
- 보스 페이즈2 극적 전환: 0.4초 프리즈 + 빨간 화면 플래시 + "PHASE 2" 경고 + 3.0x 셰이크 + 색수차 + outline_glow 셰이더

#### 2. VFX Library 셰이더 전투 적용
- flash_white.gdshader: 적 피격 시 백색 플래시 (flash_amount 트윈)
- poison.gdshader: 독 상태이상 녹색 펄스 (status_changed 시그널 연동)
- burning.gdshader: 화상 상태이상 열 왜곡 + 엣지 버닝
- outline_glow.gdshader: 보스 페이즈2 빛나는 아웃라인
- dissolve.gdshader: 적 사망 시 디졸브 효과 (준비)

#### 3. 아군 조작 시스템 (세이블)
- BattleManager.set_ally_command(): 플레이어 지정 행동 설정
- ally_command_pending 플래그: 턴 종료 시 자동 랜덤 대신 지정 행동 실행
- 전투 UI: Heal/Strike/Weaken/Guard 4버튼 HBoxContainer
- 선택 하이라이트: 선택된 버튼 노란색, 나머지 회색
- Guard 방어 행동: 50% 데미지 감소 (player_defending 플래그)

#### 4. 기억 연소 월드 반응
- desaturation.gdshader: 연소량 비례 채도 감소 (0~40%) + 보이드 보라 틴트 (5회+ 연소 시)
- MapEffects.add_burn_desaturation(): CanvasLayer 기반 풀스크린 포스트프로세싱
- 5개 맵 전체 적용 (rim_forest, verdan_market, crumbling_coast, the_seam, bl07_void)

#### 5. 맵 비주얼 강화
- 반딧불 파티클: 림 숲(초록 12개) + The Seam(앰버 20개) — GPUParticles2D + 그라디언트 페이드
- 열기 왜곡 셰이더: heat_haze.gdshader (screen_texture + 사인파 왜곡) — 해안/시장 적용
- MapEffects.add_fireflies(): 범용 반딧불 생성기 (색상/수량 파라미터)
- MapEffects.add_heat_haze(): 범용 열 왜곡 효과 (강도 파라미터)
- MapEffects.update_ambient_pulse(): 동적 CanvasModulate 색상 변화 유틸

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `scripts/systems/battle_manager.gd` | 콤보 확장, 아군 커맨드, 방어, 보스 페이즈 시그널 |
| `scenes/battle/battle_scene.gd` | 히트스톱/셰이크/VFX 셰이더/아군 UI/보스 페이즈 연출 |
| `assets/shaders/desaturation.gdshader` | **신규** — 기억 연소 채도 감소 |
| `assets/shaders/heat_haze.gdshader` | **신규** — 대기 열 왜곡 |
| `scripts/utils/map_effects.gd` | add_burn_desaturation/add_fireflies/add_heat_haze/update_ambient_pulse |
| `scenes/maps/rim_forest.gd` | 반딧불 + 연소 탈색 |
| `scenes/maps/verdan_market.gd` | 열 왜곡 + 연소 탈색 |
| `scenes/maps/crumbling_coast.gd` | 열 왜곡 + 연소 탈색 |
| `scenes/maps/the_seam.gd` | 반딧불 + 연소 탈색 |
| `scenes/maps/bl07_void.gd` | 연소 탈색 |

### 다음
- F5 테스트: 전투 VFX + 맵 비주얼 확인
- 감정별 포트레이트 대화 JSON 적용

---

## S47 — 2026-04-10 (삽화 93장 대규모 통합)

### 개요
유저가 assets/cg/에 93개의 새 이미지 파일 추가. 한국어/gemini 파일명을 게임용으로 리네이밍하고, 포트레이트 19장 분리, CG ~60장 정리, 전체 대화 JSON + 맵 전투 스크립트에 연결.

### 완료

#### 1. 이미지 리네이밍 및 분류 (~93파일)
- 한국어/gemini 파일명 → 영문 게임용 이름 (예: "기억 원혼" → memory_wraith2.jpg)
- 포트레이트용 19장 → `assets/portraits/`로 복사
- CG용 ~60장 → `assets/cg/`에 정리

#### 2. PORTRAIT_MAP 확장 (dialogue_box.gd)
- 19개 신규 키 추가: arrel_default2, arrel_cold2, arrel_heroic, arrel_wounded2, arrel_burn, arrel_exhausted, elia_wind, elia_default2, elia_void2, elia_calm2, elia_wind2, elia_mature, nera_bureau, malet_smirk, malet_casual, seric_clipboard, sable_portrait, kairos_portrait, tobias_uniform
- 총 PORTRAIT_MAP 46키

#### 3. 대화 JSON CG/포트레이트 업그레이드 (~42개 편집)
- **chapter1_dialogue.json** (6): ch1_twisted_forest, arrel_combat4, ch1_ash_walk, elia_wind, ch1_campfire, ch1_stump2, ch1_arrel_ghost
- **chapter2_dialogue.json** (7): verdan5, sump_entrance, malet_smirk, sump_interior3, item_extractor3, extraction4, arrel_exhausted, ch2_arrel_malet
- **chapter3_dialogue.json** (5): ch3_seam_coast, ch3_arrel_elia_coast, ch3_kairos_cliff, village_seam3, ch3_elia_healing
- **chapter4_dialogue.json** (6): village_seam3, ch4_sable_house, ch5_void_entrance, ch4_sable_porch, ch4_elia_window, ch1_elia_flower
- **chapter5_dialogue.json** (6): ch5_void_entrance, void_islands, ch5_arrel_elia_void, battle_memory_burn, ch5_climax, arrel_burn
- **chapter6_dialogue.json** (12): village_seam4, arrel_exhausted, elia_mature, ch4_arrel_sable, ch5_climax2, arrel_wounded2, great_tree.png, ch4_sable_porch

#### 4. 맵 전투 배경/적 이미지 업그레이드 (5개 맵)
- **rim_forest.gd**: ch1_twisted_forest/twisted_forest2, ash_crawler3, void_beast3
- **crumbling_coast.gd**: ch3_seam_coast, void_beast3
- **the_seam.gd**: village_seam3, memory_wraith2, ch5_void_entrance, void_husk
- **bl07_void.gd**: void_islands.png, memory_wraith3, void_husk2
- **verdan_market.gd**: (S46에서 이미 업그레이드)

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `assets/cg/` | ~60 CG 파일 리네이밍 |
| `assets/portraits/` | 19장 신규 포트레이트 복사 |
| `scripts/ui/dialogue_box.gd` | PORTRAIT_MAP 19키 추가 (총 46키) |
| `data/chapter1_dialogue.json` | CG/포트레이트 6곳 업그레이드 |
| `data/chapter2_dialogue.json` | CG/포트레이트 7곳 업그레이드 |
| `data/chapter3_dialogue.json` | CG/포트레이트 5곳 업그레이드 |
| `data/chapter4_dialogue.json` | CG/포트레이트 6곳 업그레이드 |
| `data/chapter5_dialogue.json` | CG/포트레이트 6곳 업그레이드 |
| `data/chapter6_dialogue.json` | CG/포트레이트 12곳 업그레이드 |
| `scenes/maps/rim_forest.gd` | 전투 이미지 업그레이드 |
| `scenes/maps/crumbling_coast.gd` | 전투 이미지 업그레이드 |
| `scenes/maps/the_seam.gd` | 전투 이미지 업그레이드 |
| `scenes/maps/bl07_void.gd` | 전투 이미지 업그레이드 |

### 다음
- F5 테스트: 새 CG/포트레이트 표시 확인
- 추가 이미지 에셋이 있으면 계속 통합

---

## S48 — 2026-04-10 (스토리 대폭 확장 + NPC 반복 대화 수정)

### 개요
스토리가 너무 빨리 끝나는 문제 해결. Ch1~Ch5 전체에 탐색 중 자동 발생하는 대화/이벤트 24개 추가 (약 355줄). NPC 재대화 반복 버그도 수정.

### 완료

#### 1. NPC 대화 반복 방지
- npc.gd: `_talked_keys` + `talked_{name}_{key}` 플래그로 재대화 차단, `repeat_line` export
- companion.gd: 동일 패턴 적용
- 4개 맵에 맥락별 repeat_line 설정 (Elia/Sable/Malet)
- verdan_market: Malet 거절 시 talked 플래그 리셋 (재시도 흐름 보존)

#### 2. Ch1 대화 확장 (5개 신규, ~65줄)
- elia_forest_walk: 연소 후 관자놀이 만지는 습관 + 버단의 남자 이야기
- elia_memory_talk: 기억 연소 느낌 묘사 ("방에 들어갔는데 왜 왔는지 잊은 것")
- elia_anchor_talk: 앵커 시스템 설명 + "3번 전에도 같은 질문을 했다"
- forest_shrine: 기억 사당 — 사람들이 태운 기억을 위해 만든 제단
- dead_burner: Grade 1 번아웃 시체 — 모든 것을 태운 사람의 말로

#### 3. Ch2 대화 확장 (5개 신규, ~70줄)
- verdan_market_walk: 기억 시장 풍경 ("어머니의 사랑 — Grade 3 — 약간 중고")
- verdan_old_burner: 이름을 잊은 노인 — 아렐의 미래 암시
- malet_backstory: 말렛의 과거 — 전직 관리국 3과, 17명의 기억으로 재건
- elia_sump_concern: Grade 2 이상 거래 금지 약속
- sump_atmosphere: 썸프 지하 분위기 (첫 키스 팔이, 다른 사람의 과거에 빠지지 말 것)

#### 4. Ch3 대화 확장 (5개 신규, ~70줄)
- coast_cliff_walk: 절벽 기억 잔류물 — 도시였던 해안선의 에코
- coast_watchtower: 폐 감시탑 — 수천 개의 연소 기록 탈리 마크
- kairos_presence: 에디터의 프로파일링 설명 — "어떤 기억이 전부를 지탱하는지 찾는다"
- coast_void_crack: 보이드 균열 — 잃어버린 기억이 안에 있다고 초대
- elia_before_separation: 앵커 없는 연소의 위험 설명 (잔류물 없음)

#### 5. Ch4 대화 확장 (5개 신규, ~85줄)
- seam_evening: 저녁 풍경 + 요리 냄새 + 아이들 + 웃음
- sable_past: 세이블의 과거 — 애쉬브릿지 7인 팀 전멸 이야기
- seam_residents: 정원사 NPC — 남편이 결혼식 기억을 태웠지만 매일 정원을 가꾸는 이야기
- elia_night_talk: 밤하늘 아래 엘리아의 고백 — "빛이 꺼지는 걸 보는 게 지쳤다"
- sable_preparation: BL-07 진입 전 전술 브리핑 + 실제 화염 랜턴

#### 6. Ch5 대화 확장 (4개 신규, ~65줄)
- void_descent: 허공에 떠 있는 문 — 집을 먹힌 기억
- void_echoes: 보이드가 보여주는 환각 — "예전에는 더 많이 웃었다"
- void_memory_fragments: 주인 잃은 기억 결정체 플랫폼
- void_before_core: 봉인 직전 엘리아의 최종 대화 — "지금의 당신으로 충분하다"

#### 7. 5개 맵 탐색 트리거 연결
- rim_forest: 5개 Area2D 트리거 (기억 사당, 죽은 버너, 엘리아 대화 3개)
- verdan_market: 5개 트리거 + _add_story_trigger 범용 함수
- crumbling_coast: 5개 트리거 (절벽, 감시탑, 카이로스, 균열, 분리 전 대화)
- the_seam: 5개 트리거 (저녁, 세이블, 주민, 엘리아 밤, BL-07 준비)
- bl07_void: 4개 트리거 (하강, 에코, 파편, 코어 직전)

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `scripts/core/npc.gd` | talked 플래그 + repeat_line |
| `scripts/core/companion.gd` | talked 플래그 + repeat_line |
| `data/chapter1_dialogue.json` | 5개 신규 대화 (65줄) |
| `data/chapter2_dialogue.json` | 5개 신규 대화 (70줄) |
| `data/chapter3_dialogue.json` | 5개 신규 대화 (70줄) |
| `data/chapter4_dialogue.json` | 5개 신규 대화 (85줄) |
| `data/chapter5_dialogue.json` | 4개 신규 대화 (65줄) |
| `scenes/maps/rim_forest.gd` | 5개 탐색 트리거 |
| `scenes/maps/verdan_market.gd` | 5개 탐색 트리거 + _add_story_trigger |
| `scenes/maps/crumbling_coast.gd` | 5개 탐색 트리거 + _add_story_trigger |
| `scenes/maps/the_seam.gd` | 5개 탐색 트리거 + _add_story_trigger |
| `scenes/maps/bl07_void.gd` | 4개 탐색 트리거 + _add_story_trigger |

### 총 대화량 변화
- Before: 544줄, 49키
- After: ~900줄, 73키 (~65% 증가)

### 다음
- 10챕터 확장 (S49에서 진행)

---

## S49 — 2026-04-10 (10챕터 확장 Phase 1: 구조 변경 + Ch3/Ch4 신규 맵)

### 목표
Part 1 완성을 위해 6챕터 → 10챕터 확장. Phase 1: 구조 재편 + 새 챕터 2개.

### 완료

#### 1. 챕터 구조 전면 재편
기존 6챕터에서 10챕터(+ 에필로그)로 확장:

| 챕터 | 맵 | 이전 챕터 | 상태 |
|------|------|-----------|------|
| Ch1 | rim_forest | Ch1 | 유지 |
| Ch2 | verdan_market | Ch2 | 유지 |
| **Ch3** | **belt_waystation** | — | **신규** |
| **Ch4** | **drift_shelter** | — | **신규** |
| Ch5 | crumbling_coast | Ch3 | 번호 변경 |
| Ch6 | the_seam | Ch4 | 번호 변경 |
| Ch7-9 | (미구현) | — | 향후 |
| Ch10 | bl07_void | Ch5 | 번호 변경 |
| Epilogue | the_seam | Ch6 | 번호 변경 |

#### 2. 대화 파일 재편
- `chapter3_dialogue.json` → **신규** (Belt Waystation, 8키 ~80줄)
- `chapter4_dialogue.json` → **신규** (Drift Shelter, 6키 ~75줄)
- `chapter5_dialogue.json` ← 구 chapter3 (Crumbling Coast)
- `chapter6_dialogue.json` ← 구 chapter4 (The Seam)
- `chapter10_dialogue.json` ← 구 chapter5 (BL-07 Void)
- `epilogue_dialogue.json` ← 구 chapter6 (에필로그)

#### 3. Ch3: Belt Waystation (Weight of Pages)
- **신규 맵** `belt_waystation.tscn` + `.gd` (25x18 타일)
- 타일: 죽은 토양(회색), 갈라진 도로, 폐허, 벽, 길, 건물 내부
- **토비아스 크레인** NPC (관리국 기록관, Class C)
- 스토리 시퀀스: 도착 → 토비아스 만남 → 백서 발견 → 카이로스 벽 낙서 → 토비아스 합류
- 백서(Blank Book) 아이템: 기억의 형태를 기록하는 기록수 섬유
- "Subject demonstrates Class Seven combustion efficiency" 벽 낙서
- 전투 2개 (Belt Scavenger, Void Wisp)
- 상자 2개 + 단서 1개
- 탐색 이벤트 2개 (belt_atmosphere, tobias_records)
- 랜덤 인카운터 3종

#### 4. Ch4: Drift Shelter (Drift)
- **신규 맵** `drift_shelter.tscn` + `.gd` (25x18 타일)
- 타일: 진흙, 잔해, 콘크리트, 벽, 길, 셸터(지붕)
- 메모리 레인 (재비 파티클)
- 스토리 시퀀스: 도착 → 읽기 능력 저하 → 앵커링 세션
- **앵커링 세션**: 엘리아가 아렐의 손을 잡고 기억 구조 안정화
- 관리국 분류 체계 설명 (Class 1~7, 토비아스)
- 밤 대화: 아렐의 비자발적 기억 소실 (11개 micro-memory 2일 내 소실)
- 전투 2개 (Memory Leech, Rubble Rat)
- 상자 1개 + 단서 2개
- 탐색 이벤트 2개

#### 5. 전체 플래그 체계 업데이트
- ch3→ch5, ch4→ch6, ch5→ch10 플래그 이름 변경 (모든 맵/UI/시스템)
- 에필로그 조건: `current_chapter >= 11` + `ch10_complete`
- 스토리 저널: 6개 신규 엔트리 (Belt, Tobias, Blank Book, Wall Writing, Drift, Anchoring)
- Fast Travel: 7개 맵 (Belt, Drift 추가)
- PauseMenu: 챕터 이름 Dictionary 전환 (비연속 챕터 번호 대응)
- credits.gd: hidden_ch4_garden → hidden_ch6_garden

#### 6. 기억 시스템 확장
- Ch3 기억 2개: "The Taste of Dead Earth" (Grade 5), "The Man Who Writes Everything Down" (Grade 4, Tobias)
- Ch4 기억 2개: "Rain That Isn't Rain" (Grade 5), "Warm Hands on Cold Palms" (Grade 3, Elia)
- 기존 Ch3/4/5 기억 → Ch5/6/10으로 재배치

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `scenes/maps/belt_waystation.tscn` | **신규** — Ch3 맵 씬 |
| `scenes/maps/belt_waystation.gd` | **신규** — Ch3 맵 스크립트 |
| `scenes/maps/drift_shelter.tscn` | **신규** — Ch4 맵 씬 |
| `scenes/maps/drift_shelter.gd` | **신규** — Ch4 맵 스크립트 |
| `data/chapter3_dialogue.json` | **재작성** — Belt Waystation (8키) |
| `data/chapter4_dialogue.json` | **재작성** — Drift Shelter (6키) |
| `data/chapter5_dialogue.json` | 구 ch3 내용 (챕터 번호만 변경) |
| `data/chapter6_dialogue.json` | 구 ch4 내용 (챕터 번호 + 타이틀 변경) |
| `data/chapter10_dialogue.json` | 구 ch5 내용 (챕터 번호 변경) |
| `data/epilogue_dialogue.json` | 구 ch6 내용 (새 파일명) |
| `scenes/maps/verdan_market.gd` | Ch2→Ch3 전환 대상 변경 |
| `scenes/maps/crumbling_coast.gd` | 챕터 3→5 번호 변경 |
| `scenes/maps/crumbling_coast.tscn` | dialogue_file 경로 변경 |
| `scenes/maps/the_seam.gd` | 챕터 4→6 번호 변경 + 에필로그 조건 |
| `scenes/maps/the_seam.tscn` | dialogue_file 경로 변경 |
| `scenes/maps/bl07_void.gd` | 챕터 5→10 번호 변경 |
| `scenes/maps/bl07_void.tscn` | dialogue_file 경로 변경 |
| `scripts/systems/memory_manager.gd` | Ch3/4 신규 기억 + 재배치 |
| `scripts/ui/story_journal.gd` | 6개 신규 엔트리 + 플래그 업데이트 |
| `scripts/ui/pause_menu.gd` | 챕터 이름 Dict + Fast Travel 7맵 |
| `scenes/ui/credits.gd` | hidden_ch6_garden 플래그 수정 |

### 총 대화량 변화
- Before: ~900줄, 73키
- After: ~1050줄, 87키 (~17% 증가)

### 챕터 전환 체인
```
Ch1 (rim_forest) → Ch2 (verdan_market) → Ch3 (belt_waystation) → Ch4 (drift_shelter)
→ Ch5 (crumbling_coast) → Ch6 (the_seam) → [Ch7-9 미구현, 직접 점프] → Ch10 (bl07_void) → Epilogue
```

### 다음
- Ch7 (The Other Side of the Flame) 구현
- Ch8 (Forest That Forgets) 구현
- Ch9 (Where Colors Stop) 구현
- 토비아스 전투 동행 시스템 (파티 시스템 확장)

---

## S50 — 2026-04-12 (10챕터 확장 Phase 2: Ch7-9 구현 + 전투 능력 확장)

### 목표
10챕터 구조 완성. Ch7-9 맵/대화/시스템 구현 + 전투 신규 능력 + 전환 체인 완성.

### 완료

#### 1. 전투 시스템 확장 — 3개 신규 적 능력
- **stun** (기절): 약한 데미지 + 다음 플레이어 턴 스킵. 콤보 차단 전술용.
- **reflect** (반사): 배리어 + 다음 공격 30% 데미지 반사.
- **charge** (차지): 1턴 대기 → 다음 적 턴 2배 데미지 강타.
- 전술 AI에 중복 회피 로직 통합 (reflect/charge/stun 중복 방지)
- 스턴 시 플레이어 턴 자동 스킵 (로그 메시지 + 딜레이)

#### 2. Ch7: Seam Outskirts (The Other Side of the Flame)
- **맵** `seam_outskirts.tscn` + `.gd` (25x18 타일)
- 세이블 진실: BL-07은 구멍이 아니라 입. 기억을 부른다.
- 에코 셸 획득 (BL-07 희생자들의 마지막 메아리)
- 세이블 시련 전투 (Threshold Shade: drain/stun/reflect)
- 전투 3개 (Void Sentinel, Ash Phantom, Threshold Crawler)
- 상자 1개 + 단서 2개 + 탐색 이벤트 2개
- **대화** `chapter7_dialogue.json` (8키 ~90줄)

#### 3. Ch8: Forgotten Forest (The Forest That Forgets)
- **맵** `forgotten_forest.tscn` + `.gd` (25x18 타일)
- 기억 기생 숲: 나무가 기억을 먹음, 유령(remnant) NPC
- 토비아스의 링 이론 (동심원 17+1개 = 소비 사건 수)
- 엘리아 앵커링 부담 (아렐이 엘리아 이름을 잊음)
- 숲 속삭임 + 유령 아이 + 돌무더기 안전지대
- 전투 3개 (Memory Leech, Hollow Walker, Root Shade)
- 상자 2개 + 단서 2개 + 탐색 이벤트 4개
- **대화** `chapter8_dialogue.json` (9키 ~95줄)

#### 4. Ch9: Colorless Waste (Where Colors Stop)
- **맵** `colorless_waste.tscn` + `.gd` (25x18 타일)
- 완전 탈색 환경 (모노크롬 타일/패럴랙스/조명)
- 메모리 나침반 획득 (아렐 몸이 BL-07에 반응)
- **카이로스 대면**: 직접 대화. Outcome A/B 두 가지 결말 예측.
- 전투 3개 (Colorless Wraith, Depth Crawler, Void Fragment)
- 상자 1개 + 단서 2개 + 탐색 이벤트 3개
- **대화** `chapter9_dialogue.json` (8키 ~90줄)

#### 5. 전환 체인 완성
- `the_seam.gd`: Ch6 완료 → Ch7 (seam_outskirts)로 변경 (기존: bl07_void)
- Ch7 → Ch8 (forgotten_forest) → Ch9 (colorless_waste) → Ch10 (bl07_void)

#### 6. 기억 시스템 확장
- Ch7 기억 2개: "The Taste of Static" (Gr5), "Voices in the Shell" (Gr3, Sable)
- Ch8 기억 2개: "Trees That Remember Being Trees" (Gr5), "A Ghost's Last Sentence" (Gr4)
- Ch9 기억 2개: "The Place Where Color Stopped" (Gr5), "The Memory Compass" (Gr2)

#### 7. UI 전면 업데이트
- **StoryJournal**: CHAPTER_NAMES 11개 챕터 + 13개 신규 이벤트 엔트리
- **AchievementManager**: Ch7/8/9 완료 업적 3개 추가 (총 28종)
- **PauseMenu**: 챕터 이름 11개 + Fast Travel 10개 맵

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `data/chapter7_dialogue.json` | **신규** — Seam Outskirts (8키) |
| `data/chapter8_dialogue.json` | **신규** — Forgotten Forest (9키) |
| `data/chapter9_dialogue.json` | **신규** — Colorless Waste (8키) |
| `scenes/maps/seam_outskirts.tscn` | **신규** — Ch7 맵 씬 |
| `scenes/maps/seam_outskirts.gd` | **신규** — Ch7 맵 스크립트 |
| `scenes/maps/forgotten_forest.tscn` | **신규** — Ch8 맵 씬 |
| `scenes/maps/forgotten_forest.gd` | **신규** — Ch8 맵 스크립트 |
| `scenes/maps/colorless_waste.tscn` | **신규** — Ch9 맵 씬 |
| `scenes/maps/colorless_waste.gd` | **신규** — Ch9 맵 스크립트 |
| `scenes/maps/the_seam.gd` | Ch6→Ch7 전환으로 변경 |
| `scripts/systems/battle_manager.gd` | stun/reflect/charge 3개 능력 추가 |
| `scripts/systems/memory_manager.gd` | Ch7/8/9 기억 6개 추가 |
| `scripts/ui/story_journal.gd` | CHAPTER_NAMES + 13개 이벤트 |
| `scripts/ui/achievement_manager.gd` | Ch7/8/9 업적 3개 |
| `scripts/ui/pause_menu.gd` | 챕터 이름 + Fast Travel 3맵 |

### 총 대화량 변화
- Before: ~1050줄, 87키
- After: ~1325줄, 112키 (~26% 증가)

### 챕터 전환 체인 (완성)
```
Ch1 (rim_forest) → Ch2 (verdan_market) → Ch3 (belt_waystation) → Ch4 (drift_shelter)
→ Ch5 (crumbling_coast) → Ch6 (the_seam) → Ch7 (seam_outskirts) → Ch8 (forgotten_forest)
→ Ch9 (colorless_waste) → Ch10 (bl07_void) → Epilogue
```

### 다음
- 토비아스 전투 동행 (Ch3+ 파티 시스템 확장)
- 카이로스 보스전 (Ch9 또는 Ch10 내부)
- 추가 사이드 퀘스트 (Ch7-9 맵용)
- CG/포트레이트 연결 (카이로스 포트레이트 등)

---

## S51 — 2026-04-12 (게임성 대폭 업그레이드 — 6대 시스템)

### 완료
- [x] **Memory Decay/Erosion** — 기억 침식 시스템 (챕터 진행 시 기억 약화, Grade 1 면역, 엘리아 관련 반감, is_faded/erosion 필드)
- [x] **Memory Echo** — 연소 후 전장 잔류 효과 (등급별 7종: Fading Warmth/Lingering Habit/Elia Anchor/Sable Shadow/Bond Fracture/Identity Fracture/Total Erasure)
- [x] **Battle Stance** — 전투 자세 3종 (Remnant/Pyre/Hollow, 챕터별 해금, 공방 배율 + 고유 효과)
- [x] **Void Corruption Modifiers** — 보이드 부패 인카운터 수정자 (연소 횟수 기반, 4등급 12종, 전투 난이도 동적 변화)
- [x] **Elia Diary** — 엘리아 일지 + 비연소 전투 기술 (일지 8항목, 기술 4종: Humming Shield/Desperate Reach/Remembered Strike/Anchor Pulse, 쿨다운 시스템)
- [x] **Memory Resonance** — 기억 공명 탐색 이벤트 (10맵 18지점, 기억 비전투 연소로 탐색 보너스, 맥동 시각 효과)
- [x] MemoryUI 침식/소실 시각화 (FADED/ERODING 상태, 알파 페이드, 침식 비율 표시)
- [x] 전투 연소 목록에서 Faded 기억 필터링 + 침식 반영 유효 파워 표시
- [x] EliaDiary 오토로드 등록 + SaveManager 연동 (세이브/로드)
- [x] Lingering Habit 에코 → 콤보 배율 +20% 연결

### 신규 파일
| 파일 | 설명 |
|------|------|
| `scripts/utils/encounter_modifiers.gd` | EncounterModifier 클래스 — 연소 횟수별 전투 수정자 |
| `scripts/ui/elia_diary.gd` | EliaDiary 오토로드 — 일지 + 전투 기술 4종 |
| `scripts/utils/memory_resonance.gd` | MemoryResonance 클래스 — 맵 공명 지점 설치 |

### 수정 파일
| 파일 | 변경 내용 |
|------|----------|
| `project.godot` | EliaDiary 오토로드 등록 |
| `scripts/systems/memory_manager.gd` | is_faded/erosion 필드, apply_erosion(), get_effective_burn_power(), burn_memory_silent() |
| `scripts/systems/battle_manager.gd` | Echo/Stance/Modifier/Elia 통합, player_use_elia_skill(), 콤보 Lingering Habit 연결 |
| `scenes/battle/battle_scene.gd` | Stance UI, Echo 표시, Elia 기술 UI, Faded 필터, 침식 파워 표시 |
| `scripts/ui/memory_ui.gd` | 침식/소실 시각화 (FADED/ERODING 표시, 알파 그라데이션) |
| `scripts/systems/save_manager.gd` | EliaDiary 세이브/로드 연동 |
| `scenes/maps/*.gd` (10파일) | MemoryResonance.setup_points() 호출 추가 |

### 다음
- 전투 밸런스 미세 조정 (침식 속도, 에코 지속턴, 수정자 확률)
- 카이로스 보스전 (Ch9)
- 엘리아 일지 UI 뷰어 (MemoryUI Diary 탭)

---

## S52 — 2026-04-12 (그래픽 대규모 업그레이드)

### 완료
- [x] **2D 그림자 시스템** — PointLight2D shadow 활성화 + LightOccluder2D 벽/나무 타일 자동 생성
- [x] **컬러 그레이딩** — 맵별 분위기 색조 보정 (10맵 바이옴별 커스텀 tint/brightness)
- [x] **캐릭터 드롭 섀도우** — 발밑 타원형 그림자 (탐색 10맵 + 전투 기존)
- [x] **캐릭터 호흡 애니메이션** — 정지 시 미세 스케일 펄스 (탐색 + 전투 3캐릭터)
- [x] **바이옴별 향상 파티클** — 꽃가루(숲), 재(황무지), 보이드 촉수(보이드) + 업데이트 루프
- [x] **스무스 카메라** — Camera2D 부드러운 추적, 드래그 마진, 환경 미세 흔들림(비/보이드 맵)
- [x] **전투 크리티컬 줌** — 200+ 데미지 시 화면 줌 펀치 + 임팩트 플래시
- [x] **연소 화면 이펙트** — 기억 연소 시 화면 가장자리 화염 비네트
- [x] **전투 호흡 스케일** — 아군/적 idle에 미세 스케일 변화 추가 (기존 Y bob + 신규 XY scale)

### 수정 파일
| 파일 | 변경 내용 |
|------|----------|
| `scripts/utils/map_effects.gd` | 신규 함수 14개 (그림자/오클루더/컬러그레이딩/드롭섀도/꽃가루/재/보이드촉수/카메라) |
| `scripts/core/player.gd` | 정지 시 호흡 스케일 애니메이션 |
| `scenes/battle/battle_scene.gd` | 크리티컬 줌, 연소 화면 이펙트, 호흡 스케일, 엘리아 기술 UI |
| `scenes/maps/*.gd` (10파일) | 컬러 그레이딩 + 스무스 카메라 + 드롭 섀도우 + 바이옴 파티클 |

### 맵별 비주얼 설정
| 맵 | 컬러 그레이딩 | 파티클 | 특수 |
|----|-------------|--------|------|
| rim_forest | 초록 틴트 | 꽃가루 12 | 그림자+오클루더 |
| verdan_market | 따뜻한 틴트 | 먼지 8 | 그림자+오클루더 |
| belt_waystation | 황토 틴트 | 먼지 15 | — |
| drift_shelter | 푸른 틴트 | (비) | 미세 흔들림 0.3 |
| crumbling_coast | 해안 틴트 | 물보라 10 | — |
| the_seam | 보라 틴트 | 잔불 8 | 그림자+오클루더 |
| seam_outskirts | 짙은 보라 | 촉수 4 | 흔들림 0.4 |
| forgotten_forest | 병적 초록 | 포자 18 | 흔들림 0.3 |
| colorless_waste | 무채색 | 잿가루 20 | — |
| bl07_void | 심연 보라 | 촉수 8 | 그림자+오클루더, 흔들림 0.6 |

### 다음
- 전투 밸런스 미세 조정
- 카이로스 보스전
- 추가 CG 연결

---

## S53 — 2026-04-12 (20대 업그레이드 — 게임성+스토리+비주얼+폴리싱)

### 완료

**게임성 (6종)**
- [x] **토비아스 전투 동행** — Ch3-6 서포트 (analyze/archive/protect), 전투 UI 커맨드
- [x] **기억 체인 번** — 연속 연소 시 +20% 누적 데미지 보너스 (_burn_chain)
- [x] **NG++ 보스 강화** — cycle 3+ 보스에 despair/charge/reflect 능력 추가
- [x] **장비 강화 시스템** — 0~3단계 업그레이드, MemoryShop 업그레이드 버튼, Grains 비용
- [x] **사이드 퀘스트 3종 추가** — echo_fragments(Ch7), forest_parasite(Ch8), colorless_compass(Ch9)
- [x] **전투 밸런스 조정** — 세이블 공격력 12-22, 침식률 50% 감소, 에코 지속시간 조정

**스토리 (4종)**
- [x] **대화 9종 추가** — Ch7-9 신규 대화 + 플래시백 3종 (Ch4/Ch6/Ch8)
- [x] **엔딩 2종 추가** — tobias 엔딩 (동행 희생), hollow 엔딩 (완전 연소), 총 6종
- [x] **카이로스 보스전** — Ch9 450HP/38ATK, 6능력, 클리어 후 대화
- [x] **토비아스 합류 이벤트** — Belt Waystation에서 tobias_joined 플래그

**비주얼 (5종)**
- [x] **NPC 아이들 애니메이션** — 8개 맵 NPC에 미세 호흡/움직임 추가
- [x] **타일 블렌딩** — auto_blend_edges() 타일 경계 자연스러운 혼합
- [x] **동적 날씨** — update_weather_intensity(), 번개 효과 (drift_shelter/crumbling_coast)
- [x] **파티클 풀링** — _get_pooled_rect/_return_to_pool 성능 최적화
- [x] **오프스크린 컬링** — cull_offscreen_particles() 화면 밖 파티클 비활성화

**폴리싱 (5종)**
- [x] **대화 상자 애니메이션** — slide-up/down 등장·퇴장
- [x] **일시정지 메뉴 애니메이션** — slide-in/out 효과
- [x] **접근성 옵션 3종** — 폰트 크기, 화면 흔들림 토글, 색맹 모드
- [x] **신규 포트레이트 3종** — kairos_cold, kairos_amused, tobias_concerned
- [x] **Windows 내보내기 설정** — export_presets.cfg

### 수정 파일 (28개)
| 파일 | 변경 내용 |
|------|----------|
| `scripts/systems/battle_manager.gd` | 토비아스 동행, 체인 번, NG++ 보스 강화 |
| `scenes/battle/battle_scene.gd` | 토비아스 커맨드 UI, 접근성 셰이크 체크 |
| `scripts/core/game_manager.gd` | 장비 강화 시스템, NG++ 보너스, 전용 장비 2종 |
| `scripts/ui/memory_shop.gd` | 장비 업그레이드 버튼 |
| `scripts/utils/side_quest.gd` | Ch7-9 사이드 퀘스트 3종 |
| `scripts/systems/memory_manager.gd` | 침식률 밸런스 (50% 감소) |
| `data/chapter4~9_dialogue.json` | 대화 9종 + 플래시백 3종 |
| `data/epilogue_dialogue.json` | tobias/hollow 엔딩 2종 |
| `scenes/maps/the_seam.gd` | 엔딩 분기 2종 추가, 플래시백 |
| `scenes/maps/colorless_waste.gd` | 카이로스 보스전 + NPC 애니메이션 |
| `scenes/maps/belt_waystation.gd` | 토비아스 합류 이벤트 |
| `scenes/maps/*.gd` (8파일) | NPC 아이들, 퀘스트 트리거, 번개 |
| `scripts/utils/map_effects.gd` | 동적 날씨, 번개, 파티클 풀/컬링 |
| `scripts/utils/tile_painter.gd` | auto_blend_edges() |
| `scripts/ui/dialogue_box.gd` | slide 애니메이션, 포트레이트 3종 |
| `scripts/ui/pause_menu.gd` | slide-in/out 애니메이션 |
| `scenes/ui/options_menu.gd` | 접근성 설정 3종 |
| `export_presets.cfg` | Windows Desktop 내보내기 템플릿 |

### 다음
- 전체 플레이 테스트
- 추가 CG 연결
- 사운드 추가

---

## S54 — 2026-04-12 (16대 업그레이드 — 스토리+전투+비주얼+시스템)

### 완료

**스토리/콘텐츠 (4종)**
- [x] **캐릭터 블립 SFX** — 언더테일 스타일 텍스트 출력 사운드, 캐릭터별 피치 (아렐1.0/엘리아1.3/세이블0.7 등)
- [x] **엔딩 갤러리** — PauseMenu에서 달성 엔딩 열람 (6종 CG+설명, 미달성 잠금), seen_endings 영구 저장
- [x] **NPC 스케줄** — 챕터별 NPC 위치/대사 변화 (말렛 Ch2→3→6, 토비아스 Ch3→4-6→7+)
- [x] **대화 연출 태그** — [shake]/[slow]/[fast]/[pause=N] 대화 텍스트 특수 효과

**전투/게임성 (4종)**
- [x] **스킬 트리 (번 패시브)** — 총 연소 횟수 기반 5종 패시브 해금 (5/10/20/30/50회)
- [x] **도감 스캔 강화** — 토비아스 분석 시 약점/저항 표시, Codex 영구 기록, Ash Sight 패시브 연동
- [x] **전투 환경 효과** — 10맵별 전투 보너스 (회피/명중/속성 데미지/상태이상/힐링)
- [x] **보스 러시 모드** — 엔딩 달성 후 해금, 연속 보스전 + 최고 기록 타이머

**비주얼/연출 (4종)**
- [x] **맵 전환 다양화** — 맵별 고유 전환 (글리치/낙엽/먼지/안개), 자동 감지
- [x] **전투 승리 화면** — VICTORY/BOSS DEFEATED 연출 + 보상 요약
- [x] **감정 포트레이트 전환** — 동일 캐릭터 크로스페이드, 다른 캐릭터 슬라이드
- [x] **대화 카메라 효과** — [zoom=N]/[pan=X,Y]/[reset] 태그로 카메라 제어

**시스템/편의 (4종)**
- [x] **튜토리얼 힌트** — 5종 상황별 첫 발생 가이드 팝업, 자동 디스미스
- [x] **자동 전투** — AUTO 버튼 토글, AI 행동 선택 (HP/상태/기억등급 기반)
- [x] **통계 화면** — 10종 플레이 통계 (시간/전투/연소/걸음 등), PauseMenu Stats 버튼
- [x] **다국어 기반** — UI 로컬라이제이션 프레임워크 (en/ko 27키), OptionsMenu 언어 전환

### 수정 파일 (25개 + 신규 1개)
| 파일 | 변경 내용 |
|------|----------|
| `scripts/ui/dialogue_box.gd` | 블립SFX, 포트레이트 전환, 대화 태그, 카메라 효과 |
| `scripts/core/game_manager.gd` | 엔딩 갤러리, NPC 스케줄, 보스러시, 통계, 로컬라이제이션 |
| `scripts/systems/battle_manager.gd` | 스킬트리 적용, 스캔, 환경효과, 자동전투, 통계 |
| `scenes/battle/battle_scene.gd` | 승리화면, 스캔UI, 환경표시, AUTO버튼, 로컬라이즈 |
| `scripts/systems/memory_manager.gd` | 번 패시브 5종, 수집 통계 |
| `scripts/core/scene_transition.gd` | 전환 스타일 4종 (글리치/낙엽/먼지/안개) |
| `scripts/ui/pause_menu.gd` | 엔딩갤러리, 통계, 로컬라이즈, 보스러시 |
| `scenes/ui/options_menu.gd` | 언어 전환 |
| `scripts/ui/codex.gd` | 스캔 데이터 표시 |
| `scripts/core/player.gd` | 걸음 통계 |
| `scripts/systems/save_manager.gd` | TutorialHints 세이브 |
| `scripts/ui/memory_shop.gd` | 튜토리얼 힌트 |
| `scenes/maps/*.gd` (10파일) | 스타일 전환, NPC 스케줄 |
| `scenes/main/main.gd` | 보스러시 버튼 |
| `scenes/ui/game_over.gd` | 스타일 전환 |
| `scripts/ui/tutorial_hints.gd` | **신규** — 튜토리얼 힌트 오토로드 |

### 다음
- 전체 플레이 테스트
- 추가 CG/사운드
- 스토리 확장

---

## S60 — 2026-04-24 (하이브리드 VN 모드 Phase 1 — 삽화 중심 스토리 전환)

### 목적
현재 탐색/전투 중심 RPG → **삽화(CG+포트레이트) 중심의 하이브리드 스토리 어드벤처**로 전환.
방식 1(풀 VN) + 2(하이브리드) + 3(장면집) 혼합: 기본은 VN 스타일 씬 시퀀스로 흐르고, 탐색(mini-exploration)·전투(클라이맥스)는 핵심 앵커에서만 삽입.

### 완료

**코어 시스템 (4종 신규)**
- [x] **SceneFlow** 오토로드 (`scripts/systems/scene_flow.gd`) — JSON 구동 VN 시퀀스 런너. CG/포트레이트/나레이션/선택지/액션(`goto_map`/`goto_battle`/`goto_scene`/`end`) 처리, 탐색·전투 후 VN 복귀 큐
- [x] **VNScene UI** (`scenes/ui/vn_scene.tscn` + `scripts/ui/vn_scene.gd`) — 풀스크린 CG 크로스페이드, 좌/우 포트레이트(말하는 쪽 강조), 대화박스(타이프라이터), 나레이션 모드, 선택지 패널, 레터박스, 시스템 로그 표시
- [x] **VNHost** 빈 씬 (`scenes/main/vn_host.tscn/.gd`) — 순수 VN 구간용 배경 컨테이너, SceneFlow 종료 시 resume 자동 처리
- [x] **시나리오 JSON 3종** (`data/vn_scenes/`)
  - `ch1_prologue.json` — 오프닝~아침까지 ~30 스텝, CG 8종 + 포트레이트 15종 사용, Grade 3 연소 후 BL-07/엘리아 허밍/기억 복기 선택
  - `ch1_after_forest.json` — 탐색 후 엘리아 대화 + Green Tree + 뷰로 타워 원경
  - `ch2_market_arrival.json` — 베르단 시장 진입 VN 인트로, 말렛 언급, goto_map으로 탐색 전환

**하이브리드 연결**
- [x] **타이틀 → VN 시작** (`scenes/main/main.gd`) — New Game 시 vn_host로 전환 후 `SceneFlow.play("ch1_prologue")`
- [x] **VN → 탐색 앵커** — `action: goto_map` + `resume_scene` 으로 VN 일시 중단 후 맵 이동, 복귀 큐에 다음 씬 기록
- [x] **탐색 → VN 복귀** (`scenes/maps/rim_forest.gd`) — resume_queue 존재 시 스토리 스킵, 자유 탐색 + 캠프 트리거에서 vn_host로 복귀 후 `SceneFlow.resume_if_queued()`
- [x] **VN → VN 체인** — `action: goto_scene` 으로 씬 간 연쇄 (ch1_after_forest → ch2_market_arrival)

**기존 자산 활용**
- CG: ch1_twisted_forest / arrel_combat4 / ch1_ash_walk / ch1_ash_rain2 / ch1_campfire / ch1_ash_forest / ch1_green_tree / bureau_tower3 / ch2_verdan_overlook / ch2_verdan5
- 포트레이트: elia_wind/concern/neutral/calm/sad/determined/hopeful, arrel_default2/cold/pensive/neutral/determined
- PORTRAIT_MAP은 DialogueBox 오토로드의 것을 공유

### 수정/신규 파일
| 파일 | 변경 내용 |
|------|----------|
| `scripts/systems/scene_flow.gd` | **신규** — VN 시퀀스 런너 오토로드 |
| `scripts/ui/vn_scene.gd` | **신규** — VN UI (CG/포트레이트/대화/선택지) |
| `scenes/ui/vn_scene.tscn` | **신규** — VN UI 씬 |
| `scenes/main/vn_host.gd/.tscn` | **신규** — VN 전용 배경 컨테이너 |
| `data/vn_scenes/ch1_prologue.json` | **신규** — Ch1 오프닝 VN 시나리오 |
| `data/vn_scenes/ch1_after_forest.json` | **신규** — Ch1 후반 VN 시나리오 |
| `data/vn_scenes/ch2_market_arrival.json` | **신규** — Ch2 진입 VN 시나리오 |
| `scenes/main/main.gd` | New Game → VN 프롤로그 재생 |
| `scenes/maps/rim_forest.gd` | VN 하이브리드 모드 — 스토리 스킵 + 캠프 트리거에서 VN 복귀 |
| `project.godot` | SceneFlow 오토로드 등록 |

### 하이브리드 플레이 흐름 (Ch1~Ch2 Phase 1)
```
타이틀 → [VN] ch1_prologue (보이드 비스트 처치 이후 CG 시퀀스 → 아침)
       → [탐색] rim_forest (보이드 사냥 미니 탐색, 전투 트리거)
       → [VN] ch1_after_forest (엘리아 대사 + Green Tree + 뷰로 타워)
       → [VN] ch2_market_arrival (베르단 오버룩 CG 인트로)
       → [탐색] verdan_market (말렛 거래 등 기존 흐름)
```

### 추가 삽화 권장 (차후 세션에서 요청)
현재 자산으로 Ch1~Ch2 VN 전환 완료. 품질 향상에 도움 될 것:
- **VN 스탠딩 전신 CG** — 현재 포트레이트는 흉상 위주. 전신 스탠딩은 VN 임팩트↑
- **챕터 전환 타이포그래피 카드** — "Chapter 1: Ash" 스타일의 전용 타이틀 CG
- **분위기 전용 배경 CG** — 각 챕터 시작·종료 순간의 "분위기 컷" (감정 여운용)
- **엘리아 감정 추가** (elia_exhausted, elia_anger, elia_tears 등)

### 다음 세션 (S61) 할 일
- [ ] Ch3~Ch10 VN 시나리오 JSON 작성 (기존 dialogue JSON을 VN steps로 변환)
- [ ] 세이브/로드에서 SceneFlow 상태 저장·복원 (current_id / current_index / resume_queue)
- [ ] VN 내 ESC 일시정지 + 대화 로그 + 스킵 기능
- [ ] PortraitMap에 스탠딩용 전신 이미지 별도 지원
- [ ] 전투 앵커 연결 (Ch3 보스 등 `goto_battle` 실제 동작)

### 테스트 포인트 (F5 실행)
1. 타이틀 → New Game → VN 프롤로그 자동 시작되는지
2. 클릭/Enter로 진행, 포트레이트 좌우 배치·말하는 쪽 강조 동작
3. CG 크로스페이드 자연스러운지
4. 캠프 밤 선택지 3종 표시·선택 후 다음 진행
5. 아침 장면 후 rim_forest 맵 진입 → 남쪽 끝 도착 → vn_host로 복귀 → ch1_after_forest 재생
6. ch1_after_forest 종료 후 ch2_market_arrival 자동 연결 → verdan_market 맵 진입

---

## S61 — 2026-04-24 (Memory Distortion — Katana ZERO 서사 트릭)

### 목적
MEMORIA 본질(기억을 태운다)을 **서사 레이어에서도 작동**시키기. 기억을 태우면 이후 그 기억과 연결된 대사·CG·포트레이트가 왜곡된 버전으로 재생. 플레이어의 선택이 게임플레이뿐 아니라 **텍스트 그 자체**를 변형시킴 (Katana ZERO 패턴).

### 완료

**1. SceneFlow 왜곡 로직 확장**
- 스텝 필드 추가: `distort_if_burned`(기억 ID) + `distorted_text` / `distorted_narrate` / `distorted_speaker` / `distorted_portrait` / `distorted_cg`
- 기억이 태워진 상태면 스텝 dict를 duplicate 후 필드 교체, `_distorted: true` 플래그로 VN UI에 신호

**2. VNScene 글리치 VFX**
- **기억 연소 순간** (MemoryManager.memory_burned 시그널) — 강한 VFX:
  - 붉은 플래시 (0.55 알파 → 페이드아웃 0.9s)
  - 색수차 분리 (CG의 R/B 채널 복사본을 좌우로 8px 오프셋 후 수렴)
  - SFX `memory_burn`
  - 텍스트 스크램블 (0.12s, `▓▒░█▄▀#@%&*?!` 로 치환 후 원래 텍스트 복원)
- **왜곡된 대사** — 약한 VFX:
  - CG 색수차 3px 약하게 1.2s 지속 후 페이드
  - 플레이어가 "뭔가 어긋났다"는 감각을 받게

**3. Ch1 프롤로그 왜곡 시퀀스 삽입**
- 재비 장면 중간에 **능동 연소 선택** 추가:
  - "Burn it. The song for passage." → `daily_campfire_song` 태움 + `burned_for_passage` 플래그
  - "Hold on to it. Find another way." → `refused_to_burn` 플래그
- 캠프 밤 선택지 이후 **엘리아 허밍 시퀀스** 추가 (3줄) — 각각 `distort_if_burned: "daily_campfire_song"`로 연소 시 다른 텍스트/포트레이트 재생:
  - 나레이션: "threadbare melody" → "He waited for the melody to mean something. It didn't."
  - 아렐: "...I know that song." → "...Is that a song? I can't tell." (포트레이트 cold로)
  - 엘리아: "Your mother used to hum it..." → "...You used to know it." (포트레이트 sad로)

### 수정 파일
| 파일 | 변경 |
|------|------|
| `scripts/systems/scene_flow.gd` | `_run_step()`에 왜곡 분기 추가 (15줄) |
| `scripts/ui/vn_scene.gd` | 글리치 레이어, `_on_memory_burned`, `_play_burn_glitch`, `_play_subtle_distortion`, `_scramble_text` (~80줄) |
| `data/vn_scenes/ch1_prologue.json` | 능동 연소 선택지 + 왜곡 대사 3줄 추가 |

### 플레이어 경험
- **태우지 않은 플레이:** 엘리아가 어머니 노래를 알아보고 기억한다고 말함. 따뜻함.
- **태운 플레이:** 엘리아가 허밍하지만 그게 뭔지 모름. 엘리아는 "너는 예전엔 알았었어"만 말함. 색수차로 화면이 미세하게 어긋남. 무게 있는 상실감.
- 이 한 장면만으로 "기억 태움 = 서사 변형"의 MEMORIA 정체성이 플레이어에게 전달됨.

### 다음 세션 (S62) 할 일
- [ ] 다른 VN 씬에도 distort_if_burned 필드 확장 (Ch2 말렛 거래, Ch3+ 엘리아 관계 등)
- [ ] 탐색 맵의 DialogueManager에도 같은 왜곡 로직 적용 (현재 VN 전용)
- [ ] 여러 기억이 동시에 태워졌을 때 왜곡 누적 (색수차 농도 증가)
- [ ] 글리치 사운드 `memory_burn` SFX 확인·생성

---

## S61b — 2026-04-24 (VN UI 입력 간섭 버그 수정)

### 문제
타이틀 → New Game → 프롤로그 끝 → rim_forest 맵 → Elia NPC 상호작용 시 화면이 멈춤. ch1_elia_talk.jpg 풀스크린 CG만 표시되고 DialogueBox가 Space/클릭에 반응 안 함.

### 원인
goto_map 액션 후 VN UI(CanvasLayer 50)가 `queue_free`되지만 한 프레임 동안 살아있으면서 `_input` 핸들러가 mouse/space 이벤트를 선점. 탐색 맵의 DialogueBox(같은 layer 50)의 `_unhandled_input`이 이벤트를 받지 못해 대사 진행 불가.

### 수정
- `vn_scene.gd._input()` — SceneFlow.is_active가 false면 즉시 early return (비활성 VN이 입력 가로채기 방지)
- `scene_flow.gd._close_vn_ui()` — queue_free 전에 `visible=false` + `set_process_input(false)` + `set_process_unhandled_input(false)` 호출해 잔여 프레임 입력 완전 차단

---

## S62 — 2026-04-24 (Memory Constellation — 기억 성좌 UI)

### 목적
기억 보관 UI를 **정적 리스트 → 동적 네트워크**로 업그레이드. 기억들이 서로 연결된 별자리처럼 보이고, 하나를 태우면 연결된 기억들에 금이 감. 플레이어가 "이 기억을 잃으면 저 기억도 왜곡된다"는 무게를 시각적으로 인지.

### 완료
- **Memory 클래스 확장** (`memory_manager.gd`) — `connections: Array` 필드 + `_refresh_connections()` 자동 계산
  - 규칙 1: 같은 `related_npc` 끼리 모두 연결 (NPC 단위 서브그래프)
  - 규칙 2: 같은 id prefix(sense/daily/rel/identity/core)의 인접 기억 연결
  - 헬퍼: `find_memory(id)`, `burned_neighbor_count(id)`
- **MemoryConstellation 오토로드** (`scripts/ui/memory_constellation.gd`) — CanvasLayer 42
  - **동심원 배치**: GRADE_5(감각)가 최외곽 → GRADE_1(핵심)이 중심
  - **노드 렌더링**: 등급별 색, 맥동 애니메이션, 호버 시 확대+밝아짐
  - **연결선**: 공통 NPC 있으면 NPC 고유색, 없으면 옅은 회색. 둘 중 하나라도 태워지면 **점선+붉은 톤**으로 "끊어진" 시각화
  - **태워진 기억**: X 마크 + 어두운 링 (완전 소실) / 흐릿한 노드 + `~` (잔존)
  - **금 효과**: 이웃 태워진 수에 비례해 노드에 붉은 균열 선 1~3개
  - **툴팁**: 호버 시 제목/등급/상태/관련 NPC/설명/연소 시 효과 (RichText BBCode)
  - **범례**: 하단에 링·선·금·X 의미 설명
- **MemoryUI 토글 버튼** — 하단 바에 "✦ Constellation" 버튼 추가, 클릭 시 Constellation 오픈 (MemoryUI 자동 숨김→복귀)
- **오토로드 등록** — project.godot에 MemoryConstellation 추가

### 플레이어 경험
- Tab/M으로 Archive 열고 "Constellation" 클릭 → 전체 기억 네트워크 조망
- 한 기억을 태우면 다음에 성좌 열었을 때: 그 기억은 X 처리되고, **연결된 기억들 주변에 균열 선이 자동 생성**됨
- 엘리아 관련 기억 클러스터가 초록선으로 묶여 있는 걸 보면 "이 관계를 파괴하지 않으려면 이 쪽은 태우지 말아야" 판단 가능

---

## S63 — 2026-04-24 (Memory Leverage — 대화 중 기억을 연료로)

### 목적
기억 연소가 전투 스킬에만 묶여 있던 걸 **대화/협상/설득에도 사용**하는 자원으로 확장. 선택지가 "텍스트"뿐 아니라 "이 선택을 위해 이 기억을 태운다"는 거래가 되게.

### 완료
- **VNScene 선택지 업그레이드** (`vn_scene.gd._show_choices`)
  - `cost_memory: "memory_id"` 필드 인식 → 버튼 텍스트에 `✦ [선택]\n    [ Burn: 기억이름 ]` 형태로 표시
  - **시각적 구분**: cost_memory 선택지는 붉은 테두리 + 어두운 바탕 (일반 선택지는 금색). 호버 시 더 강렬한 붉은 톤.
  - **자동 비활성화**: 태울 기억이 이미 태워졌거나 존재하지 않으면 선택지 자체 제외
  - `requires_memory_intact`와 조합 가능 (예: 기억이 살아있어야 선택 가능)
- **SceneFlow 처리** (`scene_flow.gd.select_choice`) — `cost_memory` 필드는 `burn_memory`의 의미적 별칭으로 동일 연소 처리
- **샘플 선택지 삽입**
  - `ch1_prologue.json`: 재비 속 "The song for passage." → cost_memory: daily_campfire_song
  - `ch2_market_arrival.json`: 뷰로 가드 앞에서 3선택지 — 거짓말 / 뇌물로 기억(daily_market_food) 태우기 / 검술 기억 있으면 돌파

### 플레이어 경험
- 선택지 창을 열었을 때 "그냥 선택" 과 "기억을 대가로 얻는 선택"이 시각적으로 명확히 구분
- 붉은 선택지는 매번 "이 기억을 정말 태울 것인가" 질문하게 만듦 (아이템을 쓰는 게 아니라 **자신의 일부를 태우는 거래**)
- Constellation UI와 연동 — 대화에서 기억을 태우면 성좌에서 즉시 X 표시 + 연결된 기억들 균열

---

## S64 — 2026-04-24 (Perception Drift — 세계가 기억에 따라 달라진다)

### 목적
기억 태움이 "내면"뿐 아니라 "외부 세계"에도 영향을 주게. 특정 기억을 태우면 NPC·오브젝트가 다르게 보이거나 사라지거나 나타남. 물리적 세계가 플레이어의 기억 상태에 따라 재구성됨.

### 완료
- **PerceptionFilter 유틸** (`scripts/systems/perception_filter.gd`) — `class_name PerceptionFilter`
  - **정적 메서드** `PerceptionFilter.apply(scene)` — 맵 _ready 말미에 호출
  - **메타 기반 필터**: 노드에 `set_meta("requires_memory_intact", "id")` 또는 `"requires_memory_burned"` 설정 → 자동 visible/collision 제어
  - **그룹 기반 필터**: `perception_intact_<id>` / `perception_burned_<id>` 그룹에 속한 노드들 일괄 처리
  - **NPC 대화 교체**: `burned_dialogue_<memory_id>` 메타로 기억 태움 시 dialogue_key 교체
  - **틴트 효과**: `on_burned_tint_memory` + `on_burned_tint` 메타로 modulate 자동 적용
  - 숨긴 CollisionObject2D는 layer/mask 0으로 리셋해 통과 가능
- **rim_forest 시범 적용** (`_setup_perception_nodes`)
  - **Song Echo**: `daily_campfire_song`을 태운 플레이어에게만 보이는 따뜻한 빛 + 부유 파티클 (캠프 근처). 다가가면 "A faint warmth. A song you no longer know." 토스트
  - **엘리아 창백 틴트**: 노래 태움 시 엘리아 스프라이트 modulate가 차가운 색(0.75, 0.8, 0.85)으로 자동 변경
  - PerceptionFilter.apply(self)를 rim_forest _ready 끝에서 호출

### 플레이어 경험
- 노래를 태우고 맵을 다시 걸으면: 전에 없던 **따뜻한 잔향 불빛**이 캠프 주변에 피어남. 그 빛은 "이 기억을 가진 다른 버전의 당신"의 흔적
- 엘리아가 **살짝 창백하게** 보임. 게임이 직접 "엘리아가 변했다"고 말하지 않지만, 플레이어는 느낄 수 있음
- 다른 맵·다른 챕터에도 `set_meta` 한 줄로 조건부 오브젝트 추가 가능 (확장 비용 낮음)

---

### 수정/신규 파일 (S62/S63/S64 합산)
| 파일 | 변경 |
|------|------|
| `scripts/systems/memory_manager.gd` | connections 필드, _refresh_connections, find_memory, burned_neighbor_count |
| `scripts/ui/memory_constellation.gd` | **신규** — Constellation UI 오토로드 |
| `scripts/ui/memory_ui.gd` | 하단 바에 Constellation 토글 버튼 |
| `scripts/ui/vn_scene.gd` | cost_memory 선택지 UI (붉은 테두리, 라벨 표시, 자동 비활성화) |
| `scripts/systems/scene_flow.gd` | select_choice에 cost_memory 연소 처리 |
| `scripts/systems/perception_filter.gd` | **신규** — 메타/그룹 기반 기억 상태 필터 유틸 |
| `data/vn_scenes/ch1_prologue.json` | 재비 속 선택을 cost_memory 형식으로 |
| `data/vn_scenes/ch2_market_arrival.json` | 뷰로 가드 3선택지 (leverage 예시) |
| `scenes/maps/rim_forest.gd` | Song Echo 파티클/라이트 + 엘리아 틴트 + PerceptionFilter.apply 호출 |
| `project.godot` | MemoryConstellation 오토로드 등록 |

### 다음 세션 (S65) 할 일
- [ ] Constellation 클릭 시 기억 상세 창(설명·연소 효과) 모달 팝업
- [ ] 다른 맵(verdan_market 등)에도 PerceptionFilter 적용 + 2~3개 조건부 NPC/오브젝트
- [ ] DialogueManager(탐색 맵 대화) 선택지에도 cost_memory UI 동기화
- [ ] Constellation에 "연결 흐름" 애니메이션 (선이 흘러가는 느낌)
- [ ] 핵심 기억(Grade 1) 태울 때 전체 성좌가 재편되는 컷씬

---

## S65 — 2026-04-24 (A안 피벗 시작 — The Cut)

### 결정
Steam 흥행을 위해 **Story-VN with Mechanics** (A안) 방향으로 피벗. LISA·OneShot·OMORI처럼 **하나의 본질에 집중**. 자산 비율(CG 130/포트레이트 49/대화 1400줄)은 RPG보다 VN에 가깝고, 솔로 개발 효율 + 차별점(메모리 메카닉 VN) 모두 A안이 유리.

### 이번 세션: The Cut (범위 축소, 코드 보존)

**1. 타이틀 화면 정리** (`scenes/main/main.gd`)
- NG+ 버튼 노출 제거 (조건부 출력 코드 삭제, 콜백은 보존)
- Boss Rush 버튼 노출 제거 (동일)
- 서브타이틀 변경: `"The Price of Oblivion"` → `"The Price of Oblivion  ·  A story of what you choose to forget"`
  - VN 카피 한 줄로 게임 본질 전달, 스토어 페이지 hero copy로도 사용 가능

**2. PauseMenu 슬림화** (`scripts/ui/pause_menu.gd`)
- **숨김**: Fast Travel (RPG 워프), Stats (통계 화면), Load Autosave (Load와 중복)
- **유지**: Resume, Journal, Codex, Achievements (Steam 기대치), Endings, Options, Save, Load, Title, Quit
- 코드는 모두 보존 — UI 진입점만 차단

### 챕터 4압축 설계 (S66+에서 구현)

기존 10챕터(Rim → Belt → Drift → Coast → Seam → Forest → Waste → Seal → Epilogue)를 **4막 구조**로 재편:

| 신규 | 기존 매핑 | 핵심 비트 | 길이 목표 |
|------|----------|---------|----------|
| **Act I — Ash** (Rim Forest) | Ch1 그대로 | 첫 연소·재비·엘리아·캠프 / **첫 보스: Void Beast** | 30분 |
| **Act II — Bargain** (Verdan Market) | Ch2 + Ch3 토비아스 압축 | 말렛 거래·뷰로 가드·세계관 노출 / **보스 없음** (대화 압박 클라이맥스) | 45분 |
| **Act III — Echo** (Seam Outskirts → Forest 압축) | Ch3·Ch4·Ch5·Ch7·Ch8 핵심 장면 | 세이블 진실·기억 기생 숲·환각 / **두 번째 보스: Memory Wraith** | 60분 |
| **Act IV — Origin** (BL-07 Void) | Ch9·Ch10 압축 + 6엔딩 | 카이로스 대면·Seal 결정 / **마지막 보스: Kairos** + 엔딩 분기 | 45분 |

**총 플레이타임: 3시간** (현 10챕터 8시간 → 압축. Steam 짧고 강한 VN 트렌드).
**보스 3전만 유지**: Void Beast / Memory Wraith / Kairos. 나머지 잡몹·랜덤 인카운터 비활성화.

### 삭제 후보 시스템 목록 (S66~S68에서 단계적 비활성화)

코드는 보존, UI/접근만 차단:
- 랜덤 인카운터 (`RandomEncounter`)
- 사이드 퀘스트 (`SideQuest`, 6종)
- 장비 시스템 + 강화
- 크래프팅 (기억 합성은 유지 — 본질 메카닉)
- 콤보 시스템 / Limit Break
- 자동 전투
- 보스 러시
- NG+
- 통계 화면
- 미니맵 (선형 진행이라 불필요)
- 파티 시스템 (세이블 동행 → VN 동행자로만, 전투 동참 없음)

### 유지·강화 시스템

VN 본질에 직결되는 것만 살림:
- 기억 연소 + Constellation (S62) + Leverage (S63) + Perception Drift (S64)
- Memory Distortion 왜곡 (S61)
- VN 씬 흐름 (SceneFlow)
- Codex - Memory Archive
- 6 엔딩 분기
- 다국어 (en/ko)
- 업적 (28종 → 스토리 중심으로 재선별)

### 수정 파일
| 파일 | 변경 |
|------|------|
| `scenes/main/main.gd` | NG+/Boss Rush 버튼 노출 코드 제거, 서브타이틀 변경 |
| `scripts/ui/pause_menu.gd` | Fast Travel/Stats/Load Autosave 메뉴 숨김 |

### 다음 세션 (S66) 할 일 — 옵션 2 채택
- Ch1 (Act I — Ash) 데모 빌드 완성

---

## S66 — 2026-04-24 (Act I — Ash 데모 빌드)

### 목표
첫 30분을 흠 없이 갈고 닦은 **Steam Next Fest 출시 가능한 데모**. rim_forest를 단일 맵으로 정리하고, 보스(Void Beast) 1전 + 핵심 대화 3개 + 캠프 + VN 후일담만 남김.

### 완료

**1. rim_forest 부수 시스템 비활성화** (코드 보존, 호출만 차단)
- `_setup_random_encounters()` 호출 제거 — 잡몹 인카운터 없음
- `_setup_side_quests()` 호출 제거 — 사이드 분기 없음
- `Minimap.update_minimap()` 비활성 — 선형 VN 진행 강조
- `RandomEncounter.update()` 비활성

**2. 핵심 트리거만 남기고 잡요소 제거**
- 히든 이벤트: 6개 → **3개 (그루터기 / 기억 사당 / 엘리아 기억 대화)**
  - 제거: dead_burner(잡 분위기), forest_walk(중복), anchor_talk(중복), MemoryResonance(미니게임)
- 전투 트리거: 2개 → **1개 (Void Beast 보스만)**
  - 제거: Ash Crawler 잡몹 — Act I는 클라이맥스 한 번만

**3. Void Beast 필수화**
- 캠프 트리거에 `ch1_void_beast_defeated` 플래그 체크 추가
- 미처치 시 토스트: *"Something blocks the path. Find what hunts these woods."*
- BattleManager.battle_ended 시그널 연결 — VICTORY 시 자동 플래그 설정

**4. 데모 종료 화면** (`scripts/ui/demo_end.gd` + `scenes/ui/demo_end.tscn`)
- ch1_after_forest VN 마지막 action을 `goto_scene: ch2_market_arrival` → **`demo_end`** 로 변경
- SceneFlow에 `demo_end` 액션 처리 추가 — `res://scenes/ui/demo_end.tscn` 로드
- 화면 구성:
  - 배경: Cover2.png + 어두운 비네트
  - 타이틀: **"Act I — Ash"** (큰 황금색)
  - 부제: **"— End of Demo —"**
  - 본문: 감사 메시지 + 풀버전 티저 (벨트, 시임, 형제, 결정)
  - 통계: *"You burned X memories. Y remain as residue."* (플레이어의 기억 선택 기록)
  - CTA 3버튼: **✦ Wishlist on Steam** (외부 링크) / **Return to Title** / **Quit**
- 순차 페이드인 애니메이션 (각 요소 0.45s 간격)

### Act I 플레이 흐름 (검증)
1. 타이틀 → New Game
2. VN 프롤로그 (ch1_prologue): 첫 연소 묘사 → 엘리아 등장 → 재비 → **연소 선택지 (cost_memory: daily_campfire_song)** → 캠프 밤 → 글리치 VFX (선택 시)
3. rim_forest: 자유 탐색 (3 히든 이벤트 + 엘리아 동행 + Memory UI/Constellation 접근)
4. **Void Beast 보스전** (필수)
5. 캠프 트리거 (남쪽) — VN으로 복귀
6. ch1_after_forest VN: 그린 트리 + 뷰로 타워 시야
7. **Demo End 화면** — 위시리스트 CTA

### 수정 파일
| 파일 | 변경 |
|------|------|
| `scenes/maps/rim_forest.gd` | 인카운터/사이드퀘스트/미니맵 호출 제거, 히든 이벤트 3개로 축소, 잡몹 전투 제거, 보스 필수 게이트 추가, battle_ended 시그널 연결 |
| `data/vn_scenes/ch1_after_forest.json` | 마지막 action을 `goto_scene: ch2_market_arrival` → `demo_end`로 |
| `scripts/systems/scene_flow.gd` | `demo_end` 액션 핸들러 추가 |
| `scripts/ui/demo_end.gd` | **신규** — 데모 종료 화면 |
| `scenes/ui/demo_end.tscn` | **신규** — 데모 종료 씬 |

### Steam Next Fest 빌드 체크리스트 (S67에서 마무리)
- [ ] Steam URL 실제 앱 ID로 교체 (`STEAM_URL` 상수)
- [ ] 데모 종료 화면 BGM 트랙 결정
- [ ] Ch1 30분 풀 플레이 검증 (실시간 측정)
- [ ] 한국어 로케일 점검 (대화·UI)
- [ ] 시작 옵션에서 Steam achievement 등록 확인 (Codex/Achievement 시스템)

### 다음 세션 (S67) 할 일
- [x] Windows export 빌드 시도 (export_presets 정리 + 헤드리스 export 검증)
- [x] Steam 상점 페이지 카피 / 태그 / 트레일러 콘티 / 스크린샷 세트 (STEAM_PAGE.md)

---

## S67 — 2026-04-24 (Windows 빌드 + Steam 상점 키트)

### 목적
S66에서 만든 Act I 데모를 실제 zip으로 배포 가능한 빌드로 굳히고, Steam 상점 페이지에 바로 붙일 마케팅 자료(카피/태그/트레일러 콘티/스크린샷 가이드)를 정리.

### 완료

**1. export_presets.cfg 완전 재작성**
- Windows Desktop (Demo) 프리셋 정의
- application 메타: 회사명 `MEMORIA Studio`, 제품명 `MEMORIA - The Price of Oblivion (Demo)`, 파일/제품 버전 0.9.0.0, 저작권 © 2026
- export_path: `build/MEMORIA-Demo-v0.1.exe`
- exclude_filter: `SESSION_LOG.md, CLAUDE.md, *.tmp, .git/*, .gitignore`
- x86_64 아키텍처, embed_pck=false (별도 .pck 파일)

**2. 헤드리스 export 시도 + 진단**
- 명령: `Godot_v4.6.2-stable_win64_console.exe --headless --export-release "Windows Desktop (Demo)" "build/MEMORIA-Demo-v0.1.exe"`
- **결과: 템플릿 미설치로 실패** — 사용자가 Godot Editor에서 직접 설치해야 함
  - 경로: `C:/Users/jc/AppData/Roaming/Godot/export_templates/4.6.2.stable/` 가 비어있음
  - **해결법: Godot Editor → Editor 메뉴 → Manage Export Templates → Download (~600MB)**
- VFX Library 플러그인 종료 시 autoload/VFX, autoload/EnvVFX 미존재 경고 — 비치명적, 게임 실행에는 무관

**3. Steam 상점 페이지 키트** (`STEAM_PAGE.md` 신규)
- **게임 이름**: MEMORIA: The Price of Oblivion
- **태그라인** (한/영): 기억을 태워 싸우는 다크 판타지 2D 어드벤처 — 잊는 만큼 세계가 바뀐다
- **짧은 설명** (한 217자 / 영 293자): Steam 검색 결과 노출용
- **About this game** (한/영 풀텍스트): 5개 핵심 메카닉 강조 — 기억 연소·대화 거래·세계 재작성·Constellation·6엔딩
- **Steam 태그 15개 우선순위** — Story Rich / Choices Matter / Dark Fantasy / RPG / 2D 핵심 5
- **30초 트레일러 콘티** (8컷, 시간 매핑·자막·캡처 소스): VN 씬→선택지→글리치→성좌→Perception→보스전→타이틀
- **스크린샷 6장 가이드**: 메인 1 + 보조 5 (각각 의도된 메시지 명시)
- **캡슐 이미지 6종 사양**: Main/Small/Header/Library Capsule + Library Hero + Logo
- **출시 전략 노트**: Wishlist 빌드업, 가격대($9.99~14.99), 출시 윈도우 회피, itch.io 동시 배포

### 수정/신규 파일
| 파일 | 변경 |
|------|------|
| `export_presets.cfg` | Windows Demo 프리셋 완전 정의 |
| `STEAM_PAGE.md` | **신규** — Steam 상점 페이지 자료 일체 |

### 사용자 액션 아이템 (수동 작업 필요)
1. **Export Templates 설치** — Godot Editor → Editor → Manage Export Templates → Download
2. (설치 후) 헤드리스 빌드 재시도 또는 Editor → Project → Export → "Export Project" 클릭
3. **Steam 앱 ID 발급** (Steamworks 가입 후) → `demo_end.gd`의 `STEAM_URL` 상수 교체
4. **캡슐 이미지 디자인** — Photoshop/Affinity 등으로 Cover2.png 베이스로 6종 제작
5. **트레일러 캡처** — 빌드 성공 후 OBS로 STEAM_PAGE.md의 8컷 따라 녹화

### 다음 세션 (S68) 후보
- [x] (사용자 빌드 성공 후) 실제 zip 패키징 워크플로우 정리 — S68에서 인프라 미리 준비
- [ ] 한/영 자막 검수 — Ch1 VN 씬 + 탐색 대화 전체 톤 정리
- [ ] 데모 BGM/SFX 누락 점검 (특히 글리치 사운드 `memory_burn` 파일 존재 여부)
- [ ] verdan_market에 PerceptionFilter 적용 — 풀버전 Act II 준비

---

## S68 — 2026-04-24 (빌드 검증 + 테스터 패키지 준비)

### 목적
S67에서 export 인프라를 깔았지만 사용자가 templates를 직접 설치해야 빌드가 굴러감. 그 사이 제가 할 수 있는 것: **빌드 전 위생 검사**, **단계별 가이드**, **테스터 피드백 양식**, **자동 패키징 스크립트** 준비. 빌드 성공 직후 5분 안에 친구한테 zip 보낼 수 있게.

### 완료

**1. 빌드 전 위생 검사 (CLI)**
- Godot 헤드리스 `--check-only --quit` 실행 → **GDScript 파싱 에러 0건, 미정의 참조 0건**. "data.tree is null" 경고는 헤드리스 모드 정상 잡음 (씬 트리 없는 상태에서 스크립트가 트리 접근 시도)
- VN JSON에서 참조하는 **CG 14개 / 포트레이트 13개 모두 존재** 확인
- 새로 짠 5개 스크립트(vn_scene/scene_flow/demo_end/memory_constellation/perception_filter) export 시 빌드 깨뜨릴 위험 없음

**2. BUILD_GUIDE.md** — 5단계 빌드 가이드
- STEP 1: Export Templates 설치 (Editor → Manage Export Templates → Download, ~600MB)
- STEP 2: 빌드 실행 (Editor GUI 또는 CLI)
- STEP 3: 빌드 결과 확인 (.exe + .pck + .console.exe)
- STEP 4: zip 패키징 (`./package_demo.sh`)
- STEP 5: 친구한테 보내기 (WeTransfer / Google Drive / itch.io 비공개 추천)
- 트러블슈팅 4건 (흰 화면, 템플릿 없음, 한국어 깨짐, Defender 차단) + 코드 서명 미적용 사실 명시

**3. TESTER_GUIDE.md** — 30분 플레이 후 답하는 피드백 양식
- 5섹션 구조: 첫인상 / 스토리 / **핵심 메카닉 (가장 비중)** / 조작감·버그 / 종합
- 핵심 질문:
  - 재 장면 cost_memory 선택지 — 뭘 골랐고 *왜* 골랐는지
  - 선택 *이후* 변화를 어디서 느꼈는지 (Perception Drift 동작 검증)
  - Constellation 한 번이라도 눌러봤는지 (UI 발견 가능성 검증)
  - Void Beast 보스전 난이도 + 의도 명확성
- 종합: 다음 챕터 유료 구매 의향 / 친구 추천 / 흥행 점수 10점 만점
- 조작 키 매핑 표 + 시작/종료 지점 명시 (사용자 혼란 방지)

**4. package_demo.sh** — 빌드 → zip 자동화 (bash, Git Bash 호환)
- 9단계 파이프라인:
  1. 빌드 산출물 존재 검증 (없으면 친절한 에러 + BUILD_GUIDE.md 참조)
  2. 스테이징 디렉터리 생성 (`build/stage/`)
  3. exe + pck 복사
  4. console.exe는 `build/debug/`로 별도 보관 (zip 부피 감소)
  5. README.txt 자동 생성 (실행법 + 조작 + 피드백 안내)
  6. TESTER_GUIDE.md 복사
  7. zip 생성 (zip 또는 7z 자동 감지)
  8. 결과 출력 (파일 경로 + 사이즈 + 다음 단계 가이드)
  9. 스테이징 폴더 정리
- 최종 산출물: `build/MEMORIA-Demo-v0.1-Windows.zip` (예상 150~300MB)

### 신규 파일
| 파일 | 용도 |
|------|------|
| `BUILD_GUIDE.md` | 사용자가 빌드부터 zip까지 따라 할 단계별 가이드 |
| `TESTER_GUIDE.md` | 친구한테 zip과 함께 보낼 피드백 양식 |
| `package_demo.sh` | 빌드 산출물을 친구 발송 가능 zip으로 자동 패키징 |

### 사용자 액션 아이템 (S68 마무리용)
1. **Godot Editor 열기 → Editor → Manage Export Templates → Download** (한 번만, ~600MB)
2. **Project → Export → "Windows Desktop (Demo)" → Export Project**
3. 본인 PC에서 `build/MEMORIA-Demo-v0.1.exe` 더블클릭 → 5분 동작 확인
4. Git Bash 또는 WSL에서 `./package_demo.sh` 실행
5. 생성된 zip을 WeTransfer/Drive/itch.io에 업로드
6. 친구 1~3명에게 링크 + TESTER_GUIDE 안내 전송

### 다음 세션 (S69) 후보
- [x] (S69에서) VN 시각 폴리싱 — Ken Burns / 선택지 덤 / 연소 잔열 / 필름 그레인
- [ ] **테스터 피드백 1차 수집 후 분석 + 우선순위 버그/UX 패치**
- [ ] 비주얼 스타일 결정 (AI 일러스트 단일화 vs 픽셀 유지)
- [ ] 챕터별 시그니처 BGM 1트랙 (Suno/Udio)
- [ ] 한/영 자막 검수 (Ch1 VN 전체 톤 정리)
- [ ] Steamworks 가입 + 앱 ID 발급 (사용자 액션, 1주~2주)

---

## S69 — 2026-04-24 (VN 시각 폴리싱 — 작은 4개로 큰 차이)

### 목적
빌드 인프라는 끝났고 테스터 회수 전 마지막으로 시각적 매력을 끌어올리기. 코드 변경 최소·임팩트 최대를 노린 4종.

### 완료

**1. CG Ken Burns** (`vn_scene.gd._start_ken_burns`)
- CG가 표시될 때마다 9~13초에 걸쳐 1.0 → 1.05 줌 + ±18px 팬 (sine ease in/out)
- 정적 일러스트가 살아 움직이는 느낌. 무의식적 시네마틱 효과.
- 매 CG마다 랜덤 팬 방향 → 같은 컷 봐도 매번 다르게 느낌

**2. 선택지 등장 시 배경 덤** (`vn_scene.gd._dim_background_for_choice`)
- `_show_choices` 호출 시 CG modulate를 0.6/0.6/0.65, 포트레이트는 0.5/0.5/0.55로 0.45s 페이드
- `_on_choice_selected` 시 1.0/1.0/1.0/1.0으로 복귀
- 효과: 결정의 무게가 시각적으로 강조 — 선택지가 덮인 공간이 화면 중심이 됨
- cost_memory 붉은 테두리와 합쳐지면 더 무겁게 읽힘

**3. 기억 연소 잔열 비네트** (`vn_scene.gd._build_glitch_layer` 확장)
- `_ember_vignette` TextureRect — GradientTexture2D radial fill (가장자리만 따뜻한 오렌지/붉은색)
- `_play_burn_glitch`에 트윈 추가:
  - 0~0.5s: 알파 0 → 0.85 (잔열 차오름)
  - 0.5~1.7s: 1.2s 유지 (인지 가능한 시간)
  - 1.7~5.2s: 알파 0.85 → 0 (3.5s 천천히 식음)
- 글리치 직후 화면 가장자리만 타고 난 듯한 연한 빛 → 플레이어가 "방금 뭔가 잃었다"는 정서적 잔여를 시각으로 받음

**4. 필름 그레인 셰이더** (`vn_scene.gd._build_glitch_layer` 확장)
- 풀스크린 ColorRect + 인라인 GLSL 셰이더 (`hash21` 노이즈, 시간으로 패턴 갱신)
- 강도 0.045 (매우 미묘 — 의식적으로 눈에 띄진 않지만 무의식엔 영향)
- 16 FPS로 노이즈 패턴 갱신 (영화 필름 그레인 느낌)
- `u_time` 셰이더 파라미터를 `_process`에서 매 프레임 업데이트
- 효과: AI 일러스트의 평면적 매끈함이 사라지고 "프레임"이라는 감각 + 시간성

### 수정 파일
| 파일 | 변경 줄 (대략) |
|------|------|
| `scripts/ui/vn_scene.gd` | +90줄 (Ken Burns / dim / ember / grain shader / process 시간) |

### 검증
- Godot 헤드리스 `--check-only` 통과 (GDScript 파싱 + 셰이더 컴파일 에러 0건)

### 누적 효과 (테스터가 무의식적으로 느낄 것)
- VN이 "정지된 일러스트북"에서 **"움직이는 영화"**로 격상
- 선택지가 단순 UI가 아니라 **분기점**으로 인지됨
- 기억 연소가 **"정보"**가 아니라 **"감각적 손실"**로 바뀜
- 전체 톤이 AI 생성 매끈함에서 **시네마틱 그레이드**로 이동

### 다음 세션 (S70) 후보 — S69까지로 데모 폴리싱 마무리, 테스터 회수 대기
- [ ] (테스터 1~3명 회수 후) 피드백 우선순위 분석
- [ ] 비주얼 스타일 결정 (현재 AI 포트레이트 + 필름 그레인 + Ken Burns 조합 평가 후 통일 여부)
- [ ] 챕터별 시그니처 BGM 1트랙 (Suno/Udio, ~1주)
- [ ] Steamworks 가입 (사용자 액션)

---

## S70 — 2026-04-24 (Full VN 전환 — 환세취호전 미학 탈출)

### 결정 배경
사용자 정직한 자기 진단: *"환세취호전 같은 느낌이야"*. 코드로 그린 타일 맵·사각형 캐릭터가 AI 일러스트와 미학적 충돌. 약점을 폴리싱하는 대신 **삭제**하기로. RPG → Visual Novel 장르 본격 전환.

### 완료

**1. 탐색 맵 진입 차단 — 흐름 재배선**
- `ch1_prologue.json` 마지막 action: `goto_map: rim_forest` → **`goto_scene: ch1_forest_walk`**
- 새 챕터 1 흐름:
  ```
  타이틀 → ch1_prologue (재비, 캠프 밤)
       ↓
  ch1_forest_walk (숲 수색 — 메뉴 형식 VN)
       ↓
  ch1_void_beast (보스전 — VN 스타일)
       ↓
  ch1_after_forest (그린 트리, 뷰로 타워)
       ↓
  demo_end
  ```
- rim_forest.tscn 자체는 코드 보존 — 호출만 끊김

**2. ch1_forest_walk.json — 메뉴식 VN 수색**
- 클리어링 도착 → 4개 선택지로 어디 갈지 결정:
  - **The carved stump** (A.E. 각인, 정체성 비트)
  - **The forest shrine** (버너의 무덤, 세계관 노출)
  - **Talk with Elia** (관계 비트, distort_if_burned 포함)
  - **Push deeper** (요구: 최소 1곳 방문 후 활성화)
- 각 방문 후 자동으로 메뉴 복귀 (`goto_scene: ch1_forest_walk, start_index: 5`)
- `requires_not_flag`로 이미 본 곳은 메뉴에서 제거
- 약 3~5분 소요. 플레이어가 *읽고 싶은 만큼* 읽고 진행

**3. ch1_void_beast.json — VN 스타일 보스전**
- 턴제 BattleManager 미사용. 순수 VN 메카닉.
- **2 라운드 + 종결** 구조:
  - Round 1: 3선택지 (검 / 기억 태움 [sense_warm_light] / 패턴 읽기)
  - Round 2: 3선택지 (속도 / 기억 태움 [sense_forest_smell] / 읽은 패턴 활용 — Round 1에서 vb_read 플래그 셋했어야 노출)
  - Endgame: 마무리 + 엘리아의 무게감 있는 한 줄 ("...You burned for it.")
- 기억 태움이 **단순 능력**이 아니라 *서사 무게*로 표현 — "그 선택의 대가가 어떻게 드러나는가"가 텍스트로 명시
- 종료 시 `ch1_void_beast_defeated` 플래그 자동 셋 → ch1_after_forest 진입

**4. SceneFlow 확장: 선택지 조건 게이팅** (`vn_scene.gd._show_choices`)
- `requires_flag: "flag_id"` — 해당 플래그가 set된 경우만 노출
- `requires_not_flag: "flag_id"` — 해당 플래그가 *미*set인 경우만 노출
- 기존 `requires_memory_intact` / `cost_memory`와 결합 가능
- 메뉴식 VN 진행에 필수 (이미 방문한 곳 자동 제거 등)

**5. 장르 표시 변경** (`STEAM_PAGE.md`)
- 우선순위 태그 1~3 변경:
  - 이전: Story Rich / Choices Matter / Dark Fantasy / **RPG** / 2D
  - 현재: **Visual Novel** / Story Rich / Choices Matter / Dark Fantasy / Psychological
- Steam 알고리즘이 VN 카테고리로 분류하도록

### 검증
- Godot 헤드리스 `--check-only` 통과
- JSON 5개 파일 (prologue/forest_walk/void_beast/after_forest/market_arrival) 전부 유효

### 수정/신규 파일
| 파일 | 변경 |
|------|------|
| `data/vn_scenes/ch1_forest_walk.json` | **신규** — 메뉴식 숲 수색 (49 steps) |
| `data/vn_scenes/ch1_void_beast.json` | **신규** — VN 보스전 (51 steps) |
| `data/vn_scenes/ch1_prologue.json` | 마지막 action을 ch1_forest_walk로 |
| `scripts/ui/vn_scene.gd` | requires_flag / requires_not_flag 선택지 게이팅 |
| `STEAM_PAGE.md` | 태그 우선순위 — Visual Novel 최상단 |

### 사라진 것
- 탐색 맵 진입 (rim_forest 코드는 살아있지만 흐름에서 끊김)
- 턴제 BattleManager 호출 (Ch1 한정)
- ExplorationHUD / Minimap 노출 (EXPLORATION 상태 안 들어가니 자동 미표시)
- 코드 픽셀 스프라이트 노출
- 자동 생성 타일 맵 노출

### 남은 강점만 살아남음
- AI CG 130장 + 포트레이트 49장
- 기억 연소 메카닉 (Constellation / Leverage / Distortion / Perception Drift)
- 분기형 텍스트
- VN 시각 폴리싱 (Ken Burns / 선택지 덤 / 잔열 / 필름 그레인)

### 플레이어 경험 변화
- 이전: VN 프롤로그 → 탑다운 맵 탐색 (**약점**) → 턴제 전투 (**약점**) → VN 후일담
- 현재: VN 프롤로그 → VN 수색 (메뉴식) → VN 보스전 (선택지식) → VN 후일담
- **전체가 시네마틱 일관 흐름**. 환세취호전 미학 → House in Fata Morgana / VA-11 Hall-A 미학.

### 다음 세션 (S71) 후보
- [ ] rim_forest.tscn 등 더 이상 안 쓰는 맵 파일 정리 (보존 vs 삭제 결정)
- [ ] BattleManager·ExplorationHUD·Minimap 코드도 데모 빌드에서 제외
- [ ] Ch1 전체 풀 플레이 검증 (실제 플레이 시간 측정)
- [ ] BGM Ch1 시그니처 1트랙 (Suno/Udio) — VN 분위기 강화
- [ ] (사용자) Godot Editor 빌드 → 친구 테스트

---

## S73 — 2026-04-24 (Stuck 버그 수정 + 책 페이지 넘김)

### 친구 1차 피드백
1. *"어느 시점에서 진행이 잘 안된다"* → 진행 멈춤 버그
2. *"대화 넘길 때 책처럼 넘겨지는 그래픽이 있으면 좋겠다"* → VN 페이지 턴 효과 요청

### 1. ch1_void_beast 무한루프 버그 (중대)

**증상**: 보스전 Round 1에서 어떤 선택을 해도 결국 끝나지 않고 같은 텍스트가 반복.

**원인**: JSON step 인덱스 잘못 매핑.
- Round 1 choice gotos: 7/14/21 → 잘못 (각 라우트가 CG/intro narrate를 건너뜀)
- **치명적 루프**: 모든 Round 1 라우트의 출구 `goto_scene start_index 23` → step 23은 read 라우트 *중간* ("When the low strike came..."). 모든 라우트가 거기 떨어져 → step 24~25 진행 → step 26 = `goto_scene start_index 23` → 다시 step 23 → **무한루프**
- Round 2 choice gotos도 28/35/42 잘못. 42는 action 스텝 자체였음.

**수정**:
- Round 1 choice goto: `[6, 13, 20]` (각 라우트의 CG/intro)
- Round 1 exit start_index: `28` (Round 2 CG)
- Round 2 choice goto: `[32, 37, 43]`
- Round 2 exit start_index: `50` (엔딩 CG)
- 8군데 모두 수정. 이제 모든 라우트가 정상 종료 → ch1_after_forest 진입.

### 2. 책 페이지 넘김 효과 (`vn_scene.gd._play_page_turn`)
- 새 라인 표시 직전 종이 엣지 그라디언트 좌→우 0.32s 스윕
- 그라디언트 5단계: 어두운 그림자 → 밝은 페이지 엣지(0.85) → 살짝 어두운 종이 → 투명
- 회전 -2° → +1.5° 동안 변화 (종이가 휘는 인상)
- 알파: 0 → 0.85(0.12s) → 0(0.20s)
- SFX `page_turn` 트리거 (오디오 매니저에 트랙 있으면 재생)
- 연속된 같은 텍스트는 발동 안 함 (`_last_displayed_text` 체크)
- 위치: 화면 하단 250px 영역에 배치 — 텍스트 박스 영역 위로 스윕

### 검증
- void_beast 인덱스 매핑: Python으로 모든 step 덤프해서 검증 완료
- Godot 헤드리스 `--check-only` 통과

### 수정 파일
| 파일 | 변경 |
|------|------|
| `data/vn_scenes/ch1_void_beast.json` | 8개 인덱스 재매핑 |
| `scripts/ui/vn_scene.gd` | 페이지 턴 오버레이 + `_play_page_turn` (~50줄) |

### 친구한테 다시 보낼 때 기대 효과
- 진행 멈춤 사라짐 → 보스전 끝까지 완주 가능
- 대사 한 줄 넘길 때마다 미세하게 종이 한 장 넘기는 인상 → VN 정체성 강화


---

## S77 — 2026-05-19 (game image asset intake and first integration)

### Done
- Imported 36 PNG files from `../이미지/game image/` into the Godot project.
- Runtime-ready illustrations copied to `assets/cg/game_image/`:
  - character full-body CG: Arrel, Elia, Tobias, Nera, Kairos, Veil
  - environment CG: frost city, memory hall, wasteland city, void cathedral, Bureau spires, frozen archive
  - dramatic Arrel ruins/rest CG
- Reference sheets copied to `assets/game_image/reference/`:
  - turnaround sheets
  - expression sheets
  - sprite-sheet references
  - UI and skill icon reference sheets
- Added `assets/game_image/README.md` explaining asset source, runtime CG, and reference-sheet usage.
- Title slideshow now includes the new high-resolution environment and Arrel CGs.
- Ch1 VN scene integration:
  - `ch1_prologue.json`: opening Arrel beat now uses `arrel_fullbody.png`.
  - `ch1_void_beast.json`: post-battle beat now uses `arrel_ruins_rest.png`.
  - `ch1_after_forest.json`: forest exit and Bureau reveal now use `env_wasteland_city.png` and `env_bureau_spires.png`.

### Verification
- VN JSON parse check passed.
- Godot `--check-only` produced only the existing `data.tree is null` plugin noise; no new parse errors or missing-resource script errors.

### Notes
- The new sheets are not sliced into animation frames yet. They are preserved as reference assets because using full reference sheets directly in gameplay would look like concept-art UI, not an in-world scene.
- Next useful step: build an in-game Artbook / Character Dossier screen that displays the turnaround and expression sheets intentionally.

---

## S78 — 2026-05-19 (Artbook / Character Dossier)

### Done
- Added an Artbook entry to the pause menu.
- Added a full-screen Artbook / Character Dossier panel inside `scripts/ui/pause_menu.gd`.
- Artbook displays selected `game image` reference sheets and CG plates:
  - character turnaround sheets
  - expression sheets
  - skill icon atlas reference
  - environment plates
  - Arrel ruins illustration
- Added preview title, asset type label, image preview, and short art-direction notes for each entry.
- Since the current build is mostly VN-driven, PauseMenu can now open during active `SceneFlow` dialogue as well as normal exploration. This makes Artbook / Options / Save accessible during the VN demo flow.

### Verification
- All Artbook `res://` paths exist.
- Godot `--check-only` reports no new parse/missing-script errors; only existing plugin/resource shutdown noise remains.

### Notes
- This intentionally treats the new sheets as in-game dossier/reference material rather than slicing them into gameplay animations immediately.
- Next polish step: add unlock conditions per character/act, or add a title-screen Artbook button for browsing without starting the demo.

---

## S79 - 2026-05-19 (Godot launch fix)

### Fixed
- Restored the `project.godot` plugin configuration so `enabled=PackedStringArray(...)` is under `[editor_plugins]` instead of `[gui]`.
- Added an `is_inside_tree()` guard to `AudioManager._on_tree_changed()` so shutdown/tree-change callbacks do not call `get_tree()` after the autoload leaves the scene tree.

### Verification
- Ran Godot 4.6.2 headless against the project.
- Main scene initialized through all autoloads and reached the title/menu flow.
- The previous repeated `AudioManager._on_tree_changed()` `data.tree is null` backtrace no longer appears.
- Remaining resource-leak messages only appear during forced `--quit-after` shutdown.

---

## S80 - 2026-05-19 (restore full-game progression after demo build)

### Fixed
- Replaced the Act I `demo_end` route in `ch1_after_forest.json` with a transition into `ch2_market_arrival`.
- Added SceneFlow support for progression metadata on VN steps:
  - `set_chapter`
  - `complete_chapter`
  - `autosave_chapter_transition`
- Updated `ch2_market_arrival.json` to enter `verdan_market.tscn` without queuing the missing `ch2_malet_deal` VN scene.
- Added `ch2_arrival_vn_seen` handling in `verdan_market.gd` so the VN arrival can hand off to the existing map-based Ch2/Malet progression without replaying the old arrival dialogue.
- Replaced missing `market_bustle.ogg` with the existing `ch2_verdan.mp3` BGM.

### Verification
- VN scene JSON parse check passed.
- Direct Godot headless launch reached title/autoload initialization with no new script errors.
- Direct Godot headless load of `verdan_market.tscn` succeeded.
- Remaining resource-leak messages are from forced `--quit-after` shutdown only.

---

## S81 - 2026-05-19 (calm GAME START title screen)

### Done
- Imported `../이미지/game image/GAME START.png` as `assets/cg/game_image/game_start.png`.
- Rebuilt the main title screen around the single GAME START illustration.
- Removed the previous stacked title effects from `scenes/main/main.gd`:
  - background slideshow
  - god rays
  - foreground ash particles
  - title burst
  - letter cascade
  - ornament/grain overlays
  - splash sequence
- Repositioned the actual menu buttons over the menu frame already baked into the image.
- Renamed title menu labels to match the image language: `NEW GAME`, `CONTINUE`, `SETTINGS`, `EXIT`.
- Kept only a soft fade-in, title BGM, a very light veil, and subtle hover/focus treatment.

### Verification
- Godot headless launch reached the title scene with no new script errors.
- `game_start.png.import` generated successfully.
- Confirmed the removed noisy title effect functions no longer exist in `main.gd`.

---

## S82 - 2026-05-20 (new illustration and character sheet integration)

### Done
- Imported the newly added `game image` batch into project runtime folders:
  - `assets/cg/game_image/` for story CG, chapter plates, item CG, ending gallery plates, and world map art.
  - `assets/game_image/reference/` for UI references, enemy sheets, item sheets, and character sprite/expression references.
- Sliced Malet's expression sheet into eight usable HD dialogue portraits and remapped Malet dialogue keys to the new portraits.
- Replaced lower-quality dialogue CG references across Ch1, Ch2, Ch3, Ch4, Ch6, Ch8, Ch9, Ch10, and VN scenes with the stronger new illustrations.
- Updated ending gallery CG paths to use the new illustration set instead of missing placeholder ending files.
- Updated the Ch9 Kairos battle setup to use the new sealed-city Kairos illustration and fixed the boss enemy constructor call.
- Expanded the pause-menu Artbook with new CG plates, Malet sheets, enemy sheets, UI references, item sheet, and the new world map.
- Removed unreferenced old low-quality CG files and matching `.import` files after verifying they were no longer referenced.
- Replaced the missing `rim_ambient.ogg` VN BGM reference with the existing `ch1_forest.mp3`.

### Verification
- Godot import generated `.import` files for the newly added PNG assets.
- JSON parse and JSON resource checks passed.
- Full `res://` string scan across `data/`, `scripts/`, and `scenes/` reports 0 missing resource references.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot headless launch and direct `colorless_waste.tscn` load report no new script/parse/missing-resource errors.
- The remaining `resources still in use at exit` message appears only during forced `--quit-after` shutdown.

---

## S83 - 2026-05-20 (Arrel / Elia sheet-only art cleanup)

### Done
- Sliced the new Arrel expression sheet into eight runtime portraits:
  - neutral, soft smile, angry, sad, shocked, determined, battle-ready, memory-fading.
- Sliced the new Elia expression sheet into eight runtime portraits:
  - neutral, gentle smile, worried, sad, surprised, determined, healing-focus, memory-restoration.
- Remapped every Arrel / Elia dialogue portrait key in `DialogueBox` to the new sheet-derived portrait files.
- Updated battle-scene portrait references to use the new sheet-derived Arrel / Elia portraits.
- Built sheet-derived runtime CG plates for Arrel, Elia, and Arrel+Elia duo moments.
- Replaced all Arrel / Elia runtime CG references in dialogue/VN data with sheet-derived plates.
- Removed old Arrel / Elia standalone CG files and old portrait files after confirming no runtime references remained.
- Kept only the new Arrel / Elia reference/expression sheets plus the newly generated sheet-derived runtime assets.
- Updated Artbook entries and the asset intake README to reflect the sheet-only Arrel / Elia pipeline.

### Verification
- Old Arrel / Elia runtime reference scan reports 0 remaining references outside the new sheet pipeline.
- Full `res://` resource scan reports 0 missing references.
- JSON parse check passed.
- Godot import generated `.import` files for all new sheet-derived portraits and CG plates.

---

## S84 - 2026-05-20 (sheet sprites in gameplay and current CG-only cleanup)

### Done
- Sliced the new Arrel and Elia sprite sheets into runtime gameplay frames under:
  - `assets/sprites/characters/arrel_sheet/`
  - `assets/sprites/characters/elia_sheet/`
- Added sheet-frame loading to `PixelSprite` so Arrel and Elia can use real image frames in exploration and battle while keeping generated placeholder fallback behavior.
- Updated the player to use Arrel sheet frames in the actual map view.
- Updated Elia's companion sprite to use Elia sheet frames in the actual map view, while preserving Sable's existing generated companion sprite.
- Updated battle presentation so Arrel and Elia use sheet-derived animated battle bodies instead of portrait stand-ins.
- Switched the remaining runtime CG references away from loose legacy files under `assets/cg/` and into the curated `assets/cg/game_image/` set.
- Removed the old loose `assets/cg/` illustrations after confirming runtime references no longer depend on them.
- Restored the custom theme font resource after Godot import rewrote it during asset import.

### Verification
- JSON parse check passed.
- Full `res://` resource scan across `data/`, `scripts/`, and `scenes/` reports 0 missing references.
- Old Arrel / Elia portrait file reference scan reports 0 remaining old portrait file references.
- Loose legacy `assets/cg/` runtime image reference scan reports no actual old image references; only VN scene fallback directory construction remains.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot headless launch, direct battle scene load, and direct Rim Forest scene load report no new script/parse/missing-resource errors.
- The remaining `resources still in use at exit` message appears only during forced `--quit-after` shutdown.

---

## S85 - 2026-05-20 (title and opening dialogue polish)

### Done
- Added a restrained cinematic layer to the title screen:
  - radial vignette for depth,
  - subtle menu backing for readability,
  - slow memory-dust particles over the existing `GAME START` illustration.
- Kept the title composition calm and image-led instead of rebuilding the screen with noisy UI effects.
- Refined the first Arrel / Elia exchange in `ch1_prologue` with a clearer recognition beat and more physical detail.
- Replaced early prologue short CG keys with explicit current-image paths:
  - `sheet_arrel_elia_duo.png`
  - `memory_loss_warning.png`
  - `void_beast_confrontation.png`
- Added direct portrait aliases for sheet-derived Arrel shock and Elia worried/smile expressions.

### Verification
- `ch1_prologue.json` parses successfully.
- New focused resource scan reports 0 missing references.
- `git diff --check` passed for the touched files; only normal CRLF working-copy warnings appeared.
- Godot headless launch reports no new script/parse/missing-resource errors.
- The remaining `resources still in use at exit` message appears only during forced `--quit-after` shutdown.

---

## S86 - 2026-05-21 (character scale and illustrated map atmosphere)

### Done
- Reduced Arrel's actual exploration sprite from the oversized runtime scale to a persistent sheet base scale.
- Fixed movement squash/stretch, sprint stretch, idle breathing, fidget, and afterimage effects so they respect the smaller base sprite scale instead of expanding back to `Vector2(1, 1)`.
- Reduced Elia's exploration companion sheet sprite scale to match the smaller player silhouette.
- Reduced Arrel and Elia battle body sprite scales so the side-view battle scene no longer feels dominated by oversized character art.
- Added `MapEffects.add_illustration_atmosphere()`, a reusable low-alpha CG overlay layer for using curated illustrations directly in map gameplay without hiding the tilemap.
- Applied illustrated atmosphere layers to:
  - Rim Forest: `chapter_sealed_zone.png`
  - BL-07 Void: `nera_void_cavern.png`
  - Colorless Waste: `kairos_sealed_city.png`
  - Crumbling Coast: `sealed_gate_plaza.png`
  - The Seam: `tobias_memory_corridor.png`

### Verification
- Focused resource scan reports 0 missing references for the touched files.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot headless launch, direct Rim Forest load, and direct battle scene load report no new script/parse/missing-resource errors.
- The remaining `resources still in use at exit` message appears only during forced `--quit-after` shutdown.

---

## S87 - 2026-05-22 (illustrated mid-chapter atmosphere and Guard Focus)

### Done
- Extended the reusable illustrated map atmosphere pass to the remaining mid/late chapter field maps:
  - Verdan Market: `malet_bureau_overlook.png`
  - Belt Waystation: `world_map_memoria.png`
  - Drift Shelter: `memory_loss_warning.png`
  - Forgotten Forest: `void_beast_confrontation.png`
  - Seam Outskirts: `sealed_city_ruins.png`
- Upgraded Defend into `Guard Focus` so it now has a stronger tactical role:
  - keeps the existing incoming damage reduction,
  - shortens active player status effects by 1 turn when pressured,
  - restores a small amount of HP when wounded and not status pressured,
  - grants a larger Limit gain when already stable.
- Added battle feedback for `Guard Focus`:
  - player-side `GUARD` status icon while defending,
  - floating Guard Focus callout for status relief, HP restore, or Limit gain,
  - brief shield wash around Arrel when the action is chosen.
- Reworked `MemoryResonance` into a clean ASCII script after finding broken comment/function formatting in the file.
- Improved memory resonance markers with a larger pulse plate, bright core, and four directional sparks so field rewards are more discoverable.
- Restored the custom font theme resource after Godot import simplified it again.

### Verification
- Focused resource scan reports 0 missing references for the newly wired illustration layers.
- `MemoryResonance` scan reports no comment-swallowed function declarations after the cleanup.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH; local/common install-path search timed out before locating it, so scene smoke tests were not run in this pass.

---

## S88 - 2026-05-22 (dialogue interface cleanup)

### Done
- Cleaned up the runtime dialogue interface so dialogue, portraits, speaker names, and choices no longer feel visually mixed together.
- Changed the dialogue box from a nearly full-width strip into a centered lower panel with stronger margins and calmer contrast.
- Added a framed portrait well and hid it on narration/system lines so empty portrait space no longer clutters narration.
- Added a subtle divider between speaker name and dialogue text, improved line spacing, and replaced the noisy next-line marker with a quiet `ENTER` hint.
- Restyled choices as wider numbered buttons above the dialogue panel with more consistent spacing and calmer borders.

### Verification
- Focused dialogue diff review confirmed the interface changes stayed scoped to layout/style behavior, without the earlier accidental comment encoding churn.
- S93 changed-script `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was still not available on PATH, so headless scene smoke tests were not run.

---

## S89 - 2026-05-22 (image-referenced graphics pass)

### Done
- Expanded image-referenced presentation using the high-resolution `env_*` CG set.
- Reworked map illustration atmosphere so CG plates read more strongly:
  - added a brighter upper detail band,
  - added lower readability shading so player/tile silhouettes remain legible,
  - kept the existing slow alpha pulse for atmosphere.
- Reassigned several map atmosphere references to more location-specific environment CG:
  - Verdan Market: `env_bureau_spires.png`
  - Belt Waystation: `env_wasteland_city.png`
  - Drift Shelter: `env_frozen_archive.png`
  - Forgotten Forest / The Seam: `env_memory_hall.png`
  - Seam Outskirts / BL-07: `env_void_cathedral.png`
- Upgraded battle backgrounds:
  - explicit battle background images now render with stronger presence,
  - battles with no explicit image now resolve a fallback CG from the return map,
  - added top/side art depth plates plus readability wash and horizon shadow for a more illustrated stage.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

---

## S90 - 2026-05-22 (VN illustration depth pass)

### Done
- Continued the image-referenced graphics upgrade on the visual-novel scene layer.
- Added a CG detail plate over the upper third of VN scenes so full-screen illustrations read with more depth instead of acting like flat backgrounds.
- Added a persistent lower readability wash behind the text area to keep dialogue legible over bright or busy CG.
- Added soft character grounding shadows under left/right portraits so standing portraits feel attached to the scene.
- Added subtle portrait scale-in motion when a new portrait enters.
- Refined the VN dialogue panel styling with darker glass, quieter borders, and slightly roomier margins.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

---

## S91 - 2026-05-22 (interface refinement pass)

### Done
- Refined the shared exploration dialogue UI:
  - calmer glass-panel borders and margins,
  - slightly clearer speaker hierarchy,
  - text shadow for stronger readability over illustrated maps,
  - quieter pulsing `NEXT` indicator instead of a static prompt.
- Restyled exploration choices as slimmer dark cards with a left accent rule and softer staggered fade-in animation.
- Matched the visual-novel choices to the same calmer card language.
- Softened system log and tutorial hint panels so they feel closer to the rest of the current UI.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

---

## S92 - 2026-05-23 (Memory Pulse exploration update)

### Done
- Added `Memory Pulse`, a new active exploration action bound to `Q` and gamepad face button 3.
- Player can now send out a pulse while exploring:
  - creates two expanding pulse rings around Arrel,
  - briefly flashes nearby Memory Resonance echoes,
  - reports the closest echo by memory title and distance in paces,
  - uses a 6-second cooldown.
- Extended `MemoryResonance` triggers with scan metadata and a `pulse_scan()` helper.
- Added temporary `ECHO` callouts to scanned resonance points.
- Added HUD support for Memory Pulse readiness/cooldown.
- Added a first-use tutorial hint for Memory Pulse.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed the new `memory_pulse` input, player action, HUD status, resonance scan helper, and tutorial hint are wired.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

---

## S93 - 2026-05-23 (foreground image utilization pass)

### Done
- Reworked chapter title cards so they now use location CG directly:
  - soft full-screen art backdrop,
  - prominent centered art plate,
  - existing chapter text layered over the illustration.
- Added chapter-to-image mapping for all major locations.
- Added an exploration location art card to the HUD:
  - appears when entering a new map,
  - shows a cropped CG thumbnail for the current region,
  - uses the current chapter/location text as context.
- Expanded `ExplorationHUD` location metadata to include all major field maps and their art references.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed chapter-title art layers and exploration location art card wiring.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S94 - 2026-05-23 (dialogue illustration staging pass)

### Done
- Added large speaker-stage illustrations to `DialogueBox`:
  - Arrel/Elia now use profile CG as translucent left/right stage art during dialogue.
  - Kairos/Nera/Tobias use full-body CG when speaking.
  - Other speakers fall back to their portrait art so every spoken line has a stronger visual presence.
- Added smooth speaker-stage transitions:
  - active speaker fades/slides in from the side,
  - inactive side fades out,
  - narration/system lines clear the stage art.
- Upgraded `CgViewer` presentation:
  - full-screen CG now gets top/bottom cinematic wash overlays,
  - CG is slightly enlarged and positioned for a less flat slideshow feel,
  - overlays fade with the CG so dialogue readability is cleaner.

### Verification
- Changed-script `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed speaker-stage art functions and CG wash overlay wiring.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S95 - 2026-05-23 (battle illustration integration pass)

### Done
- Added battle-stage illustration layers to `BattleScene`:
  - Arrel battle CG now sits faintly behind the player side.
  - Enemy side resolves to enemy image, character full-body art, void beast CG, or the current battle background.
  - Left/right wash panels keep the art readable behind sprites and UI.
- Added attack cut-in illustrations:
  - normal Arrel attacks flash `sheet_arrel_battle_ready`,
  - memory burn attacks flash `memory_loss_warning`,
  - enemy attacks flash the resolved enemy-side illustration.
- Kept cut-ins short and translucent so they add impact without blocking tactical UI.

### Verification
- Changed-script `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed battle-stage art and action cut-in wiring.
- Replaced an unsafe TextureRect stretch constant with Godot's existing `STRETCH_KEEP_ASPECT_CENTERED`.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S96 - 2026-05-23 (battle break gameplay loop)

### Done
- Added an enemy BREAK system to `BattleManager`:
  - weakness hits add major break pressure,
  - neutral hits add minor pressure,
  - resisted hits add no pressure,
  - bosses gain break pressure more slowly.
- Broken enemies:
  - lose their next turn,
  - take +35% damage while broken,
  - trigger battle log feedback and a first-time tutorial hint.
- Added BREAK UI to the enemy battle panel:
  - dedicated BREAK bar under enemy HP,
  - BROKEN label/status icon while the enemy is staggered,
  - animated bar updates and enemy squash feedback on break.
- Applied break pressure to Attack, Burn skills, and Elia's offensive void skill.

### Verification
- Changed-script `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed BREAK variables, signals, UI handlers, and tutorial hint wiring.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S97 - 2026-05-23 (title and early dialogue stability pass)

### Done
- Fixed the title menu overlap risk:
  - moved the GAME START button stack lower into the intended frame,
  - widened the backing plate,
  - reduced button height/font size and increased spacing so four buttons no longer crowd each other.
- Calmed the early Arrel/Elia dialogue composition:
  - speaker-stage art is smaller, dimmer, and constrained away from the lower dialogue box,
  - full-screen CG dialogue lines now suppress side speaker art so the scene does not stack multiple competing illustrations.
- Hardened dialogue CG handling:
  - restored the CG caption panel reference that could be skipped by a malformed/commented line,
  - added null-safe caption panel access,
  - added close-state guards so repeated dialogue advances or deferred CG closing do not race the fade tween.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Changed-script `res://` reference scan reports 0 missing resources.
- Focused scan confirmed title sizing, CG-line suppression, and CG close guard wiring.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S98 - 2026-05-23 (VN crash and Arrel/Elia composition fix)

### Done
- Rewrote `CgViewer` with clean ASCII comments and explicit control flow:
  - removed a broken commented `if auto_close_sec > 0` line that could leave `await` under an invalid block,
  - replaced meta-based caption panel lookup with a direct `_text_panel` reference,
  - kept close/fade race guards for repeated dialogue advance.
- Fixed several `VNScene` lines where code had been swallowed into comments:
  - restored CG change checks, continue indicator creation, ember vignette creation, Ken Burns startup, burn flash, page-turn positioning, and choice dimming color handling.
- Reduced early Arrel/Elia visual clutter in the actual VN flow:
  - VN portraits are smaller,
  - Arrel/Elia conversations now use one active portrait instead of keeping both sides visible,
  - the duplicated CG detail overlay is disabled so the same illustration is not stacked over itself.
- Kept the DialogueBox safeguard from S97:
  - Arrel/Elia full-stage side illustrations remain disabled outside the VN flow too.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Changed-script `res://` reference scan reports 0 missing resources.
- Focused scan confirmed clean `CgViewer` auto-close flow, single-portrait VN composition, and disabled Arrel/Elia stage art.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S99 - 2026-05-24 (progression crash cleanup)

### Done
- Rechecked the latest Godot logs and older crash evidence:
  - current log reached Chapter 2 exploration without a fatal stack trace,
  - older logs showed `Lambda capture at index 2 was freed` after Verdan exploration triggers.
- Hardened `MapEffects.add_npc_wander`:
  - recurring tween callbacks now keep a `WeakRef` instead of a direct captured NPC node,
  - this prevents delayed wander callbacks from touching freed map/NPC nodes after scene transitions.
- Cleaned `SceneFlow` after the previous encoding/comment cleanup:
  - kept a single `resume_queue` declaration,
  - kept a single `current_index += 1` in `goto_scene`.
- Improved VN CG resilience:
  - short CG names now search `assets/cg/game_image/` before `assets/cg/`,
  - legacy Ch1 CG names map to existing images,
  - unresolved short CG names fall back to a safe existing chapter image instead of emitting missing-resource warnings.
- Removed an accidental duplicate `_start_ken_burns` function declaration in `VNScene`.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- All JSON files under `data/` parse successfully with PowerShell JSON parsing.
- Focused duplicate-function scan found no duplicate top-level `func` declarations in the touched files.
- Changed-script `res://` reference scan reports 0 missing resources.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S100 - 2026-05-24 (second error audit and callback hardening)

### Done
- Ran a project-only error audit excluding addon/example files:
  - duplicate top-level GDScript function scan,
  - `res://` resource existence scan,
  - full `data/**/*.json` parse check,
  - callback/timer review for delayed node access.
- Removed the missing Windows native icon reference from `project.godot`:
  - `config/icon` still points to the existing `res://icon.svg`,
  - `config/windows_native_icon` no longer points to absent `res://icon.ico`.
- Cleaned a remaining VN animation duplication:
  - `_swap_cg()` now starts Ken Burns once instead of twice on the same CG node.
- Hardened delayed callbacks that could fire after scene/UI teardown:
  - battle cut-in hide callback checks node validity,
  - hit flash material cleanup uses `WeakRef`,
  - enemy warning glow cleanup checks `enemy_sprite`,
  - memory burn preview hide callback checks popup nodes,
  - delayed layered SFX playback checks its `AudioStreamPlayer` before playing.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- All JSON files under `data/` parse successfully.
- Project-only `res://` scan reports 0 missing resources.
- Project-only duplicate-function scan reports 0 duplicate top-level `func` declarations.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S101 - 2026-05-24 (integrated graphics, gameplay, and story pass)

### Done
- Upgraded the title screen presentation:
  - bumped the displayed version to `v0.9.2`,
  - added a restrained gold menu rule and small caption above the menu frame,
  - kept the calmer `GAME START` image-first composition without moving buttons back into the crowded area.
- Improved VN choice presentation:
  - added optional `choice_title` and `choice_hint` support,
  - widened the choice column and added visible effect text per choice,
  - removed duplicate setup lines left in `VNScene`.
- Added reusable SceneFlow reward hooks:
  - choices and regular steps can now grant Grains, items, or HP through JSON fields,
  - reward processing now happens after step gating so skipped conditional steps cannot grant rewards.
- Strengthened the Chapter 1 story branch:
  - the first memory-spend decision now shows its mechanical consequence,
  - burning the campfire song now has its own follow-up narration and Elia reaction,
  - the old "No fire" narration is gated to the refusal branch only.
- Connected early story choices to gameplay:
  - burning the song gives the next battle +18 Limit and +22 BREAK pressure,
  - refusing to burn grants a Smoke Bomb and guards the first enemy blow in the next battle,
  - listening to Elia's humming can heal and gives +10 Limit in the next battle,
  - alternate reflection choices can grant Grains or a Firebomb.

### Verification
- All JSON files under `data/` parse successfully.
- Project-only `res://` scan reports 0 missing resources.
- Project-only duplicate-function scan reports 0 duplicate top-level `func` declarations.
- Focused swallowed-comment scan on changed GDScript files found no suspicious commented-out code.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S102 - 2026-05-24 (title screen art-only rollback)

### Done
- Simplified the title screen back to an illustration-first presentation:
  - removed code-generated shade, vignette/gradient, menu backing, dust particles, caption, gold rule, and version label,
  - kept `GAME START.png` as the only visible intro image,
  - converted title menu buttons into invisible hit targets so they no longer overlap the artwork text.
- Removed now-unused title variables after hiding the visible overlay UI.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed no title overlay/gradient variables remain in `scenes/main/main.gd`.
- Chapter 1 VN JSON still parses successfully.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S103 - 2026-05-24 (Godot warning-as-error launch fix)

### Done
- Fixed launch-blocking warning-as-error failures reported by the Godot editor:
  - typed `WeakRef` variables explicitly in delayed callbacks,
  - typed `get_ref()` callback locals explicitly in `MapEffects`, `BattleScene`, and `AudioManager`,
  - removed Variant inference warnings from `MemoryResonance` setup, trigger, scan, and reward code.
- Verified the title rollback did not reintroduce overlay references.

### Verification
- Focused scan found no remaining `:= weakref(...)`, `:= get_ref(...)`, or `:= INF` patterns.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project load passed.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load passed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S104 - 2026-05-24 (new game VN start crash fix)

### Done
- Fixed the crash reported after pressing GAME START:
  - removed a duplicated empty `if step.has("cg"):` block in `VNScene._on_step_changed`,
  - removed a duplicated local `var c` declaration in `_dim_background_for_choice`.
- Added and removed a temporary smoke scene to test the real startup path:
  - set `SceneFlow.pending_scene_id = "ch1_prologue"`,
  - transitioned through `res://scenes/main/vn_host.tscn`,
  - allowed the VN UI and first prologue step to load.

### Verification
- Godot 4.6.2 headless `res://scenes/ui/vn_scene.tscn` load passed after the fix.
- Godot 4.6.2 smoke run for `GAME START -> vn_host -> ch1_prologue` passed with no output errors.
- `git diff --check` passed before the final smoke run; only normal CRLF working-copy warnings appeared.

## S105 - 2026-05-24 (supporting character and boss art routing)

### Done
- Expanded dialogue-stage art beyond Arrel and Elia:
  - added Sable, Seric, and Veil stage-art routing,
  - added Veil portrait aliases and default portrait support,
  - added distinct dialogue blip pitch values for Nera, Seric, and Veil.
- Improved battle art selection:
  - added name-based enemy art fallback for Kairos, Nera, Tobias, Veil, and Void/Shade/Sentinel/Wraith/Fragment/Lurker enemies,
  - made enemy sprites use the same fallback art when `BattleManager.enemy_image` is empty,
  - updated the Chapter 9 Kairos boss fight to use `kairos_fullbody.png` as the enemy image while keeping `kairos_sealed_city.png` as the battle background.

### Verification
- Project-only `res://` scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load passed with no script or parse errors.
- Temporary battle-art smoke scene loaded Kairos and Shade Sentinel battle scenes with no script or parse errors, then was removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S106 - 2026-05-25 (remaining art coverage and battle codex routing)

### Done
- Rechecked image coverage across `assets/cg/game_image`, `assets/portraits`, and `assets/game_image/reference`.
- Centralized enemy image fallback in `BattleManager.resolve_enemy_image_by_name`:
  - Kairos/Nera/Tobias/Veil route to full-body art,
  - Void Beast/Shade/Sentinel/Threshold route to the void confrontation art,
  - Void Wisp/Wraith/Fragment/Lurker route to the void creature sheet,
  - Crawler/Soldier route to the memory-lost soldier sheet,
  - Guardian route to the forgotten guardian sheet.
- Made `BattleManager.start_battle` assign fallback enemy images when callers pass an empty image path.
- Updated `Codex` bestiary previews to use the same fallback image routing, including older entries without saved `image_path`.
- Added a visible Tobias backline support sprite in battle when Tobias is in the party.

### Verification
- Project-only `res://` scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load produced no script or parse errors.
- Temporary art-routing smoke scene loaded Kairos and Void Wraith battle scenes with Tobias support active and produced no script or parse errors, then was removed.

## S107 - 2026-05-25 (integrated battle presentation and reward loop update)

### Done
- Upgraded battle openings:
  - enemy art now appears inside the battle intro overlay,
  - weakness/resistance/class tags are shown before the first turn,
  - `BattleManager` emits a short tactical hint at encounter start.
- Strengthened the 30-second battle loop:
  - scanning an enemy now matters economically,
  - victories grant a small Codex tactical bonus when the current enemy was scanned,
  - the animated victory screen now shows the Codex Bonus row.
- Kept the image routing unified:
  - `BattleScene` now delegates enemy art fallback to `BattleManager.resolve_enemy_image_by_name`,
  - this keeps battle sprites, cut-ins, and Codex previews aligned.

### Verification
- Project-only `res://` scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load produced no script or parse errors.
- Temporary S107 battle smoke scene loaded Kairos and Void Wraith battle openings, Tobias support, and the Codex Bonus victory screen with no script or parse errors, then was removed.

## S108 - 2026-05-25 (Arrel face-only portrait crop fix)

### Done
- Created face-only Arrel portrait crops for all current Arrel emotion states:
  - neutral, angry, battle ready, determined, memory fading, sad, shocked, and soft smile.
- Rewired `DialogueBox.PORTRAIT_MAP` so Arrel dialogue portraits use the cropped face images instead of the larger sheet images.
- Rewired the battle HP panel portrait to use `arrel_face_neutral.png`.
- Kept the original full illustration/sheet assets intact for CG, stage, and cut-in use.

### Verification
- Godot 4.6.2 headless asset smoke loaded all new `arrel_face_*.png` textures with no script or parse errors, then the temporary smoke scene was removed.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load produced no script or parse errors.
- Project-only `res://` scan reports 0 missing resources.
- Focused scan found no remaining gameplay/UI references to the old Arrel sheet portrait files.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.

## S109 - 2026-05-25 (Elia face-only portrait consistency pass)

### Done
- Created face-only Elia portrait crops for all current Elia emotion states:
  - neutral, determined, gentle smile, healing focus, memory restoration, sad, surprised, and worried.
- Rewired `DialogueBox.PORTRAIT_MAP` so Elia dialogue portraits use the cropped face images instead of larger sheet images.
- Rewired the battle ally portrait fallback to use `elia_face_neutral.png`.
- Kept the original Elia full illustration/sheet assets intact for CG, stage, and cut-in use.

### Verification
- Godot 4.6.2 headless asset smoke loaded all new `elia_face_*.png` textures with no script or parse errors, then the temporary smoke scene was removed.
- Godot editor headless import pass generated `.import` metadata for the new Elia face crops.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load produced no script or parse errors.
- Project-only `res://` scan reports 0 missing resources.
- Focused scan confirms Elia gameplay/UI portrait references now use `elia_face_*.png`.

## S110 - 2026-05-25 (major NPC face portrait and Malet visibility pass)

### Done
- Created face-only portrait crops for major non-Arrel/Elia characters:
  - Malet: neutral, amused, calculating, deal accepted, disappointed, price revealed, smile, and warning.
  - Sable, Tobias, Kairos, Nera, and Seric: neutral/key-state face crops.
- Rewired `DialogueBox.PORTRAIT_MAP` so Malet/Mallet, Sable, Tobias, Kairos, Nera, and Seric dialogue portraits use the new face crops instead of raw full portrait files.
- Added Mallet spelling aliases to prevent image fallback gaps when the merchant name is entered with the alternate spelling.
- Made major NPC map sprites prefer the new portrait crops over generic PixelSprite placeholders when a matching face crop exists.
- Added a merchant portrait slot to `MemoryShop`, so Malet's image appears directly in the memory exchange UI.
- Updated battle ally/support fallback art for Sable and Tobias to use the new face crops.

### Verification
- Godot editor headless import pass generated `.import` metadata for all 17 new face crops and reported no new script or parse errors.
- Godot 4.6.2 headless game boot reached the main menu with no script or parse errors.
- Focused scan confirms old raw major-NPC portrait files are no longer referenced by gameplay/UI scripts.
- Focused `git diff --check` passed for the files touched in this pass; a pre-existing trailing-whitespace warning remains in `scripts/ui/vn_scene.gd`.

## S111 - 2026-05-25 (character presentation micro-polish)

### Done
- Added speaker-colored portrait framing in `DialogueBox`, including a bottom accent strip that changes per active speaker.
- Added extra speaker color handling for Mallet, Nera, Seric, Tobias, and Veil so dialogue and journal surfaces avoid the generic fallback color.
- Improved major NPC map presentation:
  - portrait-based NPCs now sit slightly higher,
  - get a small ground shadow,
  - and receive a thin speaker-colored frame so they read as authored character objects instead of loose pasted images.
- Refined the MemoryShop header with a merchant caption line, giving Malet's shop a more intentional broker/merchant presentation.

### Verification
- Godot 4.6.2 headless game boot reached the main menu with no script or parse errors.
- Godot 4.6.2 headless `res://scenes/maps/verdan_market.tscn` load instantiated Malet successfully with no script or parse errors.
- Focused `git diff --check` passed for the files touched in this pass; only normal CRLF working-copy warnings appeared.

## S112 - 2026-05-26 (premium global graphics lens pass)

### Done
- Added a reusable screen-space premium lens shader:
  - subtle paper grain,
  - stronger cinematic edge darkening,
  - faint letterbox weight,
  - slow diagonal light-shaft shimmer.
- Added `MapEffects.add_premium_map_lens()` and connected it across all 10 exploration maps with biome-specific tint/strength settings.
- Added the same lens language to battle presentation so combat shares the map/VN visual tone.
- Restored the VN CG detail overlay path by removing the early return in `_sync_cg_presentation_layers()`, so the top-detail layer can actually animate again.
- Fixed a discovered `forgotten_forest` fog-call parse error while validating the upgraded map pass.

### Verification
- `git diff --check` passed for the files touched in this pass; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Godot 4.6.2 headless `res://scenes/battle/battle_scene.tscn` load passed.
- Godot 4.6.2 headless `res://scenes/ui/vn_scene.tscn` load passed.
- Godot 4.6.2 headless load passed for all 10 map scenes:
  - rim_forest, verdan_market, belt_waystation, drift_shelter, crumbling_coast,
  - the_seam, seam_outskirts, forgotten_forest, colorless_waste, bl07_void.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S113 - 2026-05-27 (tactical battle objective gameplay loop)

### Done
- Added per-encounter Tactical Objectives to make ordinary battles more deliberate:
  - Pressure Point: trigger BREAK before victory,
  - Clean Hands: win without burning a memory,
  - Archivist's Eye: scan/analyze before victory,
  - Measured Assault: reach Combo x3 before victory.
- Added objective state tracking in `BattleManager`:
  - objective generation at battle start,
  - complete/fail state updates,
  - mid-battle checks for BREAK, scan, combo, and memory burn failure,
  - victory-time objective reward finalization.
- Added objective rewards:
  - bonus Grains,
  - occasional item rewards tied to the objective type,
  - objective rows in the animated victory reward screen.
- Added an in-battle objective panel so the current tactical goal is visible during combat and changes color when completed or lost.

### Verification
- Focused `git diff --check` passed for the changed battle files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Godot 4.6.2 headless `res://scenes/battle/battle_scene.tscn` load passed.
- Temporary tactical-objective smoke scene confirmed the real `BattleManager.start_battle()` path creates an objective and resolves a positive objective reward, then the temporary files were removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S114 - 2026-05-28 (memory compass world-rewrite system)

### Done
- Added `MemoryCompass` as a new exploration autoload that turns the original Memory Compass lore into a live play surface:
  - contextual needle states for Rim Forest, Verdan, Belt Waystation, Drift Shelter, The Seam, Forgotten Forest, Colorless Waste, and BL-07,
  - memory-density driven fallback states when the current scene has no bespoke compass profile,
  - lore lines pulled from the project's core themes: soil remembers rain, stone remembers pressure, and BL-07 melts direction itself.
- Made memory burns visibly rewrite the world outside battle:
  - every `memory_burned` signal now refreshes the current scene's `PerceptionFilter`,
  - a screen pulse and compass shock call out the exact contour that vanished,
  - Elia, Tobias, Sable, and identity-grade memories get special burn language.
- Added a compact cinematic compass panel below the existing location card, with animated needle drift, burn-pulse feedback, and `C` key hide/show support.

### Verification
- `git diff --check -- project.godot scripts/ui/memory_compass.gd` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Temporary MemoryCompass smoke scene burned a real memory through `MemoryManager.burn_memory_silent()`, triggered connected systems, and exited with no script or parse errors, then the temporary files were removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S115 - 2026-05-28 (world rewrite director and visible loss echoes)

### Done
- Added `WorldRewriteDirector` as a new autoload that turns memory loss into a directed world event instead of a passive state change.
- Added bespoke rewrite rules for major original-memory contours:
  - Verdan taste/vendor recognition,
  - Elia's campfire song and anchoring warmth,
  - the reaching hand relationship memory,
  - Arrel's first sword identity memory,
  - Arrel's name origin,
  - Tobias's record memory,
  - Sable's witness memory.
- Memory burns now set durable story flags such as `world_rewrite_elia_hum_unmoored` and `world_forgot_<memory_id>`, giving future dialogue, maps, endings, and encounters concrete hooks for irreversible consequences.
- Memory burns and fades now spawn visible in-map loss echoes:
  - spectral shards near the player/current scene,
  - short authored consequence text,
  - grade-based color and intensity,
  - automatic fade/float cleanup.
- Connected the director back into `MemoryCompass`, so the compass now reports the authored consequence from the rewrite director instead of only showing generic burn text.
- Added residual absence behavior when entering a scene after memories have already been burned, making revisited spaces feel like they have adjusted around what Arrel lost.

### Verification
- `git diff --check -- project.godot scripts/ui/memory_compass.gd scripts/systems/world_rewrite_director.gd` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Temporary world-rewrite smoke scene burned `daily_campfire_song`, confirmed the director set `world_rewrite_elia_hum_unmoored`, and exited with no script or parse errors, then the temporary files were removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S116 - 2026-05-28 (living loss chronicle)

### Done
- Extended `WorldRewriteDirector` with a readable loss-record API:
  - `get_loss_records()` returns burned/faded memory records,
  - `get_rewrite_report(memory_id)` exposes the authored world consequence for future systems,
  - each record includes memory title, grade class, world consequence, compass reading, and durable story hook.
- Added a new `Losses` tab to `StoryJournal`, turning irreversible memory burn into a readable player biography instead of a temporary notification.
- The Losses tab now lists every burned/faded memory with its consequence text from the world rewrite director, so players can review exactly how Arrel and the world have changed.
- The system keeps the original "Blank Book / record-tree contour" theme alive mechanically: the game now records the shape of what was lost, even when the memory itself is gone.

### Verification
- `git diff --check -- scripts/systems/world_rewrite_director.gd scripts/ui/story_journal.gd` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Temporary loss-journal smoke scene burned `identity_first_sword`, confirmed an identity loss record was generated, opened and closed `StoryJournal`, and exited with no script or parse errors, then the temporary files were removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S117 - 2026-06-18 (generated rewrite art integration)

### Done
- Generated 10 local dark-fantasy CG/system-art PNGs under `assets/cg/generated/`:
  - `memory_burn_first_sword.png`
  - `memory_burn_elia_song.png`
  - `memory_burn_reaching_hand.png`
  - `memory_burn_arrel_name.png`
  - `world_rewrite_verdan_market.png`
  - `world_rewrite_tobias_record_tree.png`
  - `world_rewrite_elia_anchor.png`
  - `world_rewrite_sable_witness.png`
  - `ui_memory_compass_close.png`
  - `ui_loss_record_blank_book.png`
- Connected generated CGs to `WorldRewriteDirector` rewrite rules so key memory burns now show a short fullscreen art flash before fading back to gameplay.
- Added fallback generated art for uncatalogued Grade 1/2 and low-grade loss records, so future memories still get visual treatment even before bespoke art exists.
- Extended `WorldRewriteDirector` loss records with `art` paths.
- Added a `StoryJournal` detail preview image for Losses entries, so the loss chronicle now shows visual memory evidence instead of text only.

### Verification
- Visually inspected generated `memory_burn_first_sword.png` and `world_rewrite_elia_anchor.png`.
- `git diff --check -- scripts/systems/world_rewrite_director.gd scripts/ui/story_journal.gd assets/cg/generated` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed and imported the generated PNG assets.
- Temporary generated-art smoke scene burned `identity_first_sword`, confirmed a `WorldRewriteArtFlash` node was created, confirmed loss record art paths exist, opened and closed `StoryJournal`, then was removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S118 - 2026-06-18 (interface cohesion upgrade)

### Done
- Upgraded `MemoryUI` into a more informative archive surface:
  - added an archive-state summary line with held/burned/fading/eroding memory counts,
  - surfaced the latest world rewrite/loss-record count at the top of the archive,
  - added generated loss-art previews to the memory detail panel when a memory has rewrite evidence,
  - added authored world-consequence text beside each applicable memory.
- Added a compact status summary to `StoryJournal` showing current chapter, held memories, burned memories, and recorded losses.
- Improved journal detail refresh behavior so stale loss artwork is cleared when switching lists or tabs.
- Added generated Memory Compass artwork as a subtle background plate behind the live compass UI, keeping the text/needle readable.
- Added recorded-loss count to the pause menu's current-run status block.

### Verification
- `git diff --check -- scripts/ui/memory_ui.gd scripts/ui/story_journal.gd scripts/ui/pause_menu.gd SESSION_LOG.md` passed; only normal CRLF working-copy warnings appeared.
- `scripts/ui/memory_compass.gd` is still an untracked project file, so it was checked separately for trailing whitespace and passed.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Temporary UI smoke scene burned `identity_first_sword`, opened and closed `MemoryUI`, opened and closed `StoryJournal`, and exited with no script or parse errors, then the temporary files were removed.

## S119 - 2026-06-18 (fast stability audit)

### Done
- Ran a fast project stability pass after the S95-S118 checkpoint.
- Verified dialogue JSON parsing with explicit UTF-8 handling to avoid false failures from PowerShell's default code page.
- Re-ran project-only integrity scans excluding addon/example noise:
  - `res://` references in active project assets/data/scenes/scripts,
  - duplicate top-level GDScript function declarations,
  - strict merge-conflict markers,
  - staged/working-tree whitespace checks.
- Ran Godot 4.6.2 headless smoke coverage across:
  - project boot,
  - main menu,
  - battle scene,
  - VN scene,
  - all 10 exploration maps.
- Added and removed a temporary runtime smoke scene that exercised:
  - `SceneFlow.play()` / `advance()`,
  - `MemoryManager.burn_memory_silent()`,
  - `WorldRewriteDirector.get_loss_records()`,
  - `MemoryUI` open/close,
  - `StoryJournal` open/close,
  - `BattleManager.start_battle()`.

### Verification
- UTF-8 JSON parse passed for all 16 files under `data/`.
- Project `res://` reference scan passed for 105 active project files.
- Project duplicate top-level function scan passed.
- Strict conflict marker scan passed.
- `git diff --check` passed.
- Godot smoke suite passed for project boot, main menu, battle scene, VN scene, and all 10 exploration maps.
- Temporary runtime systems smoke passed and the temporary files were removed.
- No stability fixes were required in this pass.

## S120 - 2026-06-18 (world rewrite art crash fix)

### Done
- Fixed a runtime crash when a memory burn tried to show generated world-rewrite art.
- Root cause:
  - `WorldRewriteDirector._show_rewrite_art()` was assigning `modulate` directly on a `CanvasLayer`.
  - Godot 4 `CanvasLayer` does not expose `modulate`, so the game crashed with `Invalid access to property or key 'modulate' on a base object of type 'CanvasLayer'`.
- Changed the flash structure so `CanvasLayer` only owns layer ordering, while a child `Control` root owns `modulate` fade-in/fade-out and contains the art, wash, and shade nodes.

### Verification
- Temporary rewrite-art crash smoke directly called `_show_rewrite_art()` and confirmed `WorldRewriteArtFlash` creates a fadeable `Control` root with no script errors.
- Temporary burn-path smoke called `MemoryManager.burn_memory("identity_first_sword")`, confirmed world-rewrite art flash and loss records are generated, and exited with no script errors.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- `git diff --check -- scripts/systems/world_rewrite_director.gd SESSION_LOG.md` passed; only normal CRLF working-copy warnings appeared.
- Temporary smoke files were removed.

## S121 - 2026-06-19 (VN protagonist illustration overlap fix)

### Done
- Fixed a clean-composition issue in `VNScene` where protagonist dialogue could show a character-focused CG and side portrait at the same time.
- Added a CG-aware portrait suppression rule:
  - when Arrel/Elia dialogue uses a CG path that already contains that speaker, `arrel_elia`, or `duo`, the side portrait slots are cleared for that line,
  - normal background CG dialogue keeps portraits,
  - the next regular Arrel/Elia line still restores the single active-side portrait composition.
- Added `_clear_portraits()` helper so both portrait slots and shadows are reset through the existing portrait state path.

### Verification
- Godot 4.6.2 headless `res://scenes/ui/vn_scene.tscn` load passed.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Static check confirmed `ch1_prologue` step 5 (`sheet_arrel_elia_duo.png` + Elia dialogue) is covered by the new suppression rule.
- `git diff --check -- scripts/ui/vn_scene.gd` passed; only normal CRLF working-copy warnings appeared.

## S122 - 2026-06-19 (VN cinematic graphics polish)

### Done
- Upgraded `VNScene` presentation layers so existing CG/portrait art reads more cinematic without needing new assets.
- Added a warm lower focus glow behind the dialogue area to give CG scenes more depth and keep the eye near the active text area.
- Added a full-screen radial vignette over CG layers so scene edges feel framed instead of flat.
- Added speaker-colored portrait rim frames:
  - Arrel, Elia, Sable, Malet/Mallet, and Tobias receive distinct accent colors,
  - active speaker frames brighten while inactive frames dim,
  - frames hide through the same state path as portraits so the S121 overlap fix stays clean.

### Verification
- `git diff --check -- scripts/ui/vn_scene.gd` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless `res://scenes/ui/vn_scene.tscn` load passed.
- Godot 4.6.2 headless project boot passed with no script or parse errors.

## S123 - 2026-06-19 (generated story CG expansion)

### Done
- Generated and integrated 5 new GPT-image story CGs under `assets/cg/generated/`:
  - `story_ch3_waystation_blank_book.png`
  - `story_ch4_drift_anchor.png`
  - `story_ch8_memory_forest_remnant.png`
  - `story_ch9_colorless_compass.png`
  - `story_ch10_bl07_core_choice.png`
- Connected the new CGs to high-impact story moments:
  - Ch3 Blank Book discovery,
  - Ch4 ash-rain anchoring,
  - Ch8 Memory Forest remnant encounter,
  - Ch9 Colorless Waste / Memory Compass discovery,
  - Ch10 BL-07 core and seal-decision beat.
- Added the new generated story CGs to the Pause Menu artbook list so they can be reviewed outside the dialogue flow.
- Hardened `VNScene` composition rules so full-scene generated story CGs clear both side portraits, preventing the earlier character-illustration overlap issue from returning on the new CGs.

### Verification
- Visually inspected all 5 generated story CGs after copying them into the project.
- UTF-8 JSON parse passed for all dialogue files under `data/` and `data/vn_scenes/`.
- `git diff --check` passed for the edited dialogue data, `pause_menu.gd`, `vn_scene.gd`, and generated CG paths; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 console executable was found and version-checked successfully, but both headless project boot and `--check-only --script` crashed immediately with engine-level signal 11 before script output. Runtime scene validation is therefore blocked in this shell until Godot can start cleanly again.

## S124 - 2026-06-20 (generated UI/UX art overhaul)

### Done
- Generated and integrated 5 new GPT-image UI/UX backdrop PNGs under `assets/cg/generated/`:
  - `ui_title_memoria_premium.png`
  - `ui_pause_archive_backdrop.png`
  - `ui_memory_archive_backdrop.png`
  - `ui_story_journal_backdrop.png`
  - `ui_memory_shop_backdrop.png`
- Reworked the title screen from image-baked invisible hit targets into readable in-engine UI:
  - new text-free generated title background,
  - visible MEMORIA title/subtitle/tagline,
  - visible New Game / Continue / Options / Quit buttons,
  - clearer hover/focus/disabled button states.
- Added generated backdrops to major menu surfaces:
  - PauseMenu now uses the archive desk / Memory Compass backdrop and moves the menu panel right to reveal the art,
  - MemoryUI now uses the Blank Book / memory-shard archive backdrop,
  - StoryJournal now uses the loss-chronicle journal backdrop,
  - MemoryShop now uses the Verdan market counter backdrop and leaves the merchant-side art visible.
- Registered the 5 new UI backdrops in the PauseMenu Artbook alongside the generated story CGs.

### Verification
- Visually inspected all 5 generated UI backdrops after copying them into the project.
- Verified all new UI PNG files have valid PNG signatures.
- `git diff --check -- scenes/main/main.gd scripts/ui/pause_menu.gd scripts/ui/memory_ui.gd scripts/ui/story_journal.gd scripts/ui/memory_shop.gd assets/cg/generated` passed; only normal CRLF working-copy warnings appeared.
- UTF-8 JSON parse still passed for all dialogue files under `data/` and `data/vn_scenes/`.
- Godot 4.6.2 console executable still reports the correct version, but headless project boot and `--check-only --script scenes/main/main.gd` continue to crash immediately with engine-level signal 11 before script output. Runtime visual validation and import generation for the newest UI PNGs are blocked in this shell until Godot starts cleanly again.

## S125 - 2026-06-20 (fantasy font and dialogue CG pass)

### Done
- Generated and integrated 4 new GPT-image dialogue CGs under `assets/cg/generated/`:
  - `dialogue_ch1_elia_finds_arrel.png`
  - `dialogue_ch2_malet_memory_trade.png`
  - `dialogue_ch5_elia_cliff_choice.png`
  - `dialogue_ch7_sable_echo_shell.png`
- Connected the new dialogue CGs to character-heavy story beats:
  - Elia finding Arrel after the first burn,
  - Malet naming the price of passage in Ch2,
  - Arrel and Elia facing the split-or-stay cliff choice,
  - Sable revealing the Echo Shell before BL-07.
- Expanded `VNScene` portrait suppression so generated dialogue CGs behave like full-scene story CGs and clear side portraits, preventing character art from overlapping the illustration.
- Strengthened the fantasy typography pass:
  - `assets/fonts/theme.tres` now prioritizes serif/fantasy-friendly font chains,
  - `UITheme` now exposes title/body font helpers,
  - title screen, VN speaker names, dialogue speaker names, and dialogue body text now receive the new font styling directly.
- Registered the 4 dialogue CGs in the PauseMenu Artbook for review outside the story flow.

### Verification
- Visually inspected the 4 generated dialogue CGs after copying them into the project.
- Verified all 4 new dialogue PNG files have valid PNG signatures.
- UTF-8 JSON parse passed for all files under `data/`.
- `git diff --check` passed for the edited dialogue data, font/theme files, UI scripts, session log, and generated dialogue CG paths; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 console executable still reports the correct version, but headless project boot and `--check-only --script scripts/utils/ui_theme.gd` continue to crash immediately with engine-level signal 11 before script output. Runtime visual validation and import generation for the newest dialogue PNGs are blocked in this shell until Godot starts cleanly again.

## S126 - 2026-06-21 (large gameplay patch: combat resonance and tactical objectives)

### Done
- Added the Combat Resonance momentum system to `BattleManager`:
  - tactical play now builds Resonance during a fight,
  - weakness pressure, BREAK, clean attacks, combo continuity, memory burns, residue burns, Guard Focus, stance shifts, Elia techniques, and companion support all feed the meter,
  - higher Resonance ranks slightly increase player damage and award post-battle Grains bonuses.
- Expanded tactical objectives from a small pool into a broader combat-challenge layer:
  - existing objectives remain: BREAK, scan/analyze, clean hands, combo x3,
  - new objectives include swift finish, no-item victory, stance shifting, resonance climb, echo weave, limit release, and companion coordination,
  - objectives can now reward Grains, items, and extra HP recovery.
- Upgraded battle result rewards:
  - victory rewards now include objective heal rewards and Resonance bonus Grains,
  - structured reward data includes objective heal, momentum rank, momentum label, and momentum bonus.
- Updated battle UI:
  - tactical objective panel now shows live Resonance rank/percent,
  - Resonance color changes by rank,
  - victory reward panel now shows Resonance bonus as its own line.
- Added progression hooks:
  - new play stats: `highest_momentum_rank`, `objectives_completed`, `momentum_surges`,
  - PauseMenu statistics now surface those values,
  - new achievements: `Field Tactician` and `Overbright`,
  - new tutorial hint explains Resonance the first time it matters.

### Verification
- UTF-8 JSON parse passed for all files under `data/`.
- `git diff --check` passed for the edited gameplay/UI/stat files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports normal ObjectDB/resource cleanup noise.
- Temporary battle smoke scene verified:
  - autoload project boot,
  - battle start,
  - tactical objective generation,
  - stance switching,
  - player attack,
  - Resonance increased to `Kindled 25.0`.
- Temporary smoke files were removed after validation.

## S127 - 2026-06-21 (GPT-image interface overhaul)

### Done
- Generated and integrated 6 new text-free GPT-image interface assets under `assets/cg/generated/`:
  - `ui_dialogue_ornate_frame.png`
  - `ui_battle_tactical_plate.png`
  - `ui_battle_victory_reward_panel.png`
  - `ui_burn_preview_ritual_panel.png`
  - `ui_options_observatory_backdrop.png`
  - `ui_game_over_void_backdrop.png`
- Upgraded the dialogue UI with a generated lower-third ornate frame behind the existing portrait/text layout, preserving readable in-engine text and avoiding character-illustration overlap.
- Upgraded battle interface surfaces:
  - tactical objective HUD now has a generated brass/obsidian backplate,
  - victory rewards now animate over a generated reward-frame layer,
  - memory-burn confirmation now uses a ritual-frame layer behind the cost/risk text and buttons.
- Upgraded menu/recovery surfaces:
  - OptionsMenu now opens over a generated archive-observatory backdrop,
  - GameOver now uses a generated void/memory-shatter backdrop with a readable center panel.
- Registered the 6 new interface assets in the PauseMenu Artbook as generated UI frames/backdrops.

### Verification
- Visually inspected the generated dialogue frame and burn-preview ritual frame after copying them into the project.
- Verified all 6 new interface PNG files have valid PNG signatures.
- `git diff --check -- scripts/ui/dialogue_box.gd scenes/ui/options_menu.gd scenes/ui/game_over.gd scenes/battle/battle_scene.gd scripts/ui/pause_menu.gd assets/cg/generated` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports the known ObjectDB/resource cleanup noise.
- Representative scene loads passed:
  - `res://scenes/ui/game_over.tscn`
  - `res://scenes/maps/verdan_market.tscn`
  - `res://scenes/battle/battle_scene.tscn`

## S128 - 2026-06-21 (extra interface finish: command ribbon and pause slab)

### Done
- Generated and integrated 2 additional text-free GPT-image interface assets under `assets/cg/generated/`:
  - `ui_battle_command_ribbon.png`
  - `ui_pause_control_slab.png`
- Upgraded the battle command bar:
  - added a generated wide command ribbon behind the bottom action buttons,
  - synchronized the ribbon visibility with `action_container`,
  - added a subtle breathing alpha to the ribbon while commands are available,
  - tightened action button colors, borders, outlines, and hover scale feedback.
- Upgraded the pause menu:
  - added a generated vertical control slab behind the menu stack,
  - animated the slab together with the existing slide-in/slide-out menu panel,
  - lowered the existing panel opacity so the generated ornament reads through without hurting text readability.
- Registered the 2 new interface assets in the PauseMenu Artbook.

### Verification
- Visually inspected the new battle command ribbon and pause control slab after copying them into the project.
- Verified both new PNG files have valid PNG signatures.
- `git diff --check -- scenes/battle/battle_scene.gd scripts/ui/pause_menu.gd assets/cg/generated/ui_battle_command_ribbon.png assets/cg/generated/ui_pause_control_slab.png` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports the known ObjectDB/resource cleanup noise.
- Representative scene loads passed:
  - `res://scenes/battle/battle_scene.tscn`
  - `res://scenes/maps/verdan_market.tscn`

## S129 - 2026-06-21 (extra interface finish: exploration HUD, toast, tutorial banner)

### Done
- Generated and integrated 3 additional text-free GPT-image interface assets under `assets/cg/generated/`:
  - `ui_exploration_hud_plate.png`
  - `ui_notification_toast_frame.png`
  - `ui_tutorial_hint_banner.png`
- Upgraded the exploration HUD:
  - added a generated top-left HUD plate behind HP, chapter, memory, grains, items, pulse, equipment, and quest text,
  - synchronized the generated plate with exploration-only visibility,
  - matched the plate to the existing slide-in animation,
  - lowered the old panel opacity so the generated frame reads through while preserving text readability.
- Upgraded notification toasts:
  - added a generated bottom-center toast frame behind save/load, memory, and warning messages,
  - matched the generated frame to the existing slide/fade toast animation,
  - softened the old flat panel style so the new frame carries the visual weight.
- Upgraded tutorial hints:
  - added a generated top-center hint banner behind first-time contextual tutorial text,
  - animated the banner with the existing hint panel,
  - lowered the old hint panel opacity for a more integrated fantasy UI look.
- Registered all 3 new interface assets in the PauseMenu Artbook.

### Verification
- Visually inspected the new exploration HUD plate and tutorial hint banner after copying them into the project.
- Verified all 3 new PNG files have valid PNG signatures.
- `git diff --check -- scripts/ui/exploration_hud.gd scripts/ui/notification_toast.gd scripts/ui/tutorial_hints.gd scripts/ui/pause_menu.gd assets/cg/generated/ui_exploration_hud_plate.png assets/cg/generated/ui_notification_toast_frame.png assets/cg/generated/ui_tutorial_hint_banner.png` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports the known ObjectDB/resource cleanup noise.
- Representative scene load passed:
  - `res://scenes/maps/verdan_market.tscn`

## S130 - 2026-06-21 (GPT-image memory-burn cut-ins)

### Done
- Generated and integrated 2 additional text-free GPT-image battle cut-ins under `assets/cg/generated/`:
  - `memory_burn_compass.png`
  - `memory_burn_void_walker.png`
- Connected 6 memory-burn illustrations to the battle flow:
  - `identity_first_sword` -> `memory_burn_first_sword.png`
  - `daily_campfire_song` -> `memory_burn_elia_song.png`
  - `rel_hand_reaching` -> `memory_burn_reaching_hand.png`
  - `core_name_origin` -> `memory_burn_arrel_name.png`
  - `identity_compass` -> `memory_burn_compass.png`
  - `identity_void_walker` -> `memory_burn_void_walker.png`
- Added keyword fallback selection so future/synthesized memories can still pick a fitting burn cut-in when their title includes sword, song, hand, name, compass, or void cues.
- Extended the existing battle action cut-in layer with an optional hold duration, then reused it for memory-burn cut-ins before the existing burn VFX and damage execution.
- Registered all 6 memory-burn cut-ins in the PauseMenu Artbook.

### Verification
- Visually inspected the new `memory_burn_compass.png` and `memory_burn_void_walker.png` assets after copying them into the project.
- Verified all 6 memory-burn PNG files have valid PNG signatures.
- `git diff --check -- scenes/battle/battle_scene.gd scripts/ui/pause_menu.gd assets/cg/generated/memory_burn_compass.png assets/cg/generated/memory_burn_void_walker.png` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports the known ObjectDB/resource cleanup noise.
- Representative scene load passed:
  - `res://scenes/battle/battle_scene.tscn`

## S131 - 2026-06-21 (opening VN readability and first-map clarity)

### Done
- Fixed the opening immersion break where production/reference sheet CGs appeared during early dialogue:
  - replaced Chapter 1 `sheet_arrel_profile`, `sheet_arrel_elia_duo`, `sheet_elia_profile`, and `sheet_elia_memory_restoration` references with story-appropriate CGs in `ch1_prologue`, `ch1_void_beast`, `ch1_forest_walk`, and `chapter1_dialogue`.
  - changed the `ch1_stump2` fallback alias to use the generated Elia/Arrel story illustration instead of a reference sheet.
- Reduced overbearing VN presentation layers:
  - lowered the lower wash, focus glow, and vignette defaults,
  - added CG-specific presentation profiles so text plates, generated story CGs, and reference fallback art receive different overlay strength,
  - hid portraits when a full-scene generated dialogue/story CG is active so character art no longer stacks awkwardly over the illustration.
- Hardened VN close behavior:
  - `SceneFlow` now asks `VNScene` to clear its visual layers before queue-free,
  - `VNScene.prepare_for_close()` hides CG, portrait, text, choice, glitch, grain, and page-turn overlays immediately,
  - disconnected VN signals on exit to avoid stale callbacks after scene transitions.
- Improved first playable Rim Forest readability:
  - reduced stacked vignette, fog, depth gradient, and premium lens darkness,
  - raised ambient lighting and player fog-light radius/energy,
  - kept the forest mood while making the opening game screen easier to read after dialogue.

### Verification
- Confirmed the targeted Chapter 1 files no longer reference the immersion-breaking sheet CGs.
- UTF-8 JSON parse passed for all edited Chapter 1 dialogue/VN files.
- `git diff --check` passed for all edited files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene loads passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/maps/rim_forest.tscn`
- Godot still reports the existing headless shutdown ObjectDB/resource cleanup noise, with no parse/script failure tied to this patch.

## S132 - 2026-06-22 (large graphics pass: story CG cleanup and map lens rebalance)

### Done
- Removed remaining runtime `res://assets/cg/game_image/sheet_*` CG usage from dialogue and major presentation fallbacks:
  - replaced late Chapter 2, 4, 5, 6, 7, 8, 10, and Epilogue sheet-CG beats with generated story/dialogue/memory illustrations.
  - updated `DialogueBox` speaker-stage fallbacks for Arrel and Elia to use portrait assets instead of sheet-derived CG plates.
  - updated battle stage/pre-attack art to use `memory_burn_first_sword.png` instead of the old battle-ready sheet plate.
  - updated ending/gallery/chapter-title fallbacks to use generated memory/world-rewrite art where appropriate.
- Strengthened generated illustration coverage across the story:
  - Ch2 Elia concern now reuses the Malet memory-trade illustration.
  - Ch4 night/anchor beats now use `story_ch4_drift_anchor.png`; home-flashback uses `memory_burn_reaching_hand.png`.
  - Ch5 separation/stay/reunion beats now use `dialogue_ch5_elia_cliff_choice.png`; first-sword recall uses `memory_burn_first_sword.png`.
  - Ch6 Seam garden/night Elia beats now use Seam/world-rewrite art.
  - Ch7 trial/void-edge beats now use memory-burn and Sable echo-shell art.
  - Ch8 anchor/flashback beats now use Elia-anchor and memory-forest art.
  - Ch10/epilogue BL-07 beats now use `story_ch10_bl07_core_choice.png`.
- Rebalanced map graphics across major maps:
  - lowered stacked vignette/lens darkness on Belt Waystation, Colorless Waste, BL-07 Void, Crumbling Coast, Drift Shelter, Forgotten Forest, Seam Outskirts, The Seam, and Verdan Market.
  - raised ambient lighting enough for gameplay readability while preserving each biome's palette.
  - reduced atmospheric illustration opacity where it was competing with tile readability.
- Refreshed Artbook entries so previously sheet-derived showcase slots now point at generated memory/dialogue art.

### Verification
- Confirmed no runtime `res://assets/cg/game_image/sheet_*` references remain in `data/`, `scripts/`, or `scenes/`.
- Confirmed all referenced `res://assets/...` paths resolve to existing files.
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- `git diff --check` passed for the edited graphics/data files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene load sweep passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/battle/battle_scene.tscn`
  - `res://scenes/maps/rim_forest.tscn`
  - `res://scenes/maps/verdan_market.tscn`
  - `res://scenes/maps/belt_waystation.tscn`
  - `res://scenes/maps/drift_shelter.tscn`
  - `res://scenes/maps/crumbling_coast.tscn`
  - `res://scenes/maps/the_seam.tscn`
  - `res://scenes/maps/seam_outskirts.tscn`
  - `res://scenes/maps/forgotten_forest.tscn`
  - `res://scenes/maps/colorless_waste.tscn`
  - `res://scenes/maps/bl07_void.tscn`
- Godot still reports the existing headless shutdown ObjectDB/resource cleanup noise and a few pre-existing anchor layout warnings on late-game maps, with no script or parse failure tied to this patch.

## S133 - 2026-06-22 (illustration overlap fix and Korean opening patch)

### Done
- Fixed generated illustration overlap during gameplay:
  - forced `MapEffects.add_illustration_atmosphere()` onto a background-only CanvasLayer (`layer <= -20`),
  - lowered atmospheric illustration opacity and removed the full-screen shade/readability overlays that were competing with the playable map,
  - changed `WorldRewriteDirector` memory/rewrite illustration flashes from full-screen overlays into a small, short-lived right-side echo card below HUD/UI layers.
- Added Korean-first localization behavior:
  - default locale is now Korean for this local build,
  - existing settings without the new Korean patch marker are migrated to `ko`,
  - dialogue/VN UI now resolves speaker names, text, narration, system logs, choices, choice hints, effects, and memory-distorted lines through localized fields.
- Korean-patched the immediate Chapter 1 opening flow:
  - `ch1_prologue`,
  - `ch1_forest_walk`,
  - `ch1_void_beast`,
  - the matching early `chapter1_dialogue` blocks for the legacy DialogueManager route.

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- `git diff --check` passed for the edited overlap/localization files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene loads passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/maps/rim_forest.tscn`
  - `res://scenes/maps/verdan_market.tscn`
  - `res://scenes/battle/battle_scene.tscn`
- Godot still reports the existing headless shutdown ObjectDB/resource cleanup noise, plus the known CanvasItem RID cleanup warning on map loads, with no parse/script failure tied to this patch.

## S134 - 2026-06-23 (gameplay stability sweep and combat flow upgrade)

### Done
- Hardened battle entry from VN/SceneFlow:
  - `goto_battle` now accepts string enemy IDs, dictionaries, or full `BattleManager.Enemy` objects,
  - added built-in enemy presets for early/story combat IDs so VN-driven battles do not fail silently,
  - `SceneFlow` now hands off to the battle scene after starting the requested battle.
- Fixed combat-result flow blockers:
  - corrected Kairos and Seam trial callbacks so they check `BattleState.VICTORY` instead of treating enum results like booleans,
  - stopped void-corruption turn ticks from continuing into ally/enemy turns after they already caused victory/defeat,
  - connected turn-limit corruption defeat to the new Last Stand safety instead of hard-failing immediately.
- Added a gameplay upgrade: Last Stand Resonance.
  - once per battle, lethal or critical-low HP pressure can leave Arrel at 1 HP,
  - grants a guarded next hit, Limit gain, resonance/momentum gain, toast feedback, and survivor achievement progress,
  - designed as a dramatic anti-frustration system rather than a free heal loop.
- Fixed VN choice softlock risk:
  - if all conditional choices are filtered out, the VN now shows a localized "continue" fallback instead of leaving the player stuck.
- Expanded Korean-first UX coverage:
  - battle objective titles/descriptions and reward labels,
  - resonance/momentum labels,
  - exploration HUD map names, chapter text, memory/grains/items/pulse labels, location cards, and main quest fallbacks.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene loads passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/maps/rim_forest.tscn`
  - `res://scenes/battle/battle_scene.tscn`
- Confirmed edited Korean strings remain valid UTF-8.
- Search pass found no remaining battle-ended callback that treats `BattleState` enum results as plain booleans.
- Godot still reports the existing headless shutdown ObjectDB/resource cleanup noise, plus the known CanvasItem RID cleanup warning on map loads, with no parse/script failure tied to this patch.

## S135 - 2026-06-25 (GPT image cinematic integration pass)

### Done
- Generated and integrated 6 new GPT image assets into the project:
  - `cinematic_last_stand_resonance.png` — Last Stand battle cut-in.
  - `chapter_splash_rim_forest.png` — Ch1 opening/VN, Rim Forest chapter card, HUD location art, Rim Forest combat backdrop.
  - `chapter_splash_verdan_market.png` — Ch2 chapter card, HUD location art, Verdan combat backdrop.
  - `chapter_splash_the_seam.png` — Ch6 chapter card, HUD location art, The Seam combat backdrop.
  - `cinematic_kairos_watcher_confrontation.png` — Kairos boss/enemy stage art.
  - `memory_compass_resonance_cinematic.png` — Colorless Waste chapter/HUD art, Kairos battle backdrop, Memory Resonance reward CG.
- Ran a Godot import pass so all 6 generated PNG files have matching `.import` metadata.
- Upgraded battle presentation:
  - added `BattleManager.last_stand_resonance` signal,
  - connected `battle_scene.gd` to play the new Last Stand cut-in, blue flash, screen shake, shield pulse, and layered SFX,
  - updated Kairos image resolution to prefer the new GPT boss confrontation art.
- Upgraded exploration/chapter presentation:
  - `ExplorationHUD.MAP_ART` now uses generated GPT art for Rim Forest, Verdan Market, The Seam, and Colorless Waste.
  - `MapEffects.show_chapter_title()` now uses generated GPT art for chapters 1, 2, 6, and 9.
  - Rim Forest atmosphere and random encounter battle backgrounds now use the new Ch1 splash art.
- Upgraded story/VN presentation:
  - `VNScene` default/fallback forest CG now points to the generated Rim Forest splash.
  - `ch1_prologue` first visual beat and legacy `chapter1_dialogue` forest CG beats now use the new Rim Forest art.
  - generated chapter/cinematic/system images are treated as story CGs for more balanced wash/glow/vignette handling.
- Upgraded Memory Resonance feedback:
  - field resonance now briefly opens the Memory Compass CG with localized caption text before applying the reward.

### Verification
- Confirmed all 6 new generated assets and their `.import` files exist in `assets/cg/generated/`.
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Godot import pass successfully scanned and reimported the 6 new PNG assets.
- Representative scene loads passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/battle/battle_scene.tscn`
  - `res://scenes/maps/rim_forest.tscn`
  - `res://scenes/maps/verdan_market.tscn`
  - `res://scenes/maps/the_seam.tscn`
  - `res://scenes/maps/colorless_waste.tscn`
- Existing headless shutdown ObjectDB/resource cleanup noise remains. The Godot import run also printed pre-existing VFX plugin/autoload warnings and shaderV duplicate UID warnings; the asset import itself completed and subsequent headless boot passed.

## S136 - 2026-06-26 (GPT image chapter splash expansion pass)

### Done
- Generated and integrated 6 new GPT image chapter splash assets:
  - `chapter_splash_belt_waystation.png` - Ch3 Belt Waystation art.
  - `chapter_splash_drift_shelter.png` - Ch4 Drift Shelter art.
  - `chapter_splash_crumbling_coast.png` - Ch5 Crumbling Coast art.
  - `chapter_splash_seam_outskirts.png` - Ch7 Seam Outskirts / threshold art.
  - `chapter_splash_forgotten_forest.png` - Ch8 Forgotten Forest art.
  - `chapter_splash_bl07_void.png` - Ch10 BL-07 Void core art.
- Ran a Godot import pass so all 6 new generated PNG files have matching `.import` metadata.
- Expanded chapter and exploration presentation:
  - `MapEffects.show_chapter_title()` now uses generated GPT art for chapters 3, 4, 5, 7, 8, and 10.
  - `ExplorationHUD.MAP_ART` now uses generated GPT art for Belt Waystation, Drift Shelter, Crumbling Coast, Seam Outskirts, Forgotten Forest, and BL-07 Void.
  - map atmosphere plates for those regions now use the matching generated art at low opacity so they stay behind gameplay.
- Expanded battle presentation:
  - battle scene return-map background resolution now covers the new generated chapter splash set.
  - The Seam, Crumbling Coast, and BL-07 explicit battle starts/random encounters now prefer generated art instead of reused generic environment plates.
  - Shade Sentinel now uses the generated The Seam backdrop through its preset.
- Expanded story/VN and gallery coverage:
  - late Ch2, Ch3, Ch5, Ch6, Ch7, Ch10, epilogue, and selected VN transition beats now use the generated chapter art where the old image was only acting as a generic environment plate.
  - ending gallery images for Preservation, Seam, and Hollow now point to generated chapter art.
  - PauseMenu artbook entries now expose the new generated chapter splash set.

### Verification
- Confirmed all 6 new generated assets and their `.import` files exist in `assets/cg/generated/`.
- Confirmed all referenced `res://assets/...` paths in `data/`, `scripts/`, and `scenes/` resolve to existing files.
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 import pass scanned and reimported the 6 new PNG assets.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene load script passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/battle/battle_scene.tscn`
  - `res://scenes/maps/belt_waystation.tscn`
  - `res://scenes/maps/drift_shelter.tscn`
  - `res://scenes/maps/crumbling_coast.tscn`
  - `res://scenes/maps/the_seam.tscn`
  - `res://scenes/maps/seam_outskirts.tscn`
  - `res://scenes/maps/forgotten_forest.tscn`
  - `res://scenes/maps/colorless_waste.tscn`
  - `res://scenes/maps/bl07_void.tscn`
- Existing headless shutdown ObjectDB/resource cleanup noise remains. The Godot import run also printed the pre-existing VFX plugin/autoload and ShaderV duplicate UID warnings; import and subsequent boot/scene-load checks passed.

## S137 - 2026-06-27 (GPT image UI/UX archive interface pass)

### Done
- Generated and integrated 4 new text-free GPT-image interface backdrops under `assets/cg/generated/`:
  - `ui_codex_archive_backdrop.png` - split bestiary / memory-record archive environment.
  - `ui_memory_constellation_backdrop.png` - mnemonic observatory with subdued orbital guides.
  - `ui_achievements_chronicle_backdrop.png` - memorial ledger wall for achievement records.
  - `ui_ending_gallery_backdrop.png` - six-niche ruined reliquary for branching endings.
- Upgraded Codex presentation and information hierarchy:
  - added the generated archive backdrop and a lighter translucent content shell,
  - added localized context copy plus live creature/memory record counts,
  - added a restrained fade/slide entrance and title typography treatment.
- Upgraded Memory Constellation UX:
  - replaced the flat fill with the generated observatory backdrop,
  - localized the title, subtitle, close action, and legend,
  - made grade radii adapt to the actual 1280x720 canvas so outer nodes remain on-screen,
  - staggered grade phases so sparse memory sets no longer collapse into one vertical line,
  - fixed the bottom legend anchor and styled the close control consistently.
- Upgraded Achievements and Ending Gallery:
  - layered generated archive art behind translucent functional panels,
  - strengthened titles, subtitles, progress context, card boundaries, and monochrome milestone glyphs,
  - expanded ending cards to align with the six generated gallery niches,
  - added shared modal entrance motion for a more intentional screen transition.
- Registered all 4 new UI backdrops in the PauseMenu Artbook.

### Verification
- Visually captured and inspected Codex, Memory Constellation, Achievements, and Ending Gallery at the project viewport size (1280x720); generated art, real text, scroll areas, cards, and node graph remained legible.
- Ran a complete Godot `--import` pass; all 4 PNG files received matching `.import` metadata.
- `git diff --check` passed for the edited UI scripts and session log; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene loads passed for `res://scenes/maps/verdan_market.tscn` and `res://scenes/main/vn_host.tscn`.
- Existing headless shutdown ObjectDB/resource cleanup noise and pre-existing VFX/ShaderV plugin warnings remain unchanged.

## S138 - 2026-06-27 (GPT image character battle cinematic pass)

### Done
- Used the built-in GPT image generator with existing character sheets, portraits, chapter art, and combat CGs as visual references to create 7 new text-free 16:9 battle cinematics:
  - `cinematic_arrel_memory_cascade.png` - Arrel's Memory Cascade ultimate.
  - `cinematic_elia_anchor_pulse.png` - Elia repairing fractured memory geometry.
  - `cinematic_sable_echo_strike.png` - Sable's shadow-crossing support action.
  - `cinematic_tobias_record_ward.png` - Tobias converting records into a battlefield ward.
  - `cinematic_void_beast_memory_devour.png` - Void Beast tearing luminous identity fragments from the field.
  - `cinematic_shade_sentinel_phase2.png` - Shade Sentinel's second-crown phase transformation.
  - `cinematic_kairos_authority_edit.png` - Kairos calmly cutting and rearranging recorded reality.
- Expanded the reusable battle cut-in layer instead of adding a parallel cinematic system:
  - Memory Cascade now receives its own high-opacity ultimate cut-in before the existing chromatic burst and impact VFX.
  - all 4 Elia techniques map to context-appropriate cinematics through `ally_action`.
  - Sable and Tobias support actions now trigger character-specific cut-ins and action labels.
  - Void Beast attacks use the new memory-devour art; Shade Sentinel and Kairos use unique phase-two plates.
  - action cut-in tweens now continue during the existing paused boss phase-transition beat.
- Promoted the new Void Beast and Shade Sentinel art into their encounter presets and name-based fallback resolver.
- Added all 7 cinematics to the PauseMenu Artbook with role-specific descriptions.

### Verification
- Ran Godot 4.6.2 `--import`; all 7 PNG files were scanned, reimported, and received matching `.import` metadata.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for `res://scenes/battle/battle_scene.tscn` and `res://scenes/maps/verdan_market.tscn`.
- A temporary runtime harness started a Shade Sentinel battle and exercised Elia, Sable, Tobias, enemy attack, phase-two, and Memory Cascade cut-in paths without runtime errors; the harness was removed afterward.
- Existing forced headless shutdown ObjectDB/resource cleanup noise remains. The import pass also printed the pre-existing VFX plugin/autoload and ShaderV duplicate UID warnings; asset import and subsequent runtime checks passed.

## S139 - 2026-06-27 (GPT image Act I first-impression overhaul)

### Done
- Used the built-in GPT image generator with the current Arrel/Elia turnarounds and established Rim Forest/battle art as references to create 9 new text-free 16:9 Act I story images:
  - `story_ch1_opening_aftermath.png` - Arrel over the dissolving remains of the opening Void Beast.
  - `story_ch1_elia_reunion.png` - the lantern-lit reunion with both protagonists' current designs.
  - `story_ch1_ash_rain_touch.png` - the first ash flake dissolving against Arrel's cheek.
  - `story_ch1_camp_humming.png` - the fireless night camp and Elia's broken memory-song thread.
  - `story_ch1_twisted_forest_path.png` - the first playable route beneath rib-like roots.
  - `story_ch1_memory_shrine.png` - the petrified stump, cairn, and anonymous residual echoes.
  - `story_ch1_void_beast_emergence.png` - the first boss uncoiling from the canopy.
  - `story_ch1_first_burn_strike.png` - the pale-gold "idea of heat" memory-burn cut.
  - `story_ch1_green_tree_dawn.png` - the Chapter 1 ending reveal of one living tree.
- Rebuilt the first-impression flow from New Game through the end of Chapter 1:
  - New Game now opens directly on the new battle aftermath instead of an empty environment plate.
  - prologue, reunion, Ash Rain, branching camp, dawn, forest-walk, optional shrine/stump, first boss, first burn, and chapter-ending checkpoints now receive scene-specific art.
  - preserved all existing choice targets and progression logic while changing only CG fields and aliases.
- Extended the same art direction into playable presentation:
  - Rim Forest exploration atmosphere and HUD location art now use the new twisted-path plate.
  - Rim Forest combat backgrounds use the same environment plate and the existing S138 Void Beast cinematic.
  - the legacy non-VN Chapter 1 dialogue path received matching opening, reunion, Ash Rain, camp, walking, stump, and shrine art.
- Corrected the most visible early-character inconsistency by anchoring new story CGs to silver-haired Arrel and honey-blonde bob-haired Elia from the current turnaround sheets.
- Registered all 9 Act I images in the PauseMenu Artbook.

### Verification
- UTF-8 JSON parse passed for all 16 `data/**/*.json` files after the CG rewiring.
- Godot 4.6.2 `--import` scanned and reimported all 9 PNG files and generated matching `.import` metadata.
- A temporary runtime harness loaded 9 representative early-game VN checkpoints through the real `SceneFlow` and `VNScene` CG resolver; every resolved path existed and loaded without runtime errors. The harness was removed afterward.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for `res://scenes/maps/rim_forest.tscn` and `res://scenes/battle/battle_scene.tscn`.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Existing forced headless shutdown ObjectDB/resource cleanup noise remains. The import pass also printed the pre-existing VFX plugin/autoload and ShaderV duplicate UID warnings; import and subsequent runtime checks passed.

## S140 - 2026-06-27 (GPT image typography and archive interface refinement)

### Done
- Used the built-in GPT image generator and the established S137-S139 blackened-silver / midnight-blue / pale-gold art direction to create 3 text-free chroma-key UI overlays:
  - `ui_vn_memory_frame_overlay.png` - a compact lower-screen dialogue frame with a dedicated speaker tab.
  - `ui_vn_choice_archive_overlay.png` - a central memory-record choice frame with three restrained decision bands.
  - `ui_exploration_archive_overlay.png` - a compact top-left exploration HUD archive frame.
- Converted all three generated assets to transparent PNGs with the imagegen skill's chroma-key helper, preserving soft anti-aliased metal edges without green spill.
- Reworked typography roles across the interface:
  - narrative dialogue, speaker names, and choice prose retain the literary serif stack,
  - controls, hints, continue prompts, HUD data, and floating feedback now use a dedicated sans-serif UI stack,
  - increased VN body size/line spacing and tightened HUD size hierarchy for Korean and English readability.
- Integrated the generated overlays into the live UI:
  - VN dialogue and choice frames swap automatically with dialogue state,
  - speaker-name placement now aligns with the generated frame tab,
  - exploration HUD frame participates in the existing slide-in animation and encloses the widened information panel.
- Preserved the legacy dialogue path while applying the same improved prose spacing, UI prompt font, and serif choice treatment.

### Verification
- Ran a complete Godot 4.6.2 `--import` pass; all 3 PNG files were scanned, reimported, and received matching `.import` metadata.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for `res://scenes/main/vn_host.tscn` and `res://scenes/maps/rim_forest.tscn`.
- Captured and visually inspected live 1280x720 renders for Act I dialogue, the first major VN choice, and Rim Forest exploration HUD; text remained legible and generated frame geometry aligned after final speaker-tab/HUD adjustments.
- Existing forced shutdown ObjectDB/resource cleanup noise, CanvasItem RID cleanup warning, and pre-existing VFX/ShaderV import warnings remain unchanged.

## S141 - 2026-06-28 (GPT image Chapter 2 narrative bridge pass)

### Done
- Committed the completed S138-S140 presentation overhaul as `ac6e7d5` (`feat(presentation): overhaul Act I visuals`) before starting the next art pass.
- Audited all current story CG references and identified Chapter 2 as the clearest illustration-density gap after the Act I overhaul.
- Used the built-in GPT image generator with the current Arrel/Elia duo sheet, Malet portrait, Kairos cinematic, Verdan splash, and existing cellar CG as explicit identity/style references to create 6 new text-free 16:9 story images:
  - `story_ch2_verdan_gate.png` - Arrel and Elia stopped at the southern Bureau checkpoint above Verdan.
  - `story_ch2_memory_market.png` - bottled affection, grief, and identity offered across the market stalls.
  - `story_ch2_old_burner.png` - the nameless old man as a quiet mirror of Arrel's possible future.
  - `story_ch2_malet_cellar.png` - the corrected three-character negotiation with current Arrel, Elia, and unhooded Malet designs.
  - `story_ch2_first_sword_extraction.png` - the first-sword memory leaving Arrel as a pale filament.
  - `story_ch2_kairos_warning.png` - Kairos revealed as a cold Bureau projection while Elia grips her cup.
- Rewired the Chapter 2 visual flow so the new art follows the emotional sequence from arrival and market horror through Malet's bargain, extraction, and the four-day Kairos threat.
- Replaced the most visible legacy character-inconsistent Chapter 2 CG usages with the new identity-anchored Malet cellar art.
- Registered all 6 images in the PauseMenu Artbook.

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- Confirmed every new `res://assets/...` reference resolves to an existing project file.
- Ran Godot 4.6.2 `--import`; all 6 PNG files were scanned, imported, and received matching `.import` metadata.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for `res://scenes/main/vn_host.tscn` and `res://scenes/maps/verdan_market.tscn`.
- Captured and visually inspected live 1280x720 VN renders for the Verdan guard checkpoint and memory-market entry; character focal points remain clear above the S140 dialogue frame.
- Existing forced shutdown ObjectDB/resource cleanup noise, CanvasItem RID cleanup warning, and pre-existing VFX/ShaderV import warnings remain unchanged.

## S142 - 2026-06-28 (GPT image midgame illustration continuity pass)

### Done
- Audited Chapter 3-6 dialogue groups and CG references after the Act I/Chapter 2 passes, prioritizing long text sequences with no illustration and repeated generic environment plates.
- Used the built-in GPT image generator with the current Arrel/Elia duo sheet, Tobias turnaround, Sable cut-in, Kairos cinematic, and matching chapter environments as explicit identity/style references to create 8 new text-free 16:9 story illustrations:
  - `story_ch3_tobias_waystation.png` - Tobias startled at his paper-covered waystation desk.
  - `story_ch3_tobias_joins.png` - the first three-person party image on the Belt road.
  - `story_ch4_reading_deterioration.png` - Arrel discovers the Blank Book is becoming unreadable to him.
  - `story_ch4_night_counting_losses.png` - Elia admits she has counted eleven involuntary micro-memory losses.
  - `story_ch5_kairos_ridge_sighting.png` - Kairos watches from the distant coast ridge without giving chase.
  - `story_ch5_seam_first_light.png` - the party meets the current short-silver-haired Sable as color returns at The Seam.
  - `story_ch6_sable_briefing.png` - Sable maps BL-07 and the Shade Sentinel in her workshop.
  - `story_ch6_stars_forgetting.png` - Arrel and Elia sit above The Seam while the sky loses its stars.
- Rewired Chapter 3-6 story beats so the illustrations form a continuous visual arc: party formation, cognitive deterioration, quiet loss, Kairos surveillance, sanctuary arrival, mission briefing, and the BL-07 eve.
- Updated Sable's outdated Chapter 5 entrance description from short dark hair to short silver hair to match the newer battle/story art direction.
- Preserved solo Seam arrival and hidden-garden branches on their original neutral artwork instead of leaking group-specific imagery into them.
- Registered all 8 illustrations in the PauseMenu Artbook.

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- Confirmed every referenced `res://assets/...` path resolves to an existing project file.
- Ran Godot 4.6.2 `--import`; all 8 PNG files were scanned, imported, and received matching `.import` metadata.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for Belt Waystation, Drift Shelter, Crumbling Coast, and The Seam.
- Captured and visually inspected live 1280x720 legacy-dialogue renders for Tobias's introduction and Elia's stars-forgetting conversation; focal subjects remain clear around the dialogue presentation layers.
- Existing forced shutdown ObjectDB/resource cleanup noise, CanvasItem RID cleanup warning, and pre-existing VFX/ShaderV import warnings remain unchanged.

## S143 - 2026-06-28 (GPT image late-game narrative illustration pass)

### Done
- Audited the Chapter 7-10 dialogue groups against the current CG tree and targeted repeated environment plates, generic item art, and unillustrated late-game turning points.
- Used the built-in GPT image generator in `stylized-concept` mode with the latest Arrel/Elia duo, Tobias, short-silver-haired Sable, Kairos, and chapter environment anchors to create 13 new text-free late-game story illustrations:
  - `story_ch7_fading_names_monument.png` - the first expedition team's names dissolving from the Seam Outskirts memorial.
  - `story_ch7_sable_confession.png` - Sable admitting why she deserted Authority Memory Disposal.
  - `story_ch7_echo_shell_whispers.png` - Arrel hearing fragments of consumed lives through the Echo Shell.
  - `story_ch8_forest_crossing.png` - the four-person party entering the memory-parasitic forest.
  - `story_ch8_ghost_child.png` - the child remnant cupping the absence of a forgotten name.
  - `story_ch8_ring_cairn.png` - Tobias discovering BL-07's accelerating consumption rings.
  - `story_ch9_kairos_confrontation.png` - the full party facing Kairos in the Colorless Waste.
  - `story_ch9_first_void_memory.png` - Arrel touching the last thought of the first Void Hole's witness.
  - `story_ch9_bl07_threshold.png` - BL-07 appearing as a door more real than the Waste.
  - `story_ch10_void_echoes.png` - the Void surrounding Arrel and Elia with fragments of almost-memory.
  - `story_ch10_orphan_memories.png` - the party finding crystallized lives kept by BL-07.
  - `story_ch10_seal_complete.png` - Arrel surviving the seal without recognition or identity.
  - `story_ch10_seal_refused.png` - Arrel pulling back from the final burn and choosing borrowed time.
- Reworked the Kairos confrontation once with an identity-preserving correction so Sable retains her current chin-length silver hair instead of drifting toward the older long-haired design.
- Connected all 13 images to their matching Chapter 7-10 dialogue entry points, replacing generic object/environment artwork only where the new scene-specific CG is narratively exact.
- Preserved the existing Chapter 10 core-choice image and pre-seal Void architecture so the new complete/refused ending plates read as distinct consequences rather than replacements for the decision itself.
- Registered the complete late-game set in the PauseMenu Artbook as the Act III visual arc.

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json` files and every referenced `res://assets/...` path resolved.
- Godot 4.6.2 `--import` scanned and imported all 13 PNG files and generated matching `.import` metadata.
- Godot 4.6.2 headless project boot passed without script or parse errors.
- Direct scene loads passed for `forgotten_forest.tscn`, `colorless_waste.tscn`, and `bl07_void.tscn`.
- Captured and visually inspected live 1280x720 standard-renderer dialogue compositions for Chapter 7 Echo Shell and Chapter 10 Void Echoes; focal faces remain clear above the dialogue presentation layers.
- `git diff --check` passed before the final log update; only normal CRLF working-copy warnings appeared.
- Existing forced shutdown ObjectDB/resource cleanup and CanvasItem RID warnings remain unchanged.

## S144 - 2026-06-29 (GPT image ending and epilogue atlas pass)

### Done
- Audited all 129 existing `assets/cg/**/*.png` files and every dialogue group's current `cg` references before selecting new scenes; exact-file duplicate detection also found no duplicate PNG hashes.
- Identified the six ending branches and two optional epilogue conversations as the highest-value non-overlapping gap because they still reused generic Seam, core-choice, placeholder rewrite, or item imagery.
- Used the built-in GPT image generator in `stylized-concept` mode with current Arrel, Elia, Tobias, Sable, ending, and Seam references to create 14 new text-free story illustrations:
  - `ending_zero_burn_canyon_watch.png` - the unnamed man watching colors he cannot name.
  - `ending_zero_burn_trying_name.png` - Arrel trying the lost name as a new choice.
  - `ending_preservation_return.png` - the party returning while BL-07 remains open.
  - `ending_preservation_building_hands.png` - hands that can build instead of burn.
  - `ending_ash_hollow_days.png` - correct answers coming from emotional absence.
  - `ending_ash_sunset_shell.png` - Arrel watching light fade without recognition.
  - `ending_seam_ordinary_moments.png` - small moments surviving the Void's appetite.
  - `ending_seam_impossible_garden.png` - a green shoot becoming the first credible answer.
  - `ending_tobias_night_press.png` - Tobias printing Ring Theory through the night.
  - `ending_tobias_twelve_archivists.png` - twelve copies escaping Authority suppression.
  - `ending_hollow_water.png` - Arrel forgetting the taste and meaning of water.
  - `ending_hollow_name_room.png` - one name echoing through an emptied life.
  - `epilogue_elia_collective_pattern.png` - Elia connecting private burns to shared-history loss.
  - `epilogue_sable_eastern_settlement.png` - Sable pointing toward the next settlement in danger.
- Replaced every generic epilogue CG at its exact narrative beat and added a second visual transition inside each major ending, without replacing the Chapter 10 choice or immediate-consequence art.
- Corrected the Zero Burn prose from outdated silver-haired Elia to her current honey-blonde design.
- Updated all six Ending Gallery thumbnails and descriptions to use branch-specific art and accurate outcome text.
- Added all 14 images to the PauseMenu Artbook.
- Added `ILLUSTRATION_CATALOG.md` with the verified 143-CG baseline, non-overlap rules, S144 scene-to-file mapping, and a role-based path toward the long-term 1,000-image target.

### Verification
- UTF-8 JSON parsing passed for all `data/**/*.json` files and all referenced project asset paths resolved.
- Verified `TOTAL_CG=143`, `NEW_SET=14`, and no exact SHA-256 duplicate PNG files under `assets/cg/`.
- Godot 4.6.2 `--import` scanned and imported all 14 new PNG files and generated matching `.import` metadata.
- Godot 4.6.2 headless project boot passed without script or parse errors.
- Direct scene load passed for `res://scenes/maps/the_seam.tscn`.
- Captured and visually inspected live 1280x720 standard-renderer dialogue compositions for the Zero Burn and Seam endings; subjects and story details remain legible around the narration/dialogue presentation layers.
- `git diff --check` passed before the final log update; only normal CRLF working-copy warnings appeared.
- Existing forced shutdown ObjectDB/resource cleanup and CanvasItem RID warnings remain unchanged.

## S145 - 2026-06-29 (GPT image Chapter 7-9 exploration and choice pass)

### Done
- Continued directly from the ending atlas by re-auditing all 143 existing CG files and the unillustrated Chapter 7-9 dialogue groups, selecting 16 scenes with distinct story, location, cast, and camera purposes.
- Used the built-in GPT image generator in `stylized-concept` mode with the current Arrel, Elia, Tobias, Sable, Kairos, and chapter environment references to create 16 new text-free 16:9 illustrations:
  - `story_ch7_controlled_burn_trial.png` - Sable testing whether Arrel can survive a deliberate burn.
  - `story_ch7_last_field_preparations.png` - the four travelers preparing below the final ridge.
  - `story_ch7_paper_forgetting_ink.png` - paper losing the concept of holding a written mark.
  - `story_ch7_crossing_the_ridgeline.png` - The Seam's last color falling behind the party.
  - `story_ch8_eighteenth_ring.png` - Tobias tracing the forest's organized consumption rings.
  - `story_ch8_whispers_as_bait.png` - false familiar faces gathering in the bark around the real party.
  - `story_ch8_white_stone_shelter.png` - Sable touching the memory-null cairn while the others give her space.
  - `story_ch8_end_of_color.png` - the forest ending abruptly at the Achromatic Waste.
  - `story_ch8_forgotten_moss.png` - matter that remembers neither growth nor decay.
  - `story_ch8_ghost_mother.png` - a mother-shaped remnant cradling an absence.
  - `story_ch8_parasitic_heart.png` - the party confronting the immense breathing knot at the forest's center.
  - `story_ch9_human_chain.png` - the party holding one another against direction-erasing wind.
  - `story_ch9_name_under_pull.png` - BL-07 reeling Arrel inward along a memory tether.
  - `story_ch9_kairos_withdrawal.png` - Kairos retreating through fractured Authority records.
  - `story_ch9_memory_depth_markers.png` - compressed lives forming towering depth markers in the Waste.
  - `story_ch9_final_colorless_view.png` - Arrel and Elia looking back across a world reduced to existence alone.
- Connected every image to its exact Chapter 7, 8, or 9 dialogue beat and registered all 16 as distinct PauseMenu Artbook entries.
- Extended `ILLUSTRATION_CATALOG.md` with the full S145 dialogue-to-asset mapping and updated the verified project baseline from 143 to 159 CG PNG files.

### Verification
- UTF-8 JSON parsing passed for `chapter7_dialogue.json`, `chapter8_dialogue.json`, and `chapter9_dialogue.json`; all 16 new dialogue references and all 16 Artbook references resolve to existing assets.
- Verified `TOTAL_CG=159`, `NEW_SET=16`, and no exact SHA-256 duplicate PNG files under `assets/cg/`.
- Godot 4.6.2 `--import` scanned and imported all 16 new PNG files and generated matching `.import` metadata.
- Godot 4.6.2 headless project boot passed without script or parse errors.
- Direct scene loads passed for `seam_outskirts.tscn`, `forgotten_forest.tscn`, and `colorless_waste.tscn`.
- Captured and visually inspected live 1280x720 standard-renderer dialogue compositions for Chapter 8 `forest_whispers` and Chapter 9 `depth_markers`; subjects, environmental storytelling, and the lower-screen UI safety area remain clear.
- Existing forced shutdown ObjectDB/resource cleanup noise, two pre-existing anchor warnings, and the VFX/ShaderV import warnings remain unchanged.

## S146 - 2026-06-29 (Gameplay + story: The Weave — 7th ending & memory-gated dialogue)

### Done
- Built a cohesive gameplay-and-story upgrade that reinforces the burn-vs-keep core, designed so the new beats have ready illustration slots for the ongoing GPT/codex image work.
- **Memory-driven dialogue engine** (`scripts/systems/dialogue_manager.gd`): brought the NPC dialogue runner to parity with the VN SceneFlow system.
  - Choice/line gating: `requires_memory_intact`, `requires_memory_gone`, `requires_flag`, `requires_not_flag`, `requires_weave`. Failing lines are skipped; failing choices are filtered out, and `select_choice` now operates on the filtered list (`_current_choices`) so indices stay correct.
  - Memory Leverage: `cost_memory` (semantic burn with a "Memory spent" toast).
  - Reward parity: `add_item` / `add_item_count` / `heal_player`, plus data-driven `record_ending` and `set_flag` on lines.
  - Legacy `requires_memory` + `burned_text` text-swap behavior is preserved untouched.
- **The Weave — 7th ending** (a preservation/"true" path that rewards playing against the burn grain):
  - `scripts/systems/memory_manager.gd`: added `is_intact()`, anchor constants (`WEAVE_PRIMARY` = the name, `WEAVE_SECONDARY` = sword/Elia-gesture/anchor-hands/Sable-trust), `intact_anchor_count()`, and `weave_unlocked()` (name intact + fewer than 4 total burns + 3 of 4 secondary anchors intact).
  - `data/chapter10_dialogue.json`: added two `requires_weave` Elia/Arrel hint lines and a gated third seal choice (`seal_weave`), plus the full `seal_weave` resolution group.
  - `data/epilogue_dialogue.json`: added `epilogue_weave` (Sable realizes preservation can seal a hole; colors return to The Seam).
  - `scenes/maps/bl07_void.gd`: `_on_seal_decision_ended` now routes `seal_weave` to a new warm-light `_execute_weave()` / `_on_weave_complete()` that keeps the name.
  - `scenes/maps/the_seam.gd`: highest-priority `epilogue_weave` branch + `record_ending("weave")` / `unlock("ending_weave")`.
  - Registered the ending everywhere: `GameManager.ENDING_DATA["weave"]`, PauseMenu ending gallery id list, `AchievementManager` `ending_weave`, and two `StoryJournal` entries (major event + choice log).
- Emergent tie-in (no new code): accepting Malet's early deal burns `identity_first_sword`, which lowers the anchor count — so taking shortcuts quietly forecloses the Weave path.

### New illustration slots (graceful text-only fallback until generated — for codex)
- `story_ch10_seal_weave.png` — Arrel reaching for every kept memory at once, not just the name.
- `story_ch10_seal_weave_fire.png` — the seal-fire braided from every color the Seam ever bled.
- `story_ch10_seal_weave_after.png` — Arrel intact but thinner, anchored, the seal closed behind him.
- `ending_weave_sealed_gate.png` — the closed BL-07 gate at The Seam.
- `ending_weave_sable_ledger.png` — Sable's pattern ledger with the impossible new column.
- `ending_weave_anchor_hand.png` — the steady weight of the part of him now holding the door shut.
- `ending_weave_colors_return.png` — colors growing back over quiet stone (Ending Gallery thumbnail).

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json`; confirmed `seal_weave`, `epilogue_weave`, and the three-option weave-gated seal choice are present and correctly flagged.
- Godot 4.6.2 `--headless --import` completed; the only error is the pre-existing `addons/vfx_lib/plugin.gd:7` dialog-parent noise.
- Godot 4.6.2 headless boot (`--quit-after 3`): **0** SCRIPT ERROR / Parse Error lines across all edited scripts.
- Verified the dialogue UI feeds the filtered choice index straight into `select_choice`, so gated choices map correctly.
- Missing new CG paths are guarded by `ResourceLoader.exists()` in `dialogue_box.gd`, so the build runs today and the art drops in later.

### Balance note (next pass)
- The Weave gate (<4 burns + 3/4 anchors intact by Ch10) is intentionally a hard "preservation run." Needs a live playthrough to confirm it's reachable without trivializing combat; tune `WEAVE_MAX_BURNS` / anchor threshold if it proves too strict.

## S147 - 2026-06-30 (Code audit + GPT image optional-story and The Weave illustration pass)

### Done
- Reviewed the complete dirty worktree after the S146 Claude Code handoff instead of treating the new Weave implementation as isolated code.
- Fixed a boss-rush cleanup race found during the wider audit:
  - boss-rush progression now listens to `battle_cleanup_finished` rather than the early `battle_ended` signal,
  - final victory can no longer overwrite the menu state with exploration after asynchronous reward cleanup,
  - boss-rush defeat returns cleanly to the title instead of falling through to the normal game-over route.
- Hardened dialogue, VN, save, and memory state boundaries:
  - rejected negative/stale choice indices,
  - clamped VN resume indices,
  - guarded valid-JSON-but-wrong-schema save previews,
  - made memory import tolerate malformed/legacy records and rebuild an empty pool safely,
  - repaired the debug store-stat dialogue count to traverse the real `dialogues` dictionary.
- Reviewed S146's seven-ending registration and fixed the stale four-ending achievement table:
  - added Preservation, Tobias, and Hollow achievement definitions,
  - corrected Zero Burn's achievement ID,
  - updated Every Path to require all seven real endings.
- Removed the repeated Control anchor warning in `WorldRewriteDirector` by using a full-rect anchors-and-offsets preset without assigning a conflicting explicit size.
- Used the built-in GPT image generator with current character/environment references to create 11 clean, text-free 16:9 story CGs:
  - `story_ch1_echo_fragment.png`
  - `story_ch1_ashen_figure_restored.png`
  - `story_ch2_sump_breathing_walls.png`
  - `story_ch2_nervous_trader_ledger.png`
  - `story_ch10_seal_weave.png`
  - `story_ch10_seal_weave_fire.png`
  - `story_ch10_seal_weave_after.png`
  - `ending_weave_sealed_gate.png`
  - `ending_weave_sable_ledger.png`
  - `ending_weave_anchor_hand.png`
  - `ending_weave_colors_return.png`
- Every prompt explicitly excluded film/photo grain, paper/canvas texture, speckle and color noise, dithering, compression artifacts, chromatic aberration, dirty overlays, and oversharpening.
- Rejected and regenerated the first Colors Return image because Sable drifted into a brown-haired male silhouette; the shipped image restores her current chin-length silver-haired identity.
- Connected the four optional-story images to their exact Chapter 1/2 dialogue beats and filled all seven image slots Claude prepared for `seal_weave` / `epilogue_weave`.
- Registered all 11 images in the PauseMenu Artbook and updated `ILLUSTRATION_CATALOG.md` to the verified 170-CG baseline.

### Verification
- Purpose-built boss-rush cleanup test passed for both victory and defeat paths (`BOSS_RUSH_CLEANUP_TEST_OK`).
- Purpose-built Weave reachability test passed after applying real Chapter 3-10 erosion:
  - pristine preservation state exposes three seal choices,
  - one secondary anchor burn still satisfies 3/4,
  - two secondary anchor burns close the Weave path and reduce the choice list to two (`WEAVE_PATH_TEST_OK`).
- UTF-8 JSON parsing passed for all `data/**/*.json`; 728 scanned `res://` references resolved with zero missing files.
- Verified `TOTAL_CG=170`, all 11 new images are RGB 1672x941, all have matching `.import` metadata, and there are zero exact SHA-256 duplicate CG groups.
- Godot 4.6.2 import scanned and imported all new PNGs. The command still exits nonzero only because of the pre-existing VFX Library editor popup/autoload teardown errors and ShaderV duplicate UID warnings.
- Godot 4.6.2 headless project boot passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Direct scene loads passed for `vn_host.tscn`, `bl07_void.tscn`, and `the_seam.tscn`.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S148 - 2026-06-30 (GPT image Chapter 2-6 optional-story and journey illustration pass)

### Done
- Re-audited the Chapter 2-6 dialogue files and the complete `assets/cg/` tree, then filled all 16 remaining dialogue groups in that range that had no story CG.
- Used the built-in GPT image generator with current character and environment references to create 16 clean, text-free 16:9 illustrations:
  - `story_ch2_ledger_found.png`
  - `story_ch2_ledger_return.png`
  - `story_ch2_ledger_burned.png`
  - `story_ch3_kairos_wall_warning.png`
  - `story_ch3_dead_belt_road.png`
  - `story_ch3_tobias_battle_notes.png`
  - `story_ch4_ash_rain_shelter.png`
  - `story_ch4_burner_classification.png`
  - `story_ch4_ash_rain_departure.png`
  - `story_ch5_warm_cliff_path.png`
  - `story_ch5_scratched_watchtower.png`
  - `story_ch6_bl07_after_sentinel.png`
  - `story_ch6_seam_gardener.png`
  - `story_ch6_sable_final_preparations.png`
  - `story_ch6_void_watcher_request.png`
  - `story_ch6_sable_vigil_reward.png`
- Explicitly excluded film/photo grain, paper/canvas texture, speckle and color noise, dithering, compression artifacts, chromatic aberration, dirty-lens overlays, muddy detail, and oversharpening from every generation prompt.
- Rejected the first Chapter 5 cliff and watchtower drafts because Arrel and Elia's identities drifted; edited both against the current short silver-haired Arrel and honey-blonde bob-haired Elia reference before integration.
- Connected each image to the first line of its exact dialogue group, preserving the lower 28 percent as a quiet dialogue-UI area rather than stacking multiple CG swaps inside one conversation.
- Registered all 16 illustrations as distinct PauseMenu Artbook entries and updated `ILLUSTRATION_CATALOG.md` to the verified 186-CG baseline.

### Verification
- UTF-8 JSON parsing passed for all dialogue data; all 16 dialogue mappings resolve to the intended file exactly once.
- Verified `TOTAL_CG=186`, `NEW_SET=16`, `MISSING_REFS=0`, and zero exact SHA-256 duplicate groups under `assets/cg/`.
- All 16 new images are RGB 1672x941 and have matching Godot `.import` metadata.
- Godot 4.6.2 import scanned and imported all 16 files. The pre-existing VFX Library popup/autoload teardown and ShaderV duplicate UID warnings remain unchanged.
- Godot 4.6.2 headless project boot passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Direct scene loads passed for `verdan_market.tscn`, `belt_waystation.tscn`, `drift_shelter.tscn`, `crumbling_coast.tscn`, and `the_seam.tscn`.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S149 - 2026-07-01 (VN demo save/load and stability hardening)

### Done
- Audited the current demo entry path and confirmed the active core flow is `main.tscn` -> New Game -> `SceneFlow.pending_scene_id = "ch1_prologue"` -> `vn_host.tscn` -> JSON VN scenes.
- Added serializable SceneFlow state containing only scene IDs, indices, resume queue, pending state, and active status; runtime JSON dictionaries and VN UI nodes are intentionally rebuilt.
- Added VN resume preparation so loading a save made in `vn_host.tscn` restores the saved VN scene and text-step index before scene transition.
- Bumped saves to `0.3.0`, normalized missing `scene_flow` data for old saves, and added a safe Chapter 1 prologue fallback for legacy VN-host saves without flow state.
- Extended save-slot information with optional `vn_scene_id` and `vn_step` fields.
- Made faded memories unavailable to normal burn lists and rejected normal/silent burns unless an explicit `allow_faded` override is used.
- Hid faded-memory cost choices in the VN UI and made SceneFlow pay a memory cost before applying flags or rewards.
- Deprecated `goto_battle` inside SceneFlow with a warning and safe advance instead of calling the incompatible legacy battle API.
- Added `scripts/tools/validate_vn_scenes.py` for JSON structure, scene links, choice indices, flags, memory IDs, CG assets/aliases, and portrait IDs.
- Added `VN_DEMO_SCOPE_REPORT.md` with required autoloads, current legacy dependencies, and later archive/disable recommendations. No legacy systems were removed.

### Verification
- VN validator passed: 5 files, 174 steps, 0 errors, 0 warnings.
- All JSON under `data/` parsed successfully.
- Temporary Godot regression scene passed active VN export/resume at `ch1_prologue` step 12, legacy-save fallback, faded burn refusal/override, and deprecated `goto_battle` safety (`VN_STABILITY_SMOKE=PASS`).
- Godot 4.6.2 headless project boot and direct `vn_host.tscn` load passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- The project-only `--check-only` invocation did not self-terminate in this Godot build, so verification used headless boot, direct scene load, the dedicated regression scene, and the JSON validator.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S150 - 2026-07-01 (Part II Aftermath vertical slice + nine-illustration integration)

### Done
- Read the supplied Story and Art conversion guides and mapped Part II Act 3 to a playable VN continuation rather than reopening the legacy map/battle loop.
- Audited the six newly supplied 1672x941 RGB illustrations (`66.png`-`71.png`) and imported them non-destructively as:
  - `ch11_executor_strike.png`
  - `env_gray_belt_panorama.png`
  - `ch13_notebook_resonance.png`
  - `ch14_confessor_hall.png`
  - `ch17_oblivion_storm.png`
  - `ch18_living_funeral.png`
- Used the built-in GPT image generator to create three clean, text-free, noise-excluded 16:9 bridge CGs:
  - `ch11_elia_bloodwork.png`
  - `ch12_sump_closed.png`
  - `ch14_arrel_burn_slash.png`
- Added a dedicated title-menu entry, `Part II: Aftermath`, that starts a clean Chapter 11 preview without disturbing the normal New Game/Continue routes.
- Added four linked VN scenes covering Chapter 11 Departure, Chapter 12 The Reader, Chapter 13 The Third Person, and Chapter 14 The Confessor's Hall: 93 new steps with two memory-cost branches and a preview-ending handoff.
- Added five Part II memories and connected chapter transitions to automatic chapter-memory grants, making the new burn decisions use the real MemoryManager rather than cosmetic flags.
- Expanded the VN validator with the guide's voice constraints: Arrel lines stay at eight words or fewer, Kairós uses no contractions, and Han/Singer receives no direct dialogue.
- Registered all nine CGs in the Artbook and documented their source/story role in `ILLUSTRATION_CATALOG.md`; Chapter 17/18 images remain Artbook-only future storyboards so later reveals are not externalized early.

### Verification
- VN validator passed: 9 files, 267 steps, 0 errors, 0 warnings.
- Purpose-built Part II smoke scene passed all four chapter loads, automatic memory grants, and the Chapter 11/14 burn-cost branches (`PART2_AFTERSHOCK_SMOKE=PASS`).
- Verified 195 total CGs, zero exact SHA-256 duplicate groups, and matching Godot `.import` metadata for all nine new 1672x941 RGB assets.
- Godot 4.6.2 headless project boot and direct `vn_host.tscn` load both exited 0 with zero `SCRIPT ERROR` / `Parse Error` lines.
- Godot import scanned all nine new PNGs. Only the pre-existing VFX Library popup/autoload teardown and ShaderV duplicate-UID warnings remain.

## S151 - 2026-07-02 (Part II Storm chapters + nine-illustration integration)

### Done
- Audited the four newly supplied 1672x941 RGB illustrations (`72.png`-`75.png`) and assigned them by canon-safe story role:
  - `ch15_lullaby_moment.png` enters the active Chapter 15 Han sequence.
  - `env_lumea_sanctum.png`, `ch20_archivist_hollow.png`, and `ch21_kairos_confront.png` remain Artbook-only future storyboards until their chapters are implemented.
- Used the built-in GPT image generator with current character sheets and supplied story plates to create five clean, text-free story CGs:
  - `ch15_echo_shell_awakening.png`
  - `ch16_eastward_road.png`
  - `ch16_nera_checkpoint.png`
  - `ch17_memory_fracture.png`
  - `ch18_tobias_close.png`
- Explicitly excluded film/photo grain, paper/canvas texture, speckle and color noise, dithering, compression artifacts, chromatic aberration, dirty-lens overlays, muddy detail, excessive bloom, and oversharpening from every generation prompt.
- Extended the playable VN chain from Chapter 14 through Chapter 18 with 97 new steps:
  - Chapter 15 `The Singer`: Han's silent humming, the Echo Shell awakening, Celah/eastern-isles hint, and a lullaby burn choice.
  - Chapter 16 `Nera`: eastbound storm omen, route burn choice, Nera's checkpoint appearance without prematurely defining her unlocked voice, and Mira's 0.3-degree report thread.
  - Chapter 17 `The Forgetting Storm`: two real memory-cost survival choices, party memory fracture, and Arrel's indirect bloodline clue.
  - Chapter 18 `Living Funeral`: Tobias rescue/loss branch driven by a Grade 2 identity-memory decision.
- Added five chapter memories and connected all new burn choices to the real MemoryManager cost path.
- Activated the earlier Chapter 17/18 plates in runtime story data, registered all nine new assets in the Artbook, and updated `ILLUSTRATION_CATALOG.md` to the 204-CG baseline.

### Verification
- VN validator passed: 13 files, 364 steps, 0 errors, 0 warnings.
- Purpose-built no-autosave Storm smoke scene passed all four scene loads, chapter-memory grants, memory burns, and branch destinations (`PART2_STORM_SMOKE=PASS`).
- Godot 4.6.2 import scanned and imported all nine PNGs successfully; only the known VFX Library popup/autoload and ShaderV UID warnings appeared.
- Godot 4.6.2 headless project boot and direct `vn_host.tscn` load passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Verified 204 total CGs, matching `.import` metadata for all nine new assets, zero missing VN CG references, and zero exact SHA-256 duplicate groups.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S152 - 2026-07-02 (15 supplied illustrations placed across active and future story beats)

### Done
- Audited user-supplied `76.png`-`90.png`; all 15 are unique 1672x941 RGB plates.
- Placed six illustrations directly into the existing VN sequence at information-changing beats rather than stacking redundant swaps:
  - Chapter 13: `ch13_relay_decoded.png`, `ch13_relay_breakthrough.png`.
  - Chapter 15: `ch15_han_memory_gift.png`.
  - Chapter 17: `ch17_storm_horizon.png`, `ch17_arrel_resist.png`.
  - Chapter 18: `ch18_tobias_platform.png`.
- Registered nine later or alternate plates in the Artbook without runtime references:
  - `ch15_han_last_hum.png`.
  - `env_lumea_inner_court.png`.
  - `ch20_archivist_memory_gallery.png`, `ch20_archivist_offer.png`, `ch20_archivist_warning.png`.
  - `ch20_celah_preserved.png`, `ch20_monolith_interior.png`.
  - `ch21_kairos_threshold.png`, `ch22_monolith_core.png`.
- Kept Chapters 19-22 reveal order intact: Celah, the Archivist, Kairós, and the Monolith core remain Artbook-only future storyboards.
- Added all 15 plates to the PauseMenu Artbook and updated `ILLUSTRATION_CATALOG.md` to the 219-CG baseline.

### Verification
- VN validator passed: 13 files, 364 steps, 0 errors, 0 warnings.
- Verified all 15 PNGs and matching `.import` metadata, six exact active VN references, 219 total CGs, and zero SHA-256 duplicate groups.
- Godot 4.6.2 import completed with zero `SCRIPT ERROR` / `Parse Error` lines; known VFX Library and ShaderV editor warnings remain unchanged.
- Godot 4.6.2 headless project boot and direct `vn_host.tscn` load both exited 0 with zero critical parse errors.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S153 - 2026-07-02 (Korean-complete story pass + characterized NPC/monster presentation)

### Done
- Added Korean companion text for every shipped story field across all 24 dialogue/VN JSON files: 1,423 titles, lines, narration beats, choices, effects, burned variants, and system logs now have a Korean runtime path.
- Added a reusable Korean localization generator with protected direction tags/placeholders, a MEMORIA terminology glossary, and a strict coverage validator.
- Expanded Korean speaker and enemy display-name registries, then localized title-menu copy, Part II entry, options/accessibility labels, game-over copy, chapter cards, NPC repeat lines, and common notification patterns.
- Replaced the old map-NPC portrait-card rendering with the same animated 48px four-direction character system used by the party. Malet, Elia, Sable, Tobias, Bureau staff, guards, traders, elders, and other NPC roles now receive distinct palettes/silhouettes instead of framed face illustrations.
- Fixed enemy archetype normalization so spaced display names such as `Void Beast`, `Memory Eater`, crawlers, walkers, wraiths, sentinels, rats, and humanoid scavengers resolve to authored 128px monster silhouettes rather than the generic fallback.
- Removed full reference-sheet images from ordinary battle presentation; named boss/human cinematics remain, while normal monsters now render as isolated in-game characters.
- Generated and integrated `assets/cg/game_image/malet_fullbody_stage.png`, a clean low-noise, text-free full-body Malet stage portrait based on the current expression and turnaround references. Dialogue stage art now uses it instead of the environmental Bureau-overlook illustration.

### Verification
- Korean localization validator passed: 24 files, 1,423 fields, 15 speakers, 0 errors.
- VN validator passed: 13 files, 364 steps, 0 errors, 0 warnings.
- Godot 4.6.2 headless editor import completed with zero `SCRIPT ERROR` / `Parse Error` lines; only the known VFX Library teardown warnings appeared.
- Direct `verdan_market.tscn` load exited 0 and reported `[NPC] Malet ready`, confirming the new animated NPC path boots in the representative merchant scene.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S147 - 2026-07-03 (Canon dialogue pass + Part III skeleton, Ch19-24)

### Done
- **Canon alignment pass** (in-place text edits only — no step indices moved):
  - ch11 step 1: canonized "Record Authority" as the formal name, "the Bureau" as Rim shorthand (bridges Part I "Bureau" and Part II "Authority" vocabulary).
  - ch15 step 5: first named appearance of **Blankers** (emotion-suppressant) — Han's song wakes Blanker-dulled voices for one held note.
  - ch16 step 13: Nera's suppression made explicit ("the fixed, even calm of a double Blanker dose") — sets up her first-emotion crack two steps later.
  - epilogue (weave): Sable's count corrected to canon — "Seventeen people I sent toward that hole" (was "Eleven teams").
- **Engine — Part III ending layer**:
  - `GameManager.evaluate_part3_ending()` — 7-way priority routing: explicit name burn → hollow (burn_ratio ≥ 0.75) → weave (weave_unlocked + conversion attempted) → ash (3+ rel_* burned or 8+ burns) → tobias (funeral witnessed + witness-record intact) → seam (celah_reunion or hidden-hope flags) → preservation (default).
  - `SceneFlow` step effects: `record_ending` (DialogueManager parity), `check_weave` (exposes `weave_unlocked()` as flag `p3_weave_ready` for VN choice filtering).
  - `SceneFlow` action `resolve_part3_ending`: computes the ending, sets `p3e_<id>`, records to gallery/achievements, advances — lets ch23 route pure-data ending blocks with `requires_flag`.
- **Part III skeleton — 6 new VN scenes** (full EN/KO bilingual, voice-locked per STORY_GUIDE §5, pure-VN like ch11-18):
  - `ch19_approach` (22 steps): Lumea white sanctum, Blanker patrol vs Han's song, Vael silhouette (body remembers), Sable death notification + ledger, chain-burn unlock, monolith gate hook. Burn choice: sense_white_noon.
  - `ch20_monolith` (21 steps): impossible geometry, memory sea, Chief Archivist's 300-year sentence ("We agreed to forget together…"), gated Celah preservation cell (`opt_celah_enabled`), reverse-burn unlock. Burn choice: daily_three_shadows.
  - `ch21_editors_turn` (21 steps): Kairós's withheld arithmetic + signature line, Nera's first emotion mid-warrant, Belor's arrival, ledger-vs-notebook sacrifice choice (rel_sable_ledger), "rounding error" exit.
  - `ch22_core` (16 steps): primal log, Elia relay choice (elia_relay_choice/refused), Vael full encounter (spoken "…Arrel?" gated `opt_vael_speaks`, silent fallback default), gated Celah lullaby, `check_weave`, decision threshold.
  - `ch23_conversion` (31 steps): Archivist sentence variation payoff (one word changed — F-code), 3-way final choice (burn name / weave / hold), Belor survives (season 2), `resolve_part3_ending`, 7 flag-gated ending blocks reusing existing ending CGs, canon Zero Burn closer ("Do you want a new name?").
  - `ch24_testimony` (14 steps): per-ending aftermath blocks, Tobias press, Han's song spreading, Vael season-2 ridge hook, final lullaby → `goto_map` credits.tscn (credits calls mark_game_completed → NG+ unlock).
- **Wiring**: ch18 tail rewired — `demo_build` flag gates demo_end (unset → continues to ch19; set the flag in demo builds to stop at the CTA). complete_chapter 18 + autosave fire either way (effects apply before requires check).
- **Registries**: `add_chapter_memories` 19-22 (7 new memories: sense_white_noon, rel_vael_reflex, sense_monolith_hum, rel_sable_ledger, identity_reverse_burn, rel_kairos_doubt, identity_relay_promise — ids match all cost_memory references). RICH_PRESENCE_CHAPTERS extended 11-24 with Part II/III names.

### Verification
- Static audit script: **ALL CHECKS PASS** — every choice goto in range, every portrait key exists in DialogueBox.PORTRAIT_MAP (vn_scene mirrors it), every cost_memory id registered, scene-chain targets exist (ch18→19→…→24→credits), required flags all set somewhere or intentionally external.
- All 30 `data/**/*.json` parse as UTF-8 JSON.
- Godot 4.6.2 headless boot: **0** SCRIPT ERROR / Parse Error.
- `git diff --check`: only the usual CRLF warnings.

### Codex handoff — expansion checklist (per STORY_GUIDE §8)
1. **CG generation (10 missing keys, all referenced & gracefully skipped until present)**: `ch19_vael_silhouette`, `ch19_monolith_gates`, `ch20_archivist_desk`, `ch21_nera_hesitation`, `ch21_belor_arrival`, `ch22_elia_relay`, `ch22_vael_oneword`, `ch22_conversion_threshold`, `ch23_conversion_wave`, `ch24_last_lullaby` — place in `assets/cg/generated/`, ART_GUIDE §2 palettes (Lumea = achromatic white + blue record-ink; monolith interior = violet ink + time-warp) and §6.3 LOCK phrases. 19 of 29 referenced CGs already exist (incl. all 7 ending sets).
2. **Portraits**: new speakers (Han, Vael, Belor, Chief Archivist, Mira) intentionally speak WITHOUT portrait keys (Handler precedent). When art is ready: add files + PORTRAIT_MAP keys per ART_GUIDE §4, then add `"portrait"` fields — no step edits needed.
3. **Text expansion**: each skeleton beat can grow 2-4 steps of texture (insert only AFTER the last goto target of each scene, or re-run the audit script to re-validate indices after any insertion).
4. **Author decisions (부록 A) wired as flags — default OFF**: `opt_celah_enabled` (Celah preserved-cell subplot, ch20/ch22), `opt_vael_speaks` (Vael's one word, ch22; silent fallback plays otherwise). Enable by adding a `set_flag` step early in ch19, or leave for the author.
5. **Known debts**: ch11-18 `text_ko`/`narrate_ko` are machine-quality (e.g. "나는 6을 세었다") — needs a Korean rewrite pass to match the S147 scenes' register. **Sable appearance canon conflict**: STORY_GUIDE/ART_GUIDE describe Sable(=Halda) as a blind old woman; ~30 existing CGs + battle sprites show a short-silver-haired younger woman. Author decision required before any Part III Sable imagery (S147 avoided depicting her — only her ledger).
6. **Part I ↔ Part III ending note**: Part I endings (the_seam epilogue, Ch10 seal) remain intact as the standalone Part 1 build; Part III re-resolves endings at ch23 via `resolve_part3_ending` on its own flag namespace (`p3e_*`) — no collision. If the full build should skip Part I's epilogue entirely, route bl07_void handlers to ch11_departure in a future session.

## S148 - 2026-07-03 (Sable canon decision: blind old woman)

### Decision (author-confirmed)
Sable (=Halda) is canonically a **blind old woman** — STORY_GUIDE §5 / ART_GUIDE §6.3 win over the existing silver-haired-young-woman art. Lore fusion adopted: she walked into a Void Hole and walked out — **her sight stayed behind as the toll**. This preserves the game's established "woman who came back" lore while satisfying canon blindness, and gives her clouded eyes a diegetic origin.

### Done (text/code — effective immediately)
- ch5 `seam_arrival`: entrance description rewritten — old woman, white hair, weathered face, clouded pale eyes, "waiting without watching" (EN+KO).
- ch6 `sable_past`: "staring at maps she'd already memorized" → fingers resting on maps memorized before her eyes went, kept for those who can see.
- ch7 `sable_trial`: "I've seen you fight" → "I've heard you fight — what stops, what doesn't."
- epilogue weave: clouded eyes turning toward BL-07 ("never needed sight to know where it was"), ledger reworked to **pin-pricked pages her fingers read**, "sketching" → "pricking new marks".
- epilogue ash: "the light behind their eyes is gone" → "listen long enough and you hear it — the room behind the voice is empty" (blind-appropriate, arguably stronger).
- `rel_sable_trust` memory: "Her eyes stayed behind as the toll."
- `pixel_sprite.gd sable_config()`: white hair + clouded gray-white eyes (map/companion placeholder sprites update immediately).
- Artbook descs: "silver-haired Sable" removed, "sees"→"knows without sight", weave ledger desc now seventeen attempts / "Eighteenth Pattern".
- Kept intentionally: "There's a pattern. I just can't see it yet." (now double-edged), "BL-07 looks like whatever you're most afraid of losing" (idiom), her role as Ch4+ battle companion (blind void-sense fighter is canon-compatible).

### Verification
- All 30 `data/**/*.json` parse OK; targeted rescan of every Sable-participating dialogue group found 0 remaining sight-verb narrations.
- Godot 4.6.2 headless boot: 0 SCRIPT ERROR / Parse Error.

### Codex handoff — Sable image regeneration queue
New LOCK phrase (ART_GUIDE §6.3, use verbatim): `old blind woman, late 60s, short white hair, weathered coast-stone face, pale clouded eyes, dark violet-toned coat, small scar, composed and unhurried`
1. **Portraits (replace, same filenames/keys)**: `assets/portraits/sable_neutral.jpg`, `sable_calm.jpg`, `sable_face_neutral.png`, `sable_face_calm.png`. Optional new key: `sable_prophetic` (ART_GUIDE §4).
2. **Artbook CGs featuring her directly (16)**: dialogue_ch7_sable_echo_shell, cinematic_sable_echo_strike, story_ch5_seam_first_light, story_ch6_sable_briefing, story_ch7_fading_names_monument, story_ch7_sable_confession, ending_preservation_return, epilogue_sable_eastern_settlement, story_ch7_controlled_burn_trial, story_ch8_eighteenth_ring, story_ch8_white_stone_shelter, ending_weave_sealed_gate, ending_weave_sable_ledger, ending_weave_colors_return, story_ch6_sable_final_preparations, story_ch6_sable_vigil_reward.
3. **Party group shots to verify/regen if she's visible**: story_ch7_last_field_preparations, story_ch7_crossing_the_ridgeline, story_ch8_forest_crossing, story_ch9_kairos_confrontation, story_ch9_human_chain.
4. **Battle sprite (128x128 side-view)** if a dedicated Sable battle sprite file exists in the battle set — regenerate to match; placeholder pixel config already updated in code.
5. Regen priority: portraits → ch5/ch6 first-meeting CGs → weave/preservation ending CGs → group shots.

## S154 - 2026-07-03 (Sable canon art regeneration + Part III bridge CGs)

### Done
- Audited the uncommitted Claude Code handoff and preserved its Part III Ch19-24 story/engine work.
- Rebuilt Sable/Halda around the confirmed canon LOCK: `old blind woman, late 60s, short white hair, weathered coast-stone face, pale clouded eyes, dark violet-toned coat, small scar, composed and unhurried`.
- Replaced all four live Sable portrait assets at their existing keys with one consistent identity: neutral/calm 1024px portraits plus 256px dialogue variants.
- Added `sable_canon_master.png` as the reusable identity reference and Artbook entry.
- Regenerated four high-priority Sable CGs in place so every existing JSON reference updates automatically:
  - `story_ch5_seam_first_light.png`
  - `story_ch6_sable_briefing.png`
  - `ending_preservation_return.png`
  - `ending_weave_sable_ledger.png`
- Generated and activated five previously missing Part III bridge CGs already referenced by the new VN files:
  - `ch19_monolith_gates.png`
  - `ch20_archivist_desk.png`
  - `ch22_conversion_threshold.png`
  - `ch23_conversion_wave.png`
  - `ch24_last_lullaby.png`
- Registered the canon master and all five Part III CGs in the PauseMenu Artbook and documented the set in `ILLUSTRATION_CATALOG.md`.
- Every generation prompt prohibited film/photo grain, paper/canvas texture, speckles, chromatic noise, compression artifacts, and excessive bloom.

### Remaining art queue
- Three Part III slots remain open: `ch21_belor_arrival`, `ch22_elia_relay`, and `ch22_vael_oneword`. The first S156 attempts were rejected for style/continuity drift and removed from the project; their VN lines intentionally have no dangling `cg` fields.

### Verification
- Korean localization validator passed: 30 files, 1,546 fields, 19 speakers, 0 errors.
- VN validator passed: 19 files, 489 steps, 0 errors, 0 warnings.
- All active VN CG references resolve after the five completed Part III assets were added and five future slots were made non-dangling.
- Godot 4.6.2 headless editor import and project parse passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S155 - 2026-07-03 (First-play graphics cleanup + directional characters)

### Done
- Reduced the first playable Rim Forest screen's visual noise: disabled premium-lens grain, softened vignette/fog/light shafts, reduced pollen and glints, and cut ash-rain density, turbulence, size, and opacity.
- Made premium-lens grain opt-in globally so low-resolution map art no longer develops crawling compression-like noise by default.
- Replaced Arrel and Elia's reused sheet rows with true procedural four-direction frames; idle/walk silhouettes now visibly change for up, down, left, and right movement.
- Upgraded Malet's map character with a gold merchant chain and two distinct hanging memory vials, layered over his existing brooch and merchant palette.
- Generated and activated `ch19_vael_silhouette.png` at Arrel's involuntary guard-response beat and `ch21_nera_hesitation.png` at Nera's first emotion in nineteen years, then registered both in the Artbook and illustration catalog.
- Both new CGs are clean 1672x941 RGB plates; their prompts explicitly prohibited grain, paper/canvas texture, speckles, dithering, chromatic noise, compression artifacts, muddy brush noise, and excessive bloom. Nera's portrait was supplied as an identity reference.

### Verification
- Godot 4.6.2 headless editor import/project parse passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Direct headless load of `res://scenes/maps/rim_forest.tscn` passed and initialized Arrel, Elia, and the map without runtime script errors.
- Direct headless load of `res://scenes/maps/verdan_market.tscn` passed and initialized Malet plus Elia's new directional companion frames without runtime script errors.
- Korean localization validator passed: 30 files, 1,546 fields, 19 speakers, 0 errors.
- VN validator passed: 19 files, 489 steps, 0 errors, 0 warnings.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S156 - 2026-07-04 (Field Focus gameplay loop)

### Done
- Added the Field Focus loop to connect exploration with combat:
  - a Memory Pulse that maps a new resonance echo banks one Field Focus charge, up to 3;
  - mapped echoes persist through story flags and cannot be farmed by reloading a map;
  - pulses now report the nearest echo's cardinal direction and distance;
  - the Exploration HUD shows Focus beside Pulse readiness;
  - the next normal battle consumes one charge and starts at Resonance 25 plus Limit 20;
  - Boss Rush does not consume exploration charges;
  - old save files receive a safe zero-charge default and NG+ resets the temporary bank.
- Added `echoes_mapped` play-stat tracking and a reusable headless regression scene at `scripts/tools/smoke_field_focus.tscn`.

### Verification
- Field Focus smoke test passed: one nearby echo mapped, one charge gained and consumed, battle opened at Resonance 25 / Limit 20.
- Korean localization validator passed: 30 files, 1,546 fields, 19 speakers, 0 errors.
- VN validator passed: 19 files, 489 steps, 0 errors, 0 warnings.
- Godot 4.6.2 headless editor import/project parse passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Direct headless load of `res://scenes/maps/rim_forest.tscn` passed with Player, Elia, HUD, compass, and resonance systems initialized.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S157 - 2026-07-04 (Style-locked Field Focus illustration pass)

### Art consistency audit
- Rechecked the active art guide and representative map CGs before generation.
- Preserved the project's intended two-track presentation: pixel characters during exploration and cinematic dark-fantasy paintings for CGs.
- Did not regenerate Belor, Vael, or other unfinalized new characters. The art guide explicitly forbids temporary renders before their character LOCK phrases are finalized.
- Chose environment-only memory echoes so no face, costume, age, or silhouette continuity could drift.

### Done
- Generated four clean 1672x941 RGB Field Focus CGs using each active map illustration as a direct style reference:
  - `resonance_rim_forest_echo.png` from `story_ch1_twisted_forest_path.png`;
  - `resonance_verdan_market_echo.png` from `chapter_splash_verdan_market.png`;
  - `resonance_crumbling_coast_echo.png` from `chapter_splash_crumbling_coast.png`;
  - `resonance_forgotten_forest_echo.png` from `chapter_splash_forgotten_forest.png`.
- Matched each source's environment geometry, palette, lighting ratio, realistic concept-art rendering, and dialogue-safe lower zone. Prompts excluded grain, paper/canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens overlays, excessive bloom, and oversharpening.
- Connected each plate to the first new Field Focus discovery in its map. A persistent map flag prevents repeated CG interruptions while later echoes still grant gameplay charges.
- Registered all four plates in the PauseMenu Artbook and documented their exact source references in `ILLUSTRATION_CATALOG.md`.
- Extended the Field Focus regression smoke test to verify every registered CG path resolves before testing echo mapping and battle carry-over.

### Verification
- Visual inspection confirmed each output remains in the source map's geometry, palette family, material rendering, and contrast range; no unapproved characters appear.
- Field Focus smoke test passed with all four CG paths resolved, one echo mapped, one charge consumed, and battle opening at Resonance 25 / Limit 20.
- Korean localization validator passed: 30 files, 1,546 fields, 19 speakers, 0 errors.
- VN validator passed: 19 files, 489 steps, 0 errors, 0 warnings.
- Godot 4.6.2 headless editor import/project parse passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Direct headless load of `res://scenes/maps/rim_forest.tscn` passed with the exploration stack initialized.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S149 - 2026-07-04 (VN gameplay upgrade: memory as a live resource)

### Goal
Part II/III is pure VN, so the burn-vs-keep economy needed to LIVE inside the VN layer. Four systems, one loop: keeping memories opens paths (keys), burning them rewrites later scenes (distortion), the ledger makes the invisible economy visible, and keys feed the ending resolver.

### Done
- **Choice UI legibility** (`vn_scene.gd`): burn choices now show the memory's grade (G5-G1) and its `story_effect` (what you lose) inline; kept-memory-gated choices get a "✧ [Kept: title]" label and a teal style distinct from burn-red. The trade is readable before you take it.
- **Memory Keys — 7 hidden choices** unlocked only by intact memories (`requires_memory_intact`), each with a distinct payoff line at scene end:
  - ch12: `daily_market_food` (Part I!) — the vendor row hides a regulars-only gap; being remembered becomes a place to hide.
  - ch14: `identity_first_sword` (Part I — lost forever if you took Malet's deal in Ch2) — the courtyard grip opens the extraction cradle without a scar.
  - ch15: `daily_campfire_song` (Part I) — Arrel's song and Han's lullaby share a First-Age refrain ("NOW WE ARE A CHOIR").
  - ch17: `sense_forest_smell` (Part I) — **the only forced-burn chapter becomes avoidable**: the storm cannot counterfeit rain on forest earth.
  - ch19: `rel_hand_reaching` — linked hands read as one signature at the Lumea barrier.
  - ch21: `rel_tobias_alliance` — standing as three lets Tobias save one page of Kairós's notebook.
  - ch22: `daily_elia_hands` — the relay opens around an anchor instead of a wound (uses new `set_flags` array support in SceneFlow).
- **Keeper Keys → ending**: `GameManager.KEEPER_KEY_FLAGS` + `count_keeper_keys()`; 3+ keys now qualifies the **seam (hope) ending** in `evaluate_part3_ending()`. Preservation play has a visible destination.
- **Burn Distortion — 8 new `distort_if_burned` variants** (Part II/III had zero): belt footsteps→ch16 pace loss, intervention vow→ch18 feet that no longer move on their own, Verdan faces→ch18 sliding face, Han→ch19 stranger's hum, campfire song→ch20 reverse-burn finds ash, Sable's ledger→ch24 ledger restarted at one, witness record→ch24 rhythm without reason, unfinished lullaby→ch24 hundred endings for one song.
- **Chapter Ledger overlay** (`scene_flow.gd::_show_chapter_ledger`): on every `complete_chapter`, a non-blocking 5s panel shows burned-this-chapter (titles), memories still held, anchors k/4 + name status, and the thread line ("The thread still holds / is fraying") — indirect Weave-path feedback, no gauges (rule 4 compliant).
- **Archive in VN**: Tab/M opens Arrel's Archive during VN scenes (DIALOGUE + SceneFlow active); closing restores the previous game state instead of forcing EXPLORATION.

### Verification
- Key insertion audit: ALL PASS — 7 key options, 7 payoff steps, every payoff inserted after the scene's max goto/start_index target (no index shifts); key memory ids all exist; KEEPER_KEY_FLAGS matches scene flags 1:1.
- Distortion audit: 8 new distort steps, all `distort_if_burned` ids exist in the memory catalog.
- Full 19-scene audit: portraits/goto/scene-chain all pass (ch1/ch2 goto-less inline choices are valid engine syntax, not errors).
- All 30 data JSON files parse; Godot 4.6.2 headless boot: 0 SCRIPT ERROR / Parse Error.

### Design notes for next pass
- Key choices deliberately reuse existing branch routes (goto to keep-branch) — Codex can later expand any key branch into its own route by appending steps after the current max-goto index and re-running the audit.
- The ledger overlay reads at chapter transitions only; if playtests want mid-chapter access, the same data is now one Tab away via the VN archive.
- Balance watch: 7 keys exist, seam ending needs 3 — reachable by keeping any 3 of the keyed memories; Malet's Ch2 deal still silently costs the ch14 key (long-range consequence preserved).

## S158 - 2026-07-04 (Claude S149 review + Memory Key illustration payoffs)

### Code review fixes
- Persisted the chapter-ledger burn snapshot through save/load; legacy saves now start from their current burn count instead of reporting all historical losses as new.
- Made intact-memory choice gates use `MemoryManager.is_intact()` and revalidate them in `SceneFlow.select_choice()`, so faded memories and stale UI choices cannot unlock a key route.
- Prevented VN input from advancing behind the archive overlay and made the archive genuinely read-only during VN playback by disabling memory synthesis.
- Replaced raw English `story_effect` leakage in Korean burn-choice previews with a localized consequence message.
- Decoupled the ledger overlay from the optional AchievementManager presence.

### Illustration integration
- Generated and placed three environment/lore-only Memory Key payoff CGs:
  - `memory_key_verdan_passage.png` in the Ch12 market-food key payoff;
  - `memory_key_confessor_hinge.png` in the Ch14 first-sword key payoff;
  - `memory_key_first_age_refrain.png` in the Ch15 campfire-song key payoff.
- Registered all three in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.
- Used active scene CGs as direct references and prohibited grain, paper/canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens overlays, muddy detail, oversharpening, and excessive bloom.

### Verification
- All three generated CGs are clean 1672x941 RGB plates and every active VN CG reference resolves.
- VN validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- Godot 4.6.2 headless editor import/project parse passed without script or parse errors; only known addon shutdown warnings remain.
- Direct headless load of `res://scenes/main/vn_host.tscn` passed without script, parse, invalid-call, or invalid-access errors.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S159 - 2026-07-04 (Memory Key illustration set completion)

### Done
- Generated and placed the remaining four Memory Key payoff CGs:
  - `memory_key_forest_rain.png` for Ch17's intact forest-scent route through the Forgetting Storm;
  - `memory_key_single_signature.png` for Ch19's linked-hands passage through Lumea's scanner;
  - `memory_key_surviving_page.png` for Ch21's alliance-preserved notebook page;
  - `memory_key_relay_anchor.png` for Ch22's relay opening around remembered warmth.
- All seven Memory Keys now have dedicated visual payoff CGs.
- Registered all four new plates in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.
- Used each chapter's active CG as a direct style reference; avoided new faces and unfinalized character designs.
- Prompts explicitly prohibited film/photo grain, paper/canvas overlays, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, and excessive bloom.

### Verification
- All seven Memory Key payoff plates are clean 1672x941 RGB images and all seven key flags resolve to dedicated existing CG paths.
- Active VN CG reference audit passed with 0 missing files.
- VN validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- Godot 4.6.2 headless editor import/project parse passed; only known addon shutdown warnings remain.
- Direct headless load of `res://scenes/main/vn_host.tscn` passed without script, parse, invalid-call, or invalid-access errors.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S160 - 2026-07-04 (Gameplay visual clarity overhaul)

### Done
- Added `Clear Gameplay View / 게임플레이 시야 선명화` to Options and made it the default for new and existing installs through a one-time settings migration.
- Defaulted Reduce Motion on and Screen Shake off for the clean-view baseline.
- Centralized exploration cleanup in `MapEffects`: interactive maps now suppress grain/lens overlays, vignette, fog layers, rain/snow, lightning, foreground pollen/ash/leaves/fireflies, void particles/tendrils, heat haze, burn desaturation, color grading washes, depth gradients, animated atmosphere plates, time-of-day tint, decorative water shimmer, transition particles, ambient camera shake, and dark CanvasModulate lighting while clean view is enabled.
- Removed Arrel's camera look-ahead, shake, sprint afterimages, and footstep debris in clean view; AshRain is also disabled.
- Cleaned battle presentation: disabled premium lens grain, vignette, duplicated depth plates, ambient dust, parallax haze, chromatic aberration, speed lines, full-screen flashes, confetti, screen shake, status particles, edge flames, and nonessential skill particle bursts. Damage numbers, status icons, silhouettes, HP, tactical UI, and concise hit feedback remain.
- Added `smoke_visual_clarity.tscn` regression coverage for exploration and battle obstruction layers.

### Verification
- Visual clarity smoke passed: `fog=0 particles=0 vignette=0 lens=0 battle_dust=0`.
- Godot 4.6.2 headless editor/project parse passed without script or parse errors.
- Direct runtime loads passed for Rim Forest, Forgotten Forest, BL-07 Void, and Verdan Market; no script, parse, invalid-call, or invalid-access errors.

## S150 - 2026-07-04 (Motion naturalness + memory-key CG verification)

### Goal
Story-first game, but exploration movement should feel alive: character appearance responds to motion, movement reads naturally, and the S149 memory-key payoffs get their art.

### Done — Player (`scripts/core/player.gd`)
- **Deceleration-aware animation**: walk animation now keyed to actual velocity (>12 px/s), not input — the character no longer foot-slides in an idle pose while momentum carries him.
- **Speed-scaled animation** (`speed_scale` 0.65–1.85): sprint and decel change stride rate, killing ground-slide; feet finally match the floor.
- **Diagonal hysteresis**: facing axis only flips when the dominant axis leads by 20% (sign reversals apply immediately) — no more left/up flicker on near-45° movement.
- **Walk bob + footfall sync**: a single bob phase (integrated from speed) drives a 1.6px body bounce, and dust now spawns exactly on footfall frames (replaces the old random dust timer). One source of truth for "a step happened."
- **Movement lean**: subtle rotation into horizontal travel (stronger at sprint), lerped back upright on stop.

### Done — Companion (`scripts/core/companion.gd`)
- **Soft-zone following**: target speed scales with distance (MIN→MIN+70px: 35%→100%), replacing the binary stop/full-speed oscillation that made Elia stutter at the player's heel.
- **Sprint catch-up**: detects player velocity >130 and raises cap to 1.6× so she arcs after a sprinting Arrel instead of falling behind linearly.
- **Acceleration (480 px/s²) + slerp direction smoothing**: she now curves into new directions rather than snap-turning.
- **Same body language as the player**: walk bob, horizontal lean, speed-scaled stride, idle breathing (1.8Hz micro-scale), and the long-distance teleport is masked with a 0.35s fade-in.

### Verified — memory-key art (folder images)
All 7 `assets/cg/generated/memory_key_*.png` illustrations are correctly wired to their S149 keeper-key payoff steps (ch12 verdan_passage / ch14 confessor_hinge / ch15 first_age_refrain / ch17 forest_rain / ch19 single_signature / ch21 surviving_page / ch22 relay_anchor), files exist with import metadata, and all 7 are registered in the PauseMenu Artbook.

### Verification
- Godot 4.6.2 headless full boot: 0 SCRIPT ERROR / Parse Error.
- Live direct-scene run of `belt_waystation.tscn` (Player + Companion instantiated, 8 frames): 0 script errors.
- 7/7 memory-key CG wiring audit passed.

### Notes
- Bob is applied via `sprite.offset` and lean via `sprite.rotation`, deliberately orthogonal to the existing squash/stretch (scale) and breathing systems — no tween fights.
- `clean_gameplay_visuals` option still suppresses dust; bob/lean remain (they're body language, not screen noise). If testers want them under the toggle too, gate the two lerp lines the same way.

## S151 - 2026-07-04 (Placeholder graphics cleanup: real sprite sheets wired into battle)

### Finding
Two complete hand-made sprite sheets were sitting unused in the repo: `assets/sprites/characters/arrel_sheet/` and `elia_sheet/` — 32 frames each at 160x160 (idle x4, move x4, move_left x4, attack x6, attack_left x6, cast x4, hurt x2, down x2), with zero code references. The battle scene was still rendering procedural pixel-rectangle sprites while this art existed. That mismatch was the single ugliest visible graphic in the game.

### Done
- **Sheet loader** (`pixel_sprite.gd::load_sheet_frames`): assembles SpriteFrames from per-frame PNGs with a verb table (idle 6fps loop / move 10 / attack 14 one-shot / cast 10 / hurt 8 / down 4, plus `_left` variants when present). Returns null when the sheet is absent → callers fall back to procedural generation, so the game runs identically on repos without the art.
- **Battle wiring** (`battle_scene.gd`): Arrel uses the real sheet at 0.625 scale (preserves the previous ~100px on-screen size), ally Elia at 0.56; procedural fallback keeps old scales.
- **Verb playback hooks**: plain attack → `attack`, burn/skill → `cast`, taking damage → `hurt`, defeat → `down` (holds last frame — no getting back up). One-shot verbs auto-return to `idle` via `animation_finished`; on procedural sprites (no such animations) the calls are silent no-ops.
- **Scale-tween bug fixed**: attack/hurt sequences tweened `player_sprite.scale` to absolute values (ending at 1.0 despite a 0.78 build scale — a pre-existing growth bug, fatal at sheet scale 0.625). Introduced `_player_sprite_base_scale`; all 6 scale tweens are now relative to it.

### Verification
- `smoke_visual_clarity.tscn` (instantiates the battle scene with a dummy enemy → exercises the sheet loader + build path): 0 errors, no assertion failures.
- Full headless boot: 0 SCRIPT ERROR / Parse Error.

### Remaining procedural visuals — Codex regeneration queue (priority order)
1. **Map 4-direction walk sheets** (48px-style, up/down/left/right × idle/walk) for Arrel + Elia — current sheets have no up/down facing, so exploration still uses procedural sprites (deliberate; S150 motion polish applies either way). Wire-up path: extend `load_sheet_frames` verb table once frames exist.
2. **Battle sheets for Sable and Tobias** (same 160px verb layout as arrel_sheet). ⚠ Sable must follow the S148 blind-old-woman LOCK phrase.
3. **Enemy battle sheets** (void_beast, shade_sentinel, kairos, eraser types) — enemies are still procedural/static images in battle.
4. **Tilesets** — TilePainter procedural tiles are stylistically acceptable but are the last fully procedural layer.

## S161 - 2026-07-05 (Claude gameplay review + narrative bridge illustrations)

### Code review
- Audited Claude's latest Memory Key economy, Field Focus, exploration motion, companion follow, chapter ledger, VN archive, and battle-sheet integration commit.
- Fixed a one-shot battle animation signal bug: `_play_actor_anim()` checked the unbound callback but connected a bound callback, so repeated attack/cast/hurt verbs could attempt duplicate `animation_finished` connections.
- Extended `smoke_visual_clarity.tscn` to play multiple one-shot verbs and assert that Arrel retains exactly one completion callback.

### Illustration integration
- Generated and placed four style-locked, environment-led story CGs:
  - `ch12_hidden_passage.png` when the Sump patrol turns back above Verdan's hidden route;
  - `ch16_moving_horizon.png` when the Forgetting Storm moves against wind and tide;
  - `ch18_broken_funeral_platform.png` on the Tobias rescue branch after every restraint opens;
  - `ch20_reverse_memory_fire.png` when memory-fire first flows outward without burning.
- Registered all four images in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.
- Preserved every existing choice/goto index by attaching CG fields to existing steps rather than inserting new VN steps.
- Used the active chapter CGs as direct visual references and avoided unfinalized faces or character redesigns.
- Every generated plate is a clean 1672x941 RGB image with explicit exclusions for grain, paper/canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, and visibility-blocking fog.

### Verification
- Godot 4.6.2 imported all four new images and parsed the project without script or parse errors; only the known third-party addon shutdown warnings remain.
- Visual clarity/battle-sheet smoke passed with `actor_callbacks=1`; repeated one-shot verbs no longer duplicate their completion signal.
- Field Focus smoke passed with one charge consumed and battle opening at Resonance 25 / Limit 20.
- Direct runtime loads passed for `vn_host.tscn` and `belt_waystation.tscn`, exercising the VN host plus Arrel/Elia movement code.
- VN validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- All 84 path-based active VN CG references resolve; all four new assets imported as 1672x941 RGB plates.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S162 - 2026-07-05 (Complete Field Focus illustration coverage)

### Done
- Expanded Claude's Field Focus discovery presentation from four illustrated maps to all ten exploration maps.
- Generated and connected six clean environment-led CGs for Belt Waystation, Drift Shelter, The Seam, Seam Outskirts, Colorless Waste, and BL-07 Void.
- Added localized English/Korean discovery captions for every new map in `MemoryResonance.FIELD_FOCUS_CG_BY_MAP`.
- Registered all six images in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.
- Strengthened `smoke_field_focus.gd`: it now requires exact coverage between `RESONANCE_POINTS` and the Field Focus CG map, and verifies every configured resource path.

### Art direction
- Used each active chapter splash or story plate as a direct reference.
- Kept the set environment-only to avoid identity drift while still visualizing each map's memory mechanic.
- Every plate is a clean 1672x941 RGB image with a quiet lower UI zone and no grain, paper/canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, or visibility-blocking fog.

### Verification
- Godot 4.6.2 imported all six images and parsed `MemoryResonance` without script or parse errors; only known third-party addon shutdown warnings remain.
- Field Focus smoke passed with `maps=10`, confirming exact coverage for every resonance map and every configured image path.
- Visual clarity/battle-sheet smoke passed with `actor_callbacks=1`.
- Direct runtime loads passed for `vn_host.tscn`, Belt Waystation, Seam Outskirts, and BL-07 Void without script, parse, invalid-call, or invalid-access errors.
- VN validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- All six new CGs and import metadata passed the 1672x941 RGB asset audit.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S163 - 2026-07-05 (Branch consequence illustration expansion)

### Done
- Generated and integrated ten new style-locked CGs across Chapters 11, 12, 14, 16, 18, 20, 21, and 22.
- Illustrated both sides of the Chapter 11 and Chapter 20 choices, plus major kept-memory, burned-memory, and late-story consequences that previously resolved as text alone.
- Attached every CG to an existing VN step, so all branch `goto` and `start_index` routing remains unchanged.
- Registered the complete set in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.
- Kept the compositions environment-led or limited to anonymous silhouettes to avoid introducing new face or costume canon.

### Art direction
- Used each chapter's active story CG as a direct visual reference for architecture, palette, lighting, and rendering language.
- Every plate is a clean 1672x941 RGB image with a quiet lower dialogue zone.
- Prompts explicitly excluded grain, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, and visibility-blocking fog.

### Verification
- Godot 4.6.2 imported all ten CGs and parsed the project without script or parse errors; only the known third-party addon shutdown warnings remain.
- VN validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- Field Focus smoke passed with `maps=10`, one charge consumed, and battle opening at Resonance 25 / Limit 20.
- Visual clarity/battle-sheet smoke passed with `fog=0 particles=0 vignette=0 lens=0 battle_dust=0 actor_callbacks=1`.
- Direct headless load of `vn_host.tscn` passed without script or parse errors.
- All 94 path-based active VN CG references resolve; all ten new assets and import metadata passed the 1672x941 RGB audit.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S164 - 2026-07-05 (Choice-result and conversion illustration pass)

### Done
- Reviewed the current Claude-authored gameplay/VN structure and identified the largest remaining visual gaps immediately after major burn/keep choices.
- Generated and integrated thirteen new CGs across Chapters 15, 16, 19, 21, 22, and 23.
- Completed paired visual outcomes for Han's lullaby, Lumea's checkpoint, the Editor chamber, Elia's relay choice, and all three core conversion routes.
- Added a dedicated story CG for the delivery of Sable's tactile ledger.
- Attached every image to an existing VN step, preserving all branch indices and scene routing.
- Registered all thirteen images in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.

### Art direction
- Used live chapter CGs as direct references for architecture, lighting, costumes, props, and energy effects.
- Rejected one draft whose foreground figure drifted from established identity and replaced it with a back-facing, ledger-led composition.
- Every final image is designed as a clean 1672x941 RGB plate with a quiet lower dialogue zone.
- Prompts excluded grain, photo noise, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, dense fog, and visibility-obscuring particles.

### Verification
- Godot 4.6.2 imported all thirteen CGs and parsed the project without script or parse errors; only the known third-party addon shutdown warnings remain.
- VN validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- Field Focus smoke passed with `maps=10`, one charge consumed, and battle opening at Resonance 25 / Limit 20.
- Visual clarity/battle-sheet smoke passed with `fog=0 particles=0 vignette=0 lens=0 battle_dust=0 actor_callbacks=1`.
- Direct headless load of `vn_host.tscn` passed without script or parse errors.
- All 107 path-based active VN CG references resolve; all thirteen new assets and import metadata passed the 1672x941 RGB audit.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S165 - 2026-07-05 (Interface clarity and mid-story illustration pass)

### UI / UX
- Replaced the full-screen Exploration HUD decoration with the compact plate whenever Clear Gameplay View is active, keeping the playfield unobstructed by default.
- Added a compact lower-right exploration control strip that switches instantly between keyboard and controller prompts for Interact, Memory Pulse, Archive, and Menu.
- Added the missing Memory Pulse input glyph mapping (`Q` / `Y`) to `InputManager`.
- Upgraded dialogue guidance from a generic `NEXT` label to input-aware `Skip / Continue` and localized Korean equivalents.
- Added direct number-key selection for dialogue choices while retaining controller focus navigation.
- Added an input-aware Continue chip to standalone CG viewing.
- Corrected the Pause menu footer so controller mode no longer advertises unsupported shoulder-button quick save/load; keyboard mode retains the real F6/F7 shortcuts.
- Pause footer hints now refresh on input-mode changes and whenever the menu opens.

### Illustrations
- Generated and connected six new style-locked CGs for Chapters 13, 14, 17, and 18.
- Added visual consequences for the completed burn signature, Kairós's unsent report, the storm-center collapse, and both Living Funeral outcomes.
- Rejected an identity-drift rescue draft and regenerated it with Arrel, Elia, and Tobias's established costume colors locked explicitly.
- Registered all six images in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.
- Attached every CG to an existing VN step, preserving all branch indices.

### Verification
- Godot 4.6.2 imported all six new CGs and parsed the expanded UI scripts without script or parse errors; only known third-party addon shutdown warnings remain.
- Enhanced visual-clarity smoke passed with `ui_hints=1`, covering the compact clean-view HUD, keyboard Memory Pulse prompt, dialogue Skip/Continue distinction, CG continue chip, controller-safe Pause footer, and battle animation callback.
- VN validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- Field Focus smoke passed with `maps=10`, one charge consumed, and battle opening at Resonance 25 / Limit 20.
- Direct headless load of `vn_host.tscn` passed without script or parse errors.
- All 113 path-based active VN CG references resolve; all six new assets and import metadata passed the 1672x941 RGB audit.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S166 - 2026-07-05 (Final illustration placement and session closeout)

### Done
- Generated and integrated four final style-locked CGs across Chapters 12, 16, 17, and 18.
- Illustrated Pell's returning name, Nera's blank dossier page, the altered calm after the Forgetting Storm, and the monolith's violet answer.
- Attached every image to an existing VN step so all choice, `goto`, and `start_index` routing remains unchanged.
- Registered the complete set in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.
- Used prop-led, back-facing, and environment-led compositions to preserve established character identity and costume colors.
- Every final image is a clean 1672x941 RGB plate with the lower 28 percent reserved for dialogue UI and explicit exclusions for grain, photo noise, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, dense fog, and visibility-obscuring particles.

### Verification
- Godot 4.6.2 imported all four CGs and loaded the expanded project without script or parse errors; only the known third-party addon and shutdown cleanup warnings remain.
- VN validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- Visual clarity smoke passed with `fog=0 particles=0 vignette=0 lens=0 battle_dust=0 actor_callbacks=1 ui_hints=1`.
- Field Focus smoke passed with `maps=10`, one charge consumed, and battle opening at Resonance 25 / Limit 20.
- Direct headless load of `vn_host.tscn` passed without script or parse errors.
- All 117 active VN CG references resolve to 105 unique files; all four new assets and import metadata passed the 1672x941 RGB audit.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S167 - 2026-07-05 (Codex review + Part II Korean literary rewrite)

### Codex work review (0e2a4fb / a78ca66 / 77be80d / fcc430d / fb28511)
- Verified clean: Field Focus loop complete (Memory Pulse discovery -> banked focus (max 3) -> battle-opening advantage, HUD/artbook integration), style-locked Field Focus echo CGs, Part III art completion, branch-consequence and choice-result CGs wired ch11-23.
- Audit: smoke_field_focus + smoke_visual_clarity pass, 19-scene structural audit 0 errors, referenced CG files 0 missing, headless boot 0 errors.

### Done — Part II Korean rewrite (default-locale quality gap)
- Rewrote every machine-translated `text_ko`/`narrate_ko`/choice label in **ch11-ch18** (~156 steps + 16 choices) to the S147+ house literary register, voice-locked per character (Arrel 단문 선언 / Elia 부드러운 정확함 / Tobias 장부 은유·하게체 / Kairos 격식 하게체 / Handler 사무체).
- Representative: "나는 6을 세었다" → "여섯. 셈은 끝냈어."
- Preserved all previously authored Korean (canon seeds, memory-key payoffs, distortion variants).
- Normalized `title_ko` across ch11-ch24 ("제N장 — 이름", 14 scenes).

### Verification
- Field-by-field structural diff vs HEAD across all 8 rewritten files: **CLEAN** — only `_ko` and `title_ko` changed.
- 30/30 data JSON parse OK; headless boot 0 SCRIPT ERROR / Parse Error.

## S168 - 2026-07-06 (VN comfort layer: AUTO mode + fast-forward)

### Rationale
Story-first game, hours of VN reading, and the runner had zero reading conveniences. Coverage audit confirmed Part I dialogue Korean is 100% present (1032/1032 lines), so the highest-value upgrade was the missing comfort layer.

### Done (`vn_scene.gd` + `scene_flow.gd`)
- **AUTO mode**: toggle with `A` key or the new "자동/AUTO" chip beside the NEXT indicator. After the typewriter finishes, waits a read-time delay (0.9s base + 0.032s/char, capped 4.5s) then advances. Session-persistent across scene instances via `SceneFlow.vn_auto_mode`; chip shows state (teal ▶ when on).
- **Fast-forward**: hold `Ctrl` — instantly completes the current line and advances every 0.09s while held.
- **Safety rails**: both modes are hard-gated — never fire during choices (`_choice_container.visible`), never while the VN is inactive or the game state left DIALOGUE (e.g. Tab archive open). All three advance paths (manual/auto/FF) go through one `_advance_step()`.
- Auto timer resets on every new line and on toggle; manual clicks don't cancel AUTO (standard VN behavior).

### Verification
- Headless boot + battle smoke scene: 0 SCRIPT ERROR / Parse Error / assertion failures.

### Next debt candidates
- Mirror a minimal AUTO toggle in the Part I map DialogueBox for parity.
- Part I dialogue `text_ko` quality spot-audit (coverage is 100%; register quality unverified).

## S169 - 2026-07-06 (DialogueBox AUTO parity + Part I Korean audit)

### Done
- **DialogueBox AUTO parity**: the Part I map dialogue box now honors the same AUTO mode as the VN runner — `A` key toggles `SceneFlow.vn_auto_mode` (shared state, toast feedback), and when ON every line auto-advances after the same read-time formula (0.9s + 0.032s/char, cap 4.5s). Narration-only auto-advance (S55) remains the fallback when AUTO is off. Choice guard unchanged.
- **Part I Korean quality audit**: systematic scan found **208 machine-register narration lines** (~습니다 narration, literal artifacts) across chapter1-10 dialogue files. Fixed the 12 worst always-seen Chapter 1 lines (hidden_stump, dead_burner, forest_shrine, sq_echoes_ash, elia talks) to the house literary register.

### Verification
- 30/30 data JSON parse OK; headless boot 0 SCRIPT ERROR / Parse Error.

## S171 - 2026-07-10 (Part I emotional-beat illustration expansion)

### Done
- Generated and integrated nine new style-locked GPT Image CGs across Chapters 2 through 10.
- Added second visual beats for the first-sword loss, Blank Book discovery, Elia anchoring, the coast separation branch, Sable buried team, the BL-07 revelation, the silent remnant, Kairos departure, and the core approach.
- Attached every image to an existing dialogue line only; event keys, dialogue order, flags, branches, and the in-progress Korean rewrite remain untouched.
- Registered all nine plates in the PauseMenu Artbook and `ILLUSTRATION_CATALOG.md`.

### Art direction
- Used the active chapter CG nearest each beat as a direct style reference.
- Reserved the lower dialogue area and used back-facing, prop-led, or silhouette compositions where a new face could weaken established identity.
- Rejected a face-forward Blank Book draft for identity drift and regenerated the final version with costume-locked hands only.
- Prompts explicitly excluded grain, image noise, paper/canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, dense fog, and visibility-obscuring particles.

### Verification
- Godot 4.6.2 imported all nine CGs and booted the project with no script or parse errors; only known third-party addon and shutdown cleanup warnings remain.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- VN structural validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Visual clarity smoke passed with `fog=0 particles=0 vignette=0 lens=0 battle_dust=0 actor_callbacks=1 ui_hints=1`.
- All 154 active dialogue CG references resolve to 117 unique assets; all nine new plates imported with matching metadata.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

### Carried-forward localization work
- **Part I full Korean rewrite pass** (S167-style): ~196 remaining machine-register lines across chapter1-10 + spot-check dialogue lines. Marker query: narration speaker=="" with ~습니다체.

## S170 - 2026-07-07 (Part I Korean narration rewrite: all 235 flagged lines)

### Done
- Completed the queued Part I Korean pass: **all 235 machine-register narration/choice lines** across chapter1-10 dialogue files + epilogue rewritten to the house literary register (문어체 서술, ~였다/~했다), in four batches:
  - Batch A: ch1 (3) + ch2 (31) + ch3 (16) — Verdan market, Malet deal, the Sump, Belt waystation, Tobias intro.
  - Batch B: ch4 (11) + ch5 (27) + ch6 (35) — Drift Shelter, Crumbling Coast, Elia separation branch, the Seam arrival, BL-07 entrance.
  - Batch C: ch7 (9) + ch8 (17) + ch9 (13) + ch10 (38) + epilogue (35) — Threshold, ghost forest, Colorless Waste, the Seal (all three resolutions incl. seal_weave), all six ending epilogues.
- Representative fixes: "느슨한 돌 뒤에는 가죽으로 묶인 장부인 유포로 싸여 있습니다" → "헐거운 돌 뒤, 기름천에 싸인 것 — 가죽 장정의 장부였다." / "운동. 그들 뒤의 능선에" was dialogue-side (queued separately).
- Ending banner lines localized (ZERO BURN/PRESERVATION/WEAVE/ASH/SEAM 엔딩 문구).

### Verification
- Marker re-scan (narration speaker=="" with ~습니다체 + choice 명령체): **0 remaining** (was 235).
- 30/30 data JSON parse OK; headless boot 0 SCRIPT ERROR / Parse Error.

### Queued (next dedicated session)
- **Part I spoken-dialogue Korean pass**: speaker lines are also machine-register in many places (e.g. "Movement."→"운동.", "I've got you."→"나는 당신을 잡았습니다.", "에릴/어렐/에렐" name typos, 존댓말/반말 혼재). Detection needs a dialogue-aware heuristic (name-typo list, 직역 패턴), scope ~500+ lines — largest remaining localization debt.

## S172 - 2026-07-11 (Canonical battle-support illustration correction)

### Done
- Audited the active illustration tree and story canon before generating more CGs; the clearest live mismatch was the young, sighted Sable battle cut-in left over from the pre-S148 design.
- Replaced `cinematic_sable_echo_strike.png` in place with canonical blind-old-woman Sable/Halda while preserving the existing support-action runtime path.
- Replaced ten additional pre-S148 Sable story/ending plates in place across Chapters 6-8 and the epilogue/Weave ending, preserving their existing JSON, Artbook, and ending references.
- Generated transparent full-body battle-stage characters for Sable and Tobias, replacing Sable's cropped dialogue portrait and Tobias's opaque gray-background full-body card in combat.
- Connected both new assets in `battle_scene.gd` with safe fallbacks and registered them in the Artbook and illustration catalog.

### Art direction
- Sable lock: old blind woman, late 60s, short white hair, weathered coast-stone face, pale clouded eyes, dark violet-toned coat, small scar, composed and unhurried.
- Tobias lock: stern middle-aged recorder, weathered brown hair, short beard, charcoal layered coat, tactile record book, restrained violet ward geometry.
- Used built-in GPT Image with current canonical portraits, full-body art, battle sheets, and cut-ins as direct references.
- Generated stage characters on flat chroma backgrounds, removed the backgrounds locally, and rejected any result that did not preserve identity or clean alpha edges.

### Verification
- Godot 4.6.2 imported both alpha PNGs, the replacement cut-in, and all ten corrected story/ending plates successfully.
- Project import completed without script or parse errors; only the known VFX Library plugin shutdown noise remains.
- Enhanced visual-clarity smoke passed with `support_art=2`, confirming the live Sable and Tobias battle TextureRects resolve to the new transparent assets.
- Field Focus smoke passed with all 10 maps covered and battle opening at Resonance 25 / Limit 20.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- VN structural validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Asset audit passed: 11 landscape replacements are clean 1672x941 RGB plates; Sable and Tobias stage characters are 1024x1536 ARGB PNGs.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S173 - 2026-07-11 (First-exploration character sheets and visual clarity repair)

### Done
- Traced the missing field art to all three exploration actor paths constructing procedural `PixelSprite` placeholders even though authored Arrel and Elia frame sheets were already imported.
- Switched Arrel, companion Elia, and opening NPC Elia to their authored directional sheets with consistent scale, grounding offset, filtering, animation names, and safe procedural fallbacks for missing assets and non-Elia actors.
- Reworked the clean exploration baseline: removed the decorative HUD plate, permanent controls strip, opening location-art card, empty archive-resource rows, and the always-visible Memory Compass. The compass now appears only for a short memory-change event.
- Added a one-time clarity settings migration that disables a previously saved FPS overlay while retaining clean visuals, reduced motion, and disabled screen shake.
- Reduced the minimap footprint, suppressed raw map trigger rectangles, shrank low-detail interactive props, and added a restrained grass texture variant for the Rim Forest clean view so characters and paths read before environmental noise.
- Added a reusable first-exploration capture harness and expanded the visual smoke test to assert authored Arrel/Elia sheets and the compact clean HUD.

### Verification
- Godot 4.6.2 visual-clarity smoke passed with `exploration_sheets=2 compact_hud=1` plus the existing fog, particle, vignette, battle-dust, input-hint, callback, and support-art checks.
- Field Focus smoke passed across all 10 maps (`maps=10`, Resonance 25, Limit 20).
- Live 1280x720 Rim Forest capture passed and resolved Arrel/Elia to `arrel_sheet/idle_01.png` and `elia_sheet/idle_01.png`; screenshot saved to `tmp/visual_audit/rim_forest_first_exploration.png`.
- Godot emitted only the known shutdown resource-cleanup warnings; no script, parse, or assertion errors occurred.

## S174 - 2026-07-11 (Story-first combat loop: WITNESS and preservation victories)

### Audit finding
- The project already had broad feature coverage, but its repeatable combat loop returned too little of the player's story decisions. BREAK, resonance, stances, items, allies, and tactical objectives mostly rewarded damage optimization, while the defining promise — deciding what to remember or burn — was felt only in isolated moments.
- The battle command ribbon also placed seven equal-weight commands in one horizontal row, making the core choice harder to parse as systems accumulated.

### Done
- Added **WITNESS / 기억 읽기**, a turn-costing story combat action that reads the human memory trapped inside an enemy, reveals scan data, builds Limit and resonance, and guards Arrel while he listens.
- Ordinary enemies can now be released without killing them or burning a memory after 2 readings (3 for most Void enemies). Each enemy family has authored Korean and English echo lines rather than generic system copy.
- Story choice echo: listening to Elia's humming or choosing to keep her close reduces non-boss Void readings from 3 to 2. Earlier dialogue decisions now change a repeatable combat strategy.
- Bosses remain mandatory story confrontations, but completing their 3-step reading fractures the command controlling them and opens a BREAK turn instead of skipping the fight.
- Preservation victories grant a dedicated Grains bonus and bank one Field Focus for the next encounter. Resolution type, bonus, and focus gain are surfaced on the victory screen; permanent witness stats and category flags are saved.
- Added the `Hold the Name` tactical objective so the existing objective economy can explicitly reward story-first play.
- Rebuilt the command ribbon as a readable 4x2 grid: Attack, WITNESS, Burn, Defend / Item, Limit, Auto, Flee. WITNESS progress is visible directly on its command button.
- Added reusable story-combat smoke and 1280x720 capture harnesses, and expanded visual regression coverage for the command grid and WITNESS route.

### Verification
- Story combat smoke passed: 2-step reading, nonviolent release, Elia choice echo, +8 preservation Grains, and +1 Field Focus.
- Visual clarity smoke passed with `command_grid=4x2 witness=1` plus all existing exploration, battle, support-art, and obstruction checks.
- Field Focus smoke passed across all 10 maps (`maps=10`, Resonance 25, Limit 20).
- Live 1280x720 battle capture passed with visible `기억 읽기 1/2` and a measured 819x108 command grid; screenshot saved to `tmp/visual_audit/story_combat_witness.png`.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- VN structural validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S171 - 2026-07-08 (Part I spoken-dialogue Korean pass)

### Done
- Completed the queued **spoken-dialogue** Korean pass across all Part I dialogue (chapter1-10 + epilogue): every character line rewritten from machine-register to voice-locked natural Korean.
  - **Global fix (50 lines):** name typos 에릴/어렐/에렐→아렐, 말렛트→말렛, The 심/솔기/심(심)→씸, 뷰로→관리국, plus stray romaji (Sable/Elia/Arrel/Tobias/Kairos/Malet, Void Hole, Bureau) folded to Korean.
  - **Voice lock:** 아렐↔엘리아 반말, 세이블 노파 하대체(S148 canon), 토비아스 학자 하게체, 카이로스 격식 하게체, 말렛 정중 거래체(존댓말 유지), 이름 잃은 노인 경어체.
  - **Fatal mistranslations fixed:** "전투기"(fighter jet)→싸움꾼, "곰팡이"(mold-fungus)→거푸집, "구운 메모리"→태워진 기억, "그냥 편지"(letters=글자)→글자, "정박"(anchoring)→앵커링, "운동"(Movement)→움직임, seal_decision prompt "당신은 무엇을합니까?"→어떻게 하겠는가? + 3 choice labels, void_core "'The Name 아렐'"→'아렐이라는 이름'.
  - Batch counts: ch1 35 · ch2 46 · ch3 49 · ch4 54 · ch5 49 · ch6 80 · ch7 62(+echo-shell item) · ch8 64 · ch9 60 · ch10 57 · epilogue 77.

### Verification
- Typo/romaji scan: **0** remaining (에릴·어렐·에렐·말렛트·솔기·뷰로·Void·Bureau·The Name·굽기 all gone).
- Residual 존댓말: 14 lines, all legitimate polite-speech characters (Malet the broker, the nameless Old Man) — intentional, not machine-register.
- 30/30 data JSON parse OK; Godot 4.6.2 headless boot 0 SCRIPT ERROR / Parse Error.

### Status
- Part I Korean localization (narration S170 + dialogue S171) now fully house-register and voice-locked. Part II/III (VN scenes) were done in S167. Whole-game Korean pass complete.

## S175 - 2026-07-11 (Hangul typography, directional field cast, and Malet sheet)

### Audit findings
- The global body font preferred Latin display serifs before Hangul-capable fonts, so mixed Korean/Latin lines changed fallback face and baseline glyph by glyph.
- Authored field sheets were only wired for Arrel and Elia. Their four direction animation names also reused the same front or side source frames, so turning often produced no visible change.
- Malet, Tobias, Kairos, Nera, and Veil had canonical reference boards but no transparent field-frame folders. Malet therefore still rendered as a procedural placeholder.

### Done
- Rebuilt the Korean font chains around `Malgun Gothic` / `Noto Sans KR`, retained locale-aware Latin display fonts for English, and standardized UI labels on the Hangul-safe sans face.
- Improved the shared interface theme with restrained dark panels, gold focus states, clearer hover/pressed buttons, and stronger text contrast.
- Enlarged and widened the dialogue surface, raised normal body text to 18px, increased line spacing, strengthened the frame, and gave choices and control hints more room.
- Extracted clean transparent field frames from the canonical Malet reference and added idle, walk-left, walk-right, and cast animation sources.
- Added matching field-frame sets for Tobias, Kairos, Nera, and Veil; normalized their 160px source canvases to the same exploration footprint as the 128px Arrel/Elia cast.
- Reworked authored sheet animation mapping so front, rear, left, and right turns read distinctly. Original sheet art is preserved for front and both side directions; a restrained rear-hair view is derived where the source board has no chibi rear frame.
- Wired authored sheets into all matching NPC names, retained safe procedural fallbacks for characters without reference art, and made NPCs face the player before dialogue. Both canonical `Malet` and legacy `Mallet` keys resolve to the same sheet.
- Added deterministic extraction tools, seven-character directional smoke coverage, and live captures for the directional cast, Korean dialogue UI, and Malet in Verdan Market.
- Preserved Claude's committed spoken-dialogue patch without modifying its dialogue data.

### Verification
- Visual clarity smoke passed with seven authored exploration sheets, four directional turns, Korean-first font chain, and the existing clean-view/combat assertions.
- Field Focus and story-combat smoke tests passed unchanged.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- VN structural validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Verdan Market headless boot and live Malet capture passed; Malet resolved to `malet_sheet/move_left_01.png` while facing the nearby player.
- 1280x720 directional-cast and Korean-dialogue captures passed with `Malgun Gothic` as the first runtime face.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S176 - 2026-07-11 (Responsive exploration screen and story-facing HUD)

### Audit finding
- The clean-gameplay preset correctly removed fog, large overlays, long trails, and raw trigger rectangles, but it also removed nearly every piece of movement anticipation and moment-to-moment visual response. The result was readable but static.
- The current objective was rendered as another line inside the HP/resource stack, so the player's next story action did not hold enough visual priority.
- Interaction feedback was a bare `[E]` above Arrel and did not distinguish talking from inspecting.

### Done
- Restored a restrained 18px camera look-ahead in clean view, keeping movement responsive without hiding more than one tile of upcoming play space.
- Added a tiny fading Memory-diamond footfall echo for clean exploration. It replaces opaque dust and afterimage trails rather than stacking with them.
- Rebuilt interaction feedback as a compact gold action chip with keyboard/controller-aware input and localized `Talk/Inspect` intent.
- Reframed the active quest as a dedicated `STORY OBJECTIVE / 이야기 목표` card with stronger hierarchy and optional multi-step progress, while keeping archive resources and the permanent controls strip suppressed.
- Replaced large proximity outlines based on invisible Area2D bounds with an 8px pulsing Memory beacon, so discovery reads as game feedback instead of debug geometry.
- Updated the first-exploration and Malet field captures to exercise the new footfall and interaction states.

### Verification
- Visual clarity smoke passed with `footfall_echo=1`, `camera_lead=1`, `story_beacon=1`, and `objective_card=1`, plus all existing character-sheet, font, battle, and obstruction assertions.
- Field Focus and story-combat smoke tests passed unchanged.
- Verdan Market headless boot passed without script, parse, or assertion errors.
- 1280x720 Rim Forest and Malet interaction captures passed; only the known Godot shutdown resource-cleanup warnings remain.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S177 - 2026-07-12 (Gameplay and quality-of-life cohesion pass)

### Audit findings
- The project had many standalone features, but the moment-to-moment route between them was weak: the objective tracker used stale placeholder flags, the minimap showed only party positions, and failure recovery did not use the existing autosave slot.
- `Retry` on Game Over returned Arrel to the map at 30% HP instead of restoring a dependable checkpoint. New map entries were not guaranteed checkpoints.
- Movement-based random encounters offered no advance warning and most Void entries blocked escape, allowing repeat combat to override story pacing.
- Korean play still surfaced English-only tutorial hints and opening control toasts.

### Done
- Rewrote the main objective fallback for Chapters 1-10 against the actual map progression flags. Chapter 1 now advances from Elia to the mandatory Void Beast to the southern camp instead of permanently showing `Find Elia`.
- Added a pulsing gold story-target marker to the minimap. It resolves live NPC targets such as Malet, Tobias, and Sable, then switches to the correct chapter exit/core position after their story beat.
- Restored live minimap updates in Rim Forest so the opening chapter receives the same player, companion, and objective guidance as later maps.
- Added delayed map-entry checkpoints to SaveManager. A newly entered exploration map autosaves after one second, after position/flag restoration, while returning from battle to the same map does not overwrite the checkpoint.
- Changed Game Over retry to load the autosave checkpoint when available, surfaced its chapter/location in the defeat panel, retained a safer 50% HP legacy fallback, and made Load fall back to autosave when no manual slot exists.
- Added advance warning at 72% of a random-encounter threshold. Movement-based ambient encounters are now explicitly tagged and always escapable; mandatory story confrontations and bosses retain their restrictions.
- Localized all eight contextual tutorial hints and the four Chapter 1 control toasts for Korean play.
- Added a dedicated gameplay-QoL regression smoke covering objective progression, Malet minimap targeting, checkpoint hook presence, encounter warning, and guaranteed ambient escape.
- Repaired an unrelated one-character syntax typo (`if show_text:6`) discovered during the full boot gate, restoring the intended `if show_text:` without changing behavior.

### Verification
- Gameplay QoL smoke passed: `objective_progression=3 minimap_target=1 checkpoint_hook=1 encounter_warning=1 ambient_flee=1`.
- Visual clarity, Field Focus, and story-combat smoke tests all passed unchanged.
- Game Over scene booted headlessly with no script, parse, or assertion errors.
- Korean localization validator passed: 30 files, 1,568 fields, 19 speakers, 0 errors.
- VN structural validator passed: 19 files, 496 steps, 0 errors, 0 warnings.
- Live 1280x720 Rim Forest capture shows the updated Void Beast objective and gold minimap target.
- Godot emitted only the known shutdown resource-cleanup warnings; `git diff --check` passed apart from normal CRLF notices.

## S178 - 2026-07-12 (Field rendering noise reduction)

### Audit findings
- The authored AI character sheets retain very fine linework and color variation. At field scale those frequencies shimmer and read as edge noise even though the source PNGs are sharp.
- Most 32px terrain painters added random brightness to every pixel, then repeated four variations across the full map. Path, stone, wall, sand, and Void surfaces therefore competed with character silhouettes.
- The clean-gameplay preset removed screen-space particles but did not simplify the tile-generation stage or character sampling stage.

### Done
- Added a cached, non-destructive runtime sheet reconstruction for clean view: character frames are reduced to 75% with Lanczos sampling and reconstructed at their original canvas size. This suppresses sub-pixel AI line noise while preserving source assets, silhouette, scale, palette, and directional animation provenance.
- Applied the same low-noise reconstruction after derived rear-facing frames are built, so front/back/side directions retain one consistent texture character.
- Added source provenance names to reconstructed textures and updated visual/capture regression checks to validate the original frame path without requiring destructive file edits.
- Added a clean terrain routing layer in TilePainter. Grass, path, masonry, sand/cliff/rock, and Void families now use broad value groups and intentional lines instead of per-pixel random brightness when Clear Gameplay View is enabled.
- Added dedicated low-noise painters for paths, masonry, natural terrain, and Void surfaces; detailed rendering remains available when the user disables Clear Gameplay View.
- Kept player/NPC filtering, dimensions, collisions, map data, and all original character/tile source files unchanged.

### Verification
- Visual clarity smoke passed with `sheet_denoise=1 terrain_noise=low` plus all existing directional sheet, HUD, objective, battle, and obstruction assertions.
- 1280x720 Rim Forest capture shows flat readable ground groups and softened Arrel/Elia edges; the former per-pixel path speckle is removed.
- 1280x720 Verdan Market capture shows stable masonry bands and a cleaner Malet/Arrel interaction silhouette.
- Six-character directional gallery capture passed after runtime reconstruction.
- Original PNG assets were not overwritten or regenerated.

## S179 - 2026-07-12 (Chapter 1 epic cold open and first-minute agency)

### Audit findings
- A new game entered a 46-step VN prologue on an aftermath image. The writing and later choices were strong, but the player learned that something grand had happened instead of seeing it happen.
- The first meaningful input arrived after roughly two dozen lines, so the opening missed the project's 30-second action-feedback-reward loop even though later Chapter 1 choices already changed resources and combat routes.
- VN presentation still constructed a full-screen procedural grain layer despite the project's clean visual direction and the recent field-noise pass.

### Done
- Added `ch1_cold_open`, an eight-step prelude that shows the Rim sky fracture, the descending Void Beast, a memory-fire strike, and the original aftermath before handing off to `ch1_prologue` at its second step.
- Added a first-minute instinct choice with three immediately explained outcomes: keep Arrel's name for 4 Grains, guard Elia's lantern for a Smoke Bomb, or read the beast's rhythm and carry `vb_read` into the next Void Beast confrontation.
- Generated and integrated `story_ch1_rim_omen.png`, a clean 16:9 establishing CG based directly on the canonical Chapter 1 Arrel, Elia, Void Beast, and Rim Forest plates.
- Added data-driven VN `cg_motion`, `impact`, and `sfx` cues with pull-back, push-in, strike, violet, memory-fire, and heavy-impact profiles. Reduced Motion and Screen Shake settings suppress camera motion and nudges while preserving readable crossfades.
- Disabled the dormant VN grain pass so authored CG detail stays clean; scale now comes from composition, light, restrained motion, and sound.
- Registered the new plate in the Artbook and illustration catalog, and added a reusable cold-open UI/branch capture harness.

### Verification
- Cold-open runtime smoke passed with three visible routes, `vb_read` carry-over, noise-free VN presentation, and the new CG resolved through Godot's importer.
- VN structural validator passed: 20 files, 504 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Visual clarity smoke passed with all seven authored field sheets, low-noise terrain, 4x2 battle grid, WITNESS, and obstruction checks unchanged.
- Gameplay QoL, story-combat, and Field Focus smokes passed unchanged.
- Godot 4.6.2 imported the new CG and scripts without script or parse errors; only known addon and shutdown cleanup warnings remain.

## S172 - 2026-07-12 (Font & atmosphere upgrade: Korean literary serif)

### Problem
The whole game ran in Korean using `Malgun Gothic` (a system UI **sans/고딕**) for both titles and body — making a dark-fantasy literary VN read like a generic system dialog. Root cause: `UITheme.make_title_font`/`make_body_font` and `theme.tres` put Malgun Gothic first in the Korean stack.

### Done
- **Serif-first Korean stacks** (`scripts/utils/ui_theme.gd`): title & body now prefer 명조 세리프 — `Noto Serif KR` (ships on Win10+) → `Batang` (always present) → `Nanum Myeongjo` → Malgun fallback. UI/HUD/buttons keep a clean sans (`Pretendard`/Malgun) for legibility. Latin stacks unchanged (Cinzel/Cormorant).
- **Render quality helper** `_tune_font()`: grayscale AA + light hinting + auto subpixel positioning + mipmaps → smoother, premium serif edges that don't muddy when scaled.
- **theme.tres sync**: `SystemFont_body` → Noto Serif KR first; both subresources get hinting/subpixel/mipmaps; RichTextLabel `line_separation` 6→10 and size 17→18 for VN reading elegance.
- Updated two stale font asserts (capture_dialogue_interface, smoke_visual_clarity) from "Malgun Gothic" → "Noto Serif KR".

### Verification
- Godot 4.6.2 headless boot: 0 script errors.
- `smoke_visual_clarity`: **VISUAL_CLARITY_SMOKE_PASS** (font_chain=ko).
- `capture_dialogue_interface`: rendered live Korean dialogue (Malet line) — confirmed **Noto Serif KR** serif rendering; visually far more literary/atmospheric than the prior sans. Title screen uses the same `apply_title_font` so it inherits the serif.

### Notes
- Zero repo bloat (no bundled font binary) — relies on Noto Serif KR (Win10+) with Batang serif as guaranteed fallback, so worst case is still a serif, never the old sans.
- If cross-OS identical rendering is later wanted, bundle Noto Serif KR (OFL-licensed) as a FontFile.

## S180 - 2026-07-12 (GPT Image 2 tactile inventory and character presentation)

### Audit findings
- Consumables were mechanically distinct but only appeared as text in battle, Malet's item exchange, and victory rewards; their reward loop had no persistent visual language.
- Elia's battle-stage support art still depended on a stale `elia_companion` story flag, while her skills and party logic used `player_data.elia_with_party`. This could hide her visual support frame even when she was present.
- Malet's backstory stated that his eyes contained seventeen borrowed memories, but the final reveal had no dedicated image.

### Done
- Generated a six-piece consumable family with built-in GPT Image 2: Potion, Hi-Potion, Antidote, Firebomb, Smoke Bomb, and Grains. Split the chroma-key source sheet with `scripts/tools/extract_item_icons.py` and converted the final icons to clean alpha PNGs.
- Added canonical item icon paths and shared `GameManager` texture helpers. Battle item buttons, Malet's item buy/sell tab, victory drops, and the Grains-earned row now reuse the same assets.
- Generated `elia_battle_anchor_fullbody.png` and made it Elia's primary battle-stage art, preserving the prior animated sheet as an asset-missing fallback.
- Replaced the stale Elia companion visibility condition with the live `player_data.elia_with_party` state used by the rest of combat, restoring her stage art whenever she is actually travelling with Arrel.
- Generated `story_ch2_malet_seventeen_eyes.png`, attached it to the final line of `malet_backstory`, and registered it in the Artbook and illustration catalog.
- Extended the visual-clarity smoke to assert all five shop items, two battle items, and all three companion-stage images (Sable, Tobias, Elia).

### Verification
- Alpha audit passed for all six item icons and Elia: all images have RGBA alpha, transparent corners, and no chroma background retained.
- `smoke_visual_clarity.tscn` passed with `support_art=3 item_icons=2 shop_icons=5`.
- `smoke_story_combat.tscn` passed with the existing WITNESS, preservation, and Field Focus assertions.
- VN validator passed: 20 files, 504 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Godot 4.6.2 imported all new PNGs and booted the affected autoload/UI paths. Only the project's known headless resource-cleanup warnings remain after smoke shutdown.

## S181 - 2026-07-12 (GPT Image 2 story-turn illustration expansion)

### Audit findings
- The Part I dialogue already had broad CG coverage, but several important emotional reversals still reused an establishing plate for their decisive narration.
- Tobias's transition from isolated recorder to companion, the cost of separating from Elia, the proof of residue after a controlled burn, and the Forgotten Forest anchor strain all benefited from a distinct story image rather than another generic landscape.

### Done
- Generated and integrated four built-in GPT Image 2 full-screen CGs with clean lower dialogue space:
  - `story_ch3_tobias_fallen_records.png` — Tobias's first interruption at the Belt waystation.
  - `story_ch5_anchorless_horizon.png` — the surviving anchor between Arrel and Elia before the coast split.
  - `story_ch7_residue_after_trial.png` — Elia steadies Arrel after the controlled burn while Sable and Tobias witness the result.
  - `story_ch8_anchor_in_gale.png` — Elia holds Arrel's name against the Forgotten Forest.
- Attached each plate to the precise existing narration line in Chapters 3, 5, 7, and 8; dialogue order, event keys, branches, and Korean text are unchanged.
- Registered all four plates in the Artbook and `ILLUSTRATION_CATALOG.md`, retaining canonical appearance locks for Arrel, Elia, Tobias, and Sable.

### Verification
- Godot 4.6.2 imported all four new CGs successfully.
- Direct story wiring audit passed: 4 expected chapter/group/step references resolve to real image files.
- VN validator passed: 20 files, 504 steps, 0 errors, 0 warnings.
- Korean localization validator passed: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Full visual clarity smoke passed with battle, shop, companion-stage, clean-view, and WITNESS assertions unchanged.
- `git diff --check` passed apart from existing CRLF normalization notices; headless shutdown produced only the known resource-cleanup warnings.

## S173 - 2026-07-12 (Battle log Korean localization — Steam readiness)

### Problem
The game runs in Korean, but the entire in-battle text feed was English-only: the persistent `[INTEL]` telegraph banner (shown every enemy turn), tactical hints, and ~130 `battle_log.emit` lines (damage, burn, break, combo, items, enemy abilities, echoes, corruption, victory/defeat, drops, status). A dark-fantasy Korean VN reading English combat text every turn is a clear "still rough for Steam" blocker. (Objectives and WITNESS lines were already localized by prior passes.)

### Done (battle_manager.gd only — codex's battle_scene.gd left untouched)
- Added `_bl(en, ko)` locale helper (returns ko in Korean locale, else en).
- Localized `get_next_turn_hint()` (all 8 branches → the `[INTEL]` banner is now Korean), `_get_opening_tactical_hint()` (`[전술]`), and the Void Beast opener.
- Localized **all ~130 `battle_log.emit` lines** via wrapped `_bl("en","ko")` — combat damage/burn/break/combo/item/flee/enemy-ability/echo/corruption/last-stand/victory/defeat/drop/status/limit/residue/auto. Korean particles handled with 이(가)/을(를)/은(는).
- Fixed 3 lines where the Korean draft reordered `%d`↔`%s` relative to the positional args (would crash at runtime) — order realigned to the arg arrays.

### Verification
- Format-specifier audit: **154 `_bl()` pairs, 0 mismatches** (every KO string keeps the same %s/%d/%% multiset as its EN source).
- Godot 4.6.2 headless boot: 0 script errors.
- `smoke_story_combat`: **STORY_COMBAT_SMOKE_PASS** (real battle runs to resolution — no format-crash).
- Live re-capture: the `[INTEL]` banner now renders `적이 기본 공격을 준비한다.` (was English). Confirmed by screenshot.

### Notes
- The `[INTEL]` bracket tag itself is prefixed in `battle_scene.gd` (codex's active file) so it stays English as a stylized label; the message body is fully Korean. When codex localizes the tag it composes cleanly.
- Remaining battle-scene visual roughness (turn pips, cut-in placement, stance/limit overlap on the command grid, Elia debug sublabel) lives in `battle_scene.gd` — codex is actively editing it, so left to that pass to avoid collision.

## S174 - 2026-07-12 (Battle VFX label localization — continuing Steam readiness)

### Problem
After S173 localized the combat log, two prominent English labels remained in `battle_vfx.gd` (a clean file, not under codex's active edit):
- The enemy special-ability **telegraph warning** ("!! Life Drain !!", "!! Dark Barrier !!", etc.) — a big red banner shown every time a boss/enemy telegraphs a special.
- The floating **"MISS"** combat indicator on dodges/misses.

### Done (battle_vfx.gd only)
- `show_ability_warning`: 12 ability display names now locale-aware — 생명 흡수 / 어둠의 장벽 / 연격 / 독성 구름 / 작열 / 저주 / 그림자 소환 / 보이드 파동 / 절망 / 기절 일격 / 거울 장벽 / 충전 중... (English retained for en locale).
- Floating miss indicator: "MISS" → "빗나감" in Korean (matches the combat-log wording from S173 for consistency).
- Left as-is: decorative "*"/"v" glyphs (symbols, not text) and the burned-memory letter-reveal (uses MemoryManager memory titles — a separate data layer).

### Verification
- Godot 4.6.2 headless boot: 0 script errors.
- `smoke_story_combat`: STORY_COMBAT_SMOKE_PASS (real battle runs, telegraph path exercised, no crash).
- Change isolated to `battle_vfx.gd`; codex's 13 active files untouched.

### Battle text localization status
Combat log (S173) + ability warnings + miss (S174) are now Korean. Remaining English in battle is the burn-skill menu names/descs and status-icon codes, whose display lives in `battle_scene.gd` (codex's active file) — deferred to avoid collision.

## S175 - 2026-07-12 (Battle scene handoff: Korean UI + layout cleanup)

### Context
User asked Claude to take over battle_scene.gd polish (codex had it uncommitted). Confirmed codex stopped; committed codex's WIP + new CG assets as a checkpoint first (clean recovery point, boots clean), then Claude worked on top.

### Done (battle_scene.gd + capture harness)
- **Localized ~38 English battle-scene UI strings** to Korean via a new `_bl(en,ko)` helper: turn indicators (당신의 턴/적의 턴/최후의 저항/필드 포커스), turn-order pips (아군/적, was PLAYER/ENEMY), stance UI (자세/잔재/화염/공허), limit label (리밋), MEMORY CASCADE indicator, [INTEL]→[정보] prefix, boss/phase tags, burn/residue/item menu titles + empty states, echo/Elia panel headers, (no techniques), guard-focus VFX, ally-action indicator (localized name + technique map), enemy intro intel (약점/저항/탈출 불가/보이드 등급), objective card defaults, enemy subtitles (보이드 비스트/적대적 존재), COMBO→콤보, ally command labels (세이블:/토비아스:), victory reward labels, burn-preview panel (침식/취소/etc.). Kept BREAK/BROKEN/HP as universal mechanic terms.
- **Bug fix**: `_update_limit_button()` compared `child.text == "LIMIT"`, which never matched in Korean ("리밋") — the LIMIT command button's enabled/disabled state never updated in Korean locale. Now stores and uses a `limit_btn` reference.
- **Layout fix**: the stance row and limit gauge were both center-anchored at 0.78 — overlapping each other AND the command grid's top. Split them left (limit) / right (stance) within the 26px band above the grid; no more overlap (verified by re-render).
- **Parse fix**: an `:=` inferred a Variant from `Dictionary.get()` (warnings-as-errors) — explicitly typed as String.
- **Capture harness**: `capture_story_combat` now sets `GameManager.change_state(BATTLE)` so captures show the real battle (exploration HUD hidden), not a HUD-over-battle artifact.

### Verification
- `smoke_story_combat`: STORY_COMBAT_SMOKE_PASS (full real battle, all localized paths + limit button exercised).
- `smoke_visual_clarity`: VISUAL_CLARITY_SMOKE_PASS (font_chain=ko).
- Live re-renders (3 iterations, grounding loop) confirm: all battle UI Korean, stance/limit no longer overlap the command grid, exploration HUD correctly hidden.

### Notes
- The framed art mid-right is the enemy sprite showing `enemy_image`; the harness used a wide party-scene CG as a stand-in, so it letterboxes. Real battles use enemy portraits / pixel enemies — not a layout bug.

## S193 - 2026-07-15 (Story archive illustration expansion)

### Done
- Generated and visually audited four 16:9 narrative archive plates with current character-shot references as identity anchors: Arrel + Elia at Drift, Arrel + Malet in Verdan, Tobias at the Belt Waystation, and Sable on The Seam's lantern watch.
- Connected every plate to its unlocked Story Journal event and the Artbook without replacing existing asset files.
- Preserved the clean charcoal-line, muted painterly palette; every output was inspected before import for identity, hands, props, and visual noise.

## S194 - 2026-07-15 (World atlas, character sheets, and illustrated interface expansion)

### Done
- Added seven reference-anchored images: Sable's canonical turnaround, four practical Seam resident roles, four major Part I story CGs, and a text-safe Story Journal atlas backdrop.
- Illustrated the Ch5 coastal separation, Ch7 seven-lantern confession, Ch9 Kairos outcomes, and Ch10 burden choice in Events and Choices.
- Expanded the World Journal from 10 to 14 chapter-gated records with new lore about information delay, coast witnesses, refuge labor, and editorial responsibility.
- Added every new image to the Artbook, added title/category search, and exposed illustrated-event progress in the Journal header.
- Corrected Sable's Journal description to match her established older Void-walker design.

### Assets
- `sable_reference_turnaround_v1.png`
- `seam_residents_reference_sheet_v1.png`
- `archive_ch5_coastal_parting_v1.png`
- `archive_ch7_seven_lanterns_v1.png`
- `archive_ch9_kairos_outcomes_v1.png`
- `archive_ch10_burden_choice_v1.png`
- `ui_story_archive_atlas_v2.png`

### Verification
- Godot 4.6.2 headless editor import completed for all seven new images with no `SCRIPT ERROR` or `Parse Error`.
- `smoke_visual_clarity`: `VISUAL_CLARITY_SMOKE_PASS`, including 14 world records and all new Artbook paths.
- Representative `verdan_market.tscn` headless load completed successfully through NPC population and dialogue startup.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- `git diff --check` passed; only normal CRLF working-copy warnings were emitted.

## S220 - 2026-08-01 (Field population expansion, generated illustrations, and threat-aware minimap)

### Audit findings
- The existing world population system already supported NPCs, visible threats, caches, curios, and an Artbook, but the ten core maps were sparse and the minimap did not surface hostile encounters.
- Several explicit NPC field-art paths existed in the population data but were being collapsed to base archetypes instead of being used as live illustrations.
- The current thread did not expose a direct Luna-mode image-generation control, so the requested field art was produced through the available built-in image-generation path and locally audited before import.

### Done
- Generated and placed four story-consistent full-body field illustrations with transparent cutouts:
  - `lantern_cartographer_field_v1.png` - Rim Forest mapmaker with brass compass and rolled route map.
  - `root_tender_field_v1.png` - moss-hooded memory gardener with seed vial and pruning knife.
  - `cinder_antler_field_v1.png` - ash-and-bone deer with an ember antler.
  - `ledger_moth_swarm_field_v1.png` - paper-wing moth swarm around a ledger-like core.
- Expanded the ten core region populations with new route witnesses and hostile variants, while preserving story-gated access and collision-safe tile placement.
- Restored explicit NPC illustration paths as live art when present, so existing and newly generated field assets are actually used by spawned population actors.
- Added red diamond threat markers to the minimap with a ten-tile reveal range and a two-line legend alongside cache, relic, and local markers.
- Added `world_population_visual_gallery.json` to the Pause-menu Artbook so the new NPC/monster illustrations are discoverable in-game.
- Extended the population and visual-clarity smoke checks to validate live generated assets, minimap threat counts, and Artbook paths.

### Verification
- `WORLD_POPULATION_SMOKE_PASS maps=19 voices=84 visible_threats=50 caches=11 curios=10 atlas_gates=7 generated_field_assets=50`.
- `VISUAL_CLARITY_SMOKE_PASS` passed with the new population gallery and all existing visual assertions.
- Godot 4.6.2 headless import completed without `SCRIPT ERROR` or `Parse Error`; only the known ObjectDB/resource cleanup notices remain at forced headless exit.
- `git diff --check` passed; normal CRLF working-copy warnings remain.

## S215 - 2026-07-28 (Korean typography and readability overhaul)

### Audit findings
- The bundled Korean fonts and `canvas_items` scaling already prevented most missing-glyph and bitmap-upscale failures, but normal dialogue still used a thin 18px serif face. At 1280x720, this made long Korean strokes look fragile even when the glyph itself was technically correct.
- Primary, narration, system, and hint colors were all intentionally muted. Combined with 48-68% translucent archive panels and illustrated backdrops, this pushed important copy below a comfortable reading contrast.
- Battle and exploration screens still contained many explicit 9-11px labels, so changing the global theme alone could not repair objectives, HP/BREAK values, command rails, Field Flow hints, or inventory metadata.

### Done
- Rebuilt the shared typography contract around bundled `NotoSansKR-VF.ttf` for body and UI copy while preserving `NotoSerifKR-VF.ttf` for large authored titles. Added medium `FontVariation` emboldening, grayscale antialiasing, normal hinting, disabled subpixel positioning, and disabled mipmaps for stable Korean strokes.
- Raised the global default to 17px, normal body copy to 20px, and established 12px metadata / 13px compact UI / 20px body minimums. Removed every remaining explicit 8-11px size from gameplay UI, archive menus, options, achievements, codex, shop, puzzle, credits, and battle controls.
- Brightened the common text palette and speaker colors, strengthened one-pixel outlines, and increased panel opacity so copy reads on the first pass without flattening the charcoal-and-amber art direction.
- Reworked field dialogue to a 20/23/26px accessibility scale, enlarged speaker names and choices, expanded the reading panel, and gave dialogue/VN copy a stronger outline, shadow, and line spacing.
- Upgraded exploration HUD, Field Flow, Memory Compass, tactical objective, combat cue, turn order, HP/BREAK, field readout, command deck, item tray, companion commands, stance/echo rails, victory report, and memory-burn preview to the new minimum-size and contrast contract.
- Raised inventory panel opacity and copy contrast while keeping the generated archive frame and item art visible. Equipment cards, filters, Quick Kit, field notes, and footer hints now share the same readable sans weight.
- Added `smoke_text_readability.tscn`, covering font source/weight, palette contrast, dialogue size/outline, opaque reading surfaces, Field Flow/Compass minimums, battle objective/HP/readout sizes, and the 15px command floor.

### Verification
- `TEXT_READABILITY_SMOKE_PASS body=20 ui_floor=12 battle=15 contrast=high weight=medium`.
- All 23 gameplay/UI smoke scenes passed with no `SCRIPT ERROR`, `Parse Error`, invalid access, or assertion failure.
- OpenGL captures were regenerated and visually inspected at native output:
  - `tmp/visual_audit/dialogue_interface_ko.png`
  - `tmp/visual_audit/hybrid_battle_stage.png`
  - `tmp/visual_audit/field_flow_approach.png`
  - `tmp/visual_audit/font_scaling_1080p.png` (1920x1080 framebuffer, 1280x720 logical viewport, 1.50 canvas scale)
  - `memoria_inventory_visual_upgrade.png` in the Godot user-data capture directory.
- VN validation passed: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization passed: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Isolated 300-frame `belt_waystation.tscn` boot passed through Arrel, Elia, Tobias, world population, visible threats, and autosave. `git diff --check` passed; only normal Windows LF-to-CRLF notices were emitted.
- Godot's known forced-exit ObjectDB/resource cleanup notices remain shutdown noise; no gameplay script or runtime-access error occurred.

## S214 - 2026-07-28 (Field Flow foundation overhaul)

### Audit findings
- MEMORIA had many combat systems, but the field still resolved to `walk -> warning toast -> forced scene change`. Movement quality, visible hunts, random pursuit, and the first battle turn did not influence one another.
- Visible world hunts were anonymous `Area2D` contact triggers. Their small floor mark did not communicate detection pressure, and touching one immediately discarded the player's approach.
- Random encounters warned at 72% but offered no active field response beyond waiting for combat and fleeing there.
- Field Focus and the new approach cue initially played underneath the long battle-identification overlay. The intro also faded its labels one after another, making ordinary encounters wait more than three seconds before control.
- Authored hostile field paintings were discarded at battle start, so many visible hunts became an unrelated generic procedural enemy on the combat stage.

### Done
- Added `FieldFlow`, a persistent exploration controller on Arrel. Real travelled distance builds a 0-100 Flow bank; standing still releases it, while moving under threat builds it faster.
- Added `PHASE STEP` on Ctrl / RB. It spends 42 Flow for a short committed burst with directional body stretch, denser live-frame afterimages, memory-blue fracture rails, pulse ring, planted dash direction, and adaptive camera zoom-out.
- Added `FieldThreat` and converted all 32 visible hunts across 19 populated maps. Distance now drives pressure, hostile lean/color response, a pulsing detection aura, sight line, contact core, and the shared bottom-center field readout.
- Rebuilt random pursuit into an active field problem. The warning feeds the same pressure readout, and a timed Phase Step can break the trail and reduce encounter progress instead of merely postponing a forced battle.
- Added three field-to-battle approach routes:
  - `AMBUSH`: cross contact during Phase Step or reach a visible threat with high Flow; starts with Resonance, Limit, and BREAK pressure.
  - `GUARDED`: hold composure as pressure closes; starts with Resonance/Limit and guards the first hostile blow.
  - `WITNESS`: answer pressure with Memory Pulse; reveals the first WITNESS record and enemy scan before blades meet.
- Added a persistent `FIELD FLOW` HUD with live Flow, pressure, approach forecast, Phase Step cost/readiness, and localized keyboard/controller hints.
- Carried the selected approach through `BattleManager` into the combat scene. The opening is now presented after enemy identification and field-directive selection, so it is a visible tactical handoff rather than a hidden bonus.
- Shortened the ordinary battle intro by holding briefly and fading all elements in parallel instead of sequentially. Existing tactical identification remains, but repeated encounters return control substantially sooner.
- Preserved visual identity by sending every visible hunt's authored field hostile image into its battle stage. No new face art or disconnected flat CG was added; this pass upgrades the live rendering and interaction path.
- Added `smoke_field_flow` and repeatable OpenGL captures for approach, live Phase Step, and the resulting battle opening. Extended world-population smoke coverage for `FieldThreat`, aura/sight telegraphs, and field-to-battle art continuity.

### Verification
- `FIELD_FLOW_SMOKE_PASS routes=3 phase_step=active pursuit_break=active battle_handoff=active`.
- All 22 gameplay/UI smoke scenes passed in three parallel groups with no `SCRIPT ERROR`, `Parse Error`, invalid access, or assertion failure.
- `WORLD_POPULATION_SMOKE_PASS`: 19 maps, 64 voices, 32 converted visible threats, 11 caches, 10 curios, seven atlas gates, and 31 authored field assets.
- OpenGL 1280x720 captures were inspected:
  - `tmp/visual_audit/field_flow_approach.png`
  - `tmp/visual_audit/field_flow_phase_step.png`
  - `tmp/visual_audit/field_flow_battle_handoff.png`
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Isolated 300-frame `belt_waystation.tscn` boot passed through Arrel, Elia, Tobias, world population, and both live visible threats.
- Godot's known forced-exit ObjectDB/resource cleanup notices remain shutdown noise; no gameplay script or runtime-access error occurred.

## S213 - 2026-07-26 (Unified field-character motion and presentation)

### Audit findings
- S210 had already replaced the frozen sliding pose with a real four-frame procedural walk cycle, and S150/S57 supplied responsive acceleration, breadcrumb following, camera lead, sprinting, and travel-distance footsteps. The remaining weakness was presentation consistency rather than missing authored frames.
- Player, companion, story NPC, and ambient walker code used separate shadow/ring rules. Static characters breathed, but background walkers still translated as a scaled sprite root without matching body weight, and entering dialogue could leave the player or companion frozen in a moving pose.
- The current field paintings already preserve the cast identities. Replacing them with newly generated faces would introduce drift, so this pass keeps every authored Arrel/Elia/Sable/NPC frame and improves the live rendering layer around it.

### Done
- Added `FieldActorVisuals`, a shared field-character presentation helper used by Arrel, companions, story NPCs, and procedural ambient citizens. It supplies two-layer oval contact shadows, a restrained role-colored Memory contact edge, scale compensation for direct-root ambient sprites, and a small live motion driver.
- Added `field_actor_finish.gdshader`: a low-noise dark silhouette edge, subtle cool grade, and character-specific upper-edge accent. The shader preserves the original illustration palette and remains compatible with map lighting.
- Upgraded Arrel and companion locomotion with planted cardinal pivots, brief step compression synchronized to real travelled distance, adaptive shadow lift/offset, and movement-state settling when dialogue or menus interrupt exploration.
- Replaced square footstep dust blocks with alternating, terrain-colored direction-aware wisps. Sprint echoes now copy the live frame offset, tilt, scale, and texture filter so they trail the rendered body rather than a detached default pose.
- Upgraded ambient market walkers with distance-driven bob, lean, foot plant, idle weight transfer, shared silhouette finishing, and scale-compensated grounding. Static story NPCs now use the same finish and receive a restrained offset/rotation weight shift without changing their authored foot baseline.
- Added `capture_field_motion.tscn` for a repeatable live 1280x720 sprint composition and expanded movement/visual clarity smoke contracts for shared finishing, pivot planting, dialogue settling, ambient motion drivers, and layered grounding.

### Verification
- Godot 4.6.2 headless editor import completed with no `SCRIPT ERROR` or `Parse Error`.
- All 21 gameplay/UI smoke scenes passed. `MOVEMENT_NATURALISM_SMOKE_PASS` reports planted turns, distance footsteps, breadcrumb following, ambient gait, shared finish, and grounding; `FIELD_ANIMATION_SMOKE_PASS` still reports 24 distinct walk-direction cycles and the 50px cast-height contract.
- OpenGL 1280x720 captures for Rim Forest, Malet interaction, and the new Verdan sprint composition were inspected. The first overly dark shader iteration was corrected; final captures preserve the authored palette while improving silhouette separation and floor contact.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Isolated 300-frame `verdan_market.tscn` boot passed through player, Malet, Elia companion, world population, and checkpoint autosave.
- `git diff --check` passed; only normal Windows LF-to-CRLF working-copy notices were emitted.

## S212 - 2026-07-26 (GPT Image battle-supply moments)

### Audit findings
- The S200-S211 passes already cover the major story, map, memory-loss, resonance, archive, and character beats with live illustration consumers. Adding more narrative interstitials here would repeat existing scenes.
- The repeatable item loop remained visually thin: sixteen usable battle items shared small inventory icons and combat-log text, even when their outcomes were mechanically distinct.
- Grouping by effect keeps feedback readable and avoids seven near-duplicate versions of the same potion or relic. The live item families are recovery, cure, burn, withdrawal, witness, guard, and scan.

### Done
- Used the built-in GPT Image skill in `stylized-concept` mode to generate seven cohesive 1672x941, prop-led battle action plates under `assets/cg/generated/gameplay_moments/`.
- Preserved cast identity by showing tools, black-gloved hands, back-facing silhouettes, memory threads, fire arcs, smoke, anchor rings, and chalk fault lines instead of inventing new character faces.
- Added `BattleManager.item_used(item_id, item_type)` and connected it to the battle scene. Every successful item use now resolves the correct generated art, a type-specific accent, and a short localized Combat Beat cue before the normal mechanical result.
- Covered all sixteen current items through the seven live types: Potion/Hi-Potion/Lantern Salve/Seed Capsule; Antidote/Root Balm; Firebomb/Cinder Vial; Smoke Bomb/Signal Jammer; Witness Ink/Name Thread/Compass Shard/Witness Knot; Anchor Lantern; and Ledger Chalk.
- Added explicit `ANCHOR` and `SCAN` labels/colors to the battle supply tray, registered all seven images in the interface Artbook manifest, and extended battle/interface smoke contracts for every path and consumer.
- Added optional real-render capture output to `smoke_battle_command_deck.gd`; it records each effect after the battle intro finishes so future art changes can be compared in the actual 1280x720 composition.

### Verification
- Godot 4.6.2 editor imported all seven PNGs with no `SCRIPT ERROR` or `Parse Error`.
- All 21 gameplay/UI smoke scenes passed. `BATTLE_COMMAND_DECK_SMOKE_PASS` reports `actions=8`, `item_moments=7`; `INTERFACE_VISUAL_UPGRADE_SMOKE_PASS` verifies all seven Artbook paths.
- OpenGL 1280x720 captures in `tmp/visual_audit/item_moment_*.png` were inspected for all seven types. The generated action remains readable behind the tactical cue and clears after a short cut-in instead of obscuring the ongoing battle.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Isolated 300-frame `verdan_market.tscn` boot passed through player, four ambient voices, Malet, Elia companion startup, world population, and checkpoint autosave.
- `git diff --check` passed; only normal Windows LF-to-CRLF working-copy notices were emitted.

## S211 - 2026-07-26 (Post-Claude upgrade audit and Story Log witness ledger)

### Audit findings
- Reviewed the S203-S210 change set through the committed diffs, live asset consumers, current manifests, and the latest 1280x720 OpenGL captures. The battle-stage grounding, x1.0/x1.5/x2.0 tempo, Quick Kit/Smart Heal flow, native-resolution text rendering, procedural walk cycles, ambient field scale, and 50 new late-story/combat/rewrite plates are all connected to active runtime paths.
- The current CG library already covers the meaningful story gaps. Another narrative batch would duplicate live beats rather than improve the playable build.
- The new Story Log was the clear presentation exception: dense recalled dialogue still used a plain near-black panel while Pause, Journal, Codex, Inventory, and Shop used a coherent iron-bound archive family.
- Opening Story Log directly with `L` did not pause the scene. Player, ambient NPC, and dialogue state could continue changing underneath the full-screen reading overlay; the existing `_was_paused` field was captured too late and never restored.

### Done
- Generated and visually inspected `ui_story_log_archive_v1.png` with built-in GPT Image, using the current Story Journal and Pause Archive surfaces as style references.
- Integrated the 1672x941, text-free witness-ledger backdrop through `StoryLog.BACKDROP_PATH`. The center remains a quiet single reading surface; quill, compass, closed records, gold inlay, and restrained memory-thread detail stay at the margins.
- Registered the new interface asset in `data/interface_visual_gallery.json` and the Artbook/interface smoke contract.
- Story Log now pauses when opened from live exploration or dialogue and restores the exact prior pause state on close. Opening it from the already-paused Pause Menu therefore remains paused, while direct `L` use safely resumes play afterward.
- Extended Story QoL smoke coverage for the generated backdrop path, live TextureRect consumer, pause-on-open behavior, and prior-state restoration.
- Preserved the two missing Godot UID sidecars generated for S210's `capture_font_scaling.gd` and `smoke_field_animation.gd`.

### Verification
- Godot 4.6.2 headless editor import completed for the new PNG with no `SCRIPT ERROR` or `Parse Error`.
- All 21 gameplay/UI smoke scenes passed. Focused results include `STORY_QOL_SMOKE_PASS`, `INTERFACE_VISUAL_UPGRADE_SMOKE_PASS`, `FIELD_ANIMATION_SMOKE_PASS`, `BATTLE_COMMAND_DECK_SMOKE_PASS`, `VISUAL_CLARITY_SMOKE_PASS`, and both 25-asset illustration consumer suites.
- Real OpenGL capture `tmp/visual_audit/story_log_ko.png` was inspected at 1280x720; Korean dialogue remains readable while the new archive frame is visible without competing with text.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Isolated 300-frame `verdan_market.tscn` boot passed through four ambient voices, Elia companion startup, world population, and checkpoint autosave with no script, parse, invalid-access, or invalid-call error.
- `git diff --check` passed; only normal Windows LF-to-CRLF notices were emitted.

## S218 - 2026-07-26 (Codex recording switch — and two mistakes it exposed)

### Done
- `Codex.suppress_recording` gates every record path (`battle_started`, `battle_ended`, memory added/burned) **and** `_save_data()` itself, so nothing slips through another route. All 20 test harnesses that fabricate battles now set it in `_ready()`.
- `Codex.purge_unknown_entries()` cleans entries outside the known roster. It is never called automatically, reports exactly what it removed, and does not write while recording is suppressed.

### Two mistakes made and corrected in this session
Both are worth recording because the second was caused by the fix for the first.

**1. The purge deleted real enemies.** Running it removed 11 entries, and two of them — `Coastal Void Beast` and `Void Wraith` — are genuine content defined inline in `crumbling_coast.gd` and `the_seam.gd` encounter pools. The S217 roster was built from `WorldPopulation` hunts plus `ENEMY_PRESETS`, which misses every enemy declared inside a map script.
The roster is now based on `GameManager.ENEMY_NAMES_KO` — a Korean name existing for an enemy is exactly the signal that it is real content — with hunt data layered on top for region hints. The deleted entries were restored (encounter counts could not be recovered). The smoke now asserts every name in `ENEMY_NAMES_KO` is in the roster, with the failure message stating the consequence: *"정리 기능이 이 항목을 지운다."*

**2. The new test then wiped the codex down to one entry.** `purge_unknown_entries()` was written to temporarily disable suppression so it could always save. The smoke called it with synthetic data and that bypass wrote the test's scratch state straight into `user://codex.json` — a switch added to *prevent* writes became the path that destroyed them. It now refuses to save while suppressed, and the test asserts suppression is on before touching anything.

### Also fixed
`smoke_movement_naturalism` was genuinely flaky, not load noise: three identical runs with no code change gave 1 failure. It asserted the player exceeds 110 px/s within exactly 8 physics frames, which is timing-dependent when engine launches are stacked. It now polls up to 20 frames for the same condition — the guarantee ("responsive") is unchanged, the timing sensitivity is gone. Four consecutive runs pass.

### Verification
- Full smoke suite (24 scenes) passes, then `codex.json` compared byte-for-byte before and after: **unchanged**. The same check was repeated across all 14 capture harnesses: **unchanged**.
- **Falsifiability checked:** removing the canonical roster base fails with `'Alley Rat' 는 실제 게임 적인데 도감 명단에 없다`.
- VN: 20 files, 504 steps, 0 errors. Korean: 31 files, 1,583 fields, 0 errors. `verdan_market.tscn` boot clean.
- Developer codex left at 6 real entries.

## S217 - 2026-07-26 (RPG depth: equipment comparison, bestiary hints, shop rates, journal leads)

### Audit findings
- **The shop showed absolute stats only.** `ATK +8` on a blade means nothing when you are already wearing `ATK +15`; players found out a purchase was a downgrade only after buying. There was also no owned-but-unequipped pool, so the shop is the one place a comparison can happen.
- **Prices were bare numbers.** `Price: 80 Grains` never said whether 80 was steep, whether you could afford it, or how it compared to similar goods.
- **The bestiary only contained enemies already met.** Unmet enemies did not exist in the UI at all, so there was no sense of completeness and nothing to chase. The unscanned message — `[Not yet scanned, use Tobias: Analyze]` — named a character but not the action, the condition, or the alternative, and was English in a Korean-first game.
- **The journal recorded only finished things.** Nothing told the player which regions still held untouched caches, relics or resonance points.

### Done
- **Equipment comparison.** `GameManager.compare_equipment()` / `format_equipment_delta()` compute the delta against the slot's current item, correctly accounting for its upgrade level. The shop detail shows the delta line and colours it green for an upgrade, red for a downgrade, violet for a sidegrade.
- **Shop rates.** Prices now carry two contexts: affordability (`잔액 420` or `20 부족`) and a market label (`시세 이하 / 수준 / 이상`) computed against the average price of comparable goods — the same slot for equipment, all consumables for items.
- **Bestiary roster and hints.** The codex now shows every enemy the game knows about, with a `기록 N / M` header. Unmet entries appear as `???` with the region where they are actually reported. **Locations are never invented:** the roster is built from `WorldPopulation` hunt data (which carries real map assignments) plus `ENEMY_PRESETS`, and any enemy without location data says only "아직 마주치지 않았습니다". The unscanned text now states what is missing, how to get it, and both routes to it, in Korean.
- **Journal leads tab.** A new `미해결` tab counts per region what remains untouched — caches, regional relics, resonance points — derived from the live flag state. It reports counts, not coordinates, so it orients without replacing exploration.
- Localized the journal's quest entries now that the quest data carries Korean.

### Verification
- `RPG_DEPTH_SMOKE_PASS compare=1 bestiary=1 rates=1 leads=1`, asserting: an empty slot makes the item's stats the gain; a worse weapon reports negative and is not flagged an upgrade; upgrade levels shift the baseline; the price line distinguishes affordable from short; cheap and expensive goods get different rate labels; the roster gathers at least eight enemies with real regions; hints never fabricate a region; the leads tab lists open regions and reports empty once everything is flagged.
- **Falsifiability checked:** zeroing the comparison baseline fails with `더 나쁜 무기는 하락으로 표시되어야 한다 (3)`.
- **Two defects were caught by rendering rather than by the tests:**
  - `_create_tab()` only builds a button and returns it; the caller must parent it. The new `미해결` tab was created but never added, so the feature existed and was unreachable. The smoke now asserts the button has a parent.
  - The bestiary header counted only roster members while the list showed everything, so the numbers disagreed. The denominator is now roster ∪ recorded.
- All 24 smoke scenes pass. VN: 20 files, 504 steps, 0 errors. Korean: 31 files, 1,583 fields, 0 errors. `verdan_market.tscn` boot clean.
- New `capture_rpg_depth` renders shop, codex and journal in one sheet.

### Note
The bestiary on this machine contains entries like `Cleanup Victory` and `Field Focus Dummy` — enemies invented by smoke scenes and written into the persistent codex, since `Codex` records on `battle_started` and saves to `user://`. Players never run those scenes, so shipped saves are unaffected, but a developer's codex accumulates test junk. Worth a `suppress_recording` switch if it becomes annoying.

## S216 - 2026-07-26 (RPG systems outside battle: quest tracker, quest Korean, minimap POIs)

### Audit findings
Zooming the minimap and HUD in real renders turned up three concrete defects, all in systems that already existed but were half-wired:
- **Side quests had no Korean at all.** `grep -c "title_ko\|desc_ko"` returned 0 across the whole quest file. The game defaults to Korean, so accepting a quest put `Echoes in the Ash` and English step text into an otherwise Korean HUD.
- **A side quest replaced the story objective.** `_update_quest_tracker` looped over quests, `break`ing on the first active one and skipping the story calculation entirely. Accepting one request made the main objective vanish — and the card still read `◆ 이야기 흐름` while showing a side quest.
- **The minimap showed nothing that exists in the world.** It drew the player, the companion and the story objective. The chests, caches, regional curios and residents actually placed in every map were invisible on it.

A fourth defect surfaced while fixing the first: `SideQuest.get_all_quests()` builds a *new* dictionary per quest, copying a fixed key list. Adding `title_ko` to the data was not enough — the accessor silently dropped it, along with `steps`, so callers could never localize or read step counts.

### Done
- **Korean for all six side quests** — titles, descriptions and all 22 step lines — plus `SideQuest.loc()` and `get_current_step_text()`. `get_all_quests()` now carries the localized fields and the step array through instead of discarding them.
- **The tracker shows both threads.** The story objective always stays; active requests are listed under an `진행 중인 의뢰` header with per-quest progress `(1/4)` and the current step text, so the player can see what to do next without opening the journal. The card header no longer mislabels a request as the story thread.
- **Minimap points of interest.** Caches, regional relics and residents now appear, each with its own colour *and* shape (relics are rotated diamonds, so the map does not depend on colour alone). They reveal only within six tiles of the player, which keeps exploration meaningful and matches the memory-pulse fiction. The legend is a `RichTextLabel` whose words are tinted to match their markers — a plain white word list would not have told anyone which colour meant what.
- Nudged the cache marker to pale amber so it no longer reads as the orange-gold objective diamond.

### Verification
- `RPG_SYSTEMS_SMOKE_PASS quests=6`, asserting: every quest title and step is Hangul under `ko` and stays English under `en`; the story objective survives accepting a request; the request line carries title, progress and next step; POIs are collected from the map; distant POIs stay hidden and near ones reveal; the legend appears with them.
- **Falsifiability checked:** removing `title_ko` from `get_all_quests()` fails with `한국어 로케일에서 'Echoes in the Ash' 는 번역되어야 한다`.
- One real bug was found by the test itself: POIs were collected once at minimap creation, but `WorldPopulation` parents its actors under a `WorldPopulation` container *after* the map's `_ready`. The collector now scans that container too and rescans lazily when the child count changes — market POIs went 2 → 7.
- All 23 smoke scenes pass. VN: 20 files, 504 steps, 0 errors. Korean: 31 files, 1,583 fields, 0 errors. 300-frame `verdan_market.tscn` boot clean.
- New `capture_rpg_hud` renders the tracker and minimap together for visual audit.

## S215 - 2026-07-26 (Removing the noise: nearest-neighbour downscaling of painted art)

### Audit findings
Rather than eyeballing, noise was **measured**: a high-frequency energy map (image minus its own 1px Gaussian blur, averaged over an 80px grid) run over real renders, which separates pixel-level speckle from authored detail.

The battle stage band scored a mean of **2.07**, with hotspots on the companion, the enemy plate, and the background. Zooming those cells showed the cause plainly: Elia's hair and robe were riddled with bright dithered dots, and the background spires broke into stair-stepped crunch.

One root cause explains all of it. `project.godot` sets `textures/canvas_textures/default_texture_filter=0` — **Nearest**. That is correct for the pixel-art tiles and character sheets the setting was chosen for, but every hand-painted illustration inherits it too. Reducing a 1672px painting to roughly 300px by throwing pixels away turns dithered alpha into speckle and clean silhouettes into staircases.

S214 set `LINEAR_WITH_MIPMAPS` on the battler plates and the measurement did not move at all. The reason: `mipmaps/generate=false` in the `.import` files, so there were no mipmaps for the filter to use and it silently degraded to plain linear. Worse, `--headless --quit` does not run the import pipeline, so the flag change appeared to do nothing until `--headless --import` was run explicitly.

### Done
- **Enabled mipmap generation** on 57 large illustration textures: `assets/cg/game_image`, `assets/cg/character_shots`, `assets/portraits/character_shots`, and the five recurring enemy cut-ins. Pixel-art directories (`assets/sprites`, tilesets) were deliberately left alone — Nearest is right for them.
- **Set linear filtering on the illustration consumers** that were silently inheriting Nearest: the battle background, the two side stage plates, the VN CG layers (current, next, detail), and the full-screen `CgViewer`.
- Left Arrel on his existing profile. He scores high on the noise metric, but zooming confirms that is authored line art, not artifact — blurring him would be the wrong fix.

### Verification
- Battle stage band: mean **2.07 → 1.50** (−28%). The companion hotspot (6.18) dropped out of the top eight entirely; the enemy/background hotspot fell 8.04 → 3.92 (−51%).
- VN CG region: mean **2.97 → 2.35** (−21%).
- Zoomed before/after crops confirm the numbers: Elia's speckle is gone and reads as painted hair, and the background spires are smooth.
- New `smoke_texture_filtering` guards both directions of the contract — illustrations must be linear, pixel-art tilemaps must not be blurred — and additionally asserts `mipmaps/generate=true` in the `.import` files, because the filter alone is meaningless without them. **Falsifiability checked:** removing the battle background's filter line fails it with `전투 배경은 선형 축소를 써야 한다 (현재 0)`.
- All 22 smoke scenes pass. VN: 20 files, 504 steps, 0 errors. Korean: 31 files, 1,583 fields, 0 errors. 300-frame `verdan_market.tscn` boot clean.

### Note for future sessions
The VN film grain is already disabled by default and `capture_ch1_cold_open` asserts it stays that way, so the remaining noise was never an overlay — it was always the sampling filter. If art ever looks speckled again, check `texture_filter` on the consumer **and** `mipmaps/generate` in the `.import`, then reimport with `--headless --import`.

## S214 - 2026-07-26 (Battle presentation: a hit-flash bug, shared lighting, plate filtering)

### Audit findings
Previous sessions judged the battle from idle frames. This one captured the fight **in motion** — a four-shot sheet of idle, lunge, impact, and 2.5 seconds later — which immediately exposed something no still frame could show:
- **The enemy plate lost its stage blend on every hit.** `_apply_hit_shader` swapped in a flash shader and its fade-out callback set `material = null`, discarding the oval mask and edge fade. Since it fires on every basic attack, the enemy spent essentially the whole fight as a hard-edged rectangular card. This is the third instance of the same "clear to null" pattern; S209 fixed two others.
- **The cast was lit independently of the arena.** The 3D stage has a warm key light and a biome accent; the 2D battlers were drawn at their source brightness. No matter how good the stage got, the characters read as pasted onto it.
- **Painterly plates were downscaled with nearest-neighbour filtering.** The project default is `default_texture_filter=0`, and the ally/support/enemy illustration plates never overrode it, so 1672px artwork reduced to roughly 300px threw away pixels. Elia's dithered alpha turned into speckle — visible in every capture since S209 and wrongly assumed to be source-art quality.

### Done
- **Fixed the hit-flash material clear.** The fade-out now calls `_restore_plate_material` instead of nulling. After the last remaining `material = null` was removed, none are left in the battle scene or `BattleVFX`.
- **Shared stage lighting.** `battle_stage_blend.gdshader` gained a key-light rim and a biome ambient tint, driven from `STAGE_KEY_DIRECTION`/`STAGE_KEY_COLOR` that mirror the 3D key light. The rim finds the silhouette from the alpha gradient and lights only the side facing the key.
  **The rim is applied only to crisply cut sprites.** The first attempt put it on everything and turned Elia's feathered portrait edge into gold speckle — the two art styles cannot take the same treatment, so painterly plates receive the ambient tint alone.
- **Mipmapped downscaling** on the ally, support, and enemy illustration plates. The procedural pixel-art enemy fallback keeps nearest, which is correct for it.

### Verification
- `HYBRID_COUPLING_SMOKE_PASS ... plate_restore=1`, with new assertions that plates use mipmapped filtering, that the stage lighting parameters are present on both a plate and the hero sprite, and that the stage blend survives a hit.
- **Falsifiability checked:** reintroducing `node.material = null` in the flash callback fails the guard with `피격 연출이 끝난 뒤 무대 블렌드가 복구되지 않았다`.
- The action-moment sheet was re-shot after the fix: the enemy returns to its soft-edged plate 2.5s after impact instead of staying a rectangle.
- All 21 smoke scenes pass. VN: 20 files, 504 steps, 0 errors. Korean: 31 files, 1,583 fields, 0 errors. 300-frame `verdan_market.tscn` boot clean.
- Added `capture_battle_moments` so future sessions judge the battle in motion rather than at rest.

### Still open
The full-screen enemy action cut-in plays at 0.72 alpha on ordinary attacks, covering the arena and cast for a beat. It does clear correctly, so it is a pacing/design question rather than a bug, but it works against the depth work of the last four sessions and is worth revisiting.

## S213 - 2026-07-26 (Foreground parallax and a stage that reacts to burning)

### Audit findings
After S212 the cast lived in the arena, but two things were still missing:
- **All 3D sat behind the characters.** Parallax reads strongest on near objects, so half the available depth was unused. Nothing ever passed in front.
- **The 3D was inert with respect to the game.** It reacted to camera events (turn, impact, BREAK) but not to memory burning — the mechanic the whole game is built around, and the one moment where an irreversible price is paid.

### Done
- **Foreground layer.** A new `FOREGROUND` stage mode: a second `HybridDepthStage` sharing the battle camera rig, composited *above* the 2D cast, with `follow_stage` copying the leader's pan/impact state each frame so the two layers agree.
  Placement was **measured rather than eyeballed.** A throwaway probe rendered the layer alone over a flat background and reported screen coverage per side: `x=±3.2 → 12%`, `x=±3.5 → 7.2%`, `x=±3.9 → 0.8%`. Chose ±3.5. Battlers stand at 18% and 78% of the frame and every panel has a higher `z_index`, so no information is covered.
  It stays enabled under Clean Gameplay View at lower alpha (0.26 vs 0.42). Gating it off there would have hidden the feature from most players, since that option defaults on.
- **The arena burns when a memory burns.** `play_memory_burn` ignites the floor grid toward ember colour and spawns rising 3D embers, scaled by memory grade and decaying over about a second. Hooked into `_play_memory_burn_then_execute`, so it fires on the real burn rather than on a menu action. Embers are skipped under Reduce Motion. This is the point where the 3D stops being decoration and starts serving the mechanic.

### Verification
- `HYBRID_COUPLING_SMOKE_PASS anchors=4 projected=(226, 424) pan_shift=12.02 foreground=1 burn_reaction=1`. New assertions: the foreground exists, follows the battle stage, composites above the cast, keeps its innermost geometry beyond `x=2.9`, and the burn flare rises on burn then decays.
- `BURN_ARENA_CAPTURE_PASS flare=0.91` with a before/after sheet showing the perspective floor grid igniting amber with embers rising.
- **A misread was caught and corrected mid-session.** An early pan capture looked like the foreground was swallowing the frame; a probe rendering the layer in isolation showed it was drawing *nothing* — the pillars were outside the frustum entirely. The darkening came from the capture harness disabling Clean Gameplay View, which re-enables haze, grain and tint. The placement sweep above replaced guesswork after that.
- All 21 smoke scenes pass. VN: 20 files, 504 steps, 0 errors. Korean: 31 files, 1,583 fields, 0 errors. 300-frame `verdan_market.tscn` boot clean.
- **Cost note:** this adds a second `SubViewport` with its own `World3D` during battle. Its content is eight boxes at 640x360, so the added draw cost is small, but it is a real second 3D render and worth remembering if battle performance is ever profiled.

## S212 - 2026-07-26 (Coupling the 2D cast to the 3D arena)

### Audit findings
S211 gave the battle a real 3D arena, but the hybrid was still **2D over 3D wallpaper**: battlers were `Control` nodes at fixed screen coordinates while the 3D moved behind them. Two specific consequences:
- When the camera panned on a turn change, the space moved and the characters did not. Nothing tied the layers together.
- The 3D focus rings sat at hand-written coordinates (±2.85) chosen independently of where the 2D characters actually stand, so the ring under Arrel was never really under Arrel.
- The turn focus pan was ±0.22 world units — under 2px of on-screen movement. Even after coupling, nobody would have perceived it.

### Done
- **Battlers now live in the arena.** `world_to_canvas` / `anchor_offset` project a 3D anchor into the logical canvas and return how far it moved from the camera's rest pose. Each battler container is wrapped in an anchor `Control` that receives that offset. At the rest pose the offset is exactly zero, so the existing layout and every position tween (which target absolute `_player_base_pos` values) are untouched — the coupling only expresses itself while the camera moves.
- **One coordinate system.** `canvas_to_floor` unprojects each battler's 2D feet position onto the arena floor plane, so the 3D anchor is *derived from* the 2D layout rather than written by hand. Contact shadows and focus rings are then placed at that exact point. Moving a battler in 2D now moves its 3D marks automatically.
- **3D contact shadows** (`arena_contact_shadow.gdshader`) lie on the floor plane, so they compress with perspective and pan with the stage. The old screen-space 2D ellipses are kept but dropped to 35% — the nodes stay because existing contracts reference them.
- **`BATTLER_PARALLAX` is 1.0, deliberately.** The first attempt damped character movement to 0.62 to keep them from "sticking to the background". That was wrong: the contact shadow and focus ring live inside the 3D layer and move at the full camera rate, so damping the character slid it off its own shadow. Perspective parallax already comes from the anchor's depth — a battler standing nearer moves more than the far pillars without any extra factor.
- **Battle-opening dolly.** `play_entrance` pushes the camera in from above and behind over 1.15s, skipped under Reduce Motion. Turn focus pan raised to ±0.85/0.92.
- **Unrelated defect found while capturing:** ordinary enemies fall back to `PixelSprite.create_battle_enemy` — flat purple diagonally-hatched diamonds — because `resolve_enemy_image_by_name` deliberately returns empty for non-bosses. That is what stands in the new arena during most encounters. Routed the five recurring archetypes (Ash Crawler, Forest Shade, Threshold Shade, Memory Eater, Void Watcher) to the S204 action-cutin plates already in the repo. An explicitly requested enemy image still wins.

### Verification
- `HYBRID_COUPLING_SMOKE_PASS anchors=4 projected=(226, 424) pan_shift=12.02`. It asserts the player's 3D anchor reprojects onto the 2D feet position, the focus ring sits within 0.05 world units of that anchor, the rest-pose offset is under 0.5px, the camera pan actually moves the character, and the character stays locked to its own contact shadow.
- **Falsifiability checked:** restoring the 0.62 damping fails the guard with `캐릭터가 자기 접지 그림자에서 벗어났다 (7.45 vs 12.02)`.
- One test defect was found and fixed along the way: comparing the anchor offset against a freshly computed one while the camera was still easing produced a false failure, so the test now lets the camera settle before measuring.
- All 21 smoke scenes pass. VN validation: 20 files, 504 steps, 0 errors. Korean: 31 files, 1,583 fields, 0 errors. 300-frame `verdan_market.tscn` boot clean.
- New `capture_hybrid_pan` stacks the player-turn and enemy-turn frames so the coupling is visible as an image. Also inspected `story_combat_witness.png`, `hybrid_battle_stage.png` and the four-biome arena sheet.

## S211 - 2026-07-26 (Battle arena in real 3D)

### Audit findings
The player reported the game reads as flat 2D. It already had a real-time 3D system — `HybridDepthStage` from S206, rendering an isolated `World3D` through a `SubViewport` into battles, the pause-menu world map, and Curio relic screens. Capturing it in isolation showed why nobody noticed:
- The battle diorama was **dashed elliptical floor traces plus nine identical boxes in one row**. There was no ground surface, so the 2D battlers had no space to stand in — the 3D added a few sticks, not depth.
- It composited at **alpha 0.36**, and the 2D ground band above it ran at 0.62, so what little geometry existed was painted over.
- Everything sat between z −0.2 and −2.3, one flat slab of depth. There was no far distance and no atmospheric falloff, so the low-poly boxes read as low-poly boxes.
- Four biome motifs (`roots`, `threshold`, `void`, `markers`) placed an **emissive accent box at x = 0, immediately in front of the camera** — invisible at 0.36 alpha, but a glowing object parked between the two combatants the moment the stage became visible.

### Done
- **A real arena floor.** New `arena_floor.gdshader` draws a ground plane that is solid near the camera and dissolves toward the horizon, with a perspective grid that fades on the same curve. This is the compromise S206 was missing: the ground exists where the characters stand, and gives way to the painted illustration above the baseline. The plane is parented to `scene_root`, not the swaying `motion_root`, so the ground never rocks under the cast.
- **Three depth layers.** Pillar rows at z −3.4, −7.2 and −12.5 with widening spread and height. Camera moves now produce parallax between layers instead of sliding one flat row.
- **Depth fog** on the battle environment. Far geometry melts into the stone colour, which is what converts identical boxes into distance for free.
- **Made it visible.** Composite alpha 0.36 → 0.72 (0.82 outside Clean View), and the 2D ground band dropped 0.62 → 0.34 so the 3D floor is the floor.
- **Cleared the centre.** `_add_flanking_landmarks` replaces the four centred accent totems with a symmetric pair set back at z −7.4, so the biome's colour still reads while the space between the combatants stays empty.
- Existing camera reactions — impact pulse on damage, side focus on turn change, gold rupture on BREAK — now land on a stage where they can actually be seen. 2D keeps ownership of collision, character identity, text and input, unchanged.

### Verification
- `HYBRID_DEPTH_SMOKE_PASS battle=147 atlas=96 relic=47 route_markers=10` (battle geometry 127 → 147). Added assertions that the arena has a real `PlaneMesh` floor, that the floor is parented outside the swaying root, that depth fog is enabled, and that the floor recedes past 30 units.
- All 21 smoke scenes pass. **Note:** the first full-suite run reported `smoke_illustration_gapfill` and `smoke_movement_naturalism` as failures; both passed standalone and the full re-run was clean, so those were transient under back-to-back engine launches, not regressions.
- VN validation: 20 files, 504 steps, 0 errors. Korean localization: 31 files, 1,583 fields, 0 errors. 300-frame `verdan_market.tscn` boot clean.
- Renders inspected: `story_combat_witness.png` before and after (the centred glowing box was caught and fixed this way), `hybrid_depth_board.png`, `hybrid_world_map.png`, `hybrid_relic_choice.png`, and a new `capture_arena_biomes` contact sheet confirming rim_forest, verdan_market, bl07_void and crumbling_coast each produce a distinct arena.

## S210 - 2026-07-26 (Text sharpness, real walk cycles, and field scale consistency)

### Audit findings
Three player-reported symptoms, each traced to a specific cause rather than treated cosmetically:
- **"폰트가 깨진다."** `project.godot` used `window/stretch/mode="viewport"`. That renders the entire game into a fixed 1280x720 framebuffer and then upscales it as a bitmap to the window: 1.5x at 1080p, 2x at 1440p. Every glyph was a stretched 720p glyph. Measured directly — at a 1920x1080 window the captured framebuffer came back 1280x720. Earlier sessions never saw it because headless captures run at exactly 1280x720, where no scaling occurs.
- **"걸을 때 팔다리가 안 움직인다."** `_create_field_sprite_frames` built `walk_<dir>` as `[texture, texture]` — the same static PNG listed twice. A prior session chose this deliberately to avoid fabricating in-between art, but the result was a cast that slid across the ground with frozen limbs. Field art is one authored pose per direction (128x160), with no walk frames anywhere in the project.
- **"NPC 크기와 움직임이 부자연스럽다."** `PixelSprite.create_npc_sprite` hardcoded `scale = 0.24` and a `-38px` foot offset, giving ambient citizens a ~35px visible height while the story cast is normalized to 50px by `apply_field_profile`. Background people read as 30% shorter adults on the wrong ground line, and used NEAREST filtering against the cast's LINEAR. Their wander gait also ran at the player's cadence while they moved at a fifth of his speed, so their feet scraped.
- While auditing the market, the last flat placeholders surfaced: Memory Resonance points were 54x54 `ColorRect`s plus four rotated 3x10 bars, sitting on the painted map canvases as hard-edged tan blocks and yellow sticks.

### Done
- **Text.** Switched to `window/stretch/mode="canvas_items"` with `aspect="keep"`. Glyphs now rasterize at the window's native resolution while the logical coordinate system stays 1280x720, so absolute-position layouts (the battle stage in particular) are untouched. The Options resolution setting now actually increases sharpness instead of enlarging the same 720p image.
- **Walk cycles.** Added `PixelSprite._build_walk_cycle`, a cached four-frame cycle generated from the single authored pose. `_detect_leg_top` scans the silhouette for where it narrows below the coat or skirt, so only real legs move — a fixed ratio tore the tabard down the middle. Legs get a squared ramp of horizontal stride plus opposite vertical lift; the upper body moves as one piece with a bob and counter-phase sway. An arm-band pass was built and then removed: slicing the shoulder columns left black bars where the pauldrons and back-slung sword were cut, and arm swing is invisible at 50px anyway.
- **Ambient scale.** Both branches of `create_npc_sprite` now route through `apply_field_profile`, using the child height for child presets. Wander gait is scaled by real travel speed against a 120px/s reference, and the position tween moved from `TRANS_QUAD` to `TRANS_SINE` so acceleration no longer outruns the stride.
- **Resonance markers.** Replaced with a shared radial-falloff texture: one soft ground glow, one bright core, four small spark points. The pulse tween was rewritten against each sprite's base scale instead of the old absolute `1.0`.
- Added `smoke_field_animation` and `capture_font_scaling`.

### Verification
- `FIELD_ANIMATION_SMOKE_PASS walk_frames=4 distinct=24 cast_height=50 stretch=canvas_items`. It asserts walk frames differ in pixel data (not just in count), differ from idle, keep foot and centre drift within 6px, advance on screen under real movement input, hold seven ambient presets within 1px of the 50/42 contract, leave no `ColorRect` under a resonance point, and preserve the `canvas_items` stretch contract.
- **Falsifiability checked:** restoring the old `[texture, texture]` stub makes the guard fail with `arrel/walk_down의 프레임이 전부 동일하다` and `distinct=0`.
- The live-playback assertion first failed against a defect in the test itself — driving the sprite with `play()` is overridden by the player's animation state machine on the next frame — so it was rewritten to press real movement input.
- `FONT_SCALING_CAPTURE_PASS framebuffer=1920x1080 viewport=(1280, 720) canvas_scale=1.50`, with 12/14/16px Korean glyphs inspected at 1080p.
- All 21 smoke scenes pass with zero `SCRIPT ERROR` or `Parse Error`. VN validation: 20 files, 504 steps, 0 errors. Korean localization: 31 files, 1,583 fields, 0 errors. 300-frame `verdan_market.tscn` boot clean.
- Renders inspected before and after: `rim_forest_first_exploration.png`, `verdan_malet_field.png`, `font_scaling_1080p.png`, `dialogue_interface_ko.png`, plus walk-cycle contact sheets for the down and left facings at each iteration.

## S209 - 2026-07-25 (Battle stage repair, battle tempo, story log, and field prop art)

### Audit findings
Grounded in real 1280x720 OpenGL captures taken before any change, not a code read:
- The battle stage was the weakest screen in the game. `battle_scene.tscn` drew a full-width solid ground band with a 2px highlight line at 58% height, which read as a ruled UI seam across the battle art. Battler shadows sat at four different heights, so Elia floated 48px above that line.
- `TextureRect.STRETCH_KEEP_ASPECT_CENTERED` centers a 16:9 enemy illustration inside a near-square box. Most enemy plates therefore drew as a small hard-edged card whose visible bottom stopped ~70px above its own contact shadow.
- Arrel rendered smaller than both his companion and the enemy, so the protagonist was not the visual anchor of his own fight. The battle background image ran at 0.88 luminance, and the figures painted inside it competed with the actual battlers.
- `_show_turn_indicator()` overwrote `turn_label.position` with `(0, 0)`, which cancelled the centered anchors and threw the turn banner into the screen's top-left corner on every turn.
- The Limit rail label was clipped to "리미" by variable-font metrics, and the disabled Limit command used a whole-button `modulate` alpha, so the command deck ornament showed through its text. Elia's technique rail displayed a permanent dead "(사용 가능한 기술 없음)" label mid-stage.
- Status-effect and boss-phase VFX set `enemy_sprite.material = null`, which would have stripped any stage-blend material mid-fight.
- The game had no dialogue backlog of any kind. Across 20 VN files (504 steps) and ~1,400 field lines, a mis-timed keypress lost the line permanently. There was no read-line tracking, so the VN's Ctrl fast-forward skipped unread story just as fast as re-read story, and field dialogue had no fast-forward at all.
- Field interactive props (barrel, crate, campfire, sign) were still stacked `ColorRect` placeholders. The first exploration capture showed brown and grey squares sitting on the tiles. Chest and clue markers were `ColorRect`s set to alpha 0 under the default Clean Gameplay View, so hidden rewards had no on-screen affordance whatsoever.

### Done
- **Battle stage.** Introduced a shared `STAGE_BASELINE_Y`; player, companion, Tobias, and enemy now derive their positions from it with deliberate perspective offsets. `_fit_battle_plate()` sizes each illustration to its actual drawn dimensions and stands it on the baseline, and `_feet_anchored_y()` keeps sheet-based sprites grounded when their scale changes. Replaced the solid ground band with a six-step gradient plus one soft stage pool (no straight edge anywhere). Receded the background image and added a five-band aerial-perspective wash that darkens distance without hiding the art. Scaled Arrel to 1.34 so he leads the frame.
- **Plate blending.** Added an `oval_mask` uniform to `battle_stage_blend.gdshader` so rectangular scene plates dissolve into the stage, and cached the blend material on the node so status and boss VFX restore it instead of nulling it.
- **Battle tempo.** Added `BATTLE_SPEED_STEPS` (x1.0 / x1.5 / x2.0) with `paced()` and `pace_timer()`. All 26 presentation waits in `battle_scene.gd` and `battle_manager.gd` route through it, so only waiting time changes: damage, hit chance, BREAK, and rewards are untouched. Surfaced as a top-left chip, a Tab hotkey handled in `_input` (`_unhandled_input` is too late; focus navigation eats Tab first), and an options row. Persists in `settings.json`.
- **Story log.** New `StoryLog` autoload records every DialogueBox line, every VN line, and both choice paths, grouped by chapter and capped at 300 entries. Opens on `L` or from the pause menu, closes on `L`/`Esc` ahead of the pause menu, and refuses to stack under it.
- **Read-aware fast-forward.** A persistent registry in `user://read_lines.json` backs `can_fast_forward()`. The VN's Ctrl fast-forward now halts at unread text with an on-screen notice, and field dialogue gained the same Ctrl fast-forward it never had. "읽은 대사만 빨리감기" is on by default and can be turned off.
- **Field prop art.** `PixelSprite.create_prop_texture()` draws cached 32x32 procedural pixel art for barrel, crate, campfire, sign, chest, and clue, each with a ground shadow and a distinct used state. `MapEffects.add_interactive_prop()` now spawns a `Sprite2D`, and `MapEffects.make_discovery_marker()` replaced the invisible chest/clue indicators in all ten maps.
- **Smaller repairs.** Centered the turn banner, gave the Limit label room for its last glyph, made disabled commands opaque and legible, hid Elia's rail when she has no technique, moved the turn-order chips clear of the combat-cue border, and softened the footprint glows that read as flat discs.
- Added `smoke_story_qol` and `capture_story_log` for focused regression and visual audit.

### Verification
- All 20 gameplay/UI smoke scenes pass with zero `SCRIPT ERROR` or `Parse Error`, including the new `STORY_QOL_SMOKE_PASS log=300 read=327 speed_steps=3 props=6`.
- `BATTLE_COMMAND_DECK_SMOKE_PASS actions=8 readout_height=77.0 player_scale=(1.34, 1.34)`; `STORY_COMBAT_SMOKE_PASS`; `HYBRID_DEPTH_SMOKE_PASS`; `VISUAL_CLARITY_SMOKE_PASS`; `WORLD_POPULATION_SMOKE_PASS`; `MOVEMENT_NATURALISM_SMOKE_PASS`.
- Real OpenGL captures inspected at 1280x720 before and after: `story_combat_witness.png`, `hybrid_battle_stage.png`, `rim_forest_first_exploration.png`, `story_log_ko.png`. The battle enemy now stands on the baseline at roughly twice its former size with dissolved edges; barrel, crate, campfire, chest, and clue read as objects instead of squares.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization: 31 files, 1,583 fields, 19 speakers, 0 errors.
- 300-frame `verdan_market.tscn` boot completed with no script, parse, or runtime-access error. `git diff --check` passed.
- Confirmed the smoke run leaves no residue: `suppress_persistence` keeps synthetic lines out of `read_lines.json`, and `battle_speed` is restored and re-saved.

## S208 - 2026-07-22 (Quick Kit inventory, Smart Heal, and battle-supply flow)

### Audit findings
- The illustrated inventory already exposed filters, item art, effects, trade values, and equipment, but it remained a read-only archive. Players could not use field recovery, search a large collection, change sort order, or carry an explicit battle loadout.
- The battle item tray rebuilt every owned item in dictionary order. It did not share selection intent with the archive and its modal had no fast keyboard path.
- The exploration HUD counted only the original six consumables, so later recovery relics and tactical tools were omitted from the carried-kit summary.

### Done
- Added a save-compatible three-slot Quick Kit stored in `player_data`, preserved through New Game+, normalized on legacy save import, and surfaced consistently in both the field archive and battle item tray.
- Reworked the inventory archive around live use: Quick Kit cards, recent-acquisition badges, text search over name/description/effect, Type/Name/Count sorting, refreshed item counts, and clear `USE NOW`, `HP FULL`, or battle-only action states.
- Added field recovery without opening a menu. `H` on keyboard or `X` on controller invokes Smart Heal, which prioritizes the smallest carried recovery that fully covers missing HP and falls back to the strongest available item when necessary.
- Added 1-3 Quick Kit shortcuts inside the battle item modal. The full supply list remains available underneath and sorts pinned supplies first, so the feature shortens common turns without removing tactical choice.
- Updated the exploration HUD to classify every current item dynamically and reveal the Smart Heal hint only while Arrel is injured and a recovery supply is carried.
- Reused the existing `ui_inventory_archive_v2` backdrop and generated item icon set rather than introducing a competing visual style. Added dedicated inventory-QoL smoke coverage and extended the interface visual contract.

### Verification
- `INVENTORY_QOL_SMOKE_PASS quick=3 search=1 smart_heal=75`, including legacy-save defaults, recent acquisition, pin order, field use, and mirrored battle slots.
- `INTERFACE_VISUAL_UPGRADE_SMOKE_PASS`; the real OpenGL inventory capture was refreshed at `tmp/visual_audit/inventory_qol_final.png`.
- `BATTLE_COMMAND_DECK_SMOKE_PASS actions=8 readout_height=77.0 player_scale=(1.0, 1.0)`; `HYBRID_DEPTH_SMOKE_PASS battle=127 atlas=96 relic=47 route_markers=10`; `STORY_COMBAT_SMOKE_PASS witness=2 release=1 choice_echo=1 preservation_bonus=8 focus=1`.
- `VISUAL_CLARITY_SMOKE_PASS`; representative `verdan_market.tscn` boot completed through population, companion, and autosave startup.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings. Korean localization: 31 files, 1583 fields, 19 speakers, 0 errors. `git diff --check` passed.
- Godot's forced-exit ObjectDB/resource notices remain the known external-plugin shutdown noise; no `SCRIPT ERROR`, `Parse Error`, or failed gameplay assertion remained.
## S207 - 2026-07-22 (Battle command deck, consequence forecast, and reactive 3D arena)

### Audit findings
- The battle system already had WITNESS, BREAK, stances, Limit, items, companion commands, directives, intent reads, and illustrated action beats, but their information was split across five competing panels. The six-line center log and ornate lower ribbon covered too much of the combat stage.
- The upper objective, cue, and enemy panels overlapped at 1280x720. Limit, stance, and companion controls also occupied the same lower strip as the primary command grid.
- Arrel's current sheet rendered smaller than the painterly enemy/support art, while rectangular shadow and glow blocks made every battler appear pasted over the scene.
- The hybrid 3D stage reacted to impacts but did not tell the player whose turn currently owned the arena.

### Done
- Generated and selected two project-bound GPT Image interface assets against the existing MEMORIA command ribbon and tactical-plate materials: `ui_battle_command_deck_v4.png` and `ui_battle_field_readout_v4.png`. Both are text-free charcoal iron, antique-gold, and restrained memory-blue surfaces; the chroma-key sources were removed, despilled, alpha-audited, and cropped at runtime through `AtlasTexture` regions.
- Rebuilt the lower battle interface as a quiet 4x2 command deck. All eight actions now show their tactical role and stable 1-8 keyboard shortcut; action focus previews current BREAK pressure, known weakness, WITNESS progress, available memories, permanent burn cost, carried tools, Limit charge, and boss escape restrictions.
- Replaced the six-line blocking combat log with a 77px field-read strip that preserves only the two most recent messages and foregrounds the latest consequence. Hover/focus forecasts temporarily take over the strip, then restore the last live battle beat.
- Separated upper information into left objective, center combat cue/stance/companion rails, and right enemy status zones. Limit now belongs to Arrel's status cluster instead of floating on the command deck.
- Enlarged Arrel's battle-sheet presentation and replaced player, ally, Tobias, and enemy rectangular ground blocks with elliptical contact shadows and restrained footprint glows.
- Added separate real-time 3D player and enemy focus rings. Player turns pan and illuminate the left relief, enemy turns focus the right, and BREAK converts the hostile ring to a brighter gold rupture state without transferring character identity or input into 3D.
- Registered both new frames in the interface Artbook manifest and added focused smoke coverage for the command-deck asset contract, eight action identities, consequences, short log, contact shadows, and numeric hotkeys.

### Verification
- `BATTLE_COMMAND_DECK_SMOKE_PASS actions=8 readout_height=77.0 player_scale=(1.0, 1.0)`.
- `HYBRID_DEPTH_SMOKE_PASS battle=127 atlas=96 relic=47 route_markers=10`, including player/enemy turn focus assertions.
- `INTERFACE_VISUAL_UPGRADE_SMOKE_PASS`, with both new interface frames loading through the Artbook manifest.
- `VISUAL_CLARITY_SMOKE_PASS`, including full-screen battle canvas, 4x2 command grid, WITNESS route, canonical support art, generated item icons, and bundled Korean font chain.
- `STORY_COMBAT_SMOKE_PASS witness=2 release=1 choice_echo=1 preservation_bonus=8 focus=1`.
- Real OpenGL captures passed and were inspected at 1280x720: `tmp/visual_audit/hybrid_battle_stage.png` and `tmp/visual_audit/story_combat_witness.png` (`witness=1/2`, `grid=4x2`).
- `git diff --check` passed. Godot's existing forced-exit ObjectDB/resource cleanup notices remain shutdown noise; no parse, script, assertion, or gameplay runtime error occurred.

## S206 - 2026-07-21 (Hybrid 2D/3D memory relief and full-screen battle repair)

### Audit findings
- MEMORIA had no live `Node3D` presentation even where depth would help: battles, the witnessed-route atlas, and regional memory-curio decisions were all flat 2D surfaces.
- The first procedural 3D pass proved the renderer worked, but large opaque ground cylinders covered the authored illustrations instead of complementing them.
- A deeper battle audit exposed a long-standing layout fault: `battle_scene.tscn` used a `Node2D` root while its full-screen backgrounds and overlays relied on Control anchors. Their runtime size remained `(0, 0)`, leaving the engine clear color exposed across most of the screen.
- Default battle presentation stacked strong haze, tint, grain, side plates, and hard-edged character images, which made the 2D art and tactical UI compete with one another.

### Done
- Added `HybridDepthStage`, a reusable real-time 3D `SubViewport` system with isolated `World3D`, perspective camera, low-cost lighting, transparent compositing, and no 3D shadows.
- Built three purpose-specific modes: biome-aware battle reliefs for all ten regions, a ten-stop interactive route atlas that focuses the selected chapter, and a floating memory-relic construct for World Curio choices.
- Kept gameplay identity in 2D while assigning depth and motion to 3D. Battle impacts nudge the 3D camera, map hover/focus moves the relief, and relic rings orbit unless Reduce Motion is enabled.
- Replaced opaque 3D ground discs with sparse contour traces, route pads, landmark silhouettes, lanterns, roots, thresholds, and memory shards so generated illustrations remain visible.
- Integrated the relief into battle backgrounds, the pause-menu World Map, and all ten regional Curio choice screens. Clean Gameplay View lowers its intensity and Reduce Motion freezes ambient orbit/drift.
- Converted the battle scene root to a viewport-sized `Control`, updated `BattleVFX` to accept the new root type, and added a regression assertion that the 2D backdrop and 3D relief both cover the full 1280x720 canvas.
- Reduced non-clean battle haze, tint, grain, letterboxing, and color-grade strength. Added a soft-edge shader for stage and character art so canonical 2D portraits blend into the scene instead of appearing as hard rectangular cards.
- Added focused hybrid smoke plus real-render capture harnesses for battle, world map, relic choice, and the three isolated 3D modes.

### Verification
- `HYBRID_DEPTH_SMOKE_PASS`: battle `59` meshes, atlas `96`, relic `47`, and all `10` route markers.
- `VISUAL_CLARITY_SMOKE_PASS` now includes `battle_canvas=full` and `hybrid_depth=1`; story combat, World Curios, movement naturalism, crash guards, gameplay QoL, Field Focus, and world population smoke all passed.
- Real OpenGL captures were inspected at 1280x720: `hybrid_battle_stage.png`, `hybrid_world_map.png`, `hybrid_relic_choice.png`, and `hybrid_depth_board.png`.
- VN validation passed with 20 files and 504 steps; Korean localization passed with 31 files, 1,583 fields, and 19 speakers.
- A 180-frame `verdan_market.tscn` boot reached Arrel, Malet, Elia, four civilians, the regional Curio, population setup, and autosave with no parse, script, assertion, or invalid-access error.
- `git diff --check` passed; the known forced-exit ObjectDB/resource cleanup messages remained unaccompanied by gameplay errors.

## S205 - 2026-07-21 (Natural exploration locomotion and companion pathing)

### Audit findings
- Arrel needed roughly a third of a second to reach sprint speed, so the first input felt soft while a reversal retained too much previous momentum.
- Directional art was four-way, but movement intent, animation state, and interaction facing did not share one stable cardinal direction. Footsteps were timed from held input rather than travelled distance, including when pushing into a wall.
- Camera drag, smoothing, look-ahead, ambient motion, and event shake all wrote overlapping motion into the same camera offset, producing delayed framing and small jitter.
- Companions chased the player's current position instead of the route the player took, which caused corner cutting, overlap, oscillation against collision, and a visible short-distance teleport. Market background walkers translated while remaining in an idle pose.
- The first-exploration render exposed a rectangular `ColorRect` beneath Arrel as the moving ground shadow.

### Done
- Rebuilt Arrel locomotion around `Input.get_vector`, preserved analog input strength, added responsive acceleration and stronger reversal braking, blended sprint entry/exit, reduced excessive sprint trails, and enabled floating top-down collision sliding.
- Unified actual travel, animation, gait, footfalls, dust/echoes, and movement statistics around distance moved. Near-diagonal input now uses cardinal hysteresis, and the interaction ray follows the same stable facing pose.
- Replaced the layered camera lag with one composed offset: damped speed-scaled anticipation plus metadata-owned ambience plus deterministic event shake. Camera drag is disabled and smoothing is tightened.
- Replaced direct companion chasing with a sampled breadcrumb trail and formation distance. Elia/Sable now follow turns the player actually took, accelerate to close large gaps, preserve personal space, and use a hidden path-point recovery only when genuinely stuck or far away.
- Added companion gait sway and direction hysteresis, directional walk/idle switching for market walkers, and a layered oval shadow that subtly reacts to Arrel's stride instead of rendering as a grey rectangle.
- Added `smoke_movement_naturalism.tscn`, covering real 60 Hz acceleration, reversal, stopping, pose hysteresis, distance footfalls, breadcrumb cornering, camera ownership, ambient gait, and grounded-shadow contracts.

### Verification
- `smoke_movement_naturalism.tscn`: **MOVEMENT_NATURALISM_SMOKE_PASS**.
- `smoke_visual_clarity.tscn`, `smoke_crash_guards.tscn`, `smoke_gameplay_qol.tscn`, `smoke_field_focus.tscn`, `smoke_world_population.tscn`, and `smoke_story_combat.tscn` all passed.
- A 120-frame `verdan_market.tscn` boot completed through Arrel, Malet, Elia, population, and ambient walkers without a gameplay script or runtime-access error.
- The first-exploration render passed and was visually inspected after the shadow replacement. Existing forced-exit ObjectDB/resource cleanup notices remained unaccompanied by `SCRIPT ERROR` or `Parse Error`.

## S204 - 2026-07-21 (Late-story illustrations and choice-driven regional RPG landmarks)

### Audit findings
- Chapters 17-21 still had five high-value dramatic turns presented as text between existing CGs, while Chapter 24's testimony sequence left its opening record, Tobias, Han, Vael, and final title beat visually unsupported.
- Only five of the ten core exploration maps had a second-discovery Field Focus plate. Drift Shelter, Seam Outskirts, and BL-07 also had only one resonance point, so a deeper discovery could never unlock there.
- Ash Crawler, Forest Shade, Threshold Shade, Void Watcher, and Memory Eater attacks fell back to their stage background instead of displaying enemy-specific action art.
- Five live memory-rewrite rules still used older flat or less specific art. Existing maps already contained chests, clues, NPCs, and hunts, but lacked a consistent region-wide landmark that asked the player to make a small RPG resource choice.

### Done
- Generated and visually audited 25 cohesive 1672x941 GPT Image plates in five sets: late-story interstitials, Testimony epilogue scenes, deep Field Resonance discoveries, recurring-enemy action cut-ins, and story-specific world rewrites.
- Integrated the late-story set into the exact Chapter 17-21 beats and the Testimony set into Chapter 24 without changing dialogue order, branch requirements, English copy, or Korean copy.
- Completed the deep discovery layer for all ten core maps. Added a second authored resonance point to Drift Shelter, Seam Outskirts, and BL-07 so every deep plate has a reachable play condition.
- Added named action-cutin routing for five recurring enemy archetypes and replaced the Verdan taste, Tobias record, Sable witness, compass, and Void-walker rewrite plates in their live rules.
- Added `WorldCurio`: one sparse illustrated landmark in each core region. The player can study it for Field Focus, salvage a biome-specific tactical item, attune for recovery, or leave it untouched and return later. Choices persist independently and the overlay safely restores exploration pause state.
- Added the 25-entry `illustration_gapfill_gallery.json` Artbook manifest, updated the illustration catalog baseline from 425 to 450 CG PNGs, and added focused smoke coverage for image consumers, aspect ratio, curio choices, rewards, pause lifecycle, and collision-safe map placement.

### Verification
- Godot 4.6.2 headless import completed for all 25 new PNGs with no project script or parse error.
- All 15 gameplay/UI smoke scenes passed, including `ILLUSTRATION_GAPFILL_SMOKE_PASS` (`25` assets, `5` categories), `FIELD_FOCUS_SMOKE_PASS` (`10` maps, `10` deep plates), `WORLD_CURIOS_SMOKE_PASS` (`10` landmarks, `4` choices), and `WORLD_POPULATION_SMOKE_PASS` (`19` maps, `64` voices, `32` visible threats, `11` caches, `10` curios).
- VN validation passed with 20 files and 504 steps; Korean localization passed with 31 files, 1,583 fields, and 19 speakers.
- Representative `verdan_market.tscn` booted through Arrel, Malet, Elia, four ambient civilians, the new regional curio, autosave, and dialogue start without a script, parse, or runtime-access error.
- Godot's existing ObjectDB/resource cleanup notices remain forced-headless-exit noise; no gameplay error accompanied them.

## S203 - 2026-07-17 (GPT Image five-category illustration expansion)

### Audit findings
- The project already had broad chapter coverage, so a volume-only batch would have duplicated existing scenes instead of improving play.
- Chapters 12-16 still had room for one additional turning-point image each, while character relationships were underrepresented in the Story Journal.
- Battle support actions reused generic plates, and five core memory-loss illustrations were visibly more abstract and textural than the current charcoal/iron-blue canon.
- Field Focus stopped at the first mapped echo even on maps with multiple resonance points, leaving exploration illustration rewards shallow.

### Done
- Generated and visually audited 25 clean 1672x941 GPT Image plates, exactly five per category:
  - Part II story moments for Chapters 12-16.
  - Character-bond records for Arrel/Elia, Arrel/Tobias, Arrel/Sable, Elia/Nera, and Arrel/Kairos.
  - Battle cut-ins for Arrel, Elia, Tobias, Sable, and Kairos.
  - World-rewrite consequences for the campfire song, reaching hand, first sword, Arrel's name, and Elia's anchor warmth.
  - Deep Field Resonance discoveries for Rim Forest, Verdan Market, Belt Waystation, The Seam, and Forgotten Forest.
- Added `data/illustration_expansion_gallery.json` and connected it to the PauseMenu Artbook; every one of the 25 images also has a live story, journal, battle, rewrite, or exploration consumer.
- Added a second-layer Field Focus discovery: after the first map CG, mapping a second echo on the five expanded maps unlocks a distinct follow-up illustration and caption.
- Added a dedicated Arrel memory-chain cut-in at chain two or higher and action-specific plates for Elia's Humming Shield, Tobias's Archive Countermeasure, Sable's intercept, and Kairos's redaction phase.
- Replaced every live reference to the five older mismatched memory-loss plates with the new canon-consistent world-rewrite set.
- Extended Story Journal chapter labels through Chapter 24 and registered all five relationship plates as story-flag-gated records.
- Added `smoke_illustration_expansion` to enforce 25 unique decodable images, five entries per category, cinematic aspect ratio, and a live game consumer for every path.
- Updated `ILLUSTRATION_CATALOG.md` to the verified 425-CG baseline and documented the full set.

### Verification
- Godot 4.6.2 headless editor import/project parse passed with no `SCRIPT ERROR` or `Parse Error`.
- All 13 smoke scenes passed, including the new `ILLUSTRATION_EXPANSION_SMOKE_PASS total=25 categories=5 live_consumers=25` and extended `FIELD_FOCUS_SMOKE_PASS maps=10 deep=5`.
- Representative `verdan_market.tscn` boot reached Arrel, Elia, Malet, four ambient NPCs, checkpoint autosave, and dialogue initialization without script/parse/runtime-access errors.
- VN validation passed: 20 files, 504 steps, 0 errors, 0 warnings.
- Korean localization validation passed: 31 files, 1,583 fields, 19 speakers, 0 errors.
- Manifest audit passed: 25 PNGs, five entries in each category, 64,718,616 total bytes.
- `git diff --check` passed; only normal CRLF working-copy notices and the existing Godot resource-cleanup warnings remain.

## S202 - 2026-07-17 (Player-chosen directives, battle grades, and reward chains)

### Design goal
- Turn the existing combo, BREAK, WITNESS, stance, item, ally, and Resonance mechanics into one readable combat loop with agency at the start, progress during the fight, and a clear payoff after victory.

### Done
- Replaced automatic tactical-objective assignment with a player-facing field-directive briefing. Every encounter offers two deterministic, valid objectives; a banked Field Focus charge opens a third choice in addition to its existing Resonance and Limit opening.
- Added compact reward previews, keyboard/controller focus, Korean/English copy, and an input-blocking briefing layer that hands control back only after the player chooses.
- Added live objective progress for BREAK, scan, WITNESS, combo, memory preservation, stance changes, Resonance, echoes, item restraint, speed, Limit, and companion coordination.
- Added a 100-point post-battle grade using objective completion, Resonance, WITNESS, BREAK, combo, and action efficiency. Grades D-S now award modest extra Grains and replace the old proxy battle-experience bar on the victory screen.
- Added a saved Directive Chain. Consecutive completions add a capped Grains bonus, every third success restores Field Focus, every fifth grants Witness Ink, and a failed objective or defeat resets the chain.
- Added grade, S-rank, and Directive Chain records to the character dossier; documented the new loop in the Field Guide; covered new-game, NG+, and old-save defaults.
- Added focused tactical-directive smoke and real-render capture tools.

### Verification
- `TACTICAL_DIRECTIVES_SMOKE_PASS options=2 choice=1 grade=S score=100 chain=3 focus=1 save=1 victory_ui=1`.
- `FIELD_FOCUS_SMOKE_PASS maps=10 count=1 resonance=25 limit=20 directives=3`.
- `STORY_COMBAT_SMOKE_PASS witness=2 release=1 choice_echo=1 preservation_bonus=8 focus=1`.
- Real OpenGL capture inspected at 1280x720: `tmp/visual_audit/tactical_directive_briefing.png`.
- `git diff --check` passed and the shipped runtime em-dash scan remained at zero.

## S201 - 2026-07-17 (GPT Image archive UI, inventory, item, and battle-supply upgrade)

### Goal
- Expand the game's usable visual library while making inventory, status, archive, shop, and battle-item interactions feel like one polished story-RPG interface.

### Done
- Generated and visually audited 16 final GPT Image assets in the existing charcoal/bronze/pale-memory archive language:
  - seven runtime UI surfaces for pause, inventory, character statistics, Codex, Story Journal, Memory Shop, and the transparent battle supply tray;
  - six high-resolution replacements for the lower-detail atlas reward items;
  - three transparent equipment-slot emblems.
- Used current inventory, pause, Codex, Journal, shop, potion, lantern, and witness-item assets as direct style references. All interface backgrounds contain no generated text; all isolated icons were chroma-keyed, despilled, and alpha-audited.
- Upgraded `FIELD ARCHIVE` with four filters, type-based ordering, HP/Grains/intact/burned/chapter telemetry, mechanical effect summaries, brighter text contrast, and illustrated weapon/armor/accessory records.
- Fixed an inventory UI bug where the equipped item copy was only added to the scene tree for empty slots.
- Rebuilt play statistics as a character dossier using Arrel's canonical portrait and current resource summary.
- Upgraded the Pause hub, Codex, Story Journal, and Malet's exchange to the new coherent background family.
- Added a transparent tactical battle-item tray. Repositioned and resized it through two real battle captures so it no longer buries the objective card, enemy state, or lower command grid.
- Added `data/interface_visual_gallery.json`, extended the Artbook loader to merge both expansion manifests, and exposed all 16 final assets in the Artbook.
- Added focused interface smoke and real-render capture scenes for inventory, character dossier, and battle item tray.

### Verification
- Image audit: seven UI plates at 1672x941; nine isolated icons at 1254x1254; all alpha icons reported `alpha=0-255` with 0 visible magenta pixels.
- `INTERFACE_VISUAL_UPGRADE_SMOKE_PASS`: all 16 assets, Artbook paths, filters, telemetry, equipped copy, equipment icons, portrait, and resource summary passed.
- Real OpenGL captures inspected at 1280x720:
  - `memoria_inventory_visual_upgrade.png`
  - `memoria_character_dossier_upgrade.png`
  - `tmp/visual_audit/battle_item_tray_v3.png`
- `VISUAL_CLARITY_SMOKE_PASS` remained green, including battle items, shops, portraits, field cast, map canvases, and Korean font checks.
- Representative `verdan_market.tscn` boot completed through world population, companion initialization, and checkpoint autosave.
- Korean localization: 31 files, 1,583 fields, 19 speakers, 0 errors.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- Godot's known forced-exit ObjectDB/resource cleanup notices and ShaderV duplicate-UID editor warnings remain shutdown/add-on noise; no parse, script, assertion, or gameplay runtime error occurred.

## S200 - 2026-07-16 (GPT Image chapter expansion, 50 integrated story CGs)

### Goal
- Expand every existing Part I chapter with at least five additional illustrations while preserving the established cast identities, low-noise dark-fantasy style, and story pacing.

### Done
- Generated and visually inspected 50 final 16:9 story CGs, exactly five for each of Chapters 1-10, under `assets/cg/generated/chapter_expansion/`.
- Held the visual language to clean anime-painterly ink linework, black-blue and ash-gray surfaces, restrained gold memory accents, and controlled violet-white Void light.
- Used the canonical character shots as identity references. Regenerated the Ch8 Ring Theory support cast, two Ch9 Arrel shots, and four Ch10 Arrel/Elia shots after detecting wrong hair color or length; only corrected results were copied into the project.
- Added `data/chapter_expansion_gallery.json` as a 50-entry source of truth for chapter, title, asset path, dialogue id, and exact narrative anchor.
- Inserted every CG into an existing story beat without changing dialogue order, English text, Korean text, choices, or flags.
- Extended the Artbook to merge the expansion manifest at runtime, so all 50 images are searchable and previewable without adding another large hard-coded constant.
- Added `smoke_chapter_expansion` to validate chapter quotas, uniqueness, 16:9 presentation resolution, resource loading, exact dialogue placement, and Artbook exposure.

### Verification
- Godot 4.6.2 headless import completed successfully for all 50 new PNGs.
- `CHAPTER_EXPANSION_SMOKE_PASS chapters=10 assets=50 placed=50`.
- `VISUAL_CLARITY_SMOKE_PASS` remained green after the Artbook loader change.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- Korean localization validation: 31 files, 1583 fields, 19 speakers, 0 errors.
- Representative `verdan_market.tscn` booted through world population and companion initialization without parse, script, or runtime access errors.
- `git diff --check` passed; known ObjectDB/resource cleanup and ShaderV duplicate-UID messages remain shutdown/editor-plugin noise.

## S199 - 2026-07-16 (Em-dash cleanup and deterministic font rendering)

### Audit findings
- Shipped dialogue JSON, VN data, and runtime UI/gameplay strings still contained 1,439 em dashes across 88 files, including interrupted speech, decorative headers, battle logs, journals, and map dialogue.
- The project theme and `UITheme` created `SystemFont` resources at runtime. Font selection therefore varied by player PC, while mipmaps plus automatic subpixel positioning made thin Hangul strokes look split or fuzzy at small sizes.

### Done
- Removed every em dash from shipped dialogue data and runtime game strings. Interrupted lines now use ellipses, decorative wrappers are removed, and mid-sentence joins use commas so timing and meaning remain readable.
- Added an em-dash rejection check to the Korean localization validator to prevent the punctuation from returning in dialogue or VN JSON.
- Bundled `NotoSerifKR-VF.ttf` for dialogue/narration/titles and `NotoSansKR-VF.ttf` for buttons/HUD, then rewired both the project theme and `UITheme` to those deterministic resources.
- Switched fonts to grayscale antialiasing with normal hinting and disabled mipmaps/subpixel positioning for cleaner small Hangul glyphs. Raised the VN continue/auto/choice-hint text from 10-13px to 12-14px.
- Updated font-focused visual smoke and capture assertions to verify the bundled resource paths and crisp-glyph profile.

### Verification
- Runtime em-dash scan: 0 remaining across `data/`, `scripts/`, and `scenes/`.
- Korean localization: 31 files, 1,583 fields, 19 speakers, 0 errors. VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- `VISUAL_CLARITY_SMOKE_PASS` and `STORY_COMBAT_SMOKE_PASS` completed with the bundled fonts and updated text.
- Real OpenGL capture `tmp/visual_audit/dialogue_interface_ko.png` confirmed clean Korean dialogue rendering at 1280x720.
- Representative `verdan_market.tscn` booted through Arrel, Malet, Elia, four ambient civilians, and world-population setup without script, parse, or assertion errors.
- `git diff --check` passed; only normal CRLF working-copy warnings were emitted.

## S198 - 2026-07-16 (Field character scale/texture and illustration style unification)

### Audit findings
- Major field characters shared 128x160 canvases, but runtime scale was hard-coded while actual alpha silhouettes varied; ambient civilian silhouettes ranged from roughly 104px to 144px high inside the same canvas. That produced visibly different body sizes and floating foot baselines.
- Player, companion, named NPC, and ambient NPC paths mixed nearest and linear filtering, amplifying high-frequency noise on some characters.
- World population data mixed twenty-four one-off NPC treatments. The six primary civilian archetypes already matched each other, while several specialist/optional-site variants shifted toward realistic miniature painting or clean anime.
- The live loss-record book and Memory Compass close-up used flat brown vector/bokeh styling that did not match the newer gothic archive interface.
- Battle support and named-character resolution still referenced six older transparent full-body drafts instead of the current canonical character-shot family.

### Done
- Added `PixelSprite.apply_field_profile()`: imported field sprites are now normalized by visible alpha bounds to a shared 50px adult height (42px for child civilians), horizontally centered, aligned to one foot baseline, and rendered with one low-noise linear sampling profile.
- Applied the profile to Arrel, companions, all named field NPCs, and every ambient world-population civilian. Ground shadows and presence rings now sit on the actual shared foot baseline instead of 17-25px below it.
- Consolidated all 64 ambient civilians into six established adult archetypes plus one newly generated lantern-child archetype. Role matching preserves courier, debtor, scribe, lantern-keeper, pilgrim, forager, and child silhouettes while removing the inconsistent one-off art from live placement.
- Generated and integrated `ui_loss_record_blank_book_v2.png`, `ui_memory_compass_close_v2.png`, and `coast_lantern_child_field_v2.png` with built-in GPT Image, using the current field cast, archive, and world-map art as references.
- Rewired Elia, Sable, Tobias, Nera, and Veil battle/Artbook images to the current `_v3` character-shot family.
- Removed the two rejected flat UI drafts and six obsolete full-body character drafts (including their Godot import metadata) after confirming that no live code/data references remained.
- Extended visual/world-population smoke coverage to assert apparent height, common foot baseline, texture filtering, seven civilian archetypes, regenerated UI paths, and removal of rejected UI assets.

### Verification
- Godot 4.6.2 headless import completed for all three new images with no parse or script error.
- `WORLD_POPULATION_SMOKE_PASS`: 19 maps, 64 civilian voices, 32 visible threats, 11 caches, seven atlas gates, and 31 live field assets.
- `VISUAL_CLARITY_SMOKE_PASS`: all seven named directional field sheets, common scale/filter profile, three battle-support images, 24 character shots, and regenerated archive UI assertions passed.
- Representative `verdan_market.tscn` booted through Arrel, Malet, Elia, four ambient civilians, population setup, and checkpoint autosave without a script/parse/runtime-access error.
- Real OpenGL capture `tmp/visual_audit/verdan_malet_field.png` confirmed the normalized Arrel/Malet/civilian silhouettes and corrected grounding in the live map.

## S197 - 2026-07-16 (Progression crash audit and transition hardening)

### Audit findings
- No Godot script stack trace, access violation, Windows application crash event, or damaged current autosave could be recovered. The retained logs only contained the known forced-shutdown resource cleanup notices.
- All 19 playable maps and the nine existing gameplay/UI smoke scenes booted without a parse, script, invalid-call, or freed-instance error.
- Scene changes had no ownership guard. Overlapping body-entry, dialogue-end, save-load, or button signals could start multiple asynchronous wipes, replace their shared tween/curtain state, and request competing scene loads.
- `SaveManager.load_game()` imported game, memory, diary, and tutorial state before proving that the saved destination scene still existed, allowing an obsolete save to leave the live session partially loaded.
- The enabled third-party VFX editor plugin pointed at a nonexistent `addons/vfx_library` tree (the installed folder is `addons/vfx_lib`) and emitted editor startup/shutdown errors despite no gameplay code using its autoloads.

### Done
- Added a single-owner transition guard to every scene-change style, including battle, iris, curtain, loading, and chapter-complete routes. Active transitions now block pointer input, reject competing requests, validate PackedScene destinations, inspect Godot's scene-change result, and reliably reset overlays and ownership on failure.
- Made save loading transactional with respect to its destination: missing/obsolete scene paths now emit `save_failed`, show a warning, and return before any mutable runtime state is imported.
- Extended the VN validator to check every `goto_map` PackedScene path.
- Disabled the unused broken VFX editor plugin while retaining the game's own battle/map VFX implementation.
- Added `smoke_crash_guards` to exercise helper cleanup, real public transition contention, input restoration, and invalid-save non-mutation.

### Verification
- `CRASH_GUARDS_SMOKE_PASS transition_mutex=true public_transition=true transactional_load=true`.
- All nine smoke scenes passed: ancillary archive, crash guards, field focus, gameplay QoL, quest illustrations, resonance choice, story combat, visual clarity, and world population.
- All 30 gameplay scene references resolve; VN validation passed with 20 files, 504 steps, 0 errors, and 0 warnings.
- Representative `verdan_market.tscn` ran for 600 frames through world population and dialogue entry without script/parse/runtime-access errors.
- Godot editor scan no longer emits the broken VFX plugin popup/autoload errors. ShaderV duplicate-UID and forced-headless-exit cleanup notices remain editor/add-on shutdown noise rather than a gameplay exception.

## S196 - 2026-07-16 (GPT Image 2 ancillary UI and save/archive usability pass)

### Audit findings
- The pause menu still exposed save and load as immediate one-click actions, despite `SaveManager` already supporting an autosave plus three manual slots and backup recovery.
- There was no persistent in-game reference for movement, exploration markers, battle rhythm, memory-burning consequences, or visual-clarity options.
- The minimap still used a plain translucent rectangle, and several Korean-facing HUD/toast strings could expose legacy mojibake or missing-glyph markers.

### Done
- Generated and visually audited four built-in GPT Image 2 ancillary assets against the current archive UI references:
  - `ui_save_archive_v1.png` - full autosave/manual-slot comparison and detail surface.
  - `ui_field_guide_v1.png` - text-safe six-block gameplay reference surface.
  - `ui_minimap_compass_frame_v1.png` - compact low-noise exploration frame.
  - `ui_empty_witness_record_v1.png` - matching Blank Book thumbnail for empty manual records.
- Replaced the pause menu's separate Save/Load buttons with `SAVE ARCHIVE` / `저장 기록고`:
  - compares autosave and manual slots 1-3;
  - shows chapter art, route, HP, Grains, burned-memory count, and timestamp;
  - protects autosave from manual overwrite;
  - requires a second confirmation before overwriting an occupied manual slot;
  - loads any occupied record through the existing safe `SaveManager` path.
- Added `FIELD GUIDE` / `필드 가이드` to the pause menu with concise controls, objective reading, battle rhythm, memory-burn permanence, witnessed-route, and accessibility guidance.
- Integrated the generated minimap frame at a restrained 108x96 footprint. Corrected the first runtime sizing pass so the 1024px source never expands beyond the compact top-right container.
- Routed Esc through named archive overlays first, so Save Archive, Field Guide, Item Archive, and World Map close before the underlying pause menu.
- Replaced font-risky toast symbols with ASCII-safe markers and supplied clean Korean map names, Memory Pulse text, save-record labels, toast messages, and pause footer hints.
- Registered the Save Archive and Field Guide plates in the Artbook and added `smoke_ancillary_archive` as a focused runtime regression test.

### Verification
- Inspected every final generated image before import; the older flat empty-slot book was rejected in context and replaced with the matching `ui_empty_witness_record_v1.png` asset.
- Captured Save Archive, Korean Field Guide, and minimap through Godot's real OpenGL renderer at 1280x720. Final layouts are readable, text-safe, and do not block the playfield.
- `ANCILLARY_ARCHIVE_SMOKE_PASS`: all four assets resolve, all four save rows and actions build, occupied and empty slot selection paths render, the Field Guide creates six reference blocks, and the generated minimap frame remains compact.
- `VISUAL_CLARITY_SMOKE_PASS` remained green with the new Artbook and ancillary asset assertions.
- Representative `verdan_market.tscn` headless boot reached population, checkpoint autosave, and Chapter 2 dialogue startup without parse/script errors.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- `git diff --check` passed; only the known Godot/VFX shutdown cleanup warnings remain.

## S195 - 2026-07-15 (GPT Image 2 story CG, Item Archive, and World Map expansion)

### Audit findings
- The pause menu still hid the existing fast-travel route list, so the ten-map journey had no player-facing world-map screen.
- `GameManager.ITEMS` already contained sixteen illustrated supplies and live quantities, but there was no dedicated inventory surface for reviewing them outside battle.
- Six key story beats still reused older plates or remained text-only in the archive: the fireless Ch1 camp, Malet's three clues, Kairos's Belt marks, reading deterioration, the Seam reunion, and Tobias's Ring Theory.

### Done
- Generated and visually audited six canonical-character 16:9 story CGs with built-in GPT Image 2. The first camp draft conflicted with the line `No fire`; it was corrected to a fireless `v2`, and the mismatched draft was removed.
- Replaced the exact CG step in Chapters 1, 2, 3, 4, 5, and 8 without changing dialogue order, branching, English text, or Claude's Korean text.
- Registered all six story plates in the Event Journal and Artbook; the Malet-information and Ring-Theory plates also illustrate their matching World records.
- Generated `ui_world_map_routes_v1.png`: a text-safe ten-region route atlas matching the actual Rim-to-BL-07 journey instead of the older unrelated high-fantasy map.
- Upgraded Fast Travel into `WITNESSED ROUTES / WORLD MAP`: ten chapter-gated destinations, current-route witness status, story descriptions, scrollable route index, and the existing safe scene-transition behavior.
- Generated `ui_inventory_archive_v1.png` and built `FIELD ARCHIVE`: live carried counts, existing item icons, item description/type/value inspection, and the equipped weapon/armor/accessory record with upgraded ATK/DEF bonuses.
- Restored `ITEM ARCHIVE` and `WORLD MAP` to the pause-menu navigation and placed the longer button list inside a scroll container so 720p layouts remain usable.
- Extended `smoke_visual_clarity` to verify all new assets, Artbook discovery, six Event art overrides, ten destination scenes, and runtime construction of both new panels.

### Final assets
- `archive_ch1_camp_humming_v2.png`
- `archive_ch2_information_price_v1.png`
- `archive_ch3_kairos_marks_v1.png`
- `archive_ch4_reading_loss_v1.png`
- `archive_ch6_reunion_v1.png`
- `archive_ch8_ring_theory_v1.png`
- `ui_world_map_routes_v1.png`
- `ui_inventory_archive_v1.png`

### Verification
- Inspected every final generated image with the local image viewer before project import; canonical faces, clothing, hands, props, story facts, and low-noise art direction passed.
- Captured both panels through Godot's real OpenGL renderer at 1280x720 and confirmed list/detail/equipment and map/route-index alignment with no view-blocking overlap.
- Godot 4.6.2 headless editor import completed with no `SCRIPT ERROR` or `Parse Error`.
- `smoke_visual_clarity`: `VISUAL_CLARITY_SMOKE_PASS`, including runtime construction of `InventoryOverlay/InventoryPanel` and `WorldMapOverlay/WorldMapPanel`.
- Representative `verdan_market.tscn` headless boot completed through population, autosave, and Chapter 2 dialogue startup.
- VN validation: 20 files, 504 steps, 0 errors, 0 warnings.
- `git diff --check` passed; only normal CRLF working-copy warnings were emitted.
