# MEMORIA Illustration Catalog

## Production target

- Long-term target: approximately 1,000 deliberate game illustrations.
- Current project baseline after S152: 219 PNG files under `assets/cg/`.
- Each production pass should remain a complete narrative set, not a volume-only batch.

## Non-overlap rules

1. Audit every `cg` reference and the full `assets/cg/` tree before selecting a scene.
2. Give each image one primary story beat, location, cast, emotional purpose, and filename.
3. Do not create a new image when an existing image already depicts the same beat with the same cast and camera purpose.
4. Environment plates, dialogue story CGs, battle cinematics, memory-burn images, and ending plates are separate asset roles.
5. Character identity anchors are fixed: short silver-haired Arrel, honey-blonde bob-haired Elia, rugged middle-aged Tobias, chin-length silver-haired Sable, and current Authority Kairos.
6. Dialogue CGs keep the lower screen visually quiet for the live interface and contain no generated text.

## S200 chapter-by-chapter story expansion

Fifty new 16:9 story CGs were generated with built-in GPT Image and integrated as five additional illustrated beats in every existing Part I chapter. All final images live under `assets/cg/generated/chapter_expansion/`; rejected identity-drift drafts remain outside the project.

| Chapter | New story CGs | Narrative coverage |
|---|---|---|
| 1 | `ch01_burn_afterimage_v1.png`, `ch01_ash_touch_v1.png`, `ch01_muscle_memory_reach_v1.png`, `ch01_hidden_initials_v1.png`, `ch01_memory_shrine_amber_v1.png` | First burn, ash memory, Elia recognition, A/E initials, amber shrine |
| 2 | `ch02_bottled_memories_market_v1.png`, `ch02_sump_waiting_alcoves_v1.png`, `ch02_malet_from_gray_v1.png`, `ch02_first_sword_empty_space_v1.png`, `ch02_nameless_burner_v1.png` | Memory commerce, Sump atmosphere, Malet, first sale, burnout cost |
| 3 | `ch03_waystation_haze_v1.png`, `ch03_tobias_accounting_v1.png`, `ch03_blank_book_warmth_v1.png`, `ch03_class_seven_wall_v1.png`, `ch03_three_on_belt_v1.png` | Waystation arrival, Tobias, Blank Book, Bureau classification, party departure |
| 4 | `ch04_memory_rain_shelter_v1.png`, `ch04_words_move_v1.png`, `ch04_anchor_tea_thread_v1.png`, `ch04_burner_classification_v1.png`, `ch04_table_for_three_memory_v1.png` | Memory rain, reading loss, anchoring, Burner classes, home flashback |
| 5 | `ch05_city_in_bedrock_v1.png`, `ch05_kairos_ridge_observer_v1.png`, `ch05_warm_stone_echo_v1.png`, `ch05_anchor_parting_paths_v1.png`, `ch05_first_impossible_color_v1.png` | Buried city, Kairos sighting, coastal residue, separation, restored color |
| 6 | `ch06_seam_living_streets_v1.png`, `ch06_sable_map_briefing_v1.png`, `ch06_impossible_garden_v1.png`, `ch06_stars_forgetting_v1.png`, `ch06_bl07_wound_edge_v1.png` | Living refuge, Sable briefing, remembered flower, night plea, BL-07 wound |
| 7 | `ch07_threshold_ridgeline_v1.png`, `ch07_sable_cleaner_confession_v1.png`, `ch07_controlled_burn_trial_v1.png`, `ch07_twelve_names_memorial_v1.png`, `ch07_echo_shell_voices_v1.png` | Map boundary, Cleaner past, burn trial, memorial, Echo Shell voices |
| 8 | `ch08_memory_forest_entry_v1.png`, `ch08_ghost_child_name_v1.png`, `ch08_ring_acceleration_v1.png`, `ch08_ghost_mother_search_v1.png`, `ch08_parasitic_heart_v1.png` | Forest entry, nameless child, Ring Theory, searching mother, parasitic core |
| 9 | `ch09_equalized_waste_v1.png`, `ch09_living_compass_v1.png`, `ch09_kairos_convergence_model_v1.png`, `ch09_depth_markers_v1.png`, `ch09_first_void_witness_v1.png` | Flattened Waste, living compass, Kairos model, depth rings, first Void memory |
| 10 | `ch10_absence_path_v1.png`, `ch10_borrowed_grief_v1.png`, `ch10_orphaned_lives_v1.png`, `ch10_before_core_confession_v1.png`, `ch10_name_or_weave_choice_v1.png` | Remembered stone, borrowed grief, preserved lives, core confession, final choice |

`data/chapter_expansion_gallery.json` is the single placement and Artbook manifest. It records each image's chapter, story anchor, and presentation metadata so additions can be validated without expanding the pause menu's compiled constant.

## S201 archive interface and field-item visual upgrade

Sixteen final GPT Image assets extend the established charcoal, black-blue, aged-bronze, muted-gold, pale-cyan memory, and restrained-violet interface language. Every background is text-free and reserves calm surfaces for runtime typography. Every isolated icon was generated on flat magenta, converted to RGBA, despilled, and audited for visible key-color pixels.

| Runtime role | Final assets | Integration |
|---|---|---|
| Core archive surfaces | `ui_pause_archive_backdrop_v2.png`, `ui_inventory_archive_v2.png`, `ui_character_status_dossier_v1.png` | Pause hub, carried-supply archive, and character/play-statistics dossier |
| Records and trade | `ui_codex_archive_backdrop_v2.png`, `ui_story_journal_backdrop_v3.png`, `ui_memory_shop_backdrop_v2.png` | Codex, Story Journal, and Malet's memory exchange |
| Battle supply surface | `ui_battle_item_tray_v3.png` | Transparent tactical item tray that preserves the battlefield, objective card, enemy state, and command grid |
| Upgraded atlas rewards | `root_balm_v2.png`, `signal_jammer_v2.png`, `lantern_salve_v2.png`, `name_thread_v2.png`, `compass_shard_v2.png`, `seed_capsule_v2.png` | Replaces the lower-resolution S187 item icons in inventory, shop, drops, and combat |
| Equipment dossier emblems | `slot_weapon_v1.png`, `slot_armor_v1.png`, `slot_accessory_v1.png` | Weapon, armor, and accessory records in the inventory loadout row |

`data/interface_visual_gallery.json` is the 16-entry Artbook manifest. The inventory now supplies four category filters, deterministic type ordering, HP/Grains/memory/chapter telemetry, concise mechanical effect copy, and icon-led equipment cards. The character dossier reuses Arrel's canonical story portrait. The first opaque battle-tray layout was not connected to gameplay; the live v3 tray uses isolated alpha so the scene behind it remains visible.

## S187 atlas expansion: maps, field roster, and reward items

