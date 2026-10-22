# RGB

TDDD23 game project developed with Unreal Engine 5.7.

## Getting started

1. Install Unreal Engine 5.7, Git, and Git LFS.
2. Clone the repository.
3. Run `git lfs install` and `git lfs pull`.
4. Right-click `RGB.uproject` and generate project files if needed.
5. Open `RGB.uproject` and build the C++ module when prompted.

Generated Unreal folders such as `Binaries`, `Intermediate`, `Saved`, and
`DerivedDataCache` are intentionally not committed.

RGB is a playable 2.5D action-puzzle platformer prototype. Colour rules connect
movement, combat, platforms, enemies and arena puzzles. C++ owns gameplay;
Blueprints configure assets, presentation and levels.

Implemented systems include arena-controlled ground, teleporter and flying AI,
progress-dependent enemy pressure, colour combat, platform puzzles, checkpoints,
respawn, menu/level fades and a HUD that follows the active arena.

The configured startup map is L_INTRO. L_MechanicsTest is the permanent mechanics
test map; L_ChromaticFoundry is a separate three-arena level prototype.

## Project reference

- [CODEX_HANDOFF.txt](CODEX_HANDOFF.txt): current state, earlier sessions and verification limits.
- [PROJECT_PLAN.md](PROJECT_PLAN.md): current delivery status and original milestone plan.
- [ARCHITECTURE.md](ARCHITECTURE.md) and [DECISIONS.md](DECISIONS.md): ownership, design decisions and implementation boundaries.
- [ENEMY_AI_SYSTEM_EXPLAINED.txt](ENEMY_AI_SYSTEM_EXPLAINED.txt): detailed AI explanation and presentation material.
- [CHROMATIC_FOUNDRY_LEVEL.md](CHROMATIC_FOUNDRY_LEVEL.md): new level layout, authoring helpers and recorded checks.

Last documentation reconciliation: 2026-10-22. Recorded builds and PIE checks
are dated in these documents; this date does not imply a new test run.
