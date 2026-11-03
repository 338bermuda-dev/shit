# Trap Snare

A fully bussed-down drum synth for trap snares and claps (VST3, plus AU on Mac). It
synthesises every hit from scratch, with no samples, so each sound can be tuned exactly.

## Sound
| Section | Controls |
|---|---|
| **Body** | Tune (Hz), Drop (pitch-drop size), Drop Time, Decay, Level. The pitched "thump" of the snare. |
| **Noise** | Tone, Decay, Snap (transient click), Level. The crisp rattle and crack. |
| **Clap** | Level, Tone, Spread (gap between claps), Decay. A layered three-burst clap. |
| **FX** | Drive (tanh saturation), Reverb, Room. |
| **Roll** | Roll mode on/off, Rate (1/8 to 1/64, with triplets, synced to tempo), Ramp (crescendo/fade), Pitch (semitones per hit, for risers). |
| **Master** | Variation (each hit slightly different, so repeats don't sound robotic), Velocity sensitivity, Level, Key Track. |

- **Rolls**: turn on Roll and hold a key. The snare repeats at the chosen rate, synced to your
  tempo. Set Pitch to +1 st for a classic rising roll, or Ramp to -100 % for a fade-out.
- **Key Track**: play the snare up and down the keyboard (C3 is the original pitch).
- **Presets**: Trap Crack, Atlanta Clap, Tight Rim, Big Room, Pitched Down, Lo-Fi Crunch, Roll Ready.
- **RANDOMIZE** rolls a new snare within musical ranges. **HIT IT** auditions the current sound.
- An output limiter keeps it from clipping.

## Install
- **Windows**: copy `Trap Snare.vst3` to `C:\Program Files\Common Files\VST3`
- **macOS**: copy `Trap Snare.vst3` to `~/Library/Audio/Plug-Ins/VST3` and
  `Trap Snare.component` to `~/Library/Audio/Plug-Ins/Components`
- **Linux**: copy `Trap Snare.vst3` to `~/.vst3`

In Ableton, rescan plug-ins, then drag Trap Snare onto a MIDI track. It's an instrument, so it
works in every Live edition.

## Building
You need CMake 3.22+ and a C++17 compiler. JUCE is downloaded automatically.
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
**No compiler?** Push this folder to a GitHub repo. The included workflow builds Windows,
macOS and Linux versions; download them from the **Actions** tab.

To run the tests: build with `-DTS_BUILD_TESTS=ON` and run `RenderTest`. It renders every
preset to `TrapSnare-demo.wav`, checks levels, roll timing and key tracking, and saves a
screenshot of the UI.