| Set | Assets | Gameplay coverage | Art direction |
|---|---|---|---|
| Chapter-earned atlas canvases | `map_belt_signal_yard_canvas_v1.png`, `map_drift_waymarker_shrine_canvas_v1.png`, `map_coast_cinder_harbor_canvas_v1.png`, `map_seam_lantern_ward_canvas_v1.png`, `map_forest_name_hollow_canvas_v1.png`, `map_waste_grey_caravan_canvas_v1.png`, `map_bl07_seed_vault_canvas_v1.png` | Seven returnable side sites: signal yard, waymarker shrine, Cinder Harbor village, Lantern Ward town, Name Hollow, Grey Caravan camp, and Seed Vault | Text-free orthographic dark-fantasy exploration plates. Broad central routes and visible landmarks remain readable beneath live collision, NPC, threat, and reward layers. |
| Atlas residents | `harbor_net_mender_field_v1.png`, `seam_baker_field_v1.png`, `drift_route_translator_field_v1.png`, `belt_code_runner_field_v1.png`, `verdan_debt_witness_field_v1.png`, `coast_lantern_child_field_v1.png`, `forest_name_keeper_field_v1.png`, `waste_caravan_quartermaster_field_v1.png`, `bl07_seed_custodian_field_v1.png`, `rim_root_tender_field_v1.png`, `seam_bridge_keeper_field_v1.png`, `ash_weather_listener_field_v1.png` | Twenty-eight named field voices across the seven new sites | Clean 128x160 transparent miniature figures with charcoal outlines, muted clothes, a role prop, and controlled shadows. |
| Atlas threats | `signal_scavenger_field_v1.png`, `ash_bone_hound_field_v1.png`, `coast_drowned_echo_field_v1.png`, `forest_mimic_shade_field_v1.png`, `waste_glass_crawler_field_v1.png`, `bl07_archive_warden_field_v1.png`, `rail_sentinel_field_v1.png`, `rain_oracle_field_v1.png`, `lantern_leech_field_v1.png`, `root_memory_swarm_field_v1.png`, `caravan_raider_field_v1.png`, `seed_guardian_field_v1.png` | Fourteen visible optional fights, two per new site | Silhouette-first corruption designs whose materials express each biome: torn wire, ash bone, tide cloth, roots, erased glass, and void archive porcelain. |
| Atlas reward icons | `root_balm.png`, `signal_jammer.png`, `lantern_salve.png`, `name_thread.png`, `compass_shard.png`, `seed_capsule.png` | Seven physical one-time caches introduce six usable tactical items | Crisp illustrated inventory icons with isolated alpha, no text, and the same restrained gold, ember, root, and pale-blue memory palette. |

All S187 sources were made with GPT Image 2 as separate chroma-key contact sheets, then split, despilled, cropped, normalized, and alpha-audited before runtime integration. New maps and item imagery follow the existing low-noise painterly-pixel field language rather than creating a second illustration style.

## S186 world population field roster

| Set | Assets | Gameplay coverage | Art direction |
|---|---|---|---|
| Civilian and survivor roster | `rootbark_forager_field_v1.png`, `verdan_debtor_field_v1.png`, `belt_courier_field_v1.png`, `drift_scribe_field_v1.png`, `seam_lanternkeeper_field_v1.png`, `waste_pilgrim_field_v1.png` | 36 interactive witnesses, workers, refugees, and travelers across twelve maps/sites | Grounded miniature dark-fantasy field figures; soot, worn leather, muted rust, oxidized blue, and a single lantern-cyan accent. Each begins from a clean 128x160 RGBA canvas so it sits at the same gameplay scale as the cast. |
| Visible hostile roster | `ash_hound_field_v1.png`, `belt_scavenger_field_v1.png`, `signal_wisp_field_v1.png`, `rootbound_echo_field_v1.png`, `colorless_wraith_field_v1.png`, `void_fragment_field_v1.png` | 18 optional visible encounters from Rim return routes through BL-07 | Silhouette-first threats with biome-specific memory corruption: ember ash, paper and scrap, signal-blue residue, roots, erased gray, and restrained violet void fractures. No UI text or noisy backdrop. |
| Specialist civilians | `rim_herbalist_field_v1.png`, `verdan_runner_field_v1.png`, `belt_mechanic_field_v1.png`, `drift_child_archivist_field_v1.png`, `seam_medic_field_v1.png`, `waste_compass_guide_field_v1.png` | Six focal NPC roles now have a unique field identity: Rim scout, sealed-note seller, signal keeper, rain-ledger scribe, healer, and compass pilgrim. | The same muted miniature dark-fantasy language, with one story-prop per figure so their silhouette communicates their role at exploration scale. |
| Rare hostile silhouettes | `ash_stalker_field_v1.png`, `belt_tag_raider_field_v1.png`, `signal_moth_wisp_field_v1.png`, `fungal_root_sentinel_field_v1.png`, `colorless_husk_field_v1.png`, `echo_shell_field_v1.png` | Six optional battle encounters gain distinct biome silhouettes rather than another repeat of the base hostile body. | Ash plate, debt tags, fragile blue wings, fungal roots, erased-grey body, and BL-07 violet shell; all stay readable on a transparent 128x160 field canvas. |

Both rosters were generated as isolated GPT Image 2 contact sheets on removable magenta, then chroma-keyed, despilled, individually cropped, and normalized. They are intentionally reusable identity archetypes rather than a second portrait style; map placement and dialogue provide the individual character roles.

| Canvas | Gameplay coverage | Art direction |
|---|---|---|
| `map_rim_root_hollow_canvas_v1.png` | Root Hollow — Chapter 1 return site | Ancient black roots around a quiet paper shrine, pale fungus, and a wide readable cross-path. |
| `map_verdan_ledger_cellar_canvas_v1.png` | Verdan Ledger Cellar — Chapter 2 return site | Iron archive shelves, debt ledgers, a collection desk, and a broad central corridor beneath the market. |

## S185 dedicated map canvases

| Canvas | Gameplay coverage | Art direction |
|---|---|---|
| `map_drift_shelter_canvas_v2.png` | Drift Shelter | Rain-darkened archive refuge, torn awnings, wet stone plaza, subtle amber sanctuary lights |
| `map_forgotten_forest_canvas_v2.png` | Forgotten Forest | Root-choked memory forest, pale fungal shelves, cool motes, open moss clearing |
| `map_colorless_waste_canvas_v2.png` | Colorless Waste | Achromatic ash basin, split monoliths, paper flakes, one muted compass landmark |
| `map_seam_outskirts_canvas_v2.png` | Seam Outskirts | Rift-edge settlement, worn crossroads, rope bridges, lantern dots, restrained violet fracture |

These four canvases finish the one-map/one-identity exploration set begun in S183. They retain broad central routes and perimeter-only clutter so the player sprite, interactables, and collision map remain readable at game scale.

