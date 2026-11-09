#!/bin/bash
# Lily Leap: build the iPhone app on a Mac (your own, a borrowed one, or a rented cloud Mac).
# Double-click this file in Finder, or run it in Terminal:  ./BuildForIPhone.command
# Needs: Xcode, Unreal Engine 5.4+ with iOS support, and the Team ID set in Config/DefaultEngine.ini.
set -euo pipefail
cd "$(dirname "$0")"
PROJECT="$PWD/LilyLeap.uproject"
OUT="$PWD/Packaged"

# Find Unreal Engine: UE_ROOT if set, otherwise the newest UE_5.x the Epic Games Launcher installed.
if [ -z "${UE_ROOT:-}" ]; then
  UE_ROOT=$(ls -d "/Users/Shared/Epic Games/UE_5."* 2>/dev/null | sort -V | tail -1 || true)
fi
if [ -z "$UE_ROOT" ] || [ ! -x "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" ]; then
  echo "Could not find Unreal Engine 5. Install it with the Epic Games Launcher (include iOS support),"
  echo "or run:  UE_ROOT=\"/path/to/UE_5.x\" ./BuildForIPhone.command"
  exit 1
fi
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Xcode is not set up. Install Xcode from the App Store, open it once, then run this again."
  exit 1
fi
if ! grep -q '^CodeSigningTeam=..*' Config/DefaultEngine.ini; then
  echo "Set your Apple Team ID first: open Config/DefaultEngine.ini and fill in CodeSigningTeam="
  exit 1
fi

echo "Using $UE_ROOT"
echo "Building Lily Leap for iPhone (the first build can take an hour)..."
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$PROJECT" -noP4 -utf8output \
  -platform=IOS -clientconfig=Shipping \
  -build -cook -stage -pak -iostore -package -distribution \
  -archive -archivedirectory="$OUT"

echo
echo "Done. Your app is in: $OUT"
echo "Next: upload it to TestFlight (see README.md, step 6)."
open "$OUT" || true
