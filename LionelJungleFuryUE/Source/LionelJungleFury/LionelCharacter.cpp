#include "LionelCharacter.h"
#include "JungleGameMode.h"
#include "JungleEnemy.h"
#include "JungleActors.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

namespace
{
	constexpr float SwitchTime = 0.32f;
	const FVector StartLoc(220.f, 0.f, 400.f);   // top of the temple, facing out over the jungle
}

ALionelCharacter::ALionelCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 75.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(100.f);

	ViewRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ViewRoot"));
	ViewRoot->SetupAttachment(Camera);
	ViewRoot->SetRelativeLocation(FVector(46.f, 22.f, -23.f));

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Camera);
	MuzzleLight->SetRelativeLocation(FVector(120.f, 20.f, -10.f));
	MuzzleLight->SetIntensityUnits(ELightUnits::Unitless);
	MuzzleLight->SetIntensity(0.f);
	MuzzleLight->SetAttenuationRadius(900.f);
	MuzzleLight->SetCastShadows(false);
	MuzzleLight->SetLightColor(FLinearColor(1.f, 0.7f, 0.35f));

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetMesh()->SetVisibility(false);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = 740.f;
	Move->JumpZVelocity = 840.f;
	Move->GravityScale = 2.4f;
	Move->AirControl = 0.6f;
	Move->BrakingDecelerationWalking = 4000.f;
	Move->GroundFriction = 10.f;
	Move->MaxStepHeight = 80.f;
	Move->SetWalkableFloorAngle(50.f);
}

void ALionelCharacter::BeginPlay()
{
	Super::BeginPlay();
	BuildViewmodels();
	ResetForGame();
}

void ALionelCharacter::ResetForGame()
{
	Health = 100.f;
	for (int32 i = 0; i < NumWeapons; ++i)
	{
		Gun[i].bOwned = i < 3;
		Gun[i].Mag = GWeapons[i].Mag;
		Gun[i].Res = GWeapons[i].Res;
	}
	Cur = Pend = 1;
	Cooldown = SwitchT = ReloadT = SwingT = VmKick = Kick = AppliedKick = Shake = MuzzleT = 0.f;
	bReloading = bPendReload = bFiring = false;
	LifeT = 0.f;
	SetActorLocation(StartLoc, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	if (AController* C = GetController())
	{
		C->SetControlRotation(FRotator(-5.f, 0.f, 0.f));
	}
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 75.f));
	ShowGun(Cur);
}

FVector ALionelCharacter::EyeLocation() const { return Camera->GetComponentLocation(); }
FVector ALionelCharacter::AimDir() const { return Camera->GetForwardVector(); }

FVector ALionelCharacter::MuzzleWorld() const
{
	return Camera->GetComponentTransform().TransformPosition(FVector(85.f, 20.f, -17.f));
}

// ============================================================================================
// Viewmodels: built in camera space (X forward, Y right, Z up), sizes in meters.
// ============================================================================================

