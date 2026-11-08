// Thornlings, Mud Golems, Hex Masks and the Idol Colossus.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JungleTypes.h"
#include "JungleEnemy.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class AJungleEnemy : public AActor
{
	GENERATED_BODY()

public:
	AJungleEnemy();

	void Init(EEnemyType InType, int32 Wave);
	virtual void Tick(float DeltaSeconds) override;
	void TakeHit(float Damage, const FVector& Dir, float Knockback);

	/** World-space center of the hit sphere (cm). */
	FVector Center() const;

	EEnemyType Type = EEnemyType::Thorn;
	bool bAlive = true;
	bool bBoss = false;
	bool bFly = false;
	float HP = 1.f;
	float MaxHP = 1.f;
	float Hr = 0.7f;     // hit radius, m
	float Hy = 1.f;      // hit center height, m
	float Rad = 0.5f;    // body radius for crowding, m
	int32 Value = 100;
	FLinearColor Col = FLinearColor::Green;

private:
	UPROPERTY() USceneComponent* Root = nullptr;
	UPROPERTY() USceneComponent* Body = nullptr;
	UPROPERTY() USphereComponent* HitSphere = nullptr;
	UPROPERTY() TArray<UStaticMeshComponent*> FlashParts;
	UPROPERTY() TArray<UMaterialInterface*> FlashOrig;
	UPROPERTY() TArray<USceneComponent*> Legs;
	UPROPERTY() TArray<USceneComponent*> Arms;

	float Speed = 5.f;
	float Dmg = 8.f;
	float Range = 1.5f;
	float Cd = 1.f;
	float T = 0.f;
	float Atk = 0.5f;
	float Shoot = 2.f;
	float Summon = 8.f;
	float Flash = 0.f;
	bool bFlashOn = false;
	float Dying = 0.f;
	float Lunge = 0.f;
	FVector2D Pos = FVector2D::ZeroVector;   // meters
	FVector2D Knock = FVector2D::ZeroVector; // m/s
	float Z = 0.f;                           // meters

	void BuildThorn();
	void BuildBrute();
	void BuildMask();
	void BuildBoss();
	void FireOrb(const FVector2D& Offset);
	void SetFlash(bool bOn);
};
