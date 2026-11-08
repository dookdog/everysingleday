# Lily Leap: Unreal Engine 5 version

The frog platformer rebuilt in Unreal Engine 5, in C++. Hop across giant 3D flowers and lily pads, flick your tongue at buzzing flies, and eat every fly in the pond.

The whole game is created by code when it starts. There are no levels, models, textures or sound files to import:

- **World:** the pond, hills, clouds, flowers, lily pads, purple bellflowers, the frog and the flies are built from Unreal's built-in shapes (spheres, cubes, cylinders and cones).
- **Levels:** generated from a seed, with the same rules as the web version. Each pond is longer and has more, faster flies.
- **Music and sound:** synthesized live. The "Morning on the Pond" title waltz, the in-game tune and every sound effect.
- **Screens and controls:** the HUD draws the score, hearts, touch buttons and every menu.

> **Status: not yet compiled.** This was written without access to Unreal Engine, so it has never been built or run. Expect to fix a few compile errors the first time you build. The errors Unreal prints will point at the exact lines.

## What you need

| To play it on… | You need |
|---|---|
| A Mac or PC, inside the editor | Unreal Engine 5.4 or newer from the Epic Games Launcher. On Windows, also Visual Studio 2022 with "Game development with C++". On a Mac, also Xcode. |
| An iPhone | A **Mac** with Xcode, Unreal Engine 5.4+, and an Apple ID (a free one can install on your own phone for 7 days; the paid Apple Developer Program, $99/year, is needed for longer installs, TestFlight or the App Store). |

## Run it on your computer

1. Install Unreal Engine 5.4 or newer from the Epic Games Launcher.
2. Double-click `LilyLeap.uproject`. When asked to rebuild the `LilyLeap` module, choose **Yes**.
   - If that fails, right-click `LilyLeap.uproject`, choose **Generate Visual Studio project files** (Windows) or **Generate Xcode project** (Mac), open the generated project, and build the `LilyLeapEditor` target. Build errors show up there.
3. In the editor, press **Play**. The game builds the pond and shows the title screen.

Controls on a computer: arrow keys or A/D to walk, Space to hop (hold for higher), X to flick the tongue, Esc to pause. Clicking the mouse works like touching the screen, so you can try the touch buttons too.

## Put it on your iPhone

1. On the Mac, open Xcode once and sign in with your Apple ID (**Xcode → Settings → Accounts**).
2. Open `Config/DefaultEngine.ini` and change `BundleIdentifier=com.yourname.lilyleap` to something unique to you, for example `com.yourlastname.lilyleap`.
3. In the Unreal editor, open **Edit → Project Settings → Platforms → iOS**:
   - Turn on **Automatic Signing** and choose your team (your Apple ID).
4. Plug in your iPhone, unlock it and tap **Trust**. On the iPhone, turn on **Settings → Privacy & Security → Developer Mode** (it will restart).
5. In the editor toolbar, open the **Platforms** menu, choose **iOS**, then your iPhone, and choose **Launch**. The first build takes a long time.
6. The first time, the iPhone may block the app. Go to **Settings → General → VPN & Device Management**, tap your Apple ID and choose **Trust**.

To make an installable package instead: **Platforms → iOS → Package Project**.

## Where things are

| File | What it does |
|---|---|
| `Source/LilyLeap/LilyPond.cpp` | The game: level generation, frog physics, tongue, flies, scoring, menus' logic, camera and all 3D models |
| `Source/LilyLeap/LilySynth.cpp` | Music and sound effects, synthesized in real time |
| `Source/LilyLeap/LilyHUD.cpp` | Everything drawn on screen: score, hearts, touch buttons and menus |
| `Source/LilyLeap/LilyPlayerController.cpp` | Keyboard and touch input |
| `Source/LilyLeap/LilyTypes.h` | Tuning: jump height, speed, gravity, camera distance, light brightness and flower colours |
| `Source/LilyLeap/LilyArt.cpp` | Helper that builds coloured shapes |
| `Config/` | Project settings: start map, landscape-only on iPhone, app name and bundle ID |

## If it looks wrong

- **Too dark or too bright:** change `SunLux`, `FillLux` or `ExposureBias` in `LilyTypes.h`.
- **Camera too close or far:** change `CamDistance` or `CamFov` in `LilyTypes.h`.
- **Jumps feel off:** change `JumpV`, `Gravity` or `Run` in `LilyTypes.h`.