void ALionelCharacter::BuildViewmodels()
{
	const FLinearColor Metal = JW::Hex(0x2a2e31), Steel = JW::Hex(0xb9c2c6), Wood = JW::Hex(0x7b4a26), Dark = JW::Hex(0x16191b),
		Gold = JW::Hex(0xd9a634), Sleeve = JW::Hex(0x4b5b2c), Skin = JW::Hex(0xc98d62), Bamboo = JW::Hex(0x9fb44c),
		RingC = JW::Hex(0x5b6b2a), Coco = JW::Hex(0x5b3a1e), Bone = JW::Hex(0xe8e0c8), Red = JW::Hex(0x9b2a22), Teal = JW::Hex(0x2f8a86),
		Glow = JW::Hex(0x9ffaff);
	const FRotator Barrel(90.f, 0.f, 0.f);

	auto NewGun = [&](const FVector& Muzzle)
	{
		USceneComponent* G = JW::Group(this, ViewRoot, FVector::ZeroVector);
		JW::Part(this, G, EShape::Cube, Skin, FVector(0.11f, 0.075f, 0.09f), FVector(-0.02f, 0.f, -0.03f));
		JW::Part(this, G, EShape::Cube, Sleeve, FVector(0.42f, 0.11f, 0.11f), FVector(-0.26f, 0.035f, -0.07f), FRotator(10.f, 0.f, 0.f));
		Guns.Add(G);
		Muzzles.Add(Muzzle);
		return G;
	};

	// 0 Machete
	{
		USceneComponent* G = NewGun(FVector(0.5f, 0.f, 0.f));
		Blade = JW::Group(this, G, FVector::ZeroVector, FRotator(14.f, -6.f, -20.f));
		JW::Part(this, Blade, EShape::Cube, Wood, FVector(0.16f, 0.045f, 0.05f), FVector(-0.02f, 0.f, 0.f));
		JW::Part(this, Blade, EShape::Cube, Dark, FVector(0.025f, 0.07f, 0.025f), FVector(0.065f, 0.f, 0.f));
		JW::Part(this, Blade, EShape::Cube, Steel, FVector(0.5f, 0.012f, 0.075f), FVector(0.32f, 0.f, 0.015f));
		JW::Part(this, Blade, EShape::Cube, Steel, FVector(0.12f, 0.012f, 0.05f), FVector(0.6f, 0.f, 0.03f), FRotator(28.f, 0.f, 0.f));
		JW::Part(this, Blade, EShape::Cube, Dark, FVector(0.45f, 0.014f, 0.02f), FVector(0.3f, 0.f, -0.02f));
	}
	// 1 Jaguar .45
	{
		USceneComponent* G = NewGun(FVector(0.27f, 0.f, 0.06f));
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.065f, 0.05f, 0.12f), FVector(0.f, 0.f, -0.02f), FRotator(-12.f, 0.f, 0.f));
		JW::Part(this, G, EShape::Cube, Metal, FVector(0.27f, 0.06f, 0.06f), FVector(0.1f, 0.f, 0.065f));
		JW::Part(this, G, EShape::Cube, Gold, FVector(0.21f, 0.062f, 0.016f), FVector(0.1f, 0.f, 0.098f));
		JW::Part(this, G, EShape::Cylinder, Steel, FVector(0.036f, 0.036f, 0.04f), FVector(0.245f, 0.f, 0.06f), Barrel);
		JW::Part(this, G, EShape::Cube, Gold, FVector(0.01f, 0.01f, 0.025f), FVector(0.21f, 0.f, 0.105f));
	}
	// 2 Monsoon SMG
	{
		USceneComponent* G = NewGun(FVector(0.5f, 0.f, 0.05f));
		JW::Part(this, G, EShape::Cube, Metal, FVector(0.36f, 0.08f, 0.09f), FVector(0.12f, 0.f, 0.05f));
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.06f, 0.05f, 0.17f), FVector(0.13f, 0.f, -0.07f), FRotator(-8.f, 0.f, 0.f));
		JW::Part(this, G, EShape::Cylinder, Steel, FVector(0.044f, 0.044f, 0.2f), FVector(0.38f, 0.f, 0.05f), Barrel);
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.2f, 0.05f, 0.06f), FVector(-0.12f, 0.f, 0.03f));
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.05f, 0.05f, 0.1f), FVector(-0.02f, 0.f, -0.03f), FRotator(-12.f, 0.f, 0.f));
		JW::Part(this, G, EShape::Cube, Sleeve, FVector(0.13f, 0.083f, 0.03f), FVector(0.1f, 0.f, 0.1f));
		JW::Part(this, G, EShape::Cube, Gold, FVector(0.02f, 0.015f, 0.04f), FVector(0.27f, 0.f, 0.11f));
	}
	// 3 Rhino Boomstick
	{
		USceneComponent* G = NewGun(FVector(0.64f, 0.f, 0.065f));
		JW::Part(this, G, EShape::Cylinder, Steel, FVector(0.052f, 0.052f, 0.62f), FVector(0.32f, 0.027f, 0.065f), Barrel);
		JW::Part(this, G, EShape::Cylinder, Steel, FVector(0.052f, 0.052f, 0.62f), FVector(0.32f, -0.027f, 0.065f), Barrel);
		JW::Part(this, G, EShape::Cube, Metal, FVector(0.18f, 0.1f, 0.085f), FVector(0.02f, 0.f, 0.05f));
		JW::Part(this, G, EShape::Cube, Wood, FVector(0.24f, 0.085f, 0.05f), FVector(0.27f, 0.f, 0.015f));
		JW::Part(this, G, EShape::Cube, Wood, FVector(0.26f, 0.075f, 0.11f), FVector(-0.15f, 0.f, -0.005f), FRotator(-10.f, 0.f, 0.f));
		JW::Part(this, G, EShape::Cube, Gold, FVector(0.05f, 0.102f, 0.02f), FVector(0.06f, 0.f, 0.07f));
	}
	// 4 Viper Crossbow
	{
		USceneComponent* G = NewGun(FVector(0.47f, 0.f, 0.08f));
		JW::Part(this, G, EShape::Cube, Wood, FVector(0.52f, 0.06f, 0.06f), FVector(0.15f, 0.f, 0.03f));
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.045f, 0.62f, 0.03f), FVector(0.38f, 0.f, 0.06f));
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.05f, 0.06f, 0.03f), FVector(0.35f, 0.31f, 0.06f), FRotator(0.f, 30.f, 0.f));
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.05f, 0.06f, 0.03f), FVector(0.35f, -0.31f, 0.06f), FRotator(0.f, -30.f, 0.f));
		JW::Part(this, G, EShape::Cube, Bone, FVector(0.006f, 0.56f, 0.006f), FVector(0.3f, 0.f, 0.065f));
		Bolt = JW::Part(this, G, EShape::Cube, Steel, FVector(0.42f, 0.012f, 0.012f), FVector(0.25f, 0.f, 0.08f));
		JW::Part(this, G, EShape::Cube, Teal, FVector(0.04f, 0.03f, 0.03f), FVector(0.06f, 0.f, 0.08f));
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.05f, 0.05f, 0.1f), FVector(-0.02f, 0.f, -0.04f), FRotator(-12.f, 0.f, 0.f));
	}
	// 5 Coconut Launcher
	{
		USceneComponent* G = NewGun(FVector(0.55f, 0.f, 0.06f));
		JW::Part(this, G, EShape::Cylinder, Bamboo, FVector(0.17f, 0.17f, 0.64f), FVector(0.2f, 0.f, 0.06f), Barrel);
		for (float X : { 0.45f, 0.22f, -0.04f })
		{
			JW::Part(this, G, EShape::Cylinder, RingC, FVector(0.18f, 0.18f, 0.03f), FVector(X, 0.f, 0.06f), Barrel);
		}
		Nut = JW::Part(this, G, EShape::Sphere, Coco, FVector(0.144f), FVector(0.52f, 0.f, 0.06f));
		JW::Part(this, G, EShape::Cube, Dark, FVector(0.05f, 0.05f, 0.12f), FVector(0.f, 0.f, -0.04f), FRotator(-12.f, 0.f, 0.f));
		JW::Part(this, G, EShape::Cube, Red, FVector(0.02f, 0.02f, 0.06f), FVector(0.3f, 0.f, 0.13f));
	}
	// 6 Storm Totem
	{
		USceneComponent* G = NewGun(FVector(0.62f, 0.f, 0.25f));
		JW::Part(this, G, EShape::Cylinder, Wood, FVector(0.048f, 0.048f, 0.8f), FVector(0.25f, 0.f, 0.12f), FRotator(72.f, 0.f, 0.f));
		Orb = JW::Part(this, G, EShape::Sphere, Glow, FVector(0.14f), FVector(0.62f, 0.f, 0.25f));
		Ring = JW::Part(this, G, EShape::Cylinder, Gold, FVector(0.22f, 0.22f, 0.012f), FVector(0.62f, 0.f, 0.25f));
		JW::Part(this, G, EShape::Cone, Red, FVector(0.04f, 0.04f, 0.12f), FVector(0.55f, 0.06f, 0.2f), FRotator(0.f, 0.f, 35.f));
		JW::Part(this, G, EShape::Cone, Teal, FVector(0.04f, 0.04f, 0.12f), FVector(0.55f, -0.06f, 0.2f), FRotator(0.f, 0.f, -35.f));
		JW::Part(this, G, EShape::Cylinder, Gold, FVector(0.07f, 0.07f, 0.05f), FVector(0.5f, 0.f, 0.19f));
	}

	Flash = JW::Part(this, ViewRoot, EShape::Sphere, JW::Hex(0xffe9a0), FVector(0.09f), FVector::ZeroVector);
	Flash->SetVisibility(false);

	TArray<UStaticMeshComponent*> Parts;
	GetComponents<UStaticMeshComponent>(Parts);
	for (UStaticMeshComponent* P : Parts)
	{
		P->SetCastShadow(false);
	}
}

