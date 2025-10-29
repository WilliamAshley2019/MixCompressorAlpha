Mix Compressor Alpha![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![JUCE](https://img.shields.io/badge/JUCE-8-blue)
![VST3](https://img.shields.io/badge/VST3-Steinberg-red)

A JUCE-based audio plugin inspired by Mike Senior's compression techniques from his seminal article "Compression Made Easy".
This is a very introductory alpha plugin that has a specific design goal in purpose. Likely needs a bit of work. (a lot of work  :)


This VST3 plugin implements a dual-stage compressor designed for professional mixing workflows, emphasizing dynamic stability, transient shaping, and level-matched comparisons.
It serves as both a practical tool and an educational reference for engineers applying compression thoughtfully in the mix.Note: VST is a trademark of Steinberg Media Technologies GmbH.
This plugin is built using the Steinberg VST 3 SDK and JUCE framework, distributed under their respective licenses (see below for details).

Core PurposeThe primary goal of Mix Compressor Alpha is to control dynamic range so tracks sit consistently in a mix without constant fader adjustments. 
If you can't set a single static fader level that works across an entire song for an instrument, compression (or automation) is essential.Technical Effect: 
The compressor applies gain reduction to signal levels exceeding a user-defined threshold, at a rate set by the ratio, over time frames governed by attack and release parameters.
This plugin embeds Mike Senior's philosophy: Compression is for stability, not loudness. It includes smart presets, visual metering, and architectural choices that teach and apply these principles directly in your DAW.
FeaturesFundamental ControlsAll core parameters are exposed via intuitive rotary knobs in the UI, with defaults tuned for practical mixing:
Control
Function
Practical Focus & Default
Threshold
Level above which compression begins
Lower = more compression; Default: -24 dB
Ratio
Slope of gain reduction above threshold
1.5:1–∞:1; Low for subtle evening (Default: 2.5:1)
Attack
How fast the compressor reacts to transients
Fast (<5 ms) suppresses punch; Slow (>30 ms) preserves it (Default: 20 ms)
Release
How fast the compressor recovers
Short = density (risk of pumping); Long = transparency (Default: 200 ms)
Make-up Gain
Compensates for level lost due to gain reduction
Auto-enabled for level-matched A/B tests (Default: Auto)

Compressor ArchitecturesThree topologies inspired by classics, selectable via a mode switch:Threshold-based (e.g., SSL/Renaissance Comp): Pull threshold down for more engagement.
Input-drive (e.g., 1176): Fixed threshold; drive input for depth (beware psychological loudness bias—always level-match!).
One-knob Peak Reduction (e.g., LA-2A): Internally coupled threshold/ratio for simplicity.

Each mode alters detector curves, knee softness, and time constants for tonal character:Optical (LA-2A/CL1B): Slow, smooth—ideal for vocals/bass.
FET (1176/Distressor): Fast, aggressive—punchy transients.
VCA (SSL/dbx 160): Clean, precise—bus/drums.
Vari-Mu (Fairchild/Manley): Warm, program-dependent—mix bus glue.

Dual-Stage CompressionBuilt-in serial processing for advanced workflows:Stage 1 (Leveler): Low ratio (e.g., 2:1) for smooth leveling.
Stage 2 (Peak Catcher): High ratio (e.g., 8:1+) for spike control.
Toggle stages independently or chain them for analog-console-like consistency and punch.

Additional DSP & Workflow ToolsSoft Knee: Adjustable for transparent vs. aggressive response.
Sidechain HPF: 80–120 Hz filter to avoid low-end pumping (e.g., on bass).
Parallel Mix: Wet/dry blend for "New York" compression effects.
Auto-Makeup Gain: Computes RMS differences for automatic level compensation—critical for unbiased A/B testing.
Tempo-Synced Release: Quantize to beat subdivisions (¼, ⅛ notes) for groove-aligned recovery.
Gain Reduction Metering: Real-time visualization that "breathes" with the music; color-coded (blue=gentle, orange=medium, red=heavy) to spot pumping vs. rhythmic interaction.

Smart Presets (Mike Senior Templates)

Loadable presets encapsulate application strategies—each teaches a mixing concept:Preset
Ratio
Attack (ms)
Release (ms)
Topology
Use Case
Vocal Leveler
2–3:1
10–20
100–250
Opto
Gentle smoothing + automation for syllabic consistency (3–6 dB GR target)
Drum Punch
4–6:1
20–40
~100
FET
Enhances crack; medium attack for transient snap
Bass Control
4:1
~5
~200
VCA
Tightens sustain; sidechain HPF at 100 Hz
Mix Bus Glue
2:1
30
200–400
VCA
Cohesion/loudness (1–2 dB GR); auto-release
Parallel Comp
4:1 (dual-stage)
10
150
FET
30% wet mix for density without dulling

Pro Tip: Always diagnose balance issues first—use "multing" (duplicate tracks) for section-specific processing before compressing. If tone collapses, EQ upstream.UI OverviewClean, Modern Layout: Grouped controls (Dynamics | Stages | Mix) with rotary sliders.
Visual Feedback:Input/Output meters for level matching.
GR envelope graph showing detector vs. gain curve.
"Punch Indicator": Alerts if attack is too fast (transient preservation).

Expert View Toggle: Reveals sidechain EQ, knee viz, and topology coloration.

Installation & BuildingPrerequisitesJUCE 8 (free/open-source framework for audio plugins).
Steinberg VST 3 SDK (for VST3 format compatibility).
Projucer (included with JUCE) for project management.
A C++ compiler 

This project focused on making a VST3 plugin for Windows, only tested on windows 11.

Install the built .vst3 file to your DAW's plugin folder intended for windows 11 use.

Prep: Mult tracks if needed; EQ for tonal balance first.
Set & Listen: Load a preset, adjust threshold/ratio for 3–6 dB GR. Watch the meter—aim for groove-sync, not pumping.
Refine: Use automation for long-term phrasing; chain with a second instance for dual-stage if peaks persist.
A/B: Toggle bypass with auto-makeup on—louder isn't better!

When Not to Compress: Fix EQ issues or automate faders instead. Compression shapes tone too—choose topology for character (e.g., FET for aggression).

License
This project is licensed under the GPLv3 / MIT License - see the LICENSE file for details. (Feel free to change this if you prefer GPL v3 to match JUCE's default open-source terms.)
Third-Party LicensesJUCE: GPL v3 (or commercial). 
Source code includes JUCE attribution.
Steinberg VST 3 SDK: Steinberg SDK License (included in SDK; no redistribution of SDK binaries).
Inspiration: Based on public-domain mixing advice from Mike Senior's Sound on Sound article. No copyrighted code extracted—only conceptual mappings.

Acknowledgmen
tsMike Senior: For the foundational article that inspired this plugin's design and presets.   NO ENDORSEMENT OF THIS PRODUCT IS OFFERED BY MIKE SENIOR HE WAS JUST A MUSE FOR THE PLUGIN CONCEPT APPLYING HIS SUGGESTIONS IN THE ARTICLE 
https://www.soundonsound.com/techniques/compression-made-easy
JUCE, Steinberg for code.

FUTURE ADD ONS - dry/wet knob click/clip reduction still sounds a little clicky and I think the signal may be mixed currently need to look into that.
and everyone making coding easier. 

 This is an alpha project lightly tested you can report any bugs, contributoins and advances to the code are welcome as the goal here is to make good plugins for making music. 
