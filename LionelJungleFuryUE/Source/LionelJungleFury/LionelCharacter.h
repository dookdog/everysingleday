// Lionel: first-person movement, the seven-weapon arsenal and its viewmodels.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JungleTypes.h"
#include "LionelCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class USceneComponent;
class UPointLightComponent;

UCLASS()
class ALionelCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALionelCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	struct FGunState
	{
		bool bOwned = false;
		float Mag = 0.f;
		int32 Res = 0;   // -1 = unlimited
	};

	float Health = 100.f;
	int32 Cur = 1;
	FGunState Gun[NumWeapons];

	/** Set every frame by the player controller. X = strafe, Y = forward. */
	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bFiring = false;

	void ResetForGame();
	void Look(float YawDeg, float PitchDeg);
	void DoJump();
	void DoReload();
	void Select(int32 Index);
	void Cycle(int32 Dir);
	void Hurt(float Damage);
	void Resupply(float Mags);
	void AddShake(float Amount) { Shake += Amount; }

	bool IsReloading() const { return bReloading; }
	FVector EyeLocation() const;

	UPROPERTY(VisibleAnywhere) UCameraComponent* Camera = nullptr;

private:
	UPROPERTY() USceneComponent* ViewRoot = nullptr;
	UPROPERTY() TArray<USceneComponent*> Guns;
	UPROPERTY() USceneComponent* Blade = nullptr;
	UPROPERTY() UStaticMeshComponent* Nut = nullptr;
	UPROPERTY() UStaticMeshComponent* Bolt = nullptr;
	UPROPERTY() UStaticMeshComponent* Orb = nullptr;
	UPROPERTY() UStaticMeshComponent* Ring = nullptr;
	UPROPERTY() UStaticMeshComponent* Flash = nullptr;
	UPROPERTY() UPointLightComponent* MuzzleLight = nullptr;
	TArray<FVector> Muzzles;

	int32 Pend = 1;
	float Cooldown = 0.f;
	float SwitchT = 0.f;
	float ReloadT = 0.f;
	float SwingT = 0.f;
	float VmKick = 0.f;
	float BobT = 0.f;
	float MoveAmt = 0.f;
	float Kick = 0.f;
	float AppliedKick = 0.f;
	float Shake = 0.f;
	float MuzzleT = 0.f;
	float SwayX = 0.f;
	float SwayY = 0.f;
	float LifeT = 0.f;
	bool bReloading = false;
	bool bPendReload = false;

	void BuildViewmodels();
	void ShowGun(int32 Index);
	void UpdateWeapon(float Dt);
	void UpdateViewmodel(float Dt);
	void Fire();
	void Melee(const FWeaponDef& W);
	void ShootRay(const FVector& Dir, const FWeaponDef& W);
	void Chain(const FWeaponDef& W);
	FVector AimDir() const;
	FVector MuzzleWorld() const;
};