void ALionelCharacter::ShowGun(int32 Index)
{
	for (int32 i = 0; i < Guns.Num(); ++i)
	{
		Guns[i]->SetVisibility(i == Index, true);
	}
	if (Flash && Guns.IsValidIndex(Index))
	{
		Flash->AttachToComponent(Guns[Index], FAttachmentTransformRules::KeepRelativeTransform);
		Flash->SetRelativeLocation((Muzzles[Index] + FVector(0.06f, 0.f, 0.f)) * JW::M);
		Flash->SetVisibility(false);
	}
}

// ============================================================================================
// Input entry points (called by the player controller)
// ============================================================================================

void ALionelCharacter::Look(float YawDeg, float PitchDeg)
{
	AController* C = GetController();
	if (!C) { return; }
	FRotator R = C->GetControlRotation();
	R.Yaw += YawDeg;
	R.Pitch = FMath::Clamp((float)FRotator::NormalizeAxis(R.Pitch + PitchDeg), -85.f, 85.f);
	C->SetControlRotation(R);
	SwayX += YawDeg * 0.08f;
	SwayY += PitchDeg * 0.08f;
}

void ALionelCharacter::DoJump() { Jump(); }

void ALionelCharacter::DoReload()
{
	const FWeaponDef& W = GWeapons[Cur];
	FGunState& S = Gun[Cur];
	if (W.Kind == EWeaponKind::Melee || W.Regen > 0.f || bReloading || SwitchT > 0.f) { return; }
	if (S.Mag >= W.Mag || S.Res == 0) { return; }
	bReloading = true;
	ReloadT = W.Reload;
	if (AJungleGameMode* GM = AJungleGameMode::Get(this)) { GM->Sfx(ESfx::Reload); }
}

