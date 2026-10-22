# RGB project plan

Status: playable prototype with implemented C++ gameplay, updated 2026-10-22.
The initial plan was reviewed and implementation authorized in earlier sessions.
Read CODEX_HANDOFF.txt for the latest implementation and verification summary.
The original milestone order is retained below as design history; it is not a
claim that every original acceptance criterion has been met.

## Current implementation

- Player movement in X/Z, camera zones and follow modes, jumping and air dash,
  coloured projectiles, health, ability pickups, checkpoints and fading respawn.
- Platform colour transitions, moving/rotating platforms, launch pads, colour
  group activation, arena progress/completion and completion-controlled exits.
- Arena-owned multi-agent coordination for ground, teleporter and flying
  enemies, attack eligibility/budgets, exclusive enemy ownership, destination
  reservations, configurable respawn and progress-dependent pressure profiles.
- Instance settings retained through enemy elimination, respawn and arena reset.
  Ground attack animation timing supplies the windup; no separate delay is added.
- Menu selection and scripted character presentation, colour reset on return,
  menu widget fade, level transitions and initial black hold/fade-in.
- Shared HUD follows the current arena, displays zero outside arena bounds,
  and removes older HUD instances when a replacement is created after respawn.
- Separate L_ChromaticFoundry prototype: red/green/blue combat puzzle rooms,
  connecting traversal, checkpoints and gated exits. Existing reference maps
  were preserved during its creation. See CHROMATIC_FOUNDRY_LEVEL.md.

Original proposed classes and mechanics are not all implemented. In particular,
do not describe authored enemy platform navigation, strategic sabotage scoring,
cannon-aware sacrifice planning or disk-backed saves as completed systems.
Ability unlocks currently live in a GameInstance subsystem, not a SaveGame file.

## Latest recorded verification and remaining work

The latest recorded code build is RGBEditor Win64 Development with UE 5.7.1 on
2026-10-18. The Foundry work recorded 78 asset checks, 47 PIE systems checks,
15 PIE movement probes and 217 HUD regression assertions. These are scoped
checks, not an uninterrupted human combat/traversal run or full milestone signoff.
No build or PIE was rerun during this documentation-only update on October 22.
Remaining assessment includes combat balance, natural traversal/controller feel,
shuttle riding, final menu transition/fade and visual confirmation of teleporter
damage/warning boundaries on different platforms. Keep L_MechanicsTest as the
permanent mechanics regression map.

## Historical repository baseline (2026-09-05; not current state)

- RGB.uproject targets UE 5.7 and declares one RGB runtime module.
- Source contains module startup, build rules, and Game/Editor targets only.
- EnhancedInput is a module dependency and the configured input implementation.
- The default game mode is Engine.GameModeBase. No map defaults are configured;
  no files were found in Content during inspection.
- Git is initialized on master with no commits; existing project files are
  untracked. Ignore and LFS attribute files exist. LFS transfer was not tested.
- Blender source and a .blend1 backup are present. Generated local build/editor
  directories exist; their presence does not establish a successful build.
- This documentation pass does not modify code, configuration, or assets and
  does not establish a build baseline.

## Game premise

Players change platform colors by jumping off or landing on platforms.
Color-specific weapons eliminate matching enemies. Eliminated enemies transfer
their color to the platform beneath them. An arena is completed by reaching its
required platform-color configuration.

## Milestones and acceptance

| # | Milestone | Scope and acceptance |
|---|---|---|
| 1 | Player movement and 2.5D camera | C++ character, Enhanced Input binding, grounded movement, jumping, collision, facing, and C++ side-view camera. On a minimal blockout, verify left/right travel, jump/land, walk off edges, stable movement plane, and no unintended depth motion. Camera must retain side orientation when facing changes, track horizontal/vertical travel, keep jumps readable, and initialize/reset without drift or unwanted orbit. Check different viewport aspect ratios and frame rates. Movement alone does not complete this milestone. |
| 2 | Mechanics test level | Organize and retain L_MechanicsTest with labeled movement/camera areas, safe spawn/reset, and reserved areas for later systems. Record controls and expected results in a test checklist. The map must be repeatable after restart and remain a development fixture. |
| 3 | Platform color system | C++ color state, visual notification, support-platform detection, and explicit landing/jump-off transitions. Resolve the color transition policy before implementation. Verify one transition per intended event, no changes from side/underside contact, walking-off behavior, repeated landings, and reset. |
| 4 | Basic combat and enemy color transfer | C++ colored weapons, hit resolution, matching-enemy elimination, and transfer to the platform beneath the eliminated enemy. Verify mismatch behavior, duplicate-hit protection, stacked platforms, and the agreed airborne/no-platform behavior. Stationary enemies are sufficient here. |
| 5 | Arena and puzzle logic | C++ platform registration, required configuration, completion evaluation, and reset. Verify correct/incorrect configurations, missing or duplicate IDs, color changes from both player and combat, one completion event per run, and reliable reset. Resolve completion timing before implementation. |
| 6 | Basic enemy AI | C++ perception/target selection and reachable-platform traversal. Enemy knows its color, supporting platform, and arena requirements; chooses chase or sabotage using the effect of a reachable target on puzzle progress. Verify unreachable targets, stable decisions, platform sabotage, and chase fallback in the test map. |
| 7 | First designed arena | Compose a playable arena from verified systems with an achievable puzzle, readable goals, enemy placement, and restart/completion flow. Validate the full playthrough and retain mechanics-map regression checks. |
| 8 | Cannon and advanced enemy AI | C++ cannon countdown, firing and color-transfer integration; AI evaluates countdown, line of fire, arrival time, and predicted puzzle damage from its elimination. Verify beneficial/damaging sacrifice choices, impossible arrival rejection, canceled/expired shots, and ordinary chase/sabotage fallback. |
| 9 | Progression, additional enemies, and additional levels | Incrementally add progression/unlocks, enemy variants, and levels. Decide persistence scope first. Validate each new mechanic in the test map before level use and check progression/restart behavior across levels. |

## Delivery workflow

1. Review this documentation before gameplay work begins.
2. For each milestone, resolve only design questions that block that milestone.
3. Implement the smallest usable C++ system with necessary editor configuration.
4. Build, exercise its mechanics-map area, and rerun affected earlier checks.
5. Record changed behavior, verification results, and open issues. Move forward
   only with a buildable, verified increment; never label untested work complete.

## Original design questions (historical; consult DECISIONS.md for resolutions)

- Before milestone 1: accept or adjust proposed X/Z movement, Y depth, fixed
  perspective side view; tune controls, movement, camera framing and smoothing.
- Before milestone 3: palette, initial/neutral color, exact landing/jump-off
  color rule, and whether walking off counts as an interaction.
- Before milestone 4: weapon selection and firing model, mismatch feedback,
  transfer range and airborne behavior, player damage/death requirements.
- Before milestone 5: per-platform targets versus aggregate counts, completion
  timing/latching, and reset scope. Per-platform targets are the proposed start.
- Before milestones 6 and 8: how enemies recolor platforms while sabotaging,
  traversal capabilities, chase/sabotage scoring, cannon control and shot rules.
- Before milestone 9: progression and save requirements.

No numerical tuning values or unresolved rules are treated as agreed designs.