## S183 playable map canvases

| Canvas | Gameplay coverage | Art direction |
|---|---|---|
| `map_rim_forest_canvas_v1.png` | Rim Forest, Forgotten Forest | Thorn-root forest, muted moss, amber memory crystal edges |
| `map_verdan_market_canvas_v1.png` | Verdan Market | Wet iron cobbles, border stalls, restrained amber lanterns |
| `map_belt_waystation_canvas_v1.png` | Belt Waystation, Drift Shelter | Ashland platform, weathered station structures, warm refuge accents |
| `map_crumbling_coast_canvas_v1.png` | Crumbling Coast | Slate coast, collapsed pier, cliff and sea boundaries |
| `map_the_seam_canvas_v1.png` | The Seam, Seam Outskirts | Twilight refuge, violet roots, bridges and lantern-lit hub |
| `map_bl07_void_canvas_v1.png` | Colorless Waste, BL-07 Void | Fractured obsidian, pale causeway, restrained violet fissures |

These are orthographic, text-free, low-noise environmental canvases. They sit under the unchanged tile collision and interaction layers, so the map visuals gain authored landmarks while navigation logic remains deterministic.

## S184 tactical item illustration

| Asset | Gameplay role | Art direction |
|---|---|---|
| assets/ui/items/witness_ink.png | Advances a WITNESS reading by one step while guarding Arrel and charging Limit | Gunmetal archive vial, violet-blue memory fluid, gold eye seal; transparent UI asset matching the existing faceted potion and Firebomb family |

The item is intentionally a tactical bridge, not a way to bypass the story route: bosses still require a complete reading and ordinary echoes are only released after the reading is finished.

## S179 Chapter 1 cold open

| VN scene | Story beat | Asset | Source |
|---|---|---|---|
| `ch1_cold_open` | Reveals the inverted ashfall, the Void Beast, and the distant Bureau route before the first line of aftermath | `story_ch1_rim_omen.png` | GPT Image built-in |

The new plate uses the live Chapter 1 Arrel, Elia, Void Beast, and Rim Forest CGs as direct identity and style references. It remains a clean, text-free 16:9 image with restrained violet light and no film grain, paper texture, speckle noise, dithering, chromatic noise, muddy detail, or visibility-obscuring fog.

## S144 ending and epilogue set

| Dialogue group | Story beat | Asset |
|---|---|---|
| `epilogue_zero_burn` | Watches colors without names | `ending_zero_burn_canyon_watch.png` |
| `epilogue_zero_burn` | Tries the name Arrel | `ending_zero_burn_trying_name.png` |
| `epilogue_preservation` | Returns with BL-07 open | `ending_preservation_return.png` |
| `epilogue_preservation` | Chooses hands that build | `ending_preservation_building_hands.png` |
| `epilogue_ash` | Answers from emotional absence | `ending_ash_hollow_days.png` |
| `epilogue_ash` | Watches light fade without recognition | `ending_ash_sunset_shell.png` |
| `epilogue_seam` | Recognizes ordinary protected moments | `ending_seam_ordinary_moments.png` |
| `epilogue_seam` | Finds the green beginning | `ending_seam_impossible_garden.png` |
| `epilogue_tobias` | Prints Ring Theory at night | `ending_tobias_night_press.png` |
| `epilogue_tobias` | Sends twelve independent copies | `ending_tobias_twelve_archivists.png` |
| `epilogue_hollow` | Forgets the taste of water | `ending_hollow_water.png` |
| `epilogue_hollow` | Repeats a name in an empty room | `ending_hollow_name_room.png` |
| `elia_epilogue_talk` | Connects burns to shared-history loss | `epilogue_elia_collective_pattern.png` |
| `sable_epilogue_talk` | Points toward the eastern settlement | `epilogue_sable_eastern_settlement.png` |

## S145 chapter 7-9 exploration and choice set

| Dialogue group | Story beat | Asset |
|---|---|---|
| `sable_trial` | Proves a controlled burn can be survived | `story_ch7_controlled_burn_trial.png` |
| `party_preparation` | Makes the last preparations below the ridge | `story_ch7_last_field_preparations.png` |
| `threshold_atmosphere` | Discovers paper forgetting how to hold ink | `story_ch7_paper_forgetting_ink.png` |
| `outskirts_departure` | Crosses from The Seam into the dead country | `story_ch7_crossing_the_ridgeline.png` |
| `tobias_theory` | Identifies the forest's eighteenth ring | `story_ch8_eighteenth_ring.png` |
| `forest_whispers` | Distinguishes the real party from baiting faces | `story_ch8_whispers_as_bait.png` |
| `clearing_rest` | Gives Sable a quiet moment at the white cairn | `story_ch8_white_stone_shelter.png` |
| `forest_departure` | Reaches the abrupt end of color | `story_ch8_end_of_color.png` |
| `forest_atmosphere` | Examines matter that forgot how to grow | `story_ch8_forgotten_moss.png` |
| `ghost_mother` | Meets a mother-shaped memory echo | `story_ch8_ghost_mother.png` |
| `forest_heart` | Confronts the parasitic forest heart | `story_ch8_parasitic_heart.png` |
| `waste_atmosphere` | Forms a human chain against erasure | `story_ch9_human_chain.png` |
| `arrel_compass_pull` | Feels BL-07 pull beneath his name | `story_ch9_name_under_pull.png` |
| `kairos_defeated` | Watches Kairos withdraw through broken records | `story_ch9_kairos_withdrawal.png` |
| `depth_markers` | Reads compressed lives as depth markers | `story_ch9_memory_depth_markers.png` |
| `waste_final_view` | Looks back across the colorless world | `story_ch9_final_colorless_view.png` |

## Suggested 1,000-image allocation

| Asset family | Target |
|---|---:|
| Main-story and branch dialogue CGs | 350 |
| Exploration landmarks and environmental events | 180 |
| Character relationships and optional conversations | 160 |
| Battle cinematics, boss phases, and skill cut-ins | 120 |
| Memory, item, lore, and codex illustrations | 100 |
| Endings, epilogues, gallery rewards, and special modes | 90 |
| **Total** | **1,000** |

The next pass should continue through currently unillustrated optional dialogue groups, prioritizing relationship scenes and lore encounters before adding another ending variant.

## S147 code review, optional-story, and The Weave set