void ALionelCharacter::Select(int32 Index)
{
	if (Index < 0 || Index >= NumWeapons || !Gun[Index].bOwned) { return; }
	if (Index == Cur && SwitchT <= 0.f) { return; }
	Pend = Index;
	SwitchT = SwitchTime;
	bReloading = false;
	bPendReload = false;
}

void ALionelCharacter::Cycle(int32 Dir)
{
	for (int32 k = 1; k <= NumWeapons; ++k)
	{
		const int32 i = ((Cur + Dir * k) % NumWeapons + NumWeapons) % NumWeapons;
		if (Gun[i].bOwned) { Select(i); return; }
	}
}

void ALionelCharacter::Hurt(float Damage)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (!GM || GM->State != EJState::Playing) { return; }
	Health -= Damage;
	GM->HurtFlash = FMath::Min(1.f, GM->HurtFlash + Damage / 26.f);
	Shake += 0.22f;
	GM->Sfx(ESfx::Hurt);
	if (Health <= 0.f)
	{
		Health = 0.f;
		bFiring = false;
		GM->GameOver();
	}
}

void ALionelCharacter::Resupply(float Mags)
{
	for (int32 i = 0; i < NumWeapons; ++i)
	{
		const FWeaponDef& W = GWeapons[i];
		if (Gun[i].bOwned && W.Max > 0)
		{
			Gun[i].Res = FMath::Min(W.Max, Gun[i].Res + FMath::CeilToInt(W.Mag * Mags));
		}
	}
}

