# Planned C++ architecture

Status: proposal for review. These classes do not exist yet. Introduce them
incrementally in their milestone; keep a single RGB runtime module initially.

## Ownership and classes

| Milestone | Proposed class/type | Responsibility |
|---|---|---|
| 1 | ARGBGameMode | Select native pawn/controller defaults and own run startup/restart orchestration. Delegate puzzle rules to the arena. |
| 1 | ARGBPlayerController | Install Enhanced Input mapping context and manage possession/input lifecycle. Asset references and mappings may be editor-configured. |
| 1 | ARGBPlayerCharacter | Use CharacterMovementComponent for constrained locomotion; bind move/jump actions in C++; own facing and expose confirmed landing/jump-off events. |
| 1 | URGBSideViewCameraComponent | Own spring arm/camera configuration and C++ follow behavior; keep fixed view orientation independent of character facing, with exposed framing/smoothing settings. |
| 2 | ARGBMechanicsTestManager | Development-only test-area reset/teleport controls and diagnostics; call normal system reset APIs without duplicating rules. |
| 3 | ERGBColor / FRGBColorChangeContext | Define gameplay colors and mutation source/context, independently of materials. Palette and neutral semantics remain open. |
| 3 | URGBColorComponent | Own current/initial color, validated changes, reset, and color-change events. Reusable by platforms and enemies. |
| 3 | ARGBColorPlatform | Own collision/visual components, stable platform ID, and application of C++ interaction policies. Notify presentation from color state. |
| 3 | URGBPlatformContactComponent | Track supporting platform from movement floor/landing information; distinguish landing, deliberate jump-off, walking off, and unrelated contact. Reuse for enemies later. |
| 4 | URGBWeaponComponent | Own equipped weapon color, firing/cooldown and hit dispatch. Keep collision/hit resolution in C++; choose projectile or trace implementation when firing design is settled. |
| 4 | ARGBEnemyCharacter | Own enemy color, matching-hit eligibility, and one-time elimination. Snapshot transfer information before actor teardown. Locomotion/AI arrives in milestone 6. |
| 4 | URGBColorTransferSubsystem (world subsystem) | Resolve a valid platform beneath an eliminated enemy and apply its color once. Centralize spatial query/filter rules and no-target handling. |
| 5 | ARGBArenaController | Own explicit platform registry, required configuration, completion state/event, and arena reset. Expose a read-only snapshot to AI. |
| 5 | FRGBPlatformColorRequirement / FRGBArenaSnapshot | Store stable platform IDs and target/current colors. Support pure configuration matching and hypothetical-change evaluation. |
| 6 | ARGBEnemyAIController | Own chase/sabotage decisions, target validity, traversal requests, and progress/failure handling. |
| 6 | URGBPlatformNavigationComponent | Represent reachable platform links and move/jump actions for the enemy's capabilities. Start with authored links; validate actions against real movement. |
| 6 | FRGBEnemyDecisionContext / FRGBEnemyActionScore | Evaluate player pursuit and reachable sabotage candidates using enemy color, support platform, arena requirements, costs, and predicted result. |
| 8 | ARGBCannon | Own countdown, firing state, line-of-fire query, and events. Route elimination through the existing combat/transfer path. |
| 8 | Cannon-aware extensions to AI context/scoring | Predict whether arrival before firing and elimination-induced color transfer would worsen the target configuration; choose intentional exposure when feasible. |
| 9 | URGBProgressionSubsystem / URGBSaveGame | Own cross-level progression and serialized progress if persistence is required. Define schema only once progression scope is agreed. |

## Movement and camera proposal

Use world X for horizontal travel, Z for vertical travel, and constrain Y to the
gameplay plane. Keep 3D collision and meshes. Player facing affects visuals and
weapon direction without rotating the camera. Start with a fixed perspective
side view looking along Y, configurable follow offset/distance, and smoothing.
Camera initialization and reset must snap to a valid view before normal follow;
test geometry must not introduce unintended arm shortening or camera rotation.
Projection and final framing remain reviewable design choices.

## State and event flow

1. C++ input handlers drive CharacterMovementComponent.
2. Confirmed movement transitions identify the source or destination platform.
   A deliberate jump captures its supporting platform before losing contact;
   a landing uses the actual supporting collision result. Contact does not
   itself imply a color transition.
3. The platform's C++ policy computes a new color. URGBColorComponent owns the
   mutation and notifies visuals and the arena only when state changes.
4. A weapon hit asks the enemy to resolve color eligibility. The first accepted
   elimination captures enemy color/location, resolves the platform beneath it,
   and transfers color before teardown. Repeated hits cannot transfer again.
   Beneath-platform queries must reject side/upper geometry and define stacked,
   airborne, range, and no-target behavior before implementation.
5. The arena evaluates relevant color changes against its requirements. Reset
   restores state as a batch, suppresses intermediate completion checks, then
   evaluates once. Completion timing/latching remains a design decision.
6. AI reads arena snapshots and reachable links, then requests ordinary movement
   and color APIs. It never directly edits arena progress or uses hidden goals.

## AI growth path

Begin with explicit, debuggable C++ decisions. Compare chase utility with the
predicted reduction in correctly configured platforms caused by sabotage, minus
travel cost/risk. Reuse the arena's hypothetical evaluation for both sabotage
and later cannon sacrifices. Add cooldown/hysteresis to avoid constant target
switching; reject unreachable targets and replan after movement failure.

For cannon tactics, query remaining countdown and shot geometry, estimate
arrival time, and predict the platform/color resulting from elimination.
Choose exposure only when feasible and damaging to player progress; otherwise
continue chase/sabotage. Exact utility weights and recoloring rules are open.
Do not assume generic ground navigation solves platform jumping. Behavior Trees
are not required initially; if later useful, reusable tasks and rules stay C++.

## Editor boundary and testing

Editor assets supply meshes, materials, input actions/mappings, sound, animation,
tuning, and placed instances. Visual Blueprint subclasses can consume events
but cannot decide damage, transitions, AI actions, or completion. Levels wire
arena membership/IDs and authored traversal links through exposed properties.

Use focused C++ tests for color rules, hit matching, configuration evaluation,
and AI scoring when introduced. Use L_MechanicsTest for real collision, movement,
camera, transfer traces, traversal, reset, and integrated event behavior. Avoid
hard-coded asset paths and global actor searches in per-frame code.
