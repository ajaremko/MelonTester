# TODO

Work that is known and not done.

- **Playback, Slicing, Editor**: features that are stubbed or not built yet. The order within each section is a suggestion: earlier items make later ones easier. When a stubbed feature is finished, tick it and move it from "Not implemented yet" to "Working now" in the Status section of [README.md](README.md).
- **Banks**: a second bank of 16 pads.
- **Controller contract**: what the Maschine MK2 script for FL Studio needs from the plugin.
- **Rough edges**: problems in the parts that already work.
- **Checks not done yet**: things that have not been verified.
- **Before first release** and **Later**.

Several items depend on how the controller reaches the plugin. The MK2 script reads and writes host parameters, which it finds by their position in the parameter list. Only the pad notes arrive as MIDI. So a new parameter always goes after the last pad parameter, where it can't move an existing one.

The order to work in is in [PLAN.md](PLAN.md), which puts every item here into a phase. An item that depends on another says so on a "Needs" line.

## Playback

- [ ] **A tail voice per pad**
  - Done when: hitting a pad that is already sounding, or cutting a pad, lets the old sound fade out while the new hit plays. Nothing stops dead.
  - Where: `SamplerEngine` has one `PadVoice` per pad, in `voices`, and `SamplerEngine::triggerPad` restarts it in place. So a re-hit cuts the old sound off at once, which clicks, and a pad that was cut can't play its release tail and be hit again at the same time.
  - Suggested: a second array of 16 tail voices. On a re-hit or a cut, the playing voice is copied into its pad's tail slot and faded out there, leaving the main voice free. `isPadPlaying` keeps reporting the main voice.
  - Test: in `tests/SamplerEngineTests.cpp`, re-hit a pad partway through its region. The samples just after the re-hit still contain the old sound fading out.

- [ ] **Edge fades, attack and release**
  - Done when: every pad has a short fixed fade (1–2 ms) at both ends of its region, so slices never click. Attack sets the fade-in when it is longer than that. Release is used only when a pad is let go in Gate mode or cut by a choke group or Mono mode; a One-shot pad plays at full level to the end of its region.
  - Where: `PadVoice` in [source/engine/](source/engine/) has no envelope. `SamplerEngine::triggerPad` needs to pass it the pad's `attack` and `release`, which are in milliseconds.
  - Needs: A tail voice per pad.
  - Test: in `tests/SamplerEngineTests.cpp`, the output rises from silence at the start of a region and reaches silence at its end. The existing exact-region test will need to allow for the edge fades.

- [ ] **Gate mode**
  - Done when: with Play mode set to Gate, the pad plays only while held and fades out over its Release time on note-off. One-shot pads keep ignoring note-off.
  - Where: `SamplerEngine::releasePad` is empty. Note-offs already reach it, both from MIDI and from clicking pads in the editor.
  - Needs: A tail voice per pad.
  - Test: note-on, then note-off in a later block. A Gate pad goes silent; a One-shot pad plays on.

- [ ] **Choke groups**
  - Done when: hitting a pad stops every other playing pad in the same choke group (1–8). Pads with the group set to Off are not affected. A cut pad fades out over its Release time.
  - Where: `SamplerEngine::triggerPad`.
  - Needs: A tail voice per pad.
  - Test: two pads in group 1 and one set to Off. Hitting the second pad cuts the first and leaves the third playing.

- [ ] **Polyphony mode**
  - Done when: in Mono, hitting any pad stops all other pads, each fading out over its Release time. In Poly, only choke groups cut pads.
  - Where: `SamplerEngine::triggerPad`, reading `params.polyMode` (0 = Poly, 1 = Mono).
  - Needs: A tail voice per pad.
  - Test: with Mono set, a second pad hit leaves only that pad playing.

- [ ] **Pitch**
  - Done when: the pad's Pitch (−24 to +24 semitones) changes playback speed, so +12 plays the region an octave up in half the time. Pitch is read when the pad is hit: turning it changes the next hit, not a pad that is already playing.
  - Where: the increment passed to `PadVoice::start` in `SamplerEngine::triggerPad`; multiply it by 2^(semitones / 12).
  - Test: at +12, a 1000-sample region finishes after 500 output samples.

- [ ] **Reverse**
  - Done when: with Reverse on, the pad plays its region from end to start.
  - Where: `PadVoice::start` and `PadVoice::render` only move forwards, with a positive increment that stops at `endPosition`.
  - Test: with `makeRampSample` from `tests/helpers/sampler_helpers.h`, the output falls instead of rising.

