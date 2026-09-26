"""Render the Godot AudioManager's procedural cues to deterministic WAV sources.

Every formula mirrors scripts/systems/audio_manager.gd (22050 Hz mono, 16-bit,
int(clamp(s)*32767)). Godot draws randf per play; here each cue uses a fixed
seed, so the noise is one stable realization of the same generator.
Layered combat cues are premixed with their source delays and layer volumes.
Usage: generate_audio_sources.py [--check]
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import random
import struct
import wave
import zlib

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Unreal/ArtSource/Audio'
SOURCE = 'scripts/systems/audio_manager.gd'
SR = 22050
TAU = math.tau


def lerp(a, b, t):
    return a + (b - a) * t


def render(duration, fn):
    return [fn(i / SR) for i in range(int(SR * duration))]


def sfx(kind, rng):
    """_generate_sfx (audio_manager.gd:245)."""
    u = rng.uniform
    if kind == 'confirm':
        d = 0.1; return render(d, lambda t: math.sin(t * 880 * TAU) * 0.3 * (1 - t / d))
    if kind == 'cancel':
        d = 0.15; return render(d, lambda t: math.sin(t * 330 * TAU) * 0.25 * (1 - t / d))
    if kind == 'burn':
        d = 0.4; return render(d, lambda t: (math.sin(t * 220 * TAU) * 0.2 + u(-0.15, 0.15)) * (1 - t / d))
    if kind == 'hit':
        d = 0.12; return render(d, lambda t: u(-0.4, 0.4) * (1 - t / d) ** 2)
    if kind == 'heal':
        d = 0.3
        return render(d, lambda t: math.sin(t * 660 * TAU) * 0.15 * math.sin(t / d * math.pi) + math.sin(t * 990 * TAU) * 0.1 * math.sin(t / d * math.pi))
    if kind == 'step_stone':
        d = 0.05; return render(d, lambda t: (math.sin(t * 1200 * TAU) * 0.08 + u(-0.12, 0.12)) * (1 - t / d) ** 2)
    if kind == 'shield':
        d = 0.35; return render(d, lambda t: (math.sin(t * 150 * TAU) * 0.2 + math.sin(t * 75 * TAU) * 0.15) * math.sin(t / d * math.pi) * 0.8)
    if kind == 'drain':
        d = 0.35; return render(d, lambda t: math.sin(t * lerp(600, 150, t / d) * TAU) * 0.25 * (t / d))
    if kind == 'phase_change':
        d = 0.6
        return render(d, lambda t: (math.sin(t * 110 * TAU) * 0.2 + math.sin(t * 55 * TAU) * 0.15 + u(-0.05, 0.05)) * math.sin(t / d * math.pi))
    if kind == 'defeat':
        d = 0.5; return render(d, lambda t: math.sin(t * lerp(440, 110, t / d) * TAU) * 0.2 * (1 - t / d))
    if kind == 'enemy_die':
        d = 0.55
        def f(t):
            fr = lerp(300, 70, t / d)
            body = math.sin(t * fr * TAU) * 0.22 + math.sin(t * fr * 1.5 * TAU) * 0.08
            return (body + u(-0.09, 0.09) * (1 - t / d)) * (1 - t / d) ** 1.6
        return render(d, f)
    if kind == 'flee':
        d = 0.2; return render(d, lambda t: math.sin(t * lerp(330, 880, t / d) * TAU) * 0.25 * (1 - t / d))
    if kind == 'memory_add':
        d = 0.4
        return render(d, lambda t: (math.sin(t * 523 * TAU) * 0.12 + math.sin(t * 659 * TAU) * 0.1 + math.sin(t * 784 * TAU) * 0.08) * math.sin(t / d * math.pi) * 0.8)
    if kind == 'ui_hover':
        d = 0.04; return render(d, lambda t: math.sin(t * 1200 * TAU) * 0.12 * (1 - t / d))
    if kind == 'ui_select':
        d = 0.08; return render(d, lambda t: (math.sin(t * 880 * TAU) * 0.2 + math.sin(t * 1320 * TAU) * 0.1) * (1 - t / d) ** 2)
    if kind == 'ui_open':
        d = 0.12; return render(d, lambda t: math.sin(t * lerp(400, 900, t / d) * TAU) * 0.15 * math.sin(t / d * math.pi) * 0.8)
    if kind == 'ui_close':
        d = 0.1; return render(d, lambda t: math.sin(t * lerp(800, 350, t / d) * TAU) * 0.12 * (1 - t / d))
    if kind == 'void_pulse':
        d = 0.5
        return render(d, lambda t: (math.sin(t * 45 * TAU) * 0.3 + math.sin(t * 7 * TAU) * 0.1) * math.sin(t / d * math.pi))
    if kind == 'battle_intro':
        d = 0.6; return render(d, lambda t: (math.sin(t * 80 * TAU) * 0.2 + math.sin(t * 120 * TAU) * 0.15) * math.sin(t / d * math.pi) * 0.7)
    raise KeyError(kind)


def layer(kind, rng):
    """_generate_combat_layer (audio_manager.gd:746)."""
    u = rng.uniform
    out, prev = [], 0.0
    if kind == 'whoosh':
        d = 0.08
        for i in range(int(SR * d)):
            t = i / SR; prev = prev * 0.6 + u(-1, 1) * 0.4; out.append(prev * 0.35 * (1 - t / d) ** 2)
        return out
    if kind == 'thud':
        d = 0.1; return render(d, lambda t: math.sin(t * lerp(180, 60, t / d) * TAU) * 0.4 * (1 - t / d) ** 3)
    if kind == 'metallic_ring':
        return render(0.25, lambda t: (math.sin(t * 2200 * TAU) * 0.12 + math.sin(t * 3300 * TAU) * 0.06) * math.exp(-t * 8))
    if kind == 'crackle':
        d = 0.12
        for i in range(int(SR * d)):
            t = i / SR; burst = u(-0.5, 0.5) if rng.random() < 0.3 else 0.0
            out.append((burst + u(-0.1, 0.1)) * (1 - t / d))
        return out
    if kind == 'deep_whomp':
        d = 0.15; return render(d, lambda t: (math.sin(t * 55 * TAU) * 0.35 + math.sin(t * 30 * TAU) * 0.15) * (1 - t / d) ** 2)
    if kind == 'sizzle_tail':
        d = 0.35
        for i in range(int(SR * d)):
            t = i / SR; prev = prev * 0.7 + u(-1, 1) * 0.3; out.append(prev * 0.15 * (1 - t / d) * 0.8)
        return out
    if kind == 'glass_crack':
        d = 0.18
        for i in range(int(SR * d)):
            t = i / SR; env = (1 - t / d) ** 2
            impact = u(-0.5, 0.5) if t < 0.02 else 0.0
            out.append(impact + math.sin(t * 3000 * TAU) * 0.1 * env + u(-0.2, 0.2) * env * 0.5)
        return out
    if kind == 'chime':
        return render(0.4, lambda t: (math.sin(t * 880 * TAU) * 0.12 + math.sin(t * 1320 * TAU) * 0.08 + math.sin(t * 1760 * TAU) * 0.05) * math.exp(-t * 4))
    if kind == 'warm_pad':
        d = 0.6
        return render(d, lambda t: (math.sin(t * 330 * TAU) * 0.1 + math.sin(t * 440 * TAU) * 0.08 + math.sin(t * 550 * TAU) * 0.05) * math.sin(t / d * math.pi) * 0.7)
    raise KeyError(kind)


# COMBAT_SFX_LAYERS (audio_manager.gd:686): (delay seconds, generator, layer dB)
LAYERED = {
    'sword_slash': [(0.0, 'whoosh', -3.0), (0.03, 'thud', -5.0), (0.05, 'metallic_ring', -8.0)],
    'burn_ignite': [(0.0, 'crackle', -4.0), (0.02, 'deep_whomp', -5.0), (0.1, 'sizzle_tail', -7.0)],
    'shield_break': [(0.0, 'glass_crack', -4.0), (0.02, 'thud', -5.0)],
    'heal_layered': [(0.0, 'chime', -5.0), (0.05, 'warm_pad', -8.0)],
}


def premix(name, rng):
    parts = [(int(round(delay * SR)), layer(gen, rng), 10 ** (db / 20)) for delay, gen, db in LAYERED[name]]
    out = [0.0] * max(offset + len(s) for offset, s, _ in parts)
    for offset, s, gain in parts:
        for i, v in enumerate(s):
            out[offset + i] += v * gain
    return out


def rising_tone(rng):
    """_play_rising_tone (audio_manager.gd:1055)."""
    d = 0.5
    def f(t):
        p = t / d; fr = lerp(80, 600, p * p)
        w = math.sin(t * fr * TAU) * 0.2 + math.sin(t * fr * 2 * TAU) * 0.08 + math.sin(t * fr * 3 * TAU) * 0.04
        return (w + rng.uniform(-0.05, 0.05) * p) * p * p * 0.8
    return render(d, f)


def wind_light(rng):
    """_generate_ambient('wind_light') (audio_manager.gd:457), 3 s loop."""
    out, prev = [], 0.0
    for i in range(int(SR * 3.0)):
        t = i / SR; prev = prev * 0.98 + rng.uniform(-1, 1) * 0.02
        out.append(prev * 0.08 * (math.sin(t * 0.5 * TAU) * 0.2 + 0.8))
    return out


def heartbeat(_rng):
    """_generate_heartbeat (audio_manager.gd:605), 1 s loop."""
    def f(t):
        if t < 0.12: return math.sin(t * 50 * TAU) * 0.35 * (1 - t / 0.12) ** 2
        if 0.25 <= t < 0.35: bt = t - 0.25; return math.sin(bt * 45 * TAU) * 0.2 * (1 - bt / 0.10) ** 2
        return 0.0
    return render(1.0, f)


SIMPLE = ['confirm', 'cancel', 'burn', 'hit', 'heal', 'step_stone', 'shield', 'drain', 'phase_change', 'defeat',
          'enemy_die', 'flee', 'memory_add', 'ui_hover', 'ui_select', 'ui_open', 'ui_close', 'battle_intro',
          'void_pulse']
SPECIAL = {'rising_tone': (rising_tone, False), 'wind_light': (wind_light, True), 'heartbeat': (heartbeat, True)}


def cues():
    for name in SIMPLE: yield name, (lambda rng, n=name: sfx(n, rng)), False
    for name in LAYERED: yield name, (lambda rng, n=name: premix(n, rng)), False
    for name, (fn, loop) in SPECIAL.items(): yield name, fn, loop


def wav_bytes(samples):
    import io
    buf = io.BytesIO()
    with wave.open(buf, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b''.join(struct.pack('<h', int(max(-1.0, min(1.0, s)) * 32767)) for s in samples))
    return buf.getvalue()


def build():
    files, manifest = {}, {'generator': 'generate_audio_sources.py/1', 'source': SOURCE, 'sample_rate': SR, 'cues': {}}
    for name, fn, loop in cues():
        samples = fn(random.Random(zlib.crc32(name.encode())))
        data = wav_bytes(samples)
        files[name + '.wav'] = data
        manifest['cues'][name] = {'file': name + '.wav', 'samples': len(samples), 'duration': round(len(samples) / SR, 6),
                                  'loop': loop, 'sha256': hashlib.sha256(data).hexdigest()}
    files['manifest.json'] = (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode()
    return files


def main():
    p = argparse.ArgumentParser(); p.add_argument('--check', action='store_true'); a = p.parse_args()
    files = build()
    if a.check:
        bad = [n for n, d in files.items() if not (OUT / n).is_file() or (OUT / n).read_bytes() != d]
        stale = sorted(f.name for f in OUT.glob('*') if f.name not in files) if OUT.is_dir() else []
        if bad or stale: raise SystemExit('Audio sources differ: ' + ', '.join(bad + stale))
        print('MEMORIA_AUDIO_SOURCES_CHECK_PASS cues=%d' % (len(files) - 1)); return
    OUT.mkdir(parents=True, exist_ok=True)
    for n, d in files.items(): (OUT / n).write_bytes(d)
    print('MEMORIA_AUDIO_SOURCES_WRITTEN cues=%d' % (len(files) - 1))


if __name__ == '__main__':
    main()
