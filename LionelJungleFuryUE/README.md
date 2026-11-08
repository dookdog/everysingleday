# Lionel: Jungle Fury (Unreal Engine 5)

An Unreal Engine 5 C++ port of the browser game in `../lionel-jungle-fury/`. Lionel holds the Temple of the Golden Jaguar against waves of jungle creatures, with seven weapons.

The project has **no binary assets**. Everything is built from C++ when the game starts:

- the terrain, river, temple, trees, vines and ferns
- Lionel's weapons and every enemy, assembled from Unreal's built-in shapes
- the HUD and the touch controls, drawn on screen
- every sound effect and both music scores, from a synthesizer in `JungleSynthCore.h`

You don't need to import any assets or set up any levels. Open the project and press Play.

## The jungle

- **Golden-hour light.** A low warm sun throws long shadows and light shafts through the canopy.
- **Real-time volumetric fog.** Two layers: a green haze between the trees and a low mist pooling on the ground and river. Sunbeams, brazier fire, explosions and glowing flowers all light up the fog.
- **Flowers everywhere:**
  - meadows of jungle daisies in white, pink, yellow, violet and coral
  - tall red and orange heliconias, and hibiscus bushes in bloom
  - lily pads and pink lotus on the river, and orchids climbing the kapok trunks
  - blossoming pink, purple and orange trees, with fallen petals carpeting the ground beneath
  - glowing blue flowers ringing the temple
  - flowers overgrowing the temple steps
- **Ambient life.** Butterflies (blue morpho, monarch, sulphur, white, scarlet) flit between flower patches. Petals drift on the breeze and fireflies hover in the undergrowth. Thick grass covers the ground.
- **A cinematic camera grade.** Warm tones, rich saturation, bloom, a soft vignette, a light lens flare and film grain.

## Real-time physics

- **Props.** Supply crates, terracotta urns, fallen temple blocks and coconuts all simulate. Shoot them and they tumble; light objects fly and heavy stone barely moves. Coconut explosions throw everything nearby. Mud golems and the Colossus shove crates aside as they walk, and Lionel can kick things with the machete or by walking into them.
- **Enemies shatter.** When an enemy dies, its body breaks into real physics pieces flung away from the killing blow. The pieces bounce down the temple steps and can be shot again.
- **Shell casings.** Every pistol, SMG and shotgun shot ejects a brass casing (a red shell for the shotgun) that bounces and rolls on the stone.

## What's in the game

- **Weapons.** You start with the Machete, Jaguar .45 and Monsoon SMG. Clearing waves 1 to 4 unlocks the Rhino Boomstick, Viper Crossbow, Coconut Launcher and Storm Totem.
- **Enemies.**
  - Thornlings: fast melee.
  - Mud Golems: slow and heavy.
  - Hex Masks: flying, ranged.
  - The Idol Colossus: a boss every fifth wave that fires volleys and summons thornlings.
- **Pickups.** Ammo crates and mangoes, which restore health.
- **Screens.** Title, pause (with aim-speed control) and game over. Best score and settings are saved.
- **Music.** A composed flute intro theme, and a combat score that changes between waves, during fights and in boss fights.

## Requirements

- **Unreal Engine 5.4 or newer**, installed from the Epic Games Launcher.
- **A C++ compiler:**
  - Windows: Visual Studio 2022 with the "Game development with C++" workload.
  - Mac: Xcode.

## Open and play

1. Right-click `LionelJungleFury.uproject` and choose **Generate Visual Studio project files** (on Mac: **Services → Generate Xcode Project**).
2. Double-click `LionelJungleFury.uproject` and answer **Yes** when asked to build the missing modules.
   - If you use a newer engine than 5.4, right-click the `.uproject` file, choose **Switch Unreal Engine version**, then try again.
3. When the editor opens, press **Play**. If the editor viewport looks empty, use **Play → Standalone Game**, because the jungle is built when the game starts.

If the automatic build fails, open the generated `.sln` (or `.xcworkspace`), build the `LionelJungleFuryEditor` target in `Development Editor`, and read the first error in the output.

## Controls