- [ ] **Loop**
  - Done when: with Loop on and Play mode set to Gate, the region repeats for as long as the pad is held, then fades out over its Release time. When reversed it repeats backwards. One-shot pads ignore Loop, and the editor shows the Loop control as inactive for them.
  - Where: `PadVoice::render`, where it currently stops at the end of the region. The Loop control is in `PadParamsPanel`.
  - Needs: Gate mode.
  - Suggested: apply the fixed edge fades at each repeat, so the jump back to the start doesn't click.
  - Test: a held Gate pad's region appears again in the output after its length has passed. A One-shot pad with Loop on stops at the end of its region.

- [ ] **Full Level switch**
  - Done when: a global Full Level switch makes every hit play at the pad's full volume. With it off, velocity scales the volume, as it does now.
  - Where: the Full Level parameter, which "Reserve the new parameters' positions" adds. Velocity is applied in `SamplerEngine::triggerPad`. Add a control to `GlobalPanel`, beside Zero Snap's, and a `fullLevel` target to the MIDI map for controllers that send CCs.
  - Needs: Reserve the new parameters' positions.
  - Test: with Full Level on, a soft hit and a hard hit render the same output.

## Slicing

- [ ] **Transient detection**
  - Done when: "Detect transients" places a slice at each detected hit. Sensitivity sets how strong a hit must be. If more than 16 are found, the 16 strongest are kept. Pads are assigned in time order, and pads without a slice are left blank. Each slice ends where the next kept slice starts, and the last ends at the end of the sample.
  - With Bank B: the limit becomes the 32 strongest hits, spread across both banks.
  - Where: `Slicer::detectTransients` in [source/slicing/Slicer.cpp](source/slicing/Slicer.cpp) returns nothing. `EditController::detectTransients` already calls it and assigns the result, including blanking unused pads. Re-enable the button in [source/ui/GlobalPanel.cpp](source/ui/GlobalPanel.cpp).
  - Watch for: detection runs on the message thread, so a long sample may freeze the window. It may need a background thread.
  - Test: in `tests/SlicerTests.cpp`, a buffer with clicks at known positions gives slices at those positions. Add cases for more than 16 hits and fewer than 16.

- [ ] **Snap to zero crossings**
  - Done when: there is a global Zero Snap switch, on by default. While it is on, a coarse move of a start or end lands on the nearest zero crossing; that covers the coarse nudge parameters, MIDI encoders and mouse drags. Fine moves never snap, so a snapped edge can still be placed by hand. A value written straight to a Start or End parameter is used as given.
  - Where: the Zero Snap parameter, which "Reserve the new parameters' positions" adds, with a control in `GlobalPanel`. `GlobalPanel::resized` has no room left, since the Sensitivity knob takes what remains, so the panel's layout needs rework to fit this switch and Full Level's. Finding the nearest crossing fits in `Slicer` as a plain function, and `EditController::adjustRegionEdge` is where it gets applied.
  - Needs: Reserve the new parameters' positions.
  - Suggested: the results of Divide equally and Detect transients snap too.
  - Test: in `tests/SlicerTests.cpp`, the nearest crossing is found in a sine wave. In `tests/EditControllerTests.cpp`, a coarse step lands on a crossing and a fine step does not.

## Editor

- [ ] **Pad colours in the editor**
  - Done when: every pad's colour can be chosen in the pad settings panel, and the pad grid draws each pad in its colour, using the palette in [pad-colors.md](pad-colors.md). A pad without a slice is drawn dark whatever its colour.
  - Where: drawing is in `PadGrid::paint`, and the control to choose a colour goes in `PadParamsPanel`. `PadGrid::getDrawState` packs what it draws into 64 bits to spot changes, and 16 colours won't fit, so that needs replacing. Keep the palette in one shared header, since the grid, the waveform and the colour control all need it.
  - Needs: Colour parameter matches pad-colors.md.
  - Test: visual; check it in the Standalone build.

- [ ] **Show every pad's region on the waveform**
  - Done when: every pad that has a slice has its region drawn faintly in its pad colour, with the selected pad's region brighter and clearly edged, so overlaps can be seen.
  - Where: `WaveformPanel::paintRegions` draws the selected region and only a thin line where each other pad starts. `WaveformPanel::getDrawState` watches the selected pad alone, so the panel would not repaint when another pad's region or colour changes.
  - Needs: Pad colours in the editor.
  - Test: visual; check it in the Standalone build.