| Dialogue group | Story beat | Asset |
|---|---|---|
| `sq_echoes_ash_frag` | Finds the child's counting fragment | `story_ch1_echo_fragment.png` |
| `sq_echoes_ash_complete` | Restores the Ashen Figure's face | `story_ch1_ashen_figure_restored.png` |
| `sump_atmosphere` | Enters the breathing undercity | `story_ch2_sump_breathing_walls.png` |
| `sq_sump_ledger_start` | Meets the frightened ledger owner | `story_ch2_nervous_trader_ledger.png` |
| `seal_weave` | Reaches for every kept memory | `story_ch10_seal_weave.png` |
| `seal_weave` | Braids the Seal from returning color | `story_ch10_seal_weave_fire.png` |
| `seal_weave` | Remains Arrel after the gate closes | `story_ch10_seal_weave_after.png` |
| `epilogue_weave` | Returns to a truly sealed BL-07 gate | `ending_weave_sealed_gate.png` |
| `epilogue_weave` | Sable discovers a twelfth pattern | `ending_weave_sable_ledger.png` |
| `epilogue_weave` | Feels the permanent anchor in his body | `ending_weave_anchor_hand.png` |
| `epilogue_weave` | Watches color return to quiet stone | `ending_weave_colors_return.png` |

All S147 images were generated as clean, text-free 16:9 CGs with explicit exclusions for grain, speckle noise, color noise, dithering, paper/canvas texture, compression artifacts, and chromatic aberration. The first Colors Return draft was rejected because Sable drifted into a brown-haired male silhouette; the shipped version corrects her to the current chin-length silver-haired design.

## S148 chapter 2-6 optional-story and journey set

| Dialogue group | Story beat | Asset |
|---|---|---|
| `sq_sump_ledger_found` | Finds the hidden forbidden ledger | `story_ch2_ledger_found.png` |
| `sq_sump_ledger_return` | Returns the record to its frightened owner | `story_ch2_ledger_return.png` |
| `sq_sump_ledger_burn` | Lets the waiting pages catch fire | `story_ch2_ledger_burned.png` |
| `kairos_wall_writing` | Discovers a warning scratched into concrete | `story_ch3_kairos_wall_warning.png` |
| `belt_atmosphere` | Walks the trade route that became a scar | `story_ch3_dead_belt_road.png` |
| `tobias_records` | Records the residue of Arrel's combat burn | `story_ch3_tobias_battle_notes.png` |
| `drift_arrival` | Shelters beneath the collapsed overpass | `story_ch4_ash_rain_shelter.png` |
| `tobias_explains_classification` | Explains the burner's memory grades | `story_ch4_burner_classification.png` |
| `drift_departure` | Leaves under the ash-rain's gray layer | `story_ch4_ash_rain_departure.png` |
| `coast_cliff_walk` | Follows the warmer path along the coast | `story_ch5_warm_cliff_path.png` |
| `coast_watchtower` | Exposes desperate marks by lantern light | `story_ch5_scratched_watchtower.png` |
| `bl07_aftermath` | Watches the Sentinel dissolve without sealing BL-07 | `story_ch6_bl07_after_sentinel.png` |
| `seam_residents` | Hears how a garden can outlast a name | `story_ch6_seam_gardener.png` |
| `sable_preparation` | Receives one real flame before entering the wound | `story_ch6_sable_final_preparations.png` |
| `sq_sable_vigil_start` | Identifies the deliberate Void Watcher | `story_ch6_void_watcher_request.png` |
| `sq_sable_vigil_complete` | Accepts Sable's freely offered oath-memory | `story_ch6_sable_vigil_reward.png` |

All S148 images are clean, text-free 16:9 RGB CGs with the lower 28 percent reserved for dialogue UI. Prompts explicitly excluded film/photo grain, paper/canvas texture, speckle and color noise, dithering, compression artifacts, chromatic aberration, dirty-lens overlays, muddy detail, and oversharpening. The first Chapter 5 cliff and watchtower drafts were rejected for Arrel/Elia identity drift; the shipped images restore short silver-haired Arrel and honey-blonde bob-haired Elia.

## S150 Part II Aftermath vertical slice

| VN scene / status | Story beat | Asset | Source |
|---|---|---|---|
| `ch11_departure` | Witnesses an Executor erase an old man's motor memory | `ch11_executor_strike.png` | User-provided 66.png |
| `ch11_departure` | Sees the Belt as a city-wide prison | `env_gray_belt_panorama.png` | User-provided 67.png |
| `ch11_departure` | Elia hides the cost of reading the blank notebook | `ch11_elia_bloodwork.png` | GPT Image |
| `ch12_reader` | Finds Verdan's Sump sealed and emptied | `ch12_sump_closed.png` | GPT Image |
| `ch13_third_person` | Joins two blank notebooks into a relay map | `ch13_notebook_resonance.png` | User-provided 68.png |
| `ch14_confessor_intervention` | Enters the Authority's shadowless extraction hall | `ch14_confessor_hall.png` | User-provided 69.png |
| `ch14_confessor_intervention` | Converts the intervention vow into a golden slash | `ch14_arrel_burn_slash.png` | GPT Image |
| `ch17_forgetting_storm` | Faces the violet Forgetting Storm | `ch17_oblivion_storm.png` | User-provided 70.png |
| `ch18_living_funeral` | Witnesses the public Living Funeral | `ch18_living_funeral.png` | User-provided 71.png |

The three generated S150 images use the six supplied Part II plates as style and identity references. They are clean, text-free 16:9 RGB CGs with a dark, low-detail lower UI zone and explicit exclusions for grain, speckle/color noise, dithering, paper/canvas overlays, chromatic aberration, and dirty-lens artifacts. The original Ch17 and Ch18 plates entered active story use in S151 after the preceding chapters were implemented.

## S151 Part II Storm vertical slice

| VN scene / status | Story beat | Asset | Source |
|---|---|---|---|
| `ch15_singer` | Han hums the Celah-linked lullaby beneath Arkein | `ch15_lullaby_moment.png` | User-provided 72.png |
| `ch15_singer` | Wakes the Echo Shell through wordless resonance | `ch15_echo_shell_awakening.png` | GPT Image |
| `ch16_nera` | Crosses the drowned eastern causeway toward the storm | `ch16_eastward_road.png` | GPT Image |
| `ch16_nera` | Meets Nera at an Authority road checkpoint | `ch16_nera_checkpoint.png` | GPT Image |
| `ch17_forgetting_storm` | Shields Elia and Tobias as memories fracture | `ch17_memory_fracture.png` | GPT Image |
| `ch18_living_funeral` | Sees Tobias hold the record through extraction | `ch18_tobias_close.png` | GPT Image |
| Reserved for Ch19 | Establishes Lumea's white sanctuary | `env_lumea_sanctum.png` | User-provided 73.png |
| Reserved for Ch20 | Encounters the hollow Monolith archivist | `ch20_archivist_hollow.png` | User-provided 75.png |
| Reserved for Ch21 | Confronts Kairós at the Editor's Turn | `ch21_kairos_confront.png` | User-provided 74.png |

