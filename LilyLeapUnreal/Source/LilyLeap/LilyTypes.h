#pragma once

#include "CoreMinimal.h"

/**
 * Shared tuning and data types for Lily Leap.
 *
 * World layout: X runs left to right through the level, Z is up, and the water surface is Z = 0.
 * All gameplay happens in the X/Z plane at Y = 0. The camera sits at negative Y looking toward +Y,
 * so scenery placed at larger Y sits further back and parallaxes naturally.
 * Units are centimetres; the numbers match the web version of the game one-to-one.
 */
namespace LilyTune
{
	// Frog movement (cm, cm/s, cm/s^2)
	constexpr float Gravity  = 2300.f;
	constexpr float Run      = 330.f;
	constexpr float Accel    = 2600.f;
	constexpr float AirAccel = 1800.f;
	constexpr float Friction = 2600.f;
	constexpr float JumpV    = 860.f;
	constexpr float MaxFall  = 1150.f;
	constexpr float BounceV  = 1330.f;   // purple bellflowers
	constexpr float Foot     = 15.f;     // half-width of the frog's feet for landing checks
	constexpr float Reach    = 265.f;    // tongue auto-aim range

	// Camera
	constexpr float CamDistance = 1400.f;
	constexpr float CamFov      = 50.f;   // horizontal degrees
	constexpr float CamPitch    = -4.f;
	constexpr float CamRestZ    = 230.f;  // lowest camera height: keeps the water near the bottom of the screen

	// Lighting (lux) and exposure. Raise or lower these if the scene looks too dark or washed out.
	constexpr float SunLux       = 3.5f;
	constexpr float FillLux      = 1.3f;
	constexpr float ExposureBias = 0.f;
}

enum class ELilyState : uint8 { Title, Play, Paused, Clearing, Clear, Over };
enum class ELilyPlat : uint8 { Island, Flower, Pad, Bloom };
enum class ELilySfx : uint8 { Jump, Land, Tongue, Gulp, Gold, Bounce, Splash, Clear, Croak, Over, Sting, Power, Hop2 };
enum class ELilySong : uint8 { Title = 0, Game = 1 };

/** sRGB hex (0xRRGGBB) to a linear colour, so colours match the web version. */
inline FLinearColor LilyHex(uint32 Hex)
{
	return FLinearColor(FColor((Hex >> 16) & 255, (Hex >> 8) & 255, Hex & 255));
}

struct FLilyPalette
{
	FLinearColor Petal, Inner, Center;
};

inline const FLilyPalette& LilyFlowerPalette(int32 Index)
{
	static const FLilyPalette Palettes[] = {
		{ LilyHex(0xF28DB2), LilyHex(0xFCD0E0), LilyHex(0xF6C445) },  // pink cosmos
		{ LilyHex(0xF47C5C), LilyHex(0xFFC0A8), LilyHex(0x7A3E2B) },  // coral
		{ LilyHex(0xA98BE0), LilyHex(0xDCCFF8), LilyHex(0xF3D25B) },  // lavender aster
		{ LilyHex(0xF7D44C), LilyHex(0xFDEEB0), LilyHex(0xB5562A) },  // buttercup
		{ LilyHex(0xF5F1E8), LilyHex(0xFFFFFF), LilyHex(0xF2B632) },  // white daisy
		{ LilyHex(0xD9488B), LilyHex(0xF5A8CB), LilyHex(0xFFE066) },  // magenta
		{ LilyHex(0x6FB6E8), LilyHex(0xC7E5FA), LilyHex(0xF6E27A) },  // cornflower
	};
	constexpr int32 Num = UE_ARRAY_COUNT(Palettes);
	return Palettes[((Index % Num) + Num) % Num];
}
constexpr int32 LilyNumPalettes = 7;

/** Floating "+10" score text, drawn by the HUD at a world position. */
struct FLilyText
{
	FVector Pos = FVector::ZeroVector;
	FString Text;
	FLinearColor Color = FLinearColor::White;
	float Size = 1.f;
	float Life = 1.1f;
};