| | Keyboard and mouse | Gamepad | iPhone |
|---|---|---|---|
| Move | WASD or arrows | Left stick | Analog thumbstick: touch anywhere on the left side. It has a dead zone, and the base follows your thumb |
| Sprint | Hold Shift | Click left stick | Push the stick forward past the ring (it turns gold) |
| Look | Mouse | Right stick | Drag on the right side. Smoothed, with faster turns for quick flicks and gentle aim assist |
| Fire | Left click | Right trigger | Hold **FIRE** (right) and slide it to aim while firing, or use the left-thumb **FIRE** next to the stick |
| Aim down sights | Hold right mouse | Left trigger | Tap **AIM**: zooms in, tightens spread, cuts recoil |
| Jump | Space | A | **JUMP** |
| Reload | R | X | **RELOAD** |
| Weapons | 1–7, mouse wheel, Q/E | Bumpers | Tap a slot in the weapon bar |
| Pause | P or Esc | Menu | **II** button |

In the editor, Esc ends the play session, so use **P** to pause.

**Shell ejection.** The pistol, SMG and shotgun each kick a casing out of their ejection port with a brass glint and a puff of smoke. As it clears the gun, the casing becomes a real physics body with bouncy brass. It skitters across the stone, rolls down the temple steps and clinks on impact.

## Running it on an iPhone

Unreal can't make an iPhone game that opens without installing. To get it onto a phone you need all of these:

- A **Mac** with Xcode.
- An **Apple Developer account** (the free tier works for your own phone; TestFlight needs the paid tier).
- The **iOS platform support** for your engine version, from the Epic Launcher's engine options.

Then:

1. Open **Project Settings → Platforms → iOS**.
2. Change the **Bundle Identifier** from `com.example.lioneljunglefury` to your own, for example `com.yourname.lionel`.
3. Sign in with your Apple team, or import your provisioning profile and certificate.
4. Connect the iPhone and choose **Platforms → iOS → Package Project**. Alternatively, launch straight to the device from the Platforms menu.

The game is set to landscape only. The HUD keeps clear of the notch and the home indicator.

## Where to tweak

| What | Where |
|---|---|
| Weapon damage, fire rate, magazine sizes | `GWeapons` table in `JungleTypes.cpp` |
| Enemy health, speed, damage | `Build*` functions in `JungleEnemy.cpp` |
| Wave sizes, unlocks, drops | `StartWave`, `WaveClear` and `EnemyKilled` in `JungleGameMode.cpp` |
| Fog thickness, sun angle and color, ground mist | `BuildSky` in `JungleWorld.cpp` |
| How many flowers, grass and butterflies | `BuildFlowers`, `BuildJungle` and `BuildLife` in `JungleWorld.cpp` (`Density` scales them all; it is 0.35 on phones) |
| Physics props, masses, push strength | `BuildProps` in `JungleWorld.cpp`, `SpawnProp` and `Explode` in `JungleGameMode.cpp` |
| Color grade, bloom, vignette | Constructor of `LionelCharacter.cpp` |
| Movement feel | Constructor of `LionelCharacter.cpp` |
| Music and sound effects | `JungleSynthCore.h` |
| HUD layout | `JungleHUD.cpp` |

## Source map

| File | Role |
|---|---|
| `JungleTypes.*` | Weapon table, terrain math, shape and material helpers |
| `JungleWorld.*` | Sky, fog, terrain, temple, trees, flowers, physics props, butterflies and petals |
| `JungleGameMode.*` | Runs waves, score, pause and game over; spawns physics bodies and explosions |
| `LionelCharacter.*` | First-person movement, weapons, viewmodels, recoil |
| `LionelPlayerController.*` | Touch twin-stick, keyboard, mouse, gamepad, menu taps |
| `JungleEnemy.*` | The four enemy types and their AI |
| `JungleActors.*` | Coconuts, enemy orbs, pickups, particles, tracers, flashes |
| `JungleHUD.*` | All on-screen UI |
| `JungleAudio.*`, `JungleSynthCore.h` | Real-time synthesizer streamed into Unreal |

## Testing status

- **The synthesizer was compiled and run outside Unreal.** It rendered the intro, combat and boss music and every sound effect with no clipping, at about 0.5% of one CPU core.
- **The game code was checked with a stand-in for the Unreal API.** Every source file was compiled with strict warnings against that stand-in, which catches typos, wrong names and C++ mistakes.
- **It has not been compiled against the real engine.** It was written in a cloud environment without Unreal Engine installed. The first build on your machine may report small API differences, especially between engine versions. If it does, the first error message is usually enough to fix it.
- **Visual tuning is a best first guess.** Fog density, light intensity, flower counts and UI sizes may need adjusting by eye.
- **Performance.** On a desktop GPU the flower and grass counts are high: about 60,000 instanced pieces with distance culling. If the frame rate drops, lower `Density` in `JungleWorld.cpp`. Volumetric fog and the glowing-flower lights are switched off on phones.