The five generated S151 images use current character sheets plus the supplied story plates as identity and art-direction references. Every prompt explicitly excludes film/photo grain, paper/canvas overlays, speckle and color noise, dithering, compression artifacts, chromatic aberration, dirty-lens effects, muddy detail, excessive bloom, and oversharpening. The three later-act plates remain Artbook-only storyboards until Chapters 19-21 are implemented, so their reveals do not leak into the Chapter 15-18 runtime sequence.

## S152 supplied illustration integration

| VN scene / status | Story beat | Asset | Source |
|---|---|---|---|
| `ch13_third_person` | Decodes the continental relay map | `ch13_relay_decoded.png` | User-provided 81.png |
| `ch13_third_person` | Exposes the register writing Arrel's signature | `ch13_relay_breakthrough.png` | User-provided 86.png |
| `ch15_singer` | Receives Han's eastern memory fragments | `ch15_han_memory_gift.png` | User-provided 76.png |
| `ch17_forgetting_storm` | Sees the storm erase the horizon | `ch17_storm_horizon.png` | User-provided 87.png |
| `ch17_forgetting_storm` | Watches Arrel resist the storm's classification | `ch17_arrel_resist.png` | User-provided 77.png |
| `ch18_living_funeral` | Watches Tobias choose the extraction platform | `ch18_tobias_platform.png` | User-provided 78.png |
| Artbook alternate | Holds Han's later quiet-song composition | `ch15_han_last_hum.png` | User-provided 82.png |
| Reserved for Ch19 | Enters Lumea's inner white court | `env_lumea_inner_court.png` | User-provided 83.png |
| Reserved for Ch20 | Crosses the Monolith's impossible interior | `ch20_monolith_interior.png` | User-provided 89.png |
| Reserved for Ch20 | Finds Celah held in a preservation apparatus | `ch20_celah_preserved.png` | User-provided 88.png |
| Reserved for Ch20 | Enters the crystalline memory gallery | `ch20_archivist_memory_gallery.png` | User-provided 80.png |
| Reserved for Ch20 | Receives the Archivist's invitation | `ch20_archivist_offer.png` | User-provided 84.png |
| Reserved for Ch20 | Faces the Archivist's warning | `ch20_archivist_warning.png` | User-provided 90.png |
| Reserved for Ch21 | Meets Kairós at the sabotage threshold | `ch21_kairos_threshold.png` | User-provided 79.png |
| Reserved for Ch22 | Reaches the Monolith core | `ch22_monolith_core.png` | User-provided 85.png |

All 15 S152 plates are 1672x941 RGB images. Six are placed directly into the existing Chapter 13/15/17/18 VN rhythm; the remaining nine are catalogued in the Artbook without runtime references so Chapters 19-22 retain their reveal order.

## S154 Sable canon regeneration and Part III bridge set

| Runtime scene / role | Story beat | Asset | Source |
|---|---|---|---|
| Dialogue portraits | Establishes Sable/Halda as the same blind veteran in every expression | `sable_neutral.jpg`, `sable_calm.jpg`, `sable_face_neutral.png`, `sable_face_calm.png` | GPT Image + deterministic size variants |
| Character reference | Locks Sable's age, scar, clouded eyes, violet coat, and brass pin | `sable_canon_master.png` | GPT Image |
| `seam_arrival` | Meets the old woman waiting without watching | `story_ch5_seam_first_light.png` | GPT Image regeneration |
| `sable_briefing` | Reads BL-07's raised route by touch | `story_ch6_sable_briefing.png` | GPT Image regeneration |
| `epilogue_preservation` | Waits inside the open gate by sound rather than sight | `ending_preservation_return.png` | GPT Image regeneration |
| `epilogue_weave` | Pricks the eighteenth pattern into her tactile ledger | `ending_weave_sable_ledger.png` | GPT Image regeneration |
| `ch19_approach` | Sees the Monolith open for a white-robed procession | `ch19_monolith_gates.png` | GPT Image |
| `ch19_approach` | Recognizes Vael's walk before remembering his name | `ch19_vael_silhouette.png` | GPT Image |
| `ch20_monolith` | Finds the Chief Archivist at a desk that barely exists | `ch20_archivist_desk.png` | GPT Image |
| `ch21_editors_turn` | Nera feels her first emotion in nineteen years | `ch21_nera_hesitation.png` | GPT Image + Nera identity reference |
| `ch22_core` | Reaches the decision threshold at the primal log | `ch22_conversion_threshold.png` | GPT Image |
| `ch23_conversion` | Watches the extraction current reverse and flow outward | `ch23_conversion_wave.png` | GPT Image |
| `ch24_testimony` | Leaves the story with a child carrying the last lullaby | `ch24_last_lullaby.png` | GPT Image |

All new story CGs are clean 1672x941 RGB plates with text-free compositions and a quiet lower dialogue zone. Prompts explicitly excluded film/photo grain, paper or canvas overlays, speckles, chromatic noise, compression artifacts, muddy detail, and excessive bloom. Sable's canon LOCK remained: `old blind woman, late 60s, short white hair, weathered coast-stone face, pale clouded eyes, dark violet-toned coat, small scar, composed and unhurried`.

## S157 style-locked Field Focus environment set

| Runtime map | Memory echo | Asset | Direct style reference |
|---|---|---|---|
| `rim_forest` | Lost footsteps retained by the Rim | `resonance_rim_forest_echo.png` | `story_ch1_twisted_forest_path.png` |
| `verdan_market` | Warm steam from a vanished market meal | `resonance_verdan_market_echo.png` | `chapter_splash_verdan_market.png` |
| `crumbling_coast` | A handprint condensed from salt wind | `resonance_crumbling_coast_echo.png` | `chapter_splash_crumbling_coast.png` |
| `forgotten_forest` | A hollow tree's unfinished sentence | `resonance_forgotten_forest_echo.png` | `chapter_splash_forgotten_forest.png` |

These plates deliberately avoid unfinalized character designs. Each one was generated from its active map CG as a direct style reference, preserving the same environment geometry, palette family, realistic dark-fantasy rendering, contrast, and lower dialogue-safe zone. All four are clean 1672x941 RGB images with no grain, paper/canvas texture, speckles, dithering, chromatic noise, compression artifacts, or dirty-lens overlays.

## S158 Memory Key payoff set

| Runtime scene | Kept-memory payoff | Asset | Direct style reference |
|---|---|---|---|
| `ch12_reader` | A remembered market taste reveals a regulars-only passage | `memory_key_verdan_passage.png` | `ch12_sump_closed.png` + `chapter_splash_verdan_market.png` |
| `ch14_confessor_intervention` | The first sword grip opens the extraction cradle without shattering it | `memory_key_confessor_hinge.png` | `ch14_confessor_hall.png` |
| `ch15_singer` | The campfire song and Han's lullaby resolve into one First-Age refrain | `memory_key_first_age_refrain.png` | `ch15_echo_shell_awakening.png` |

