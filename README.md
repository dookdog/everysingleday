# Every Single Day — a Unity 2D Platformer

A complete, polished 2D platformer built in Unity. It ships with the
"game-feel" fundamentals that make a platformer feel *good* to play —
coyote time, jump buffering, variable jump height, double jump, wall
jumping, a smooth look-ahead camera and screen shake — plus enemies,
collectibles, hazards, moving platforms, checkpoints, lives, score, a
title screen, synthesized audio and a HUD.

The whole game is generated **procedurally at runtime with zero imported
art, audio or hand-authored scenes**, so you can clone, open in Unity,
press Play, and immediately be playing a real level with sound.

---

## How to run

**Unity version:** 2022.3 LTS (any 2022.3.x). Open the folder as a project.

Then either:

1. **Just press Play** in any scene (even the empty default one).
   `GameBootstrap` auto-spawns via `[RuntimeInitializeOnLoadMethod]` and
   builds the entire level, player, camera and HUD on the fly. A scene
   whose name contains "menu" boots the title screen instead. *(Simplest.)*

2. Or generate saved scenes first: menu **Every Single Day → Create All
   Scenes (Menu + Game)**, which creates `Assets/Scenes/MainMenu.unity` and
   `Assets/Scenes/Main.unity`, adds them to Build Settings in order (menu
   first), so a standalone build boots to the title screen.

## Title screen & audio

- A procedural **main-menu / title screen** (`MainMenuBootstrap`): animated
  drifting backdrop, pulsing title, **Play** / **Quit** buttons, the
  persistent high score, and `Enter` to start.
- **All sound effects and the background music are synthesized in code**
  (`SfxLibrary`) — jumps, coins, gems, stomps, turret shots, hurt/death
  stings, checkpoint/goal jingles, a game-over melody, UI clicks and a
  looping chiptune track. No audio files required; assign your own clips on
  any component to override the synthesized defaults.

## Controls

| Action       | Keys                    |
|--------------|-------------------------|
| Move         | `A` / `D` or `←` / `→`  |
| Jump         | `Space` (press again in air to **double-jump**) |
| Wall jump    | Hold toward a wall while falling, then `Space` |
| Pause        | `Esc`                   |
| Start (menu) | `Enter`                 |
| On end banner | `R` restart/next · `M` main menu |

Variable jump height: tap for a short hop, hold for a full jump.

## Gameplay

- Stomp enemies from above to defeat them (and bounce); touching them from
  the side costs health.
- Collect **coins** (100 = extra life) and **gems** for score.
- Avoid **spikes** and don't fall into pits — both cost a life.
- Dodge **turret** projectiles.
- Ride the **moving platform** across the big gap.
- Touch the **checkpoint flag** to set your respawn point.
- Reach the green **goal** to complete the level.

You start with 3 lives and 3 hearts. Lose all hearts and you respawn at
the last checkpoint; lose all lives and it's game over. High score is
saved between runs.

---

## Project structure

```
Assets/Scripts/
  Player/
    PlayerController.cs   — movement, jump feel, wall jump, double jump
    PlayerHealth.cs       — HP, i-frames, knockback, death/respawn
    PlayerAnimator.cs     — drives an Animator + SFX from controller events
  Enemies/
    PatrolEnemy.cs        — walks ledges/walls, stompable
    TurretEnemy.cs        — fires at the player in range/line-of-sight
    Projectile.cs         — travelling damage
  Collectibles/
    Collectible.cs        — coins / gems / health / extra-life pickups
  Level/
    MovingPlatform.cs     — waypoint platform that carries the player
    Hazard.cs             — spikes / lava (damage or instant kill)
    DeathZone.cs          — pit kill volume
    Checkpoint.cs         — sets respawn point
    LevelGoal.cs          — end-of-level trigger
    Parallax.cs           — multi-layer scrolling background
  Camera/
    CameraFollow.cs       — smooth follow + look-ahead + bounds
    CameraShake.cs        — global screen shake
  Systems/
    GameManager.cs        — score/coins/lives, pause, win/lose, persistence
    AudioManager.cs       — static one-shot SFX + looping music
    SfxLibrary.cs         — synthesizes every SFX + music in code (no files)
    PrimitiveFactory.cs   — generates sprites in code (no art needed)
    GameBootstrap.cs      — builds the whole playable level at runtime
    ParticleAutoDestroy.cs
  UI/
    MainMenuBootstrap.cs  — code-built title screen + scene flow
    RuntimeHUD.cs         — code-built HUD + win/lose/pause banners
    HUDController.cs       — TextMeshPro HUD variant (for authored scenes)
    MenuController.cs      — pause/win/gameover panel wiring
Assets/Editor/
    SceneBuilder.cs       — menu items to create/open menu + game scenes
```

All gameplay components are decoupled and reusable — drop them onto your
own prefabs and authored levels. `GameBootstrap` and `MainMenuBootstrap`
show exactly how each one is wired up, so they double as documentation.

## Designing your own levels

The components are plain `MonoBehaviour`s — build a scene by hand with
tilemaps/sprites and add:

- a `PlayerController` + `PlayerHealth` on your character (with child
  `GroundCheck`/`WallCheck` transforms and the ground `LayerMask` set),
- `CameraFollow` on the main camera,
- `PatrolEnemy`/`TurretEnemy`, `Collectible`, `Hazard`, `MovingPlatform`,
  `Checkpoint`, `LevelGoal`, `DeathZone` placed around the level,
- one `GameManager` and one `AudioManager` in the scene,
- a HUD (`RuntimeHUD.Create()` at runtime, or the TMP `HUDController`).

Tune jump feel on `PlayerController`: `jumpHeight`, `timeToApex`,
`fallGravityMultiplier`, `coyoteTime` and `jumpBufferTime` are the big
knobs. SFX play synthesized defaults automatically; assign `AudioClip`s in
the inspector to use your own.
