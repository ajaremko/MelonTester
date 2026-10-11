# MelonTester

MelonTester is a 16-pad sampler plugin for chopping a long sample across the pads, MPC style, without leaving your pad controller. Load one sample, hit a pad, and turn a knob to move where that pad starts and ends.

Most samplers let you trigger slices from hardware but make you reach for the mouse to decide what each pad actually plays. MelonTester exposes each pad's playback region to MIDI, so the whole chopping workflow can be done from the controller. It is designed around a Native Instruments Maschine in MIDI mode, and should work with any controller that has 16 pads and endless encoders.

## Status

Early development. **Nothing described below is implemented yet.** The plugin currently builds as an empty instrument; this document describes the intended design.

## How it works

1. Load one long audio file: a break, a loop, a whole track.
2. Each of the 16 pads plays its own region of that file.
3. Hit a pad. It plays, and it becomes the pad you are editing.
4. Turn the start and end encoders to move that pad's region. Hit the pad again to hear the result.
5. Repeat across the pads until the sample is chopped the way you want it.

To get a starting point quickly, the sample can be split into 16 equal slices or sliced at detected transients, then adjusted pad by pad.

## Planned features

### Chopping

- **Manual start and end per pad**, anywhere in the sample. Regions can overlap.
- **Equal divisions**: split the sample into 16 equal slices in one action.
- **Transient detection**: place slices on detected hits, with a sensitivity control.

### Per-pad playback

- **One-shot or gate**: play the whole slice, or only while the pad is held.
- **Choke or poly**: a new pad cuts the previous one, or pads ring over each other.
- **Pitch** and **gain**.
- **Attack and release** to remove clicks at slice edges.
- **Reverse** and **loop**.

### Editor

- Waveform view showing all 16 regions, plus a pad grid.
- Everything that can be done from the controller can also be done with the mouse.
- Load a sample by drag and drop or from a file dialog.

## MIDI control

Pads are played with MIDI notes. Editing uses relative (endless encoder) CCs that always act on the **last pad you hit**, so one small set of controls covers all 16 pads.

| Control | Type | Acts on |
|---|---|---|
| Pads 1–16 | 16 notes | Plays the pad and selects it for editing |
| Slice start | Relative CC | Selected pad |
| Slice end | Relative CC | Selected pad |
| Pitch | Relative CC | Selected pad |
| Gain | Relative CC | Selected pad |
| Attack / release | Relative CC | Selected pad |
| One-shot / gate | Button | Selected pad |
| Reverse | Button | Selected pad |
| Loop | Button | Selected pad |
| Choke / poly | Button | All pads |
| Divide equally | Button | Whole sample |
| Detect transients | Button | Whole sample |
| Transient sensitivity | Relative CC | Whole sample |

Note and CC numbers have not been assigned yet.

## Controller feedback

The plugin publishes its state as host parameters so it can be shown outside the plugin window: which pad is selected, that pad's start and end positions, and its playback settings. When you hit a different pad, those parameters update to describe the new pad.

## Using it with Maschine

MelonTester is intended to be used with a Maschine controller in **MIDI mode**, with the plugin loaded in any DAW. A Controller Editor template will be provided with the project, mapping the pads to notes and the eight knobs to relative encoders for the controls above.

## Open questions

- **Display feedback in MIDI mode.** In MIDI mode the Maschine displays show values from the template's own CCs, not host parameters. Keeping the knob readouts in sync with the selected pad may also need the plugin to send CC values over MIDI out. To be tested on hardware.
- **Encoder behaviour.** Which relative CC encoding to support, and how coarse and fine adjustment should work when a sample is minutes long.
- **Snap to zero crossings** when moving slice points.
- **Pad banks**, for more than 16 slices of one sample.
- **Time-stretching**, so pitch and length can be changed independently.
- **Session storage.** Whether a saved project stores a path to the sample or embeds the audio.

## Building

Requires CMake 3.25 or newer and a C++23 compiler. Ninja is recommended.

```bash
git clone --recurse-submodules <repo-url>
cd MelonTester

cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds --config Debug
ctest --test-dir Builds --verbose --output-on-failure
```

On Windows, run these from a Visual Studio developer shell so the MSVC toolchain is on the path.

The build produces VST3, CLAP and Standalone versions, plus AU and AUv3 on macOS. Use `-DCMAKE_BUILD_TYPE=Release` and `--config Release` for a build to make music with.

## Credits

Built with [JUCE](https://juce.com/) on the [Pamplejuce](https://github.com/sudara/pamplejuce) template.
