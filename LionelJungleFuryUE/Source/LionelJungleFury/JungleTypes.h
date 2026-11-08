// Lionel: Jungle Fury - shared types, tuning tables and the procedural-shape toolkit.
// Every model in the game is assembled at runtime from the engine's BasicShapes, so the
// project ships with no binary assets of its own.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JungleTypes.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class USceneComponent;

enum class EShape : uint8 { Cube, Sphere, Cylinder, Cone };
enum class EJState : uint8 { Title, Playing, Paused, Over };
enum class EEnemyType : uint8 { Thorn, Brute, Mask, Boss };
enum class EWeaponKind : uint8 { Melee, Ray, Proj, Chain };
enum class ESfx : uint8
{
	Slash, Pistol, Smg, Shotgun, Crossbow, Launcher, Boom, Zap, Hit, Kill, Hurt,
	Pickup, Reload, Empty, EnemyShot, Roar, Unlock, Death, Chirp
};

/** Static tuning for one weapon. Distances are meters, times are seconds. Res/Max of -1 = unlimited. */
struct FWeaponDef
{
	const TCHAR* Short;
	const TCHAR* Full;
	EWeaponKind Kind;
	float Dmg;
	float Rate;
	float Spread;
	int32 Mag;
	int32 Res;
	int32 Max;
	float Reload;
	float Kick;
	ESfx Sfx;
	uint32 Tracer;
	float Thick;
	int32 Pellets;
	bool bPierce;
	float Range;
	float Arc;
	float Splash;
	float Regen;
};

constexpr int32 NumWeapons = 7;
extern const FWeaponDef GWeapons[NumWeapons];

/** World math shared by the terrain builder, enemies, pickups and particles. Units: meters, Z up. */
namespace JW
{
	constexpr float M = 100.f;          // meters -> Unreal units (cm)
	constexpr float Water = -1.7f;      // water surface height
	constexpr float Arena = 86.f;       // playable radius
	constexpr float Tier = 0.75f;       // temple step height
	extern const float Pyr[4];          // temple tier half-sizes

	float BaseH(float X, float Y);
	float GroundH(float X, float Y);    // natural terrain, flattened around the temple
	float Hgt(float X, float Y);        // terrain + temple steps
	float FloorH(float X, float Y);     // what a walker stands on (wading depth clamp)
	float Rand(float A, float B);
	FLinearColor Hex(uint32 C);

	/** Adds a colored BasicShape part. Size is in meters (cube/sphere: full extents; cylinder/cone: diameter, diameter, height). */
	UStaticMeshComponent* Part(AActor* Owner, USceneComponent* Parent, EShape Shape, const FLinearColor& Color,
		const FVector& SizeM, const FVector& PosM, const FRotator& Rot = FRotator::ZeroRotator, bool bCollide = false);

	/** Adds an empty pivot for animating a group of parts. */
	USceneComponent* Group(AActor* Owner, USceneComponent* Parent, const FVector& PosM, const FRotator& Rot = FRotator::ZeroRotator);
}

/** Holds the engine shapes and a cache of colored material instances for the whole game. */
UCLASS()
class UJungleAssets : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UJungleAssets* Get(const UObject* WorldContext);

	UStaticMesh* Mesh(EShape Shape) const;
	UMaterialInstanceDynamic* Mat(const FLinearColor& Color);

	UPROPERTY() UStaticMesh* Cube = nullptr;
	UPROPERTY() UStaticMesh* Sphere = nullptr;
	UPROPERTY() UStaticMesh* Cylinder = nullptr;
	UPROPERTY() UStaticMesh* Cone = nullptr;
	UPROPERTY() UMaterialInterface* BaseMat = nullptr;

private:
	UPROPERTY() TMap<uint32, UMaterialInstanceDynamic*> Mats;
};
