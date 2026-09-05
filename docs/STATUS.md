# Acceptance and current state

- [x] Public repository, original game specification, incremental history.
- [x] Deterministic combat: attacks, guarding, crouch/jump, throws, hit reactions.
- [x] Rules tested: KO, ring-out, timeout, draw, first-to-two and rematch.
- [x] Native PS1 build: EXE and bootable BIN/CUE.
- [x] Flat-shaded articulated 3D fighters and raised arena rendered in emulator at 60 FPS.
- [x] Title, selection, CPU play, two-player versus, pause and result screen.
- [x] SPU attack / hit / round output captured from the emulator (audio sample linked below).
- [x] Emulator playthrough and two-controller input verified.
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

Final integration (`d93966e`, Actions run [33941217198](https://github.com/GOROman/gpt-6-astra-ps1-game-benchmark/actions/runs/33941217198)): all three arcade stages cleared via emulated pad inputs; ARCADE CHAMPION displayed. Complete campaign: 3,777 frames / 3,777 VBlanks, zero slow intervals. Two-Ember versus: 3,197 frames / 3,197 VBlanks, zero slow intervals, including pause. P2 character selection, movement, punch damage, pause/resume, P1 crouch high-attack avoidance and backstep/ring-out were observed. Pause and result overlays are legible. See [final frame audit](frame-audit-final.txt) and screenshots 015–018 in the implementation history.

Original Pad1/Pad2 bindings were restored and compared with the saved preset; TTY and file logging were returned to their original disabled state. Only one DuckStation instance was used. Physical PS1 and physical gamepad testing remain unperformed. These measurements verify the recorded emulator runs, not every possible gameplay sequence.

Final presentation correction: attract-mode entry resets the displayed stage and mode instead of retaining the previous arcade/versus HUD. Release packaging remains pending.
