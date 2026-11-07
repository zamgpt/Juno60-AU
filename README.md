# Juno60-AU

A native macOS Audio Unit instrument inspired by the Roland Juno-60, built with JUCE and C++ for Logic Pro, with Apple Silicon as a primary target.

## Status

**Prototype foundation committed.** The current source establishes a JUCE AU instrument, MIDI-triggered polyphonic oscillator voices, basic saw/pulse/sub controls, attack/release envelope parameters, host parameter attachments, and a first-pass control panel.

This is not yet a faithful Juno-60 emulation and has not yet been built or validated in Logic Pro. The current oscillator and envelope are deliberately simple starting points.

## Build requirements

- macOS with Xcode command-line tools
- CMake 3.22 or newer
- Internet access on first configure (JUCE is fetched from its upstream repository)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Juno60AU_AU
```

The AU bundle is generated under `build/Juno60AU_artefacts/Release/AU/`. Install `Juno-60.component` into `~/Library/Audio/Plug-Ins/Components/` and rescan Audio Units in Logic Pro.

## Planned sound engine

1. Juno-style DCO behaviour: saw, pulse-width modulation, sub oscillator and noise.
2. Circuit-informed 24 dB/octave low-pass filter, including resonance and nonlinear behaviour.
3. Envelope, LFO, modulation routing and amplifier response.
4. Stereo BBD-inspired chorus, with noise, modulation and companding characteristics.
5. Oversampling/anti-aliasing, parameter smoothing and voice/performance optimization.

Models will be progressively validated against reference behaviour and measurements. A circuit-inspired design is not automatically a circuit-accurate model.

## Planned product work

- Full front-panel-inspired interface and section layout.
- Preset browser, factory/user preset management and reliable session recall.
- Complete MIDI learn and host automation.
- Apple Silicon build and Logic Pro AU validation.
- Automated DSP tests and audio regression tests.

## Dependency and licensing

JUCE is fetched as a separate dependency and remains subject to its own licensing terms. No Roland firmware, ROM, samples, or proprietary source code are included. Project licensing will be finalized before binary distribution.
