# Lily Leap: Unreal Engine 5 version

The frog platformer as a real Unreal Engine 5 game in C++. Hop across giant 3D flowers and lily pads, flick your tongue at buzzing flies, and eat every fly in the pond.

- **The pond:** flowers, lily pads that drift and bob, and purple bellflowers that launch you sky-high.
- **Flies:** golden flies are worth 5x, and eating flies quickly builds combos.
- **Dragonfly:** one per pond. Eat it for 12 seconds of double jump.
- **Wasps:** they patrol from pond 2 on, and a sting costs a heart.
- **Time of day:** ponds cycle morning, noon, sunset and night. Night has stars, a moon, fireflies and glowing flies.
- **Music and sound:** synthesized live. A "Morning on the Pond" title waltz, an in-game tune and every sound effect.
- **Phone-ready:** on-screen touch buttons, menus, landscape-only on iPhone, and a saved best score and audio settings.

Everything is built by code when the game starts, from Unreal's built-in shapes. There are no models, textures, levels or sound files to import.

## How far it has been tested

| Check | Result |
|---|---|
| C++ compiles against stand-ins for the Unreal API, with every warning treated as an error | Passes |
| Headless run: ponds 1–8 generate and are jumpable; eating every fly clears a pond; the dragonfly, double jump, wasp stings, game over, pause, menus and the HUD all work; the music and sound effects render cleanly | Passes, with no crashes or memory errors |
| Built with the real Unreal Engine | **Not yet.** Unreal can't be installed where this was written. |

Run the checks again with `Tools/HeadlessCheck/run_check.sh`. They catch typos and logic bugs, but only a real Unreal build can confirm every engine call. If the first real build shows errors, copy them and they can be fixed quickly.

---

## Put it on your iPhone without owning a Mac

Apple only lets iPhone apps be built on a Mac. If you don't have one, rent one in the cloud and deliver the app to your phone through **TestFlight**, Apple's free app for installing test builds.

### What you need

- **The Apple Developer Program:** $99/year, at developer.apple.com/programs. It's needed for TestFlight.
- **A Mac with at least 150 GB free:** Unreal needs about 60 GB and Xcode about 40 GB.
  - **A borrowed Mac works best.** It skips steps 2 and 6, and the app can go straight onto your phone over a USB cable. A free Apple ID is enough that way, but the app stops opening after 7 days until you install it again.
  - **Or rent a cloud Mac.** Services such as **MacinCloud**, **Scaleway Apple silicon** and **AWS EC2 Mac** rent macOS machines by the hour or day, and you use them from your browser or a remote-desktop app. Prices and minimum rental times change, so check the current offer, and pick a plan with enough disk space.
- **A free Epic Games account,** to download Unreal Engine.

### 1. Join the Apple Developer Program

Enroll at developer.apple.com/programs. Approval can take a day or two.

When you're approved, open **developer.apple.com → Account → Membership details** and note your **Team ID**. It's 10 characters, like `AB12CD34EF`.

### 2. Rent a cloud Mac

Sign up with a cloud Mac service and start a Mac with **150 GB or more** of disk. Connect to it as the service describes.

Everything from step 3 onward happens on that Mac.

### 3. Install the tools on the Mac

1. Open the **App Store**, install **Xcode**, then open Xcode once and accept the license. It's large, so allow time.
2. In Xcode, open **Settings → Accounts**, click **+**, and sign in with your Apple Developer account.
3. Download the **Epic Games Launcher** from unrealengine.com and sign in.
4. In the launcher's **Unreal Engine** tab, install **5.4 or newer**. Before it starts, click **Options** and turn on **iOS**.

### 4. Get the project and set it up

1. On the Mac, open Safari and download this repository: **github.com/dookdog/everysingleday**, then **Code → Download ZIP**. Unzip it, and open the `LilyLeapUnreal` folder.
2. Open `Config/DefaultEngine.ini` in TextEdit:
   - Set `CodeSigningTeam=` to your Team ID, for example `CodeSigningTeam=AB12CD34EF`.
   - Change `BundleIdentifier=com.yourname.lilyleap` to something unique to you, for example `com.yourlastname.lilyleap`.
