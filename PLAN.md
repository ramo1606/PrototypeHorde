# PLAN.md — Boxhead 3D, phases and tasks

Live document. Update as phases and tasks complete.

---

## Project goal

Clone of Boxhead 2Play with 3D assets and a fixed isometric camera with
low-angle perspective. Mechanics faithful to the original: 8-direction
movement, jump, growing waves, full arsenal, deployables. Cel-shaded 3D
engine.

**Primary target:** Anbernic family (RG35XX Plus/H/40/V on muOS),
including devices without analog sticks. Variable resolution, stable
30 FPS.

**Secondary target:** PC. Variable resolution, 60 FPS.

**Modes:** singleplayer + local multiplayer 2-4 (PC, shared screen
following the centroid of the live players).

---

## Status legend

- ✅ done
- 🔄 in progress
- ⏭️ next up
- ⬜ pending

---

## Phase 0 — Scaffolding, game loop, foundations ✅

- ✅ 0.1 Project structure + build system
- ✅ 0.2 Memory arenas with rmem
- ✅ 0.3 Config + per-platform constants
- ✅ 0.4 Game loop with fixed timestep
- ✅ 0.5 Resource manager *(later removed: kit refactor pivot)*
- ⬜ 0.6 KayKit asset validation
- ✅ 0.7 Level manager with transitions
- ✅ 0.8 Debug overlay
- ✅ 0.9 Refactor to module-per-file architecture
- ✅ 0.10 Kit refactor: separate reusable `lib/` modules from project code
- ✅ 0.11 Docs reorganization (CLAUDE / PLAN / REFERENCES split) + level simplification
  (single `level_sandbox` replaces test A/B)

---

## Phase 1 — Renderer + camera ✅

- ✅ 1.1 Renderer module: registration + draw
- ✅ 1.2 Renderer interpolation
- ✅ 1.3 Frustum culling (Gribb-Hartmann)
- ✅ 1.4 Material sorting
- ✅ 1.5 Cel shader
- ✅ 1.6 Isometric camera (rewrite from orbit/aim)
- ✅ 1.7 Blob shadows

---

## Phase 2 — Physics world ✅

- ✅ 2.1 Collider structs
- ✅ 2.2 Registration + pool
- ✅ 2.3 Collision tests (6 combinations)
- ✅ 2.4 Broad-phase detection + resolution
- ✅ 2.5 Queries: raycast + overlap
- ✅ 2.6 Debug draw + F4 panel
- ⬜ 2.7 Pending improvements (generation handles, compact active list,
  trigger buffer in scratch arena)

---

## Phase 3 — Player ⏭️ *— next up*

Character controller with 8-dir movement, jump, strafe lock,
animations, registered in renderer + physics.

The current `level_sandbox` already wires a placeholder dummy with
WASD-on-XZ movement against the physics world. Phase 3 replaces that
with the real input system and a full character controller.

- ⬜ 3.1 Input abstraction (8 dirs + strafe lock + Anbernic mapping)
- ⬜ 3.2 8-direction movement
- ⬜ 3.3 Jump
- ⬜ 3.4 Player animations
- ⬜ 3.5 Renderer + physics registration

### Input mapping (Anbernic)

```
D-pad    Move (8 directions)  +  Aim (same direction)
A        Fire
B        Jump
X        Switch weapon
Y        Use (deployable / interact)
L        Strafe lock — locks aim while held; D-pad only moves
R        Secondary weapon (grenade / airstrike)
Start    Pause
Select   (reserved, debug builds)
```

PC mapping: WASD = D-pad, Mouse = aim (overrides strafe), LMB = A,
Space = B, Q = X, E = Y, LShift = L, RMB = R.

---

## Phase 4 — Basic weapon + basic enemy ⬜

Minimum core loop: move, shoot, kill, die.

- ⬜ 4.1 Weapon struct + shoot system
- ⬜ 4.2 Pistol (hitscan)
- ⬜ 4.3 Enemy pool + spawn
- ⬜ 4.4 Basic zombie: seek AI
- ⬜ 4.5 Player health + game over
- ⬜ 4.6 Debug panels F5 (gameplay) + F6 (spawn/cheats)

---

## Phase 5 — Score, waves, progression ⬜

