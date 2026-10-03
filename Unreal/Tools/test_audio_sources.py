import json
import unittest
import wave
import io
import generate_audio_sources as g

# Durations stated in scripts/systems/audio_manager.gd for each rendered cue.
SOURCE_DURATIONS = {
    'confirm': 0.1, 'cancel': 0.15, 'burn': 0.4, 'hit': 0.12, 'heal': 0.3, 'step': 0.06, 'step_stone': 0.05, 'shield': 0.35,
    'drain': 0.35, 'phase_change': 0.6, 'defeat': 0.5, 'enemy_die': 0.55, 'flee': 0.2, 'memory_add': 0.4,
    'ui_hover': 0.04, 'ui_select': 0.08, 'ui_open': 0.12, 'ui_close': 0.1, 'battle_intro': 0.6,
    'sword_slash': 0.05 + 0.25, 'burn_ignite': 0.1 + 0.35, 'shield_break': 0.18, 'heal_layered': 0.05 + 0.6,
    'rising_tone': 0.5, 'wind_light': 3.0, 'heartbeat': 1.0, 'void_pulse': 0.5, 'rain': 3.0,
}


class AudioSourceTests(unittest.TestCase):
    def setUp(self):
        self.files = g.build()
        self.manifest = json.loads(self.files['manifest.json'])

    def test_every_cue_matches_source_duration(self):
        self.assertEqual(set(self.manifest['cues']), set(SOURCE_DURATIONS))
        for name, expected in SOURCE_DURATIONS.items():
            self.assertAlmostEqual(self.manifest['cues'][name]['duration'], expected, delta=1.5 / g.SR, msg=name)

    def test_generation_is_deterministic(self):
        self.assertEqual(self.files, g.build())

    def test_wav_format_matches_godot_stream(self):
        for name, cue in self.manifest['cues'].items():
            with wave.open(io.BytesIO(self.files[cue['file']])) as w:
                self.assertEqual((w.getnchannels(), w.getsampwidth(), w.getframerate()), (1, 2, 22050), name)
                self.assertEqual(w.getnframes(), cue['samples'], name)

    def test_only_ambient_and_heartbeat_loop(self):
        self.assertEqual({n for n, c in self.manifest['cues'].items() if c['loop']}, {'wind_light', 'heartbeat', 'rain'})

    def test_layer_delays_offset_the_premix(self):
        rng = g.random.Random(1)
        slash = g.premix('sword_slash', rng)
        # Only the whoosh layer sounds before the 0.03 s thud offset.
        self.assertEqual(len(slash), int(round(0.05 * g.SR)) + int(g.SR * 0.25))


if __name__ == '__main__':
    unittest.main()