3. Save the file.

### 5. Build the app

1. Double-click `LilyLeap.uproject`. When asked to rebuild **LilyLeap**, click **Yes**. The first time takes a while.
2. If the editor opens, press **Play** to try the game on the Mac. Arrow keys walk, Space jumps and X flicks the tongue.
3. Close the editor, then double-click **`BuildForIPhone.command`**. If macOS blocks it, right-click it, choose **Open**, then **Open** again.
4. Wait. The first iPhone build can take an hour. The finished app appears in the `Packaged` folder.

If the build stops with errors, copy the red error lines and send them back for a fix.

### 6. Send it to your iPhone with TestFlight

1. Go to **appstoreconnect.apple.com → Apps → +** and choose **New App**.
   - **Platform:** iOS
   - **Name:** Lily Leap (if that name is taken, use another)
   - **Bundle ID:** pick the one you set in step 4. Building in step 5 registers it with Apple automatically; if it isn't listed yet, add it at developer.apple.com → Identifiers.
   - **SKU:** anything, for example `lilyleap`
2. Upload the build:
   - If the `Packaged` folder holds a **.ipa** file: install **Transporter** from the Mac App Store, sign in, and drag the .ipa into it.
   - If the build appears in **Xcode → Window → Organizer**: select it, then choose **Distribute App → App Store Connect → Upload**.
3. Wait about 10–30 minutes for Apple to process the build. In App Store Connect, open your app's **TestFlight** tab and add yourself under **Internal Testing**.
4. On your iPhone, install **TestFlight** from the App Store and open the invitation email. Tap **Install**.

Lily Leap is now on your home screen. You can stop renting the cloud Mac.

Rebuilding later: change the code, run step 5 again, then upload the new build. TestFlight updates the app on your phone.

---

## Using a borrowed Mac with a cable instead

You don't need TestFlight or the paid program for this:

1. Do steps 3 and 4 above, using your free Apple ID in Xcode and the Team ID that Xcode shows for it.
2. Plug in your iPhone, unlock it and tap **Trust**. Turn on **Settings → Privacy & Security → Developer Mode**; the phone restarts.
3. Open `LilyLeap.uproject`. From the toolbar, choose **Platforms → iOS → your iPhone → Launch**.
4. The first time, on the iPhone go to **Settings → General → VPN & Device Management**, tap your Apple ID and choose **Trust**.

With a free Apple ID the app stops opening after 7 days. Launch it from the Mac again to renew it.

---

## Where things are

| File | What it does |
|---|---|
| `Source/LilyLeap/LilyPond.cpp` | The game: level generation, frog physics, tongue, flies, dragonfly, wasps, time of day, scoring, camera and all 3D models |
| `Source/LilyLeap/LilySynth.cpp` | Music and sound effects, synthesized in real time |
| `Source/LilyLeap/LilyHUD.cpp` | Everything drawn on screen: score, hearts, Super hop timer, touch buttons, menus, stars and fireflies |
| `Source/LilyLeap/LilyPlayerController.cpp` | Keyboard and touch input |
| `Source/LilyLeap/LilyTypes.h` | Tuning: jump height, speed, gravity, camera, light brightness and flower colours |
| `Config/DefaultEngine.ini` | App name, bundle ID, Team ID, landscape-only, start map |
| `Build/IOS/Resources/` | The iPhone app icon |
| `BuildForIPhone.command` | One-step iPhone build on a Mac |
| `Tools/HeadlessCheck/` | The compile and gameplay checks that run without Unreal |

## If something looks off

- **Too dark or too bright:** change `SunLux`, `FillLux` or `ExposureBias` in `LilyTypes.h`.
- **Camera too close or far:** change `CamDistance` or `CamFov` in `LilyTypes.h`.
- **Jumps feel off:** change `JumpV`, `Gravity` or `Run` in `LilyTypes.h`.
