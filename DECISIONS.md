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

## Future decision entry format

Use a stable D-number, title, date, status (Proposed/Accepted/Open/Superseded),
context, decision, consequences, and relevant milestone. Identify the source
of acceptance and link a replacement when superseding a decision.