The three plates turn hidden kept-memory choices into visible rewards without introducing any unfinalized character design. Each uses an active scene CG as a direct reference and preserves a quiet lower dialogue zone. Prompts explicitly excluded film/photo grain, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens overlays, muddy brush noise, oversharpening, and excessive bloom.

## S159 Memory Key payoff completion set

| Runtime scene | Kept-memory payoff | Asset | Direct style reference |
|---|---|---|---|
| `ch17_forgetting_storm` | True rain on forest earth reveals a safe route through false weather | `memory_key_forest_rain.png` | `ch17_oblivion_storm.png` |
| `ch19_approach` | Linked hands pass Lumea's barrier as one signature | `memory_key_single_signature.png` | `ch19_monolith_gates.png` |
| `ch21_editors_turn` | Three allies preserve one page from Kairós's burning notebook | `memory_key_surviving_page.png` | `ch21_kairos_confront.png` |
| `ch22_core` | Remembered warmth opens the relay around an anchor, not a wound | `memory_key_relay_anchor.png` | `ch22_conversion_threshold.png` |

This completes a dedicated visual payoff for all seven S149 Memory Keys. Ch17 and Ch21 remain environment/prop-only; Ch19 and Ch22 use tightly cropped hands with established costume colors, avoiding new faces or character redesigns. Every plate preserves a low-detail lower dialogue zone and excludes film/photo grain, paper or canvas overlays, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, and excessive bloom.

## S161 narrative bridge set

| Runtime scene | Story beat | Asset | Direct style reference |
|---|---|---|---|
| `ch12_reader` | Finds the patrol-lit route behind Verdan's abandoned stalls | `ch12_hidden_passage.png` | `ch12_sump_closed.png` |
| `ch16_nera` | Watches the Forgetting Storm move the horizon itself | `ch16_moving_horizon.png` | `ch16_eastward_road.png` |
| `ch18_living_funeral` | Escapes after the extraction platform releases every restraint | `ch18_broken_funeral_platform.png` | `ch18_living_funeral.png` + `ch18_tobias_platform.png` |
| `ch20_monolith` | Discovers memory-fire flowing outward instead of being consumed | `ch20_reverse_memory_fire.png` | `ch20_monolith_interior.png` + `ch20_archivist_desk.png` |

All four plates were generated with GPT Image as clean 1672x941 RGB story CGs. Each keeps the lower dialogue zone quiet, preserves its chapter's established architecture and palette, and avoids introducing unfinalized faces. Prompts explicitly excluded film/photo grain, canvas or paper texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, and visibility-blocking fog.

## S162 complete Field Focus environment set

| Runtime map | Memory echo | Asset | Direct style reference |
|---|---|---|---|
| `belt_waystation` | A dead-soil footprint reconnects to Tobias's abandoned ledger | `resonance_belt_waystation_echo.png` | `chapter_splash_belt_waystation.png` |
| `drift_shelter` | Elia's remembered warmth leaves one dry circle on the shelter table | `resonance_drift_shelter_echo.png` | `chapter_splash_drift_shelter.png` |
| `the_seam` | Sable's trust survives as a tactile route through white flowers | `resonance_the_seam_echo.png` | `chapter_splash_the_seam.png` |
| `seam_outskirts` | The Echo Shell turns one clear note into a bridge | `resonance_seam_outskirts_echo.png` | `chapter_splash_seam_outskirts.png` |
| `colorless_waste` | The memory compass returns color to named stones | `resonance_colorless_waste_echo.png` | `story_ch9_final_colorless_view.png` |
| `bl07_void` | The Void bends around one footprint it failed to erase | `resonance_bl07_void_echo.png` | `chapter_splash_bl07_void.png` |

Together with the four S157 plates, all ten `MemoryResonance.RESONANCE_POINTS` maps now have a dedicated first-discovery CG. All six new plates are clean 1672x941 RGB images generated with GPT Image from their live chapter art, with no invented faces and explicit exclusions for film/photo grain, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, and visibility-blocking fog.

## S163 branch consequence illustration set

| Runtime scene / branch | Story beat | Asset | Direct style reference |
|---|---|---|---|
| `ch11_departure` / burn | The burned walking cadence opens a fast route through the Belt | `ch11_burned_stride.png` | `env_gray_belt_panorama.png` |
| `ch11_departure` / keep | The maintenance alleys preserve the memory but prolong the danger | `ch11_maintenance_alley.png` | `env_gray_belt_panorama.png` |
| `ch12_reader` / keep | The party keeps Verdan's faces and descends into black water | `ch12_black_service_stair.png` | `ch12_hidden_passage.png` |
| `ch14_confessor_intervention` / keep | Tobias locates the extraction cradle's third conduit | `ch14_third_conduit.png` | `ch14_confessor_hall.png` + `memory_key_confessor_hinge.png` |
| `ch16_nera` / burn | A sacrificed route reveals the flooded shortcut | `ch16_flooded_shortcut.png` | `ch16_moving_horizon.png` |
| `ch18_living_funeral` / keep | Tobias remains as an unnamed witness while the notebook escapes | `ch18_witness_without_name.png` | `ch18_living_funeral.png` + `ch18_tobias_platform.png` |
| `ch20_monolith` / burn | Three companion shadows become an absence that parts the memory sea | `ch20_absence_parts_sea.png` | `ch20_monolith_interior.png` |
| `ch20_monolith` / keep | Han's note holds the sea back one hand's width | `ch20_hans_note_rim.png` | `ch20_monolith_interior.png` |
| `ch21_editors_turn` | The party descends a stair absent from every official ledger | `ch21_unlisted_stair.png` | `ch21_kairos_confront.png` |
| `ch22_core` | The primal log completes itself as an address | `ch22_book_becomes_address.png` | `ch22_monolith_core.png` + `ch22_conversion_threshold.png` |

All ten CGs were generated with GPT Image as clean 1672x941 RGB plates and attached to existing VN steps, preserving every choice and `goto` index. The set remains environment-led or uses anonymous silhouettes so no unfinalized face or costume canon is introduced. Every prompt reserved a quiet lower dialogue zone and explicitly excluded film/photo grain, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, and visibility-blocking fog.

## S164 choice-result and conversion set