// ============================================================================================
// Tick
// ============================================================================================

void ALionelCharacter::Tick(float Dt)
{
	Super::Tick(Dt);
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	AController* C = GetController();
	if (!GM || !C) { return; }

	if (GM->State == EJState::Title)
	{
		// Slow panorama of the jungle behind the title screen.
		FRotator R = C->GetControlRotation();
		R.Yaw += 6.f * Dt;
		R.Pitch = -8.f;
		C->SetControlRotation(R);
		ViewRoot->SetVisibility(false, true);
		return;
	}
	if (GM->State == EJState::Over)
	{
		const FVector Rel = Camera->GetRelativeLocation();
		Camera->SetRelativeLocation(FMath::VInterpTo(Rel, FVector(0.f, 0.f, -50.f), Dt, 3.f));
		ViewRoot->SetVisibility(false, true);
		return;
	}
	if (GM->State != EJState::Playing) { return; }

	if (!ViewRoot->IsVisible())
	{
		ViewRoot->SetVisibility(true, true);
		ShowGun(Cur);
	}
	LifeT += Dt;

	// Movement
	const FRotator Yaw(0.f, C->GetControlRotation().Yaw, 0.f);
	FVector2D Mv = MoveInput;
	if (Mv.SizeSquared() > 1.f) { Mv.Normalize(); }
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Mv.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Mv.X);
	MoveAmt = GetCharacterMovement()->IsMovingOnGround() ? FMath::Min(1.f, (float)Mv.Size()) : 0.f;
	const FVector L = GetActorLocation() / JW::M;
	GetCharacterMovement()->MaxWalkSpeed = JW::Hgt(L.X, L.Y) < JW::Water - 0.2f ? 460.f : 740.f;

	// Recoil climbs and settles back.
	Kick *= FMath::Exp(-Dt * 9.f);
	const float Delta = Kick - AppliedKick;
	AppliedKick = Kick;
	if (FMath::Abs(Delta) > KINDA_SMALL_NUMBER)
	{
		FRotator R = C->GetControlRotation();
		R.Pitch = FMath::Clamp((float)FRotator::NormalizeAxis(R.Pitch + FMath::RadiansToDegrees(Delta)), -85.f, 85.f);
		C->SetControlRotation(R);
	}

	// Screen shake
	Shake *= FMath::Exp(-Dt * 7.f);
	const float Sx = Shake > 0.01f ? JW::Rand(-1.f, 1.f) * Shake * 12.f : 0.f;
	const float Sz = Shake > 0.01f ? JW::Rand(-1.f, 1.f) * Shake * 12.f : 0.f;
	Camera->SetRelativeLocation(FVector(0.f, Sx, 75.f + Sz));

	UpdateWeapon(Dt);
	UpdateViewmodel(Dt);
}

void ALionelCharacter::UpdateWeapon(float Dt)
{
	Cooldown -= Dt;

	for (int32 i = 0; i < NumWeapons; ++i)
	{
		const FWeaponDef& W = GWeapons[i];
		const bool bJustUsed = (i == Cur && Cooldown > -0.4f);
		if (W.Regen > 0.f && Gun[i].bOwned && !bJustUsed)
		{
			Gun[i].Mag = FMath::Min<float>(W.Mag, Gun[i].Mag + W.Regen * Dt);
		}
	}

	if (SwitchT > 0.f)
	{
		SwitchT -= Dt;
		if (SwitchT <= SwitchTime / 2 && Cur != Pend)
		{
			Cur = Pend;
			ShowGun(Cur);
		}
		SwitchT = FMath::Max(0.f, SwitchT);
	}

	const FWeaponDef& W = GWeapons[Cur];
	FGunState& S = Gun[Cur];
	if (bReloading)
	{
		ReloadT -= Dt;
		if (ReloadT <= 0.f)
		{
			bReloading = false;
			const int32 Need = W.Mag - FMath::FloorToInt(S.Mag);
			const int32 Take = S.Res < 0 ? Need : FMath::Min(Need, S.Res);
			S.Mag += Take;
			if (S.Res >= 0) { S.Res -= Take; }
		}
	}
	if (bPendReload && Cooldown <= 0.f)
	{
		bPendReload = false;
		DoReload();
	}
	if (bFiring && SwitchT <= 0.f && !bReloading && Cooldown <= 0.f)
	{
		Fire();
	}
}

