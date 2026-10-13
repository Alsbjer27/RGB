# AI coordination test checklist

Use `L_MechanicsTest` or a small arena containing the normal `RGBArenaControl`
and `RGBArenaCameraZone`. Keep the player and every enemy inside the camera
zone so the arena owns their registration and respawn lifecycle.

## Ground coordination

1. Place three ground enemies on one continuous platform and assign them to the
   arena. Set `Maximum Simultaneous Attackers` to 1 on `EnemyCoordinator`.
2. Enter PIE and approach the group.
3. Confirm one enemy occupies the close melee position while the others move to
   wider pressure positions instead of stacking on the player.
4. Confirm only one attack windup can be active at a time.
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
