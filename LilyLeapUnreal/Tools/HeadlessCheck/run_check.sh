#!/bin/bash
# Compile Lily Leap's C++ against small stand-ins for the Unreal API and run a headless gameplay + audio test.
# This catches typos, type errors, crashes and logic bugs without installing Unreal Engine.
# It cannot prove the code matches the real engine's API; only a real Unreal build can.
#   Tools/HeadlessCheck/run_check.sh      (needs g++ with C++20)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/../../Source/LilyLeap"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/inc" "$WORK/src"
cp "$HERE/ue_mock.h" "$WORK/inc/"
for h in Camera/CameraActor.h Camera/CameraComponent.h Components/DirectionalLightComponent.h Components/InputComponent.h \
         Components/StaticMeshComponent.h Components/SynthComponent.h CoreMinimal.h Engine/Canvas.h Engine/DirectionalLight.h \
         Engine/Engine.h Engine/Font.h Engine/PostProcessVolume.h Engine/StaticMesh.h Engine/World.h GameFramework/Actor.h \
         GameFramework/GameModeBase.h GameFramework/HUD.h GameFramework/PlayerController.h GameFramework/SaveGame.h \
         InputCoreTypes.h Kismet/GameplayStatics.h Materials/MaterialInstanceDynamic.h Materials/MaterialInterface.h \
         Math/RotationMatrix.h Modules/ModuleManager.h; do
  mkdir -p "$WORK/inc/$(dirname "$h")"; printf '#pragma once\n#include "ue_mock.h"\n' > "$WORK/inc/$h"
done
cp "$SRC"/*.h "$SRC"/*.cpp "$WORK/src/"
cp "$HERE/test_gameplay.cpp" "$WORK/src/"
for g in "$WORK"/src/*.h; do : > "$WORK/inc/$(basename "${g%.h}").generated.h"; done
# Unreal's GENERATED_BODY() provides "Super"; add the equivalent typedef in the scratch copy.
python3 - "$WORK/src" <<'PY'
import sys, re, glob
for p in glob.glob(sys.argv[1] + '/*.h'):
    s = open(p, encoding='utf-8').read()
    s = re.sub(r'(class LILYLEAP_API (\w+) : public (\w+)\s*\{\s*GENERATED_BODY\(\))', lambda m: m.group(1) + f'\n\ttypedef {m.group(3)} Super;', s)
    open(p, 'w', encoding='utf-8').write(s)
PY
echo 'UEngine* GEngine; const FKey EKeys::Left{},EKeys::Right{},EKeys::A{},EKeys::D{},EKeys::SpaceBar{},EKeys::Up{},EKeys::W{},EKeys::Z{},EKeys::X{},EKeys::J{},EKeys::K{},EKeys::LeftShift{},EKeys::RightShift{},EKeys::Enter{},EKeys::Escape{},EKeys::P{};' > "$WORK/src/zz_globals.cpp"
cd "$WORK/src"
echo "Compiling (warnings are errors, including shadowed variables)..."
for f in *.cpp; do
  g++ -std=c++20 -fsyntax-only -Wall -Wextra -Wshadow -Werror -Wno-unused-parameter -I"$WORK/inc" -I. -include "$WORK/inc/ue_mock.h" "$f"
done
echo "Running the headless gameplay and audio test..."
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -Wno-unused-parameter -I"$WORK/inc" -I. -include "$WORK/inc/ue_mock.h" *.cpp -o lily_test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ./lily_test