- ⬜ 5.1 Score + combo system
- ⬜ 5.2 Unlock thresholds
- ⬜ 5.3 Wave system
- ⬜ 5.4 Spawn points
- ⬜ 5.5 Temporary HUD

---

## Phase 6 — Full arsenal ⬜

- ⬜ 6.1 Projectile pool
- ⬜ 6.2 Uzi (high cadence hitscan)
- ⬜ 6.3 Shotgun (multi-ray cone)
- ⬜ 6.4 Rockets (projectile + AOE)
- ⬜ 6.5 Grenades (parabola + timer + bounce)
- ⬜ 6.6 Railgun (penetrating hitscan)
- ⬜ 6.7 Deployable pool
- ⬜ 6.8 Mines (sphere trigger → AOE)
- ⬜ 6.9 Barrels (destructible AABB, chain explosion)
- ⬜ 6.10 Fake walls (block enemies, not player)
- ⬜ 6.11 Turrets (autonomous deployable)
- ⬜ 6.12 Airstrike (timer + massive AOE, limited uses)
- ⬜ 6.13 Weapon switching + inventory

---

## Phase 7 — All enemies ⬜

- ⬜ 7.1 Fast zombie (Devil)
- ⬜ 7.2 Ranged zombie (Mummy)
- ⬜ 7.3 Tank zombie
- ⬜ 7.4 AI debug panel (F7)
- ⬜ 7.5 Wave balance (iterative)

---

## Phase 8 — Pickups + upgrades ⬜

- ⬜ 8.1 Pickup pool + spawn
- ⬜ 8.2 Ammo + health pickups
- ⬜ 8.3 Upgrade system
- ⬜ 8.4 Visual feedback (floating text, particles)

---

## Phase 9 — Levels, menus, flow ⬜

- ⬜ 9.1 Main menu (Level + raygui)
- ⬜ 9.2 Arena selection
- ⬜ 9.3 Multiple arenas (3-4 layouts)
- ⬜ 9.4 Results screen
- ⬜ 9.5 Specific transitions (system already implemented)
- ⬜ 9.6 Pause screen
- ⬜ 9.7 Persistent settings via rini

---

## Phase 10 — Visual polish, audio, final UI ⬜

- ⬜ 10.1 Outline shader (optional)
- ⬜ 10.2 Particle system
- ⬜ 10.3 Screen shake
- ⬜ 10.4 Hit feedback (flash, damage numbers, knockback)
- ⬜ 10.5 Audio (raylib)
- ⬜ 10.6 Custom HUD: atlas, font, animations
- ⬜ 10.7 Custom menus

---

## Phase 11 — Handheld optimization (tentative) ⬜

Only if profiling shows it's needed for stable 30 FPS.

- ⬜ 11.1 Profile on real hardware
- ⬜ 11.2 Dynamic enemy cap
- ⬜ 11.3 Animation LOD
- ⬜ 11.4 Batching / instancing
- ⬜ 11.5 Simplified shader for handheld
- ⬜ 11.6 Reduced render resolution

---

## Phase 12 — Local multiplayer 2-4P (PC) ⬜

Shared screen (NOT split-screen). Common camera following the
centroid; distance scales with group spread.

- ⬜ 12.1 PlayerManager multi-slot
- ⬜ 12.2 Common camera (centroid + spread)
- ⬜ 12.3 AI multi-target
- ⬜ 12.4 Per-player HUD
- ⬜ 12.5 Multiplayer balance

---

## Dependency summary

```
Phase 0 ✅
  └→ Phase 1 (renderer + iso camera + cel shading) ✅
       └→ Phase 2 (physics) ✅
            └→ Phase 3 (player 8-dir + jump + strafe) ⏭
                 └→ Phase 4 (pistol + zombie = core loop)
                      └→ Phase 5 (score + waves)
                           └→ Phase 6 (arsenal)
                           └→ Phase 7 (enemies)
                                └→ Phase 8 (pickups)
                                     └→ Phase 9 (menus + flow)
                                          └→ Phase 10 (polish)
                                               └→ Phase 11 (handheld optim, tentative)
                                                    └→ Phase 12 (MP 2-4P PC)
```

Each phase ships a runnable executable.