void ALionelCharacter::UpdateViewmodel(float Dt)
{
	const FWeaponDef& W = GWeapons[Cur];
	BobT += Dt * (4.f + MoveAmt * 7.f);
	VmKick *= FMath::Exp(-Dt * 13.f);
	SwayX *= FMath::Exp(-Dt * 8.f);
	SwayY *= FMath::Exp(-Dt * 8.f);

	const float Sw = SwitchT > 0.f ? (SwitchT > SwitchTime / 2 ? (SwitchTime - SwitchT) / (SwitchTime / 2) : SwitchT / (SwitchTime / 2)) : 0.f;
	const float Rl = bReloading && W.Reload > 0.f ? FMath::Sin(PI * (1.f - ReloadT / W.Reload)) : 0.f;

	ViewRoot->SetRelativeLocation(FVector(
		46.f - VmKick * 7.f,
		22.f + FMath::Sin(BobT) * 1.2f * MoveAmt - SwayX * 2.f,
		-23.f + FMath::Abs(FMath::Cos(BobT)) * 1.2f * MoveAmt - Sw * 35.f - Rl * 12.f + SwayY * 1.5f));
	ViewRoot->SetRelativeRotation(FRotator(
		FMath::RadiansToDegrees(VmKick * (W.Pellets > 1 ? 0.35f : 0.16f) + Rl * 0.6f - Sw * 0.5f),
		SwayX * 3.f,
		FMath::RadiansToDegrees(Rl * 0.35f)));

	if (Blade)
	{
		const float P = SwingT > 0.f ? 1.f - SwingT / 0.32f : 0.f;
		SwingT = FMath::Max(0.f, SwingT - Dt);
		const float K = FMath::Sin(P * PI);
		Blade->SetRelativeRotation(FRotator(14.f - K * 86.f, -6.f - K * 63.f, -20.f - K * 34.f));
	}
	if (Orb && Cur == 6)
	{
		const float E = Gun[6].Mag / GWeapons[6].Mag;
		Orb->SetRelativeScale3D(FVector(0.1f + 0.06f * E + FMath::Sin(LifeT * 9.f) * 0.012f));
		Ring->SetRelativeRotation(FRotator(LifeT * 115.f, LifeT * 170.f, 0.f));
	}
	if (Nut && Cur == 5) { Nut->SetVisibility(Gun[5].Mag >= 1.f && !bReloading); }
	if (Bolt && Cur == 4) { Bolt->SetVisibility(Gun[4].Mag >= 1.f && !bReloading); }

	if (MuzzleT > 0.f)
	{
		MuzzleT -= Dt;
		Flash->SetVisibility(true);
		Flash->SetRelativeScale3D(FVector(JW::Rand(0.07f, 0.13f)));
		MuzzleLight->SetIntensity(MuzzleT > 0.f ? 8000.f : 0.f);
	}
	else if (Flash->IsVisible())
	{
		Flash->SetVisibility(false);
		MuzzleLight->SetIntensity(0.f);
	}
}

// ============================================================================================
// Weapons
// ============================================================================================

