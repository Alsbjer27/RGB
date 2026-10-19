# AI coordination test checklist

Use `L_MechanicsTest` or a small arena containing the normal `RGBArenaControl`
and `RGBArenaCameraZone`. Keep the player and every enemy inside the camera
zone so the arena owns their registration and respawn lifecycle.

## Arena progress profiles (2026-10-17)

On the placed Arena Control, open `RGB > Arena > AI` and enable `Use Arena
Enemy Tuning`. Set `Teleport Target Points` to the placed points belonging to
that arena; enabled profiles replace tag discovery. Use at least three clear
destinations for two teleporters. Points represent the floor surface and need
room above and around them for the capsule, entirely inside the camera zone.

Each profile value has `At No Progress`, `Near Completion` and an optional
`Response Curve` (completion on X, bounded blend on Y). The profiles replace
the older shared speed multiplier. The respawn-delay toggle remains independent.

1. Use several assigned platforms with a known required color. Compare low
   progress with 50 and 75 percent, keeping the arena incomplete.
2. Set `Coordination Tuning > Maximum Attackers` to 1 and 4 for its endpoints.
   With linear response the budget is 1 at 0%, 2 at 50% and 3 at 75%. Check
   `Current Maximum Attackers` on the arena and `Current Assignments` on its
   EnemyCoordinator component. Ground, flying and teleporter attacks must share
   this limit. Executing attacks finish if progress/budget decreases.
3. Ground: observe random patrol directions. Approach on the same platform
   with a clear line of sight and observe the chase boost. Hide behind a wall
   and verify loss of pursuit. More ground enemies should qualify for chase
   with higher completion. Ledge protection must still hold.
4. Ground attack: preserve the Blueprint's existing `Attack Hit Delay`,
   `Attack Play Rate` and `Attack Duration`. The attack animation should start
   immediately. Damage should occur at its original configured moment at both
   low and high progress; only the profile's cooldown changes repetition.
5. Teleporter: compare idle intervals and destination pressure at low/high
   progress. Stand on a separate platform far above/below the teleporter while
   sharing its X position; no attack should qualify or damage occur. Stand at
   the same combat level within Horizontal Attack Range; it should qualify.
   Move beyond the actual AoE Radius or onto another floor during the warning;
   damage should be rejected. Same Level Height Tolerance defaults to 60 cm
   between the characters' feet. Existing warning/charge visuals must remain synchronized.
6. Flying: compare speed, acceleration, tracking and avoidance using deliberately
   different endpoints. Reduce progress and verify the attributes return toward
   their low-progress values. Verify full capsule confinement at the arena edges.
7. Make a second arena with its own camera zone, enemy list, profiles and Target
   Points. Verify each teleporter uses only its owning arena's points and that
   the inactive arena cannot grant attack permissions.
8. Eliminate each enemy type, reset while one is waiting, and reset after arena
   completion. Expect exactly one restored instance for each authored enemy,
   preserving instance tuning, random-color settings and arena ownership.

Automated checks: `Automation RunTests RGB.ArenaAI` covers bounded progress
curves, dangerous-destination scoring, horizontal range after level eligibility, the shared budget,
preservation of attack hit timing, and configured instances across elimination,
reset and reset after completion. Visual movement and animation timing still
require the PIE checks above.

Verification recorded on 2026-10-17:

- Build succeeded with UE 5.7: `Build.bat RGBEditor Win64 Development
  C:\Users\manun\Desktop\Spelprojekt\RGB\RGB.uproject -WaitMutex -NoUBA
  -NoHotReloadFromIDE` (PowerShell invocation uses the full engine path).
- Ran `UnrealEditor-Cmd.exe RGB.uproject -unattended -nullrhi -nosound
  -nosplash -ExecCmds="Automation RunTests RGB.ArenaAI"
  -TestExit="Automation Test Queue Empty"` with reports in
  `Saved/Automation/ArenaAI` and log `Saved/Logs/ArenaAI-Automation.log`.
- Final report after the teleporter clarification and damage guards: five tests
  passed, zero failed, zero skipped. The isolated
  lifecycle actors deliberately have no visual materials; their missing-material
  warnings are present in that passing test's report.
- Actual visual PIE movement, animation synchronization, two-arena map wiring
  and pressure tuning have not been run for this change. Follow the sequence
  above in `L_MechanicsTest` before treating presentation tuning as verified.

The following original checks apply to arenas using legacy tuning.

## Damage-source diagnosis

In PIE, enter `rgb.DebugPlayerDamage 1`. A successful health hit must print the
attacker name, draw a red line to its location, and add a `LogRGBPlayerDamage`
entry with damage, health and X/Z separation. Check the player instance's Last
Damage Source/Amount as well. `rgb.DebugPlayerDamage 0` disables screen visuals.
Teleporter attacks must reject players on a different combat level and damage
must stay inside AoE Radius. Temporarily setting both Horizontal Attack Range endpoints to zero
isolates teleporter attack damage while retaining teleport movement.

The `RGB.ArenaAI.InactiveEnemyDamage` regression exercises real melee and AoE
damage callbacks: active actors damage the controlled player; hidden and
inactive actors cannot damage through a stale callback.

Damage and teleporter clarification verification on 2026-10-17: RGBEditor Win64
Development linked successfully with the editor closed. The full RGB.ArenaAI
suite passed all five tests, including active melee/AoE damage, different-floor
attack rejection, actual AoE damage bounds and hidden/inactive callback guards.
Visual PIE confirmation in the user's configured arena remains pending.

## Ground coordination

1. Place three ground enemies on one continuous platform and assign them to the
   arena. Set `Maximum Simultaneous Attackers` to 1 on `EnemyCoordinator`.
2. Enter PIE and approach the group.
3. Confirm one enemy occupies the close melee position while the others move to
   wider pressure positions instead of stacking on the player.
4. Confirm only one attack can be active at a time; its animation starts immediately.
5. Move through the group and confirm assignments remain stable instead of
   changing every frame.

## Lifecycle

1. Eliminate the active attacker and confirm another enemy takes its role.
2. Confirm the eliminated enemy respawns after the configured arena delay and
   receives a new assignment.
3. Reset the arena while an enemy has a pending respawn. Confirm exactly one
   enemy returns for every recorded spawn.
4. Complete the arena while an attack or respawn is pending. Confirm no enemy
   attacks or respawns afterward.

## Teleporters

1. Place two teleporters with at least three shared teleport points.
2. Confirm they never reserve or complete a teleport to the same point.
3. Occupy a destination during the charge and confirm that teleport aborts and
   retries after a new random idle interval.
4. Confirm teleport idle time begins after a completed or aborted attempt.
5. Confirm a teleporter AoE and a melee attack do not begin simultaneously when
   `Maximum Simultaneous Attackers` is 1.

## Flying enemy

1. Create a Blueprint child of `RGBFlyingEnemyCharacter`, assign a spherical
   mesh and the normal enemy color materials, and place two instances in an
   arena at different heights.
2. Confirm each enemy remains on its original Y plane while following the player
   freely in X and Z.
3. Lead them around walls, ceilings, floors, and corners. Confirm they steer for
   clear space and remain blocked by geometry rather than entering it.
4. Confirm the flying enemies separate from each other and other enemies without
   physically pushing them.
5. Eliminate and respawn one flying enemy. Confirm it returns as the same class,
   resumes flying mode, and rejoins coordination.

## Diagnostics

Select the arena's `EnemyCoordinator` component during PIE. Check registered
enemy count, active attack permissions, and active target reservations. Enable
`Debug Logging` temporarily to print role changes. Disable it after testing.
