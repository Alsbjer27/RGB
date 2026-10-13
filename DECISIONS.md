# Technical and design decisions

Status: initial record, 2026-09-05. Accepted entries reflect explicit user
requirements; proposed entries require review and may change. No implementation
or verification is implied by an entry.

## D001 - C++ first

Status: Accepted (user requirement).

Gameplay, player/camera behavior, weapons, platforms, arena state, and enemy AI
live in C++. Blueprints are limited to necessary asset assignment, exposed
configuration, visual setup, editor assets, and level composition. This keeps
reusable rules reviewable and testable in source.

## D002 - Incremental milestone order and initial review

Status: Accepted (user requirement).

Follow PROJECT_PLAN.md milestones 1-9 in order. Show the initial documentation
before gameplay implementation. First playable includes movement and correct
2.5D camera together. Keep each increment buildable and verify systems in the
mechanics test level.

## D003 - Permanent mechanics test level

Status: Accepted (user requirement); map name/setup are proposed.

Maintain convenient testing areas for all systems, independent of the first
designed arena. Proposed path: Content/RGB/Maps/L_MechanicsTest. Create a minimal
blockout to verify milestone 1, then organize/expand it in milestone 2. Add
repeatable reset and a checklist rather than polishing it as a game level.

## D004 - Native locomotion and fixed side view

Status: Proposed for milestone 1.

Use ACharacter/CharacterMovementComponent and Enhanced Input. Constrain Y and
move in X/Z. Start with perspective projection, fixed side orientation, and
C++ follow behavior independent of facing. These choices retain 3D assets and
collision while making plane constraints explicit. Orthographic projection,
controls, camera bounds, and tuning remain open for review/playtesting.

## D005 - One module and event-driven state

Status: Proposed.

Keep the existing RGB runtime module. Let color components own mutations and
publish changes; let the arena own requirement evaluation. Avoid early module
splits, pervasive Tick polling, and speculative future scaffolding.

## D006 - Color rules are not yet specified

Status: Open; resolve by milestones 3-4.

The user has established landing/jump-off color changes, matching weapon kills,
and eliminated-enemy color transfer to the platform beneath. Palette, transition
function, walking-off behavior, airborne transfer/range, and mismatched hit
effects are undecided. Do not silently invent a cycle or neutral-color rule.

## D007 - Arena requirements and completion

Status: Proposed/open; resolve by milestone 5.

Start with stable platform IDs mapped to required colors. Aggregate counts are
an alternative if the intended puzzles require them. Decide immediate versus
delayed completion and whether success remains latched after later changes.
Reset should batch state restoration before evaluating the result.

## D008 - Reachability-aware chase and sabotage

Status: Accepted direction; implementation proposed for milestone 6.

AI must know its own color, supporting platform, arena requirements, and
reachable disruptive targets. Propose authored traversal links and C++ utility
scoring with travel costs and hysteresis. Resolve enemy platform-recoloring
rules before scoring them. Do not equate a geometrically close platform with
a reachable one.

## D009 - Intentional cannon exposure

Status: Accepted direction; deferred to milestone 8.

Enemies eventually understand cannon countdown and line of fire, intentionally
entering it when elimination would damage player progress. Reuse normal combat,
transfer, and arena prediction rules. Timing/reachability must constrain the
choice; cannon mechanics and score weights remain open.

## D010 - Existing Git/LFS setup

Status: Existing configuration observed, not changed.

The repository has no commits yet. .gitattributes covers Unreal binary assets
and several art/audio formats; .gitignore excludes generated Unreal products.
The observed .blend1 backup is not matched by *.blend. Decide whether to ignore
backups or track them through LFS before committing that file. No commits or
LFS configuration changes are part of this documentation task.

## D011 - Reusable level transition areas

Status: Accepted and implemented, 2026-10-12.

Level-to-level traversal uses a reusable C++ overlap actor with an editor-assigned
soft level reference. Entering its trigger locks local player input, fades the
camera to black, optionally holds black, and opens the destination level once.
The destination gameplay controller owns the existing initial black hold and
fade-in, keeping level composition and timing configurable without Level Blueprint
gameplay logic.

## D012 - Arena-scoped enemy coordination and flying enemies

Status: Accepted and implemented, 2026-10-13.

Each arena owns a timer-driven enemy coordinator. The coordinator assigns one
melee attacker by default, places remaining enemies in pressure slots, limits
simultaneous attack permission, and owns shared target reservations. Enemy
characters retain authoritative movement, attacks, health, color, and variant
capabilities. Registration follows the arena's existing spawn, delayed respawn,
reset, and completion lifecycle.

Enemy behavior exposes a shared action-state enum for diagnostics. Attack damage
uses explicit C++ windup and completion timers; animation montages are optional
presentation. Teleporters share attack permission for AoE windups and use the
arena coordinator for destination reservations when available.

Flying enemies use CharacterMovement in flying mode, constrained to the X/Z
gameplay plane. They always follow the player's live position while the
coordinator controls attack permission. Enemy-separation and geometry-probe
steering modify the pursuit direction locally. Their pawn collision overlaps
other pawns to avoid physical pushing while swept movement continues to block
level geometry. Arena registration explicitly supplies the arena camera zone;
flying movement clamps the full capsule inside that zone. Flying attacks reuse
the shared windup, cooldown, coordination, and player-health path with a
sphere-to-player-capsule range check suitable for airborne contact. Arena-owned
enemies remain in Idle until the player enters their camera zone and return to
Idle when the player exits; movement, targeting, attacks, teleport charges, and
AoE warnings pause while the arena is inactive.

Platform traversal and sabotage remain deferred until authored reachability and
the living-enemy platform interaction rule are agreed.

## Future decision entry format

Use a stable D-number, title, date, status (Proposed/Accepted/Open/Superseded),
context, decision, consequences, and relevant milestone. Identify the source
of acceptance and link a replacement when superseding a decision.