void ALionelCharacter::Fire()
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (!GM) { return; }
	const FWeaponDef& W = GWeapons[Cur];
	FGunState& S = Gun[Cur];

	if (W.Kind != EWeaponKind::Melee)
	{
		if (S.Mag < 1.f)
		{
			if (W.Regen > 0.f) { GM->Sfx(ESfx::Empty); Cooldown = 0.3f; return; }
			if (S.Res != 0) { DoReload(); return; }
			GM->Sfx(ESfx::Empty);
			Cooldown = 0.3f;
			GM->Banner(TEXT("OUT OF AMMO"), TEXT("Switching to the Jaguar .45"), 1.2f);
			Select(1);
			return;
		}
		S.Mag -= 1.f;
	}

	Cooldown = W.Rate;
	Kick += W.Kick;
	VmKick = 1.f;
	GM->Sfx(W.Sfx);
	const FVector Aim = AimDir();

	switch (W.Kind)
	{
	case EWeaponKind::Melee:
		SwingT = 0.32f;
		Melee(W);
		break;
	case EWeaponKind::Ray:
		for (int32 i = 0; i < W.Pellets; ++i)
		{
			const FVector Dir = (Aim + FVector(JW::Rand(-1, 1), JW::Rand(-1, 1), JW::Rand(-1, 1)) * W.Spread).GetSafeNormal();
			ShootRay(Dir, W);
		}
		MuzzleT = 0.05f;
		break;
	case EWeaponKind::Proj:
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SP.Owner = this;
		const FVector Start = EyeLocation() + Aim * 80.f - FVector(0.f, 0.f, 15.f);
		if (AJungleProjectile* P = GetWorld()->SpawnActor<AJungleProjectile>(AJungleProjectile::StaticClass(), Start, FRotator::ZeroRotator, SP))
		{
			P->Init(Aim * 3100.f + FVector(0.f, 0.f, 350.f), true, false, W.Dmg, W.Splash);
		}
		MuzzleT = 0.04f;
		break;
	}
	case EWeaponKind::Chain:
		Chain(W);
		break;
	}

	if (W.Regen <= 0.f && W.Kind != EWeaponKind::Melee && S.Mag < 1.f && S.Res != 0)
	{
		bPendReload = true;
	}
}

void ALionelCharacter::Melee(const FWeaponDef& W)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	const FVector Me = GetActorLocation() / JW::M;
	const FVector Fwd = FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X);
	const float EyeZ = EyeLocation().Z / JW::M;
	bool bHit = false;
	const TArray<AJungleEnemy*> Snapshot = GM->Enemies;
	for (AJungleEnemy* E : Snapshot)
	{
		if (!E || !E->bAlive) { continue; }
		const FVector Ep = E->GetActorLocation() / JW::M;
		const FVector2D D(Ep.X - Me.X, Ep.Y - Me.Y);
		const float Dist = D.Size();
		if (Dist > W.Range + E->Rad) { continue; }
		if (Dist > 0.01f && FVector2D::DotProduct(D / Dist, FVector2D(Fwd.X, Fwd.Y)) < FMath::Cos(W.Arc)) { continue; }
		if (FMath::Abs(E->Center().Z / JW::M - EyeZ) > 2.6f) { continue; }
		E->TakeHit(W.Dmg, FVector(D.X, D.Y, 0.f).GetSafeNormal(), 5.f);
		GM->FX->Burst(E->Center(), 8, E->Col, 500.f, 15.f, 0.5f, 1200.f, 100.f);
		bHit = true;
	}
	if (bHit) { Shake += 0.12f; }
}

