// Coconuts, hex orbs, pickups and the pooled effects system.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JungleTypes.h"
#include "JungleActors.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UPhysicalMaterial;
class UPrimitiveComponent;

/** A coconut from the launcher (arcs, explodes) or an enemy orb (flies straight at Lionel). */
UCLASS()
class AJungleProjectile : public AActor
{
	GENERATED_BODY()

public:
	AJungleProjectile();
	void Init(const FVector& Velocity, bool bCoconut, bool bBossOrb, float Damage, float SplashM);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() USceneComponent* Root = nullptr;
	UPROPERTY() UStaticMeshComponent* Mesh = nullptr;
	FVector Vel = FVector::ZeroVector;   // cm/s
	bool bNut = false;
	bool bBoss = false;
	float Dmg = 0.f;
	float Splash = 0.f;
	float Age = 0.f;
	bool bReady = false;
};

/** Ammo crate or mango. Drifts toward Lionel when he is close. */
UCLASS()
class AJunglePickup : public AActor
{
	GENERATED_BODY()

public:
	AJunglePickup();
	void Init(bool bInAmmo);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() USceneComponent* Root = nullptr;
	bool bAmmo = true;
	float T = 0.f;
};

/** Pooled particles, tracers and one roaming flash light. Sizes in cm. */
UCLASS()
class AJungleFX : public AActor
{
	GENERATED_BODY()

public:
	AJungleFX();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void Burst(const FVector& At, int32 Count, const FLinearColor& Color, float Speed, float Size, float Life, float Gravity = 1400.f, float Up = 0.f);
	void Tracer(const FVector& A, const FVector& B, const FLinearColor& Color, float Thick, float Life);
	void Flash(const FVector& At, const FLinearColor& Color, float Intensity, float Duration);
	/** Ejects a physics-simulated shell casing (cm, cm/s). */
	void Casing(const FVector& At, const FVector& Velocity, bool bShotgunShell);

private:
	struct FParticle { FVector P; FVector V; float Life = 0.f; float Max = 1.f; float Size = 0.f; float Grav = 0.f; FRotator Spin; };
	struct FTracer { float Life = 0.f; float Max = 1.f; float Thick = 1.f; };

	UPROPERTY() USceneComponent* Root = nullptr;
	UPROPERTY() TArray<UStaticMeshComponent*> PComps;
	UPROPERTY() TArray<UStaticMeshComponent*> TComps;
	UPROPERTY() UPointLightComponent* Light = nullptr;
	UPROPERTY() UPhysicalMaterial* Brass = nullptr;   // springy, low friction: casings bounce and skitter
	TMap<TWeakObjectPtr<UPrimitiveComponent>, int32> Clinks;
	float LastClink = 0.f;
	float Now = 0.f;

	UFUNCTION()
	void OnCasingHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	TArray<FParticle> Ps;
	TArray<FTracer> Ts;
	int32 PNext = 0;
	int32 TNext = 0;
	float LightT = 0.f;
	float LightDur = 1.f;
	float LightI = 0.f;
};
