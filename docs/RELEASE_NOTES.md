Original flat-shaded 3D fighting game for PlayStation 1.

- Two articulated polygon fighters with distinct speed and power.
- Arcade and two-controller versus modes.
- Punch, kick, high/low guard, throw, heavy kick and advancing shoulder ram.
- KO, ring-out, time-out and first-to-two rounds.
- Tracking camera, round introductions, winner closeups, top-screen health and timer.
- Crouching, backsteps, eased motion, fixed limb lengths, hitstop, hit sparks and screen shake.
- Original procedural SPU impact sound effects.
- Emulator verification: full three-stage arcade clear and two-pad versus; 6,974 measured frames with no missed VBlank intervals. Physical-console testing is not yet performed.

Open `facet.cue` with the accompanying `facet.bin` in a PlayStation emulator.
`facet.exe` is also supplied for compatible homebrew loaders. No BIOS is included.
Consult `CONTROLS.md` for inputs and rules, and the repository's `docs/STATUS.md`
and illustrated implementation history for the precise validation scope.

The release workflow builds these files on GitHub Actions from the release tag.
Verify the unmodified downloads using `sha256sum -c SHA256SUMS` (Linux) or
`shasum -a 256 -c SHA256SUMS` (macOS). Physical-console validation is separate
from emulator validation.