void ALionelCharacter::ShootRay(const FVector& Dir, const FWeaponDef& W)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	const FVector Start = EyeLocation();
	const FVector End = Start + Dir * 9500.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LionelShot), false, this);
	FVector Impact = End;

	for (int32 Pass = 0; Pass < 8; ++Pass)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) { break; }
		if (AJungleEnemy* E = Cast<AJungleEnemy>(Hit.GetActor()))
		{
			E->TakeHit(W.Dmg, Dir, W.bPierce ? 4.f : (W.Pellets > 1 ? 1.2f : 1.6f));
			GM->FX->Burst(Hit.ImpactPoint, W.Pellets > 1 ? 2 : 5, E->Col, 400.f, 12.f, 0.4f, 1200.f, 100.f);
			if (!W.bPierce) { Impact = Hit.ImpactPoint; break; }
			Params.AddIgnoredActor(E);
			continue;
		}
		Impact = Hit.ImpactPoint;
		GM->FX->Burst(Hit.ImpactPoint, W.Pellets > 1 ? 1 : 4, JW::Hex(0x7a6a48), 300.f, 10.f, 0.5f, 1200.f, 150.f);
		break;
	}
	GM->FX->Tracer(MuzzleWorld(), Impact, JW::Hex(W.Tracer), W.Thick * JW::M, W.bPierce ? 0.22f : 0.07f);
	GM->FX->Flash(MuzzleWorld(), JW::Hex(0xffb050), W.Pellets > 1 ? 14000.f : 7000.f, 0.06f);
}

void ALionelCharacter::Chain(const FWeaponDef& W)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	const FVector Eye = EyeLocation();
	const FVector Aim = AimDir();
	TArray<FVector> Pts;
	Pts.Add(MuzzleWorld());

	AJungleEnemy* Best = nullptr;
	float BestA = 0.34f;
	for (AJungleEnemy* E : GM->Enemies)
	{
		if (!E || !E->bAlive) { continue; }
		const FVector To = E->Center() - Eye;
		const float D = To.Size();
		if (D > 3600.f || D < 1.f) { continue; }
		const float A = FMath::Acos(FMath::Clamp((float)FVector::DotProduct(To / D, Aim), -1.f, 1.f));
		if (A >= BestA) { continue; }
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(StormLOS), false, this);
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Eye, E->Center(), ECC_Visibility, Params) && Hit.GetActor() != E;
		if (!bBlocked) { BestA = A; Best = E; }
	}

	if (!Best)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(StormMiss), false, this);
		const FVector End = Eye + Aim * 2000.f;
		Pts.Add(GetWorld()->LineTraceSingleByChannel(Hit, Eye, End, ECC_Visibility, Params) ? FVector(Hit.ImpactPoint) : End);
	}
	else
	{
		TSet<AJungleEnemy*> Seen;
		AJungleEnemy* CurE = Best;
		float Dmg = W.Dmg;
		for (int32 k = 0; k < 5 && CurE; ++k)
		{
			Seen.Add(CurE);
			const FVector C = CurE->Center();
			Pts.Add(C);
			CurE->TakeHit(Dmg, FVector::ZeroVector, 0.f);
			Dmg *= 0.82f;
			AJungleEnemy* Next = nullptr;
			float NextD = 1100.f;
			for (AJungleEnemy* E : GM->Enemies)
			{
				if (!E || !E->bAlive || Seen.Contains(E)) { continue; }
				const float D = FVector::Dist(E->Center(), C);
				if (D < NextD) { NextD = D; Next = E; }
			}
			CurE = Next;
		}
	}

	// Jagged bolts between each hop.
	for (int32 i = 0; i + 1 < Pts.Num(); ++i)
	{
		FVector A = Pts[i];
		const FVector B = Pts[i + 1];
		constexpr int32 Segs = 6;
		for (int32 s = 1; s <= Segs; ++s)
		{
			FVector P = FMath::Lerp(Pts[i], B, s / float(Segs));
			if (s < Segs) { P += FVector(JW::Rand(-45, 45), JW::Rand(-45, 45), JW::Rand(-45, 45)); }
			GM->FX->Tracer(A, P, JW::Hex(0x8ff8ff), 6.f, 0.16f);
			GM->FX->Tracer(A, P, FLinearColor::White, 2.f, 0.1f);
			A = P;
		}
		GM->FX->Burst(B, 6, JW::Hex(0x9ffaff), 500.f, 10.f, 0.35f, 400.f, 0.f);
	}
	GM->FX->Flash(Pts.Last(), JW::Hex(0x8ff8ff), 30000.f, 0.15f);
}
