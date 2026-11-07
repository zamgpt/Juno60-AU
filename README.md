# Juno60-AU

A native macOS Audio Unit instrument inspired by the Roland Juno-60, built with JUCE and C++ for Logic Pro. Apple Silicon is the primary target; the CI build produces a universal Apple Silicon + Intel AU.

## Current development status

The project is an actively developed prototype, not yet a finished or historically exact Juno-60 emulation. The current engine has six MIDI voices, saw and pulse oscillators with a polynomial band-limiting correction, a true half-frequency sub oscillator, noise, an amplitude ADSR, a resonant low-pass filter, and a stereo chorus effect. The interface exposes oscillator mix, pulse width, filter, envelope and chorus controls.

The filter and chorus are currently JUCE DSP implementations used as stable foundations. They are not claimed to be component-accurate IR3109 / BA662 / BBD circuit models. Further work is required on filter topology and nonlinearities, envelope routing, chorus behaviour, modulation, presets, UI fidelity, and validation.

## Build requirements

- macOS 11 or newer
- Xcode command-line tools
- CMake 3.22 or newer
- Internet access on first configure (JUCE is fetched from its upstream repository)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Juno60AU_AU
```

The Audio Unit bundle is generated under `build/Juno60AU_artefacts/Release/AU/Juno-60.component`. Install it in `~/Library/Audio/Plug-Ins/Components/` and restart or rescan Audio Units in Logic Pro.

## Automated build

GitHub Actions builds the AU on macOS, verifies the component bundle and its architectures, and uploads the resulting `.component` bundle as a workflow artifact. A successful compile is not, by itself, proof of correct behaviour in Logic Pro; host validation remains a release gate.

## Planned completion work

1. Establish a clean, repeatable AU build and validate the bundle.
2. Add DSP unit tests and improve oscillator, filter, envelope and chorus models.
3. Complete the instrument panel, preset handling, MIDI/automation behaviour and session recall.
4. Run Audio Unit validation and verify the plugin in Logic Pro.
5. Publish a versioned installable build with installation instructions.

## Dependency and licensing

JUCE is fetched as a separate dependency and remains subject to its own licensing terms. No Roland firmware, ROM, samples, or proprietary source code are included. Project licensing and third-party notices will be finalized before binary distribution.