- [ ] **Dragging region edges with the mouse**
  - Done when: the selected pad's start and end can be dragged on the waveform. The dragged edge stops a minimum length short of the other edge, and lands on zero crossings while Zero Snap is on.
  - Where: `WaveformPanel` has no mouse handling. Add a function to `EditController` that sets an edge to a position, next to `adjustRegionEdge`, and call it from the panel so mouse edits take the same path as other edits.
  - Test: in `tests/EditControllerTests.cpp`, setting an edge moves the selected pad only and stops short of the other edge.

- [ ] **Show start and end as time**
  - Done when: Start and End read as a time into the sample, such as 1:23.456, in the plugin window and in the parameter text that the MK2 script shows on the controller. This covers the Selected parameters and each pad's own.
  - Where: their text comes from `withDecimals (4)` in [source/params/ParameterLayout.cpp](source/params/ParameterLayout.cpp), which only sees the 0–1 value. Turning that into a time needs the loaded sample's length, which `SampleStore` holds. But `createParameterLayout` takes no arguments and runs before `SampleStore` exists, because `apvts` is the first member of `PluginProcessor`, and a host can ask for the text from any thread. So the text function needs a small thread-safe length value that is created before the parameters and updated when a sample loads. Typing a time into a knob's text box should work too.
  - Watch for: when a new sample loads, the same 0–1 value means a different time. The host has to be told the text changed (JUCE's `updateHostDisplay`), or the controller keeps showing old times.
  - Suggested: with no sample loaded, show a dash.
  - Test: in `tests/ParameterLayoutTests.cpp`, a known position in a sample of known length gives the expected text.

- [ ] **A status line for messages**
  - Done when: the editor has one place that shows short messages, such as a file that failed to load, a load in progress, or an invalid MIDI map.
  - Where: there is none. The only message today is the missing-sample text drawn in `WaveformPanel::paint`, and `PluginEditor::resized` gives all the space to the four panels.
  - Needed by: Tell the user when a sample fails to load; Load samples off the message thread; Report an invalid MIDI map.

## Banks

- [ ] **Bank B: 16 more pads**
  - Done when: there are 32 pads in two banks of 16. The plugin window shows one bank at a time, with an A/B switch. Bank B's pads play from notes 52–67, so both banks are always playable. The Bank parameter says which bank is showing, and the script can switch it. Divide equally and Detect transients fill both banks: 32 equal slices, or the 32 strongest hits.
  - Where: `kNumPads` is 16 and stands for both "pads in a bank" and "all pads" in `ParamRefs`, `SelectedPadMirror`, the voices in `SamplerEngine`, `PadGrid`, `EditController::assignSlices` and `MidiMap`, whose `padNotes` must be exactly 16 notes. Each of those needs to know which of the two it means. Bank B's parameters already exist at 216–407 (see "Reserve the new parameters' positions"), so a pad's parameters are no longer found by one formula: bank A's start at 17 and bank B's at 216.
  - Suggested: Selected Pad stays 1–16 and means the pad within the bank that is showing, so that existing parameter doesn't change. Hitting a pad in the other bank switches Bank, and the window follows. The waveform shows the regions of the bank that is showing.
  - Watch for: Divide equally changes from 16 slices to 32. The README says 16 pads and 16 slices in several places, and [pad-colors.md](pad-colors.md) gives positions for bank A's colours only.
  - Needs: Reserve the new parameters' positions.
  - Test: a note from 52–67 plays the right bank B pad from its own region. Switching Bank changes which pad the Selected parameters describe. Divide equally gives 32 regions with no gaps.

## Controller contract

What the Maschine MK2 script for FL Studio needs from the plugin. The script already exists and is a separate project.

- [ ] **Pin the parameter positions in a test**
  - Done when: a test fails if any existing parameter moves, so a change that would break the MK2 script is caught at once. Do this before adding any parameter.
  - Where: `tests/ParameterLayoutTests.cpp`. The order today, from `createParameterLayout`: the 5 global parameters at 0–4 (Polyphony, Zoom, View Position, Transient Sensitivity, Selected Pad), the 12 Selected parameters at 5–16, then 12 for each pad starting at 17. Within a set the order is Start, End, Pitch, Gain, Attack, Release, Play Mode, Reverse, Loop, Choke Group, Colour, Assigned. So Selected Start is 5, Selected Colour is 15, and Pad N's set starts at 17 + 12 × (N − 1). This matches the positions in [pad-colors.md](pad-colors.md).
  - Note: a per-pad set can't grow, because 12 per pad is part of the positions. A new per-pad setting would be a separate block at the end, and `allPadParams`, `PadParams` and `SelectedPadMirror` all assume one run of 12.

- [ ] **Reserve the new parameters' positions**
  - Done when: every planned parameter exists at its final position, doing nothing until its feature is built: Full Level at 209, Zero Snap at 210, Start Coarse at 211, Start Fine at 212, End Coarse at 213, End Fine at 214, Bank at 215, and bank B's pads at 216–407, 12 each in the usual order, so pad B N's set starts at 216 + 12 × (N − 1). The positions test covers all of them.
  - Why: the MK2 script finds parameters by position. Fixing every position before any feature uses them means the script never has to be renumbered.
  - Where: new groups added last in `createParameterLayout`. The Global group is first in the list, so it can't take new parameters. `kNumPads` is 16 and stands for both "pads in a bank" and "all pads"; bank B's parameters are the first thing that needs those to be two numbers, though nothing has to read them yet.
  - Needs: Pin the parameter positions in a test.
  - Test: extend the positions test in `tests/ParameterLayoutTests.cpp` to all 408 parameters.

- [ ] **Colour parameter matches pad-colors.md**
  - Done when: each colour parameter has 16 steps, 0–15, with value = index / 15, as [pad-colors.md](pad-colors.md) specifies. All pads start at colour 0. The parameters keep their positions.
  - Why: pad-colors.md describes the MK2 script reading these parameters to light its pads, and writing Selected Colour to recolour the selected pad.
  - Where: `makePadParam` in [source/params/ParameterLayout.cpp](source/params/ParameterLayout.cpp) creates the `colour` parameter with 17 values (0 for none, then 1–16).
  - Note: pad-colors.md suggests pad N starts at colour N − 1. That was not chosen; every pad starts at the same colour.
  - Test: in `tests/ParameterLayoutTests.cpp`, each colour parameter has 16 steps and index i reads back from the value i / 15.

- [ ] **One place that reacts to parameter writes**
  - Done when: `EditController::processPending` notices which parameters have changed since it last ran, whoever changed them, and applies the rules in a fixed order: mirror sync, then region limits, then nudges. Values restored from a saved project are taken as they are, not treated as edits.
  - Why: writing Zoom, View, Start and End from the script already works, through `SelectedPadMirror`. What is missing is rules for those writes. The rules live in `EditController::apply` and the functions it calls, such as the limit in `adjustRegionEdge`, and only edits that arrive as MIDI CCs get there. The MK2 script, the on-screen knobs (`ParamKnob` in [source/ui/ParamControls.h](source/ui/ParamControls.h)) and host automation write parameters directly. `SelectedPadMirror::sync` is the only code that reacts, and it keeps a record of the selected pad's values alone.
  - Where: `EditController::processPending`, with `SelectedPadMirror::sync` becoming the first step of the pass. Comparing every parameter value 60 times a second costs next to nothing.
  - Test: in `tests/EditControllerTests.cpp`, the existing mirror tests still pass, and restoring a saved state changes nothing beyond what was saved.
  - Needed by: Keep start before end on every path; Nudge parameters for coarse and fine start and end.

- [ ] **Nudge parameters for coarse and fine start and end**
  - Done when: there are four parameters: Start Coarse, Start Fine, End Coarse and End Fine. When the script adds its step to one of them, the selected pad's edge moves by 1/512 of the visible waveform per step for coarse, or 1/5120 for fine. Coarse moves follow Zero Snap; fine moves do not. The minimum region length applies. The plugin then returns the parameter to its centre, ready for the next step. The script chooses which of the two a knob drives.
  - Where: the four nudge parameters at 211–214, which "Reserve the new parameters' positions" adds. They are watched in the pass from "One place that reacts to parameter writes", which calls `EditController::adjustRegionEdge`; that function needs a fine variant. Don't push onto `editQueue` from a parameter callback instead: that queue allows only one writing thread, and the audio thread is it. Add `startFine` and `endFine` targets to the MIDI map so controllers that send CCs get the same thing.
  - Find out first: the fixed amount the MK2 script adds to a parameter per knob click. It adds the same step to every parameter, and the plugin has to treat that amount as one step.
  - Suggested: work from the change since the value last seen, and return the parameter to centre only once the knob has been still for a moment. Then the script never reads a value the plugin is about to change mid-turn, which could lose or double steps. Check fast turning on the hardware.
  - Watch for: a nudge value restored from a saved project must not be treated as steps.
  - Needs: Reserve the new parameters' positions; One place that reacts to parameter writes.
  - Test: in `tests/EditControllerTests.cpp`, moving Start Coarse one step above centre moves the selected pad's start by 1/512 of the view, and the parameter is back at centre afterwards. The same for Start Fine at 1/5120.

- [ ] **Keep plugin-made parameter changes apart from user edits**
  - Done when: changes the plugin makes by itself are not recorded as automation in FL Studio and don't count as the last-tweaked control, while the MK2 script still sees the new values. These changes are the Selected parameters updating when a pad is hit, a nudge parameter returning to centre, a start or end being limited, and the view scrolling to the selected pad.
  - Where: `setParameterNormalised` in [source/params/ParamUtils.h](source/params/ParamUtils.h) is used for every change and reports each one to the host as a user gesture. Hitting a pad changes all 12 Selected parameters that way.
  - Find out first: what FL Studio does with these today. Hit pads with automation recording on and see whether the Selected parameters get recorded. If they don't, tick this item with no work.
  - If work is needed: JUCE's `updateHostDisplay` tells a host that parameter values changed without reporting an edit. Whether FL Studio and the script pick the new values up that way needs trying.
  - Watch for: automation recorded on a Selected parameter plays back onto whichever pad is selected at that moment. Hiding them from automation may not be possible, since pad-colors.md describes the script as reading a plugin's automatable parameters.

## Rough edges

Problems in the parts that already work, with the ones most likely to bite first.

- [ ] **Keep start before end on every path**
  - Done when: a pad's start can never end up after its end, or closer to it than a minimum length (suggested 5 ms), however it is changed. The edge being moved stops; the other edge stays where it is.
  - Where: only the MIDI encoders are limited, in `EditController::adjustRegionEdge`, and they allow a region of zero length. The MK2 script writes Selected Start and Selected End directly, as do the on-screen knobs in `WaveformPanel` and host automation, and none of those are limited. The script adds a fixed step and does not limit the result itself. A pad with its start after its end is silent.
  - Watch for: the correction lands up to one timer tick after the write, so the audio thread can see the uncorrected values for a moment. `SamplerEngine::triggerPad` already refuses a region whose end is not after its start.
  - Needs: One place that reacts to parameter writes.
  - Test: in `tests/EditControllerTests.cpp`, set the Selected Start parameter above the end and call `processPending()`. The pad's start is the minimum length before its end.

- [ ] **Locate button for a missing sample**
  - Done when: while a project's sample is missing, a Locate sample button is shown. The file chosen with it is loaded with every pad's region as it was saved. The normal Load always re-divides the pads, including while the sample is missing.
  - Where: every load from the editor goes through `EditController::loadSample`, which calls `divideEqually()` and overwrites all 16 regions. It needs a second function that loads without dividing. `SampleStore::isMissing()` says when to show the button, and the missing-sample message is drawn in `WaveformPanel::paint`.
  - Test: restore a state whose sample is missing, locate the file, and check the regions are unchanged.

- [ ] **Load samples off the message thread**
  - Done when: loading a long file doesn't freeze the window, and the editor shows that a load is in progress.
  - Needs: A status line for messages.
  - Where: `SampleStore::loadFile` reads the whole file before returning. It is called from the editor through `EditController::loadSample`, and from `PluginProcessor::setStateInformation`.
  - Watch for: `EditController::loadSample` divides the pads straight after loading, and the tests in `tests/EditControllerTests.cpp` expect the sample to be there when it returns. Both will need to wait for the load to finish.

- [ ] **Make saving and restoring safe from other threads**
  - Done when: `getStateInformation` and `setStateInformation` are safe when a host calls them from a thread other than the message thread, which some hosts do.
  - Where: in `PluginProcessor`, `setStateInformation` calls `SampleStore::loadFile` and `getStateInformation` calls `SampleStore::getFile`. Apart from `getSampleForAudio`, `SampleStore` is for the message thread only.
  - Goes with: the previous item, since a background load needs the same care.

- [ ] **Tell the user when a sample fails to load**
  - Done when: choosing or dropping a file that can't be read shows a message saying so.
  - Where: `GlobalPanel::chooseSample` and `WaveformPanel::filesDropped` ignore the result of `EditController::loadSample`, so nothing visible happens.
  - Needs: A status line for messages.

- [ ] **Waveform redraw cost on long samples**
  - Done when: zooming and scrolling stay smooth with a sample several minutes long.
  - Where: `WaveformPanel::rebuildPeaks` scans every visible sample each time the view changes.
  - Measure first: this has not been timed. If it is slow, compute a reduced peak table once per sample, or use `juce::AudioThumbnail`.

- [ ] **Tests use the machine's MIDI map**
  - Done when: the tests behave the same whether or not the developer has their own `midi_map.json`.
  - Where: `PluginProcessor`'s constructor calls `MidiMap::load()`, which prefers the user's file. The helpers in `tests/helpers/sampler_helpers.h` look notes and CCs up in whatever map was loaded, and return -1 when a control isn't mapped.

- [ ] **Report an invalid MIDI map**
  - Done when: if the user's `midi_map.json` can't be used, the editor says so and says the built-in map is in use. Better still if it says what is wrong with the file.
  - Where: `MidiMap::load` falls back to the built-in map silently, and `MidiMap::fromJson` gives no reason when it rejects a file.
  - Priority: low. The MK2 script only uses the map for its pad notes.
  - Needs: A status line for messages.
  - Test: in `tests/MidiMapTests.cpp`, each kind of invalid map gives a reason.

- [ ] **Reload the MIDI map without restarting the plugin**
  - Done when: an edited `midi_map.json` can be picked up from the editor, without removing and re-adding the plugin.
  - Where: the map is read once, when `PluginProcessor`'s constructor builds the `MidiRouter`. The audio thread reads the map on every MIDI message, so replacing it needs a hand-off like the one `SampleStore` uses for samples.
  - Priority: low, for the same reason.

## Checks not done yet

- [ ] **Check the CI run on all three platforms**
  - Done when: the latest run of `.github/workflows/build_and_test.yml` passes on Linux, macOS and Windows with no compiler warnings.
  - Why: the code has only been built with MSVC, and Clang on macOS and Linux reports warnings MSVC doesn't. The workflow's IPP and code-signing steps were also commented out during setup, which is untested until a run passes.

- [ ] **Listen to it, then try it in FL Studio**
  - Done when: the Standalone build has been run with a real sample and the pads played while listening. Then, in FL Studio: a saved project reopens with its sample and chops, a file dragged from FL's browser onto the waveform loads, and FL lists the parameters in the order given under Controller contract. Anything wrong gets added to this file.
  - Why: playback has only been checked by tests that compare rendered samples, and FL Studio is the host the plugin is meant for.

- [ ] **Check the MK2 script against the plugin**
  - Done when: with the Maschine MK2 and its FL Studio script, the 16 pads play and select the right pads (notes 36–51, pad 1 lowest); the eight knobs move Zoom, View, Start, End, Pitch, Gain, Attack and Release; the script's modifier layer reaches Choke group and Sensitivity; and the display shows the Selected parameters' text. Once Pad colours is done, the pads light as [pad-colors.md](pad-colors.md) describes.
  - Why: the script finds parameters by position, so running the two together is the only real test of the contract.

## Before first release

- [ ] **Choose a licence**
  - Done when: the `LICENSE` file says what MelonTester is released under, and the choice fits JUCE's terms.
  - Why: `LICENSE` is still the template's (MIT, copyright Sudara Williams). JUCE is licensed under AGPLv3 or a commercial JUCE licence; see `JUCE/LICENSE.md`.

- [ ] **Set up code signing**
  - Done when: the CI build signs and notarizes the macOS plugins and installer, and signs the Windows installer.
  - Why: without it, macOS Gatekeeper and Windows SmartScreen block or warn on the plugin. The signing steps in `.github/workflows/build_and_test.yml` were commented out during setup, and `CLAUDE.md` links to the guides. macOS needs an Apple Developer account; Windows uses Azure Trusted Signing.

- [ ] **Version the saved state**
  - Done when: the saved state carries a version number, and `PluginProcessor::setStateInformation` reads it.
  - Why: `PluginProcessor::getStateInformation` saves the parameter tree and the sample path with nothing to say which version wrote them. Pad colours changes what a stored colour value means, and later changes can't be handled without knowing what an old project contains.

## Later

Kept on the list, not planned in detail.

- [ ] **Time-stretching**: change a slice's pitch and length independently.
