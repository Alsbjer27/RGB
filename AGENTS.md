# Development instructions

## Scope and review gate

RGB is an Unreal Engine 5.7 C++ 2.5D action-puzzle platformer.
Read PROJECT_PLAN.md, ARCHITECTURE.md, and DECISIONS.md before changing gameplay.
The initial documentation is proposed for user review. Do not begin gameplay
implementation until the user has reviewed it and authorized the next step.

## C++ first

- Implement gameplay rules, player and camera behavior, weapons, platform state,
  arena evaluation, enemy decisions, and reusable systems in C++.
- Use Blueprints only where needed to assign assets, tune exposed values, create
  editor assets, configure visuals, and compose levels. No core gameplay logic
  in Blueprint graphs, including Level Blueprints.
- Presentation may respond to C++ events; it must not own authoritative state,
  damage matching, color transitions, AI decisions, or completion rules.
- Expose intentional configuration through UPROPERTY and narrowly scoped APIs.
  Keep runtime state protected and change it through validated C++ methods.

## Implementation conventions

- Work on one milestone at a time in the agreed order. Add future classes only
  when their milestone needs them; architecture names are proposals, not stubs
  to generate immediately.
- Keep the RGB runtime module initially. Group code by responsibility under
  Source/RGB (Player, Camera, Platforms, Combat, Arena, AI, Testing, Progression).
- Follow Unreal naming and reflection conventions: A/U/F/E prefixes, RGB project
  prefix, matching file/class names, generated header last, explicit includes,
  forward declarations where appropriate, and safe UObject references.
- Prefer CharacterMovementComponent for character locomotion, Enhanced Input
  for input, events for state changes, and timers for periodic decisions. Tick
  only when continuous behavior needs it; avoid repeated world-wide searches.
- Keep rule evaluation deterministic where possible. Validate missing assets,
  invalid arena IDs, and destroyed actor references with useful diagnostics.
- Do not add plugins, dependencies, networking, or broad frameworks without a
  concrete milestone need. Record consequential changes in DECISIONS.md.
- Preserve unrelated user work. Do not commit, reset, or rewrite Git history
  unless requested. Use codex/ when creating a development branch.

## Assets and verification

- Maintain Content/RGB/Maps/L_MechanicsTest as the permanent mechanics test map
  once created. Extend it for every implemented system, with labeled areas,
  expected results, and a repeatable reset. Keep it convenient and unpolished.
- Milestone 1 needs a minimal playable blockout for movement AND camera checks;
  milestone 2 turns it into the organized mechanics test level.
- Before completing a code milestone, build RGBEditor in Development Editor
  for Win64 using UE 5.7 and run relevant Play In Editor checks in the test map.
  Record actual commands/results and manual observations; never claim unrun
  checks passed. If the editor/build is unavailable, report pending verification.
- Add focused C++ automation tests for deterministic rules and regressions when
  useful. Manual checks remain necessary for movement, camera, and map setup.
- Keep Source, Config, documentation, and required Content assets in version
  control. Respect .gitattributes LFS rules. Check large new source formats for
  LFS coverage; .blend1 backups are not currently covered by the .blend rule.
- Never commit generated Binaries, Intermediate, Saved, DerivedDataCache, .vs,
  or generated solution files. Do not hand-edit binary Unreal assets.
- Keep documentation current with implemented behavior, pending design choices,
  milestone acceptance results, and any verification limitations.
