class_name MemoryGradeCodec
extends RefCounted

## Memory strength has two representations:
## - legacy internal rank 0..4, persisted by existing saves
## - current canon grade 1..5, shown to players
## Keep the conversion explicit so changing terminology never corrupts old saves.

const INTERNAL_SCHEMA: String = "internal_rank_0_4_v1"
const CANON_SCHEMA: String = "canon_grade_1_5_v1"

const EN_NAMES: Array[String] = ["Ember", "Flame", "Blaze", "Sun", "Zero"]
const KO_NAMES: Array[String] = ["잔불", "불꽃", "화염", "태양", "영점"]

static func canon_number_from_internal(rank: int) -> int:
	return clampi(rank, 0, 4) + 1

static func internal_from_canon(grade: int) -> int:
	return clampi(grade, 1, 5) - 1

static func label_from_internal(rank: int, korean: bool = false) -> String:
	var normalized := clampi(rank, 0, 4)
	var name: String = KO_NAMES[normalized] if korean else EN_NAMES[normalized]
	return "%d %s" % [normalized + 1, name]

static func normalize_saved_grade(raw_grade: int, schema: String) -> int:
	match schema:
		CANON_SCHEMA:
			return internal_from_canon(raw_grade)
		INTERNAL_SCHEMA, "":
			return clampi(raw_grade, 0, 4)
		_:
			push_warning("[MemoryGradeCodec] Unknown grade schema '%s'; treating value as legacy internal rank." % schema)
			return clampi(raw_grade, 0, 4)