| Runtime scene / branch | Story beat | Asset | Direct style reference |
|---|---|---|---|
| `ch15_singer` / burn | The final lullaby note becomes a route and disappears | `ch15_burned_last_note.png` | `ch15_echo_shell_awakening.png` |
| `ch15_singer` / keep | Han repeats the intact refrain until dawn | `ch15_dawn_refrain.png` | `ch15_echo_shell_awakening.png` |
| `ch16_nera` / keep | The party preserves the map while the patrol closes | `ch16_checkpoint_pursuit.png` | `ch16_nera_checkpoint.png` + `ch16_eastward_road.png` |
| `ch19_approach` / burn | Arrel crosses Lumea after burning his first impression | `ch19_blank_first_impression.png` | `env_lumea_sanctum.png` + `ch19_monolith_gates.png` |
| `ch19_approach` / keep | Han stretches one sustaining note across the barrier | `ch19_han_wire_note.png` | `ch19_monolith_gates.png` + `ch15_echo_shell_awakening.png` |
| `ch19_approach` | Sable's tactile ledger reaches the outer ring | `ch19_sables_ledger_arrives.png` | `ch19_monolith_gates.png` + `ending_weave_sable_ledger.png` |
| `ch21_editors_turn` / burn | Seventeen ledger names become a wall against Belor | `ch21_seventeen_name_wall.png` | `ch21_kairos_confront.png` + `ending_weave_sable_ledger.png` |
| `ch21_editors_turn` / retreat | Kairós's notebook burns in orderly white flame | `ch21_notebook_white_flame.png` | `ch21_kairos_confront.png` + `ch21_unlisted_stair.png` |
| `ch22_core` / accept | Arrel steps back and lets Elia become the reader | `ch22_relay_accepts_elia.png` | `ch22_monolith_core.png` + `ch22_book_becomes_address.png` |
| `ch22_core` / refuse | Joined hands prevent the doorway from standing alone | `ch22_anchor_refusal.png` | `ch22_monolith_core.png` + `memory_key_relay_anchor.png` |
| `ch23_conversion` / zero burn | Arrel's name unspools through the Monolith | `ch23_name_unspooled.png` | `ch23_conversion_wave.png` + `ending_zero_burn_trying_name.png` |
| `ch23_conversion` / weave | Intact memories braid the torn core together | `ch23_braided_conversion.png` | `ch23_conversion_wave.png` + `ending_weave_colors_return.png` |
| `ch23_conversion` / preservation | The conversion wave forms a shoreline before the far towns | `ch23_partial_shoreline.png` | `ch23_conversion_wave.png` + `ending_preservation_return.png` |

All thirteen plates were generated with built-in GPT Image and attached to existing choice-result steps, so every `goto` and `start_index` remains stable. Direct chapter art locked the architecture, palette, costume silhouettes, and energy language. A draft with character identity drift was rejected and replaced by the final back-facing, prop-led `ch21_seventeen_name_wall.png`. Every final prompt reserved a quiet lower dialogue zone and excluded film/photo grain, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, dense fog, and visibility-obscuring particles.

## S165 mid-story consequence set

| Runtime scene | Story beat | Asset | Direct style reference |
|---|---|---|---|
| `ch13_third_person` | Tobias confronts the party beneath Arkein's reading wall | `ch13_tobias_reading_wall.png` | `ch13_relay_decoded.png` |
| `ch14_confessor_intervention` | The dying hall completes Arrel's burn signature | `ch14_signature_registered.png` | `ch14_confessor_hall.png` + `ch14_arrel_burn_slash.png` |
| `ch14_confessor_intervention` | Kairós closes an incident report instead of sending it | `ch14_unsent_incident_report.png` | `ch21_kairos_confront.png` + `kairos_reference_turnaround.png` |
| `ch17_forgetting_storm` | Elia and Tobias fall at the center of the storm | `ch17_storm_center_fall.png` | `ch17_oblivion_storm.png` + `ch17_arrel_resist.png` |
| `ch18_living_funeral` / rescued | Tobias leaves the broken platform alive | `ch18_tobias_rescued_aftermath.png` | `ch18_living_funeral.png` + `ch18_tobias_close.png` |
| `ch18_living_funeral` / lost | The crowd remembers an empty human-shaped absence | `ch18_crowd_forgets_tobias.png` | `ch18_witness_without_name.png` + `ch18_broken_funeral_platform.png` |

All six plates were generated with built-in GPT Image and attached to existing VN steps without shifting branch indices. A first rescued-Tobias draft was rejected because it flattened all three costumes into black; the final version restores Arrel's blue armor, Elia's white-gold cloak, and Tobias's dark archivist coat. The common prompt set preserved a quiet lower dialogue zone and excluded grain, photo noise, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, dense fog, and visibility-obscuring clutter.

## S166 final session illustration set

| Runtime scene | Story beat | Asset | Direct style reference |
|---|---|---|---|
| `ch12_reader` | Pell's hidden name returns beneath Elia's thumb | `ch12_pell_name_returns.png` | `ch12_hidden_passage.png` |
| `ch16_nera` | Nera finds a page already erased from her dossier | `ch16_blank_dossier_page.png` | `ch16_nera_checkpoint.png` |
| `ch17_forgetting_storm` | The storm recedes and leaves subtly altered edges | `ch17_storm_afterimage.png` | `ch17_storm_center_fall.png` |
| `ch18_living_funeral` | The eastern monolith answers with violet light | `ch18_monolith_answers.png` | `ch18_living_funeral.png` |

All four plates were generated with built-in GPT Image and attached to existing VN steps, preserving every branch index. The set uses prop-led, back-facing, or environment-led compositions to protect established identities and reserves the lower 28 percent for dialogue UI. Prompts excluded film or photo grain, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, dense fog, and visibility-obscuring particles.

## S171 Part I emotional-beat expansion

| Runtime event | Story beat | Asset | Direct style reference |
|---|---|---|---|
| `chapter2_dialogue` / `malet_deal` | A teacher's hand closes over Arrel's first wooden sword | `story_ch2_lost_instructor_grip.png` | `story_ch2_first_sword_extraction.png` |
| `chapter3_dialogue` / `blank_book_discovery` | The Blank Book warms in Arrel's hands | `story_ch3_blank_book_warmth.png` | `story_ch3_waystation_blank_book.png` |
| `chapter4_dialogue` / `anchoring_session` | Elia anchors one small memory of bread and warmth | `story_ch4_anchor_binding.png` | `story_ch4_night_counting_losses.png` |
| `chapter5_dialogue` / `elia_separates_response` | Elia walks south without looking back | `story_ch5_elia_southbound.png` | `dialogue_ch5_elia_cliff_choice.png` |
| `chapter6_dialogue` / `sable_past` | Sable folds the map after naming the team she buried | `story_ch6_sable_buried_maps.png` | `story_ch6_sable_briefing.png` |
| `chapter7_dialogue` / `sable_truth` | BL-07 is revealed as a mouth, not a hole | `story_ch7_hungry_mouth.png` | `dialogue_ch7_sable_echo_shell.png` |
| `chapter8_dialogue` / `ghost_encounter` | The party leaves a wordless remnant behind | `story_ch8_silent_remnant.png` | `story_ch8_memory_forest_remnant.png` |
| `chapter9_dialogue` / `kairos_truth` | Kairos becomes an absence in the Colorless Waste | `story_ch9_kairos_absence.png` | `story_ch9_kairos_confrontation.png` |
| `chapter10_dialogue` / `void_before_core` | Arrel and Elia face the core with choice intact | `story_ch10_choice_at_core.png` | `chapter_splash_bl07_void.png` |

