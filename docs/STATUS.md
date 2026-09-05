# Acceptance and current state

- [x] Public repository, original game specification, incremental history.
- [x] Deterministic combat: attacks, guarding, crouch/jump, throws, hit reactions.
- [x] Rules tested: KO, ring-out, timeout, draw, first-to-two and rematch.
- [x] Native PS1 build: EXE and bootable BIN/CUE.
- [x] Flat-shaded articulated 3D fighters and raised arena rendered in emulator at 60 FPS.
- [x] Title, selection, CPU play, two-player versus, pause and result screen.
- [x] SPU attack / hit / round output captured from the emulator (audio sample linked below).
- [ ] Emulator playthrough and two-controller input verified.
- [x] Build instructions, controls and evidence published.
- [ ] Completion tag and GitHub Release artifacts published.

Physical-console testing must be reported separately from emulator testing.

## Environment observations

2026-09-05: Empty local repository at task start. Origin existed as private.
SSH to `mac-studio` timed out; `mac-studio.local` did not resolve. Continue independent implementation and recheck the build host before integration.

Portable combat tests pass under AddressSanitizer and UndefinedBehaviorSanitizer, including 20,000 deterministic CPU simulation frames. This does not yet verify PS1 runtime behavior.

Build direction updated by user: GitHub Actions builds; tag and GitHub Release only after completion.

GitHub Actions run [33926563885](https://github.com/GOROman/gpt-6-astra-ps1-game-benchmark/actions/runs/33926563885) compiled the native EXE and BIN/CUE and uploaded the `facet-ps1` artifact.

DuckStation: camera sign error fixed; articulated flat-shaded fighters and raised arena verified visually at approximately 60 FPS. See [implementation history](IMPLEMENTATION.md).

2026-09-05 `f556430`: two CPU fighters with octagonal geometry, fixed-length arms, tracking camera and HUD rendered at 59.82 FPS / 59.82 VPS (60 FPS NTSC class). This is an observed sample, not yet a worst-case sustained performance audit.

SPU verification (`f556430`): 70.615 seconds of emulator-native stereo PCM at 44,100 Hz, peak 11,236 / 32,768, RMS 1,510.89, 1,203,880 nonzero samples. [18-second sample](audio/facet-spu-demo.flac). This verifies emulator audio output, not the physical speaker or a real PS1.

2026-09-05 `83a2c80`: continuous VBlank audit found nine two-VBlank intervals in 4,200 frames, with diagnostic TTY output enabled. Sustained 60 FPS acceptance remains open; investigate logging overhead and frame workload. See [raw sample](frame-audit.txt).

2026-09-05 `fddb383`: synchronous combat logging removed. Five complete CPU demo matches total 9,338 frames / 9,338 VBlanks, zero slow frames. Interactive title, selection, arcade, pause/resume, result/rematch, 2P character selection, 2P pause and movement/ring-out verified through emulator pad macros. HOW TO PLAY layout verified. This is emulated-controller verification, not physical-controller testing.

Remaining integration: `3ce1d20` builds successfully and shares projected floor vertices to improve headroom after one slow interval in a two-Ember versus run. Its image is downloaded and checksum-verified but not yet booted: the existing emulator window became unavailable to CUA (`noWindowsAvailable`). Recheck that build, KO/pause overlay fix, full arcade completion, and sustained two-player frame delivery before release. Restore the saved original controller preset after input verification.