All nine plates were generated with built-in GPT Image and attached to existing Part I dialogue lines without changing event keys, dialogue ordering, branches, or localization. A face-forward Blank Book draft was rejected for character-identity drift; the final plate uses only costume-locked hands and silhouettes. The common prompt set preserved a quiet lower dialogue zone and excluded film/photo grain, noise, paper or canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, oversharpening, excessive bloom, and visibility-obscuring particles.

## S172 Canon battle-support correction

| Runtime role | Asset | Identity/style references |
|---|---|---|
| Sable support action cut-in | `cinematic_sable_echo_strike.png` | `sable_canon_master.png`, previous cut-in composition, Tobias cut-in finish |
| Sable battle-stage character | `sable_battle_fullbody.png` | `sable_canon_master.png`, Tobias full-body presentation |
| Tobias battle-stage character | `tobias_battle_fullbody.png` | `tobias_fullbody.png`, `tobias_sprite_sheet_reference.png` |
| Canon story correction | `story_ch6_sable_final_preparations.png`, `story_ch6_sable_vigil_reward.png`, `story_ch7_fading_names_monument.png`, `story_ch7_sable_confession.png`, `story_ch7_controlled_burn_trial.png`, `story_ch8_eighteenth_ring.png`, `story_ch8_white_stone_shelter.png` | Original scene composition, `sable_canon_master.png`, `story_ch5_seam_first_light.png` |
| Canon ending correction | `epilogue_sable_eastern_settlement.png`, `ending_weave_sealed_gate.png`, `ending_weave_colors_return.png` | Original ending vista, `sable_canon_master.png`, `story_ch5_seam_first_light.png` |

The outdated young, sighted Sable cut-in and ten remaining high-visibility story/ending plates were replaced in place with the confirmed Halda canon: an old blind woman in her late sixties with short white hair, clouded eyes, a weathered face, a small scar, and a dark violet coat. The two stage characters were generated on flat chroma backgrounds with built-in GPT Image, converted to alpha PNGs locally, visually inspected, and connected to the existing Sable/Tobias battle support paths. Prompts excluded youthful identity drift, opaque stage backgrounds, text, watermarks, heavy particles, grain, paper/canvas texture, and excessive bloom.

## S180 tactile inventory and Malet revelation pass

| Runtime role | Asset | Integration |
|---|---|---|
| Consumable inventory family | `assets/ui/items/{potion,hi_potion,antidote,firebomb,smoke_bomb,grains}.png` | Battle item chooser, Malet's item trade tab, victory item drops, and Grains earned row |
| Elia battle-stage character | `elia_battle_anchor_fullbody.png` | Prioritized for Elia's battle companion frame, with the existing directional sheet retained as a safe fallback |
| Malet character revelation | `story_ch2_malet_seventeen_eyes.png` | `chapter2_dialogue` / `malet_backstory`, final amber-eye narration; also registered in the Artbook |

The consumables were generated as a single coherent six-icon set on a flat chroma field, split into individual PNGs, and locally converted to alpha. The item family uses a shared gold rim light, deep violet shadows, and clear color-coded silhouettes so choices remain legible at button scale: cobalt healing, ivory-green cleansing, black-gold fire, charcoal smoke, and blue-gold Grains. Elia's staff and navy silhouette are based on the canonical portrait palette. Malet's plate preserves his charcoal coat, amber eyes, violet-black room, and an empty lower dialogue band; it was attached only to the line that reveals the seventeen stolen memories. Prompts excluded text, watermarks, grain, paper/canvas texture, speckles, dithering, chromatic noise, muddy detail, excessive bloom, and visibility-obscuring particles.

## S182 corrected story-turn illustration expansion

| Runtime event | Story beat | Asset | Direct story anchor |
|---|---|---|---|
| `chapter3_dialogue` / `tobias_encounter` | Tobias's journals spill before he explains why he records the Belt | `story_ch3_tobias_record_spill_v2.png` | `story_ch3_tobias_waystation.png` |
| `chapter5_dialogue` / `elia_before_separation` | Arrel and Elia almost let their anchor slip above the gray coast | `story_ch5_threaded_horizon_v2.png` | `story_ch5_warm_cliff_path.png` |
| `chapter7_dialogue` / `trial_complete` | Elia steadies Arrel while Sable witnesses the surviving residue | `story_ch7_residue_witness_v2.png` | `story_ch7_controlled_burn_trial.png` |
| `chapter8_dialogue` / `elia_anchor_strain` | Elia holds Arrel's name beneath the forest roots | `story_ch8_anchor_under_roots_v2.png` | `story_ch8_eighteenth_ring.png` |

The prior four S181 plates were removed after a direct art-style audit. Their replacements were generated with built-in GPT Image 2 against the established in-game plates above: fine charcoal linework, muted iron-blue/umber values, grounded proportions, restrained warm light, and no fabricated dialogue band. All four attach to the same narrative line without changing event order, choice indexes, or Korean localization. Tobias retains his dark hair, wire spectacles, and ink-stained archivist look; Arrel retains short silver hair and blue-black gear; Elia retains her honey-blonde bob, white-gold clothing, and navy cape; Sable remains elderly and blind. Prompts exclude text, watermarks, film or photo grain, paper/canvas texture, speckles, dithering, chromatic noise, compression artifacts, dirty-lens effects, muddy detail, excessive bloom, dense fog, and visibility-obscuring particles.

## S198 field-cast and interface style unification

| Runtime role | Replacement | Integration |
|---|---|---|
| Child civilian field archetype | `coast_lantern_child_field_v2.png` | All world-population names containing `Child`; shared 42px apparent-height profile |
| Loss-record fallback | `ui_loss_record_blank_book_v2.png` | World-rewrite loss plate and save-record chapter fallback |
| Memory Compass close-up | `ui_memory_compass_close_v2.png` | Exploration Memory Compass art plate |
| Battle support cast | `elia_anchor_v3.png`, `sable_warden_v3.png`, `tobias_ledger_v3.png` | Battle-stage portraits and Artbook |
| Bureau/recon enemy cast | `nera_dossier_v3.png`, `tobias_ledger_v3.png`, `veil_recon_v3.png` | BattleManager named-character image resolution |

The two flat brown UI drafts and six mismatched transparent anime-style full-body drafts were removed after all live references moved to the established charcoal, iron-blue, antique-brass, and violet-memory visual language. Ambient civilians now resolve through seven coherent field archetypes instead of mixing twenty-four unrelated texture treatments. Imported player, companion, named-NPC, and ambient-NPC art is normalized from its alpha bounds to a common visible height, centered silhouette, foot baseline, and linear low-noise sampling profile.
