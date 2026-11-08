#include "JungleEnemy.h"
#include "JungleGameMode.h"
#include "JungleActors.h"
#include "LionelCharacter.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

namespace
{
	const float HitPad = (PLATFORM_IOS || PLATFORM_ANDROID) ? 1.35f : 1.1f;   // friendlier hitboxes on touch screens
}

AJungleEnemy::AJungleEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);

	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
	HitSphere->SetupAttachment(Root);
	HitSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitSphere->SetCollisionObjectType(ECC_WorldDynamic);
	HitSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	HitSphere->SetGenerateOverlapEvents(false);
}

void AJungleEnemy::Init(EEnemyType InType, int32 Wave)
{
	Type = InType;
	switch (Type)
	{
	case EEnemyType::Thorn: BuildThorn(); break;
	case EEnemyType::Brute: BuildBrute(); break;
	case EEnemyType::Mask:  BuildMask();  break;
	case EEnemyType::Boss:  BuildBoss();  break;
	}
	const float Mult = 1.f + 0.11f * (Wave - 1) + (bBoss ? 0.25f * FMath::Max(0, Wave / 5 - 1) : 0.f);
	HP = MaxHP = HP * Mult;
	Speed *= 1.f + FMath::Min(0.35f, 0.025f * Wave);
	Atk = JW::Rand(0.3f, 1.f);
	Shoot = JW::Rand(1.2f, 2.6f);
	T = JW::Rand(0.f, 10.f);

	HitSphere->SetSphereRadius(Hr * JW::M * HitPad);
	HitSphere->SetRelativeLocation(FVector(0.f, 0.f, Hy * JW::M));

	const FVector L = GetActorLocation() / JW::M;
	Pos = FVector2D(L.X, L.Y);
	Z = L.Z;

	TArray<UStaticMeshComponent*> Parts;
	GetComponents<UStaticMeshComponent>(Parts);
	for (UStaticMeshComponent* P : Parts)
	{
		FlashParts.Add(P);
		FlashOrig.Add(P->GetMaterial(0));
	}
}

FVector AJungleEnemy::Center() const
{
	return GetActorLocation() + FVector(0.f, 0.f, Hy * JW::M);
}

// Models face +X. Sizes in meters.
void AJungleEnemy::BuildThorn()
{
	const FLinearColor Bd = JW::Hex(0x4f8a2b), Sp = JW::Hex(0xa8322a), Eye = JW::Hex(0xffe14a);
	JW::Part(this, Body, EShape::Sphere, Bd, FVector(1.0f, 1.1f, 1.44f), FVector(0.f, 0.f, 0.9f));
	JW::Part(this, Body, EShape::Cone, Sp, FVector(0.24f, 0.24f, 0.42f), FVector(0.f, 0.f, 1.65f));
	for (int32 k = 0; k < 6; ++k)
	{
		const float A = k / 6.f * 2.f * PI;
		const FVector Out(FMath::Cos(A), FMath::Sin(A), 0.4f);
		JW::Part(this, Body, EShape::Cone, Sp, FVector(0.16f, 0.16f, 0.36f), FVector(Out.X * 0.42f, Out.Y * 0.45f, 0.95f), FRotationMatrix::MakeFromZ(Out.GetSafeNormal()).Rotator());
	}
	JW::Part(this, Body, EShape::Sphere, Eye, FVector(0.16f), FVector(0.42f, 0.17f, 1.05f));
	JW::Part(this, Body, EShape::Sphere, Eye, FVector(0.16f), FVector(0.42f, -0.17f, 1.05f));
	JW::Part(this, Body, EShape::Cube, JW::Hex(0x2a0a05), FVector(0.05f, 0.3f, 0.06f), FVector(0.46f, 0.f, 0.82f));
	Legs.Add(JW::Part(this, Body, EShape::Cube, Bd, FVector(0.12f, 0.12f, 0.5f), FVector(0.f, -0.2f, 0.25f)));
	Legs.Add(JW::Part(this, Body, EShape::Cube, Bd, FVector(0.12f, 0.12f, 0.5f), FVector(0.f, 0.2f, 0.25f)));
	Hr = 0.68f; Hy = 0.95f; HP = 35.f; Speed = 6.3f; Dmg = 9.f; Range = 1.5f; Cd = 0.75f; Value = 100; Rad = 0.5f; Col = JW::Hex(0x9be04a);
}

void AJungleEnemy::BuildBrute()
{
	const FLinearColor Bd = JW::Hex(0x6b4a2e), Moss = JW::Hex(0x3f6a2a), Eye = JW::Hex(0xff8a1e), Tusk = JW::Hex(0xe8dcc0);
	JW::Part(this, Body, EShape::Cube, Bd, FVector(1.0f, 1.5f, 1.5f), FVector(0.f, 0.f, 1.65f));
	JW::Part(this, Body, EShape::Cube, Bd, FVector(0.7f, 0.8f, 0.7f), FVector(0.12f, 0.f, 2.75f));
	JW::Part(this, Body, EShape::Cube, Moss, FVector(1.15f, 1.7f, 0.28f), FVector(0.f, 0.f, 2.45f));
	JW::Part(this, Body, EShape::Cube, Eye, FVector(0.05f, 0.16f, 0.1f), FVector(0.48f, 0.2f, 2.82f));
	JW::Part(this, Body, EShape::Cube, Eye, FVector(0.05f, 0.16f, 0.1f), FVector(0.48f, -0.2f, 2.82f));
	JW::Part(this, Body, EShape::Cone, Tusk, FVector(0.14f, 0.14f, 0.3f), FVector(0.5f, 0.25f, 2.5f), FRotator(-25.f, 0.f, 0.f));
	JW::Part(this, Body, EShape::Cone, Tusk, FVector(0.14f, 0.14f, 0.3f), FVector(0.5f, -0.25f, 2.5f), FRotator(-25.f, 0.f, 0.f));
	for (float Side : { 1.f, -1.f })
	{
		USceneComponent* A = JW::Group(this, Body, FVector(0.f, 1.0f * Side, 2.3f));
		JW::Part(this, A, EShape::Cube, Bd, FVector(0.45f, 0.45f, 1.6f), FVector(0.f, 0.f, -0.75f));
		JW::Part(this, A, EShape::Cube, Moss, FVector(0.5f, 0.5f, 0.2f), FVector(0.f, 0.f, -0.05f));
		Arms.Add(A);
		Legs.Add(JW::Part(this, Body, EShape::Cube, Bd, FVector(0.55f, 0.55f, 0.95f), FVector(0.f, 0.4f * Side, 0.47f)));
	}
	Hr = 1.2f; Hy = 1.8f; HP = 170.f; Speed = 2.7f; Dmg = 22.f; Range = 2.5f; Cd = 1.3f; Value = 300; Rad = 1.f; Col = JW::Hex(0x7a5a34);
}

void AJungleEnemy::BuildMask()
{
	const FLinearColor Wood = JW::Hex(0x8b5a2b), Red = JW::Hex(0xb3302a), White = JW::Hex(0xe6dcc4), Eye = JW::Hex(0xd36bff);
	const FLinearColor Feather[3] = { JW::Hex(0xc23a2a), JW::Hex(0x2f9a8e), JW::Hex(0xe0b22e) };
	JW::Part(this, Body, EShape::Cube, Wood, FVector(0.2f, 0.95f, 1.25f), FVector::ZeroVector);
	JW::Part(this, Body, EShape::Cube, Red, FVector(0.22f, 0.97f, 0.12f), FVector(0.f, 0.f, 0.3f));
	JW::Part(this, Body, EShape::Cube, White, FVector(0.22f, 0.97f, 0.08f), FVector(0.f, 0.f, -0.32f));
	JW::Part(this, Body, EShape::Cube, Eye, FVector(0.05f, 0.22f, 0.12f), FVector(0.11f, 0.22f, 0.12f));
	JW::Part(this, Body, EShape::Cube, Eye, FVector(0.05f, 0.22f, 0.12f), FVector(0.11f, -0.22f, 0.12f));
	JW::Part(this, Body, EShape::Cube, JW::Hex(0x1a0806), FVector(0.05f, 0.4f, 0.14f), FVector(0.11f, 0.f, -0.18f));
	JW::Part(this, Body, EShape::Cube, Wood, FVector(0.25f, 0.18f, 0.3f), FVector(0.12f, 0.f, -0.02f));
	for (int32 k = 0; k < 5; ++k)
	{
		const float A = (k - 2) * 0.38f;
		JW::Part(this, Body, EShape::Cone, Feather[k % 3], FVector(0.08f, 0.18f, 0.6f), FVector(-0.05f, FMath::Sin(A) * 0.55f, 0.75f + FMath::Cos(A) * 0.25f), FRotator(0.f, 0.f, FMath::RadiansToDegrees(A)));
	}
	Hr = 0.8f; Hy = 0.f; HP = 55.f; Speed = 4.2f; Dmg = 10.f; Range = 0.f; Cd = 2.2f; Value = 200; Rad = 0.6f; Col = JW::Hex(0xc07aff); bFly = true;
}

void AJungleEnemy::BuildBoss()
{
	const FLinearColor St = JW::Hex(0x6f7560), Moss = JW::Hex(0x3d6a2a), Eye = JW::Hex(0xff3322), Gold = JW::Hex(0xd9a634), Rune = JW::Hex(0xffb84a);
	for (float Side : { 1.f, -1.f })
	{
		Legs.Add(JW::Part(this, Body, EShape::Cube, St, FVector(1.1f, 1.1f, 2.4f), FVector(0.f, 1.f * Side, 1.2f)));
	}
	JW::Part(this, Body, EShape::Cube, St, FVector(1.9f, 3.1f, 2.8f), FVector(0.f, 0.f, 3.7f));
	JW::Part(this, Body, EShape::Cube, Moss, FVector(2.1f, 3.3f, 0.4f), FVector(0.f, 0.f, 5.f));
	JW::Part(this, Body, EShape::Cube, Rune, FVector(0.05f, 0.7f, 0.7f), FVector(0.97f, 0.f, 3.8f), FRotator(0.f, 0.f, 45.f));
	JW::Part(this, Body, EShape::Cube, St, FVector(2.f, 2.2f, 2.f), FVector(0.1f, 0.f, 6.1f));
	JW::Part(this, Body, EShape::Cube, Eye, FVector(0.05f, 0.45f, 0.25f), FVector(1.12f, 0.5f, 6.3f));
	JW::Part(this, Body, EShape::Cube, Eye, FVector(0.05f, 0.45f, 0.25f), FVector(1.12f, -0.5f, 6.3f));
	JW::Part(this, Body, EShape::Cube, JW::Hex(0x120505), FVector(0.05f, 1.2f, 0.25f), FVector(1.12f, 0.f, 5.6f));
	for (int32 k = 0; k < 5; ++k)
	{
		JW::Part(this, Body, EShape::Cone, Gold, FVector(0.36f, 0.36f, 0.7f), FVector(0.1f, (k - 2) * 0.45f, 7.45f));
	}
	for (float Side : { 1.f, -1.f })
	{
		USceneComponent* A = JW::Group(this, Body, FVector(0.f, 2.1f * Side, 4.8f));
		JW::Part(this, A, EShape::Cube, St, FVector(0.9f, 0.9f, 3.f), FVector(0.f, 0.f, -1.4f));
		JW::Part(this, A, EShape::Cube, Gold, FVector(1.f, 1.f, 0.3f), FVector(0.f, 0.f, -2.6f));
		Arms.Add(A);
	}
	Hr = 2.7f; Hy = 4.2f; HP = 1500.f; Speed = 1.9f; Dmg = 30.f; Range = 4.6f; Cd = 2.f; Value = 5000; Rad = 2.f; Col = JW::Hex(0xb8b090); bBoss = true;
}

void AJungleEnemy::SetFlash(bool bOn)
{
	if (bFlashOn == bOn) { return; }
	bFlashOn = bOn;
	UJungleAssets* A = UJungleAssets::Get(this);
	for (int32 i = 0; i < FlashParts.Num(); ++i)
	{
		FlashParts[i]->SetMaterial(0, bOn ? A->Mat(FLinearColor(1.f, 0.95f, 0.85f)) : FlashOrig[i]);
	}
}

void AJungleEnemy::TakeHit(float Damage, const FVector& Dir, float Knockback)
{
	if (!bAlive) { return; }
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	HP -= Damage;
	Flash = 1.f;
	SetFlash(true);
	if (!bBoss && Knockback > 0.f)
	{
		const float K = Knockback / (Type == EEnemyType::Brute ? 3.f : 1.f);
		Knock += FVector2D(Dir.X, Dir.Y) * K;
	}
	if (HP <= 0.f)
	{
		bAlive = false;
		Dying = 0.5f;
		HitSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetFlash(false);
		if (GM) { GM->EnemyKilled(this); }
	}
	else if (GM)
	{
		GM->HitMarker(false);
		GM->Sfx(ESfx::Hit);
	}
}

void AJungleEnemy::FireOrb(const FVector2D& Offset)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* P = GM ? GM->Player() : nullptr;
	if (!P) { return; }
	const FVector From = bFly ? GetActorLocation() : GetActorLocation() + FVector(0.f, 0.f, (Hy + 1.5f) * JW::M);
	const FVector To = P->EyeLocation() + FVector(Offset.X * JW::M, Offset.Y * JW::M, -30.f);
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AJungleProjectile* Orb = GetWorld()->SpawnActor<AJungleProjectile>(AJungleProjectile::StaticClass(), From, FRotator::ZeroRotator, SP))
	{
		Orb->Init((To - From).GetSafeNormal() * (bBoss ? 1500.f : 1700.f), false, bBoss, bBoss ? 14.f : 10.f, 0.f);
	}
	GM->Sfx(ESfx::EnemyShot);
}

void AJungleEnemy::Tick(float Dt)
{
	Super::Tick(Dt);
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (!GM || (GM->State != EJState::Playing && GM->State != EJState::Over)) { return; }

	if (!bAlive)
	{
		Dying -= Dt;
		Body->AddLocalRotation(FRotator(-Dt * 170.f, 0.f, 0.f));
		Body->SetRelativeScale3D(Body->GetRelativeScale3D() * FMath::Exp(-Dt * 3.f));
		AddActorWorldOffset(FVector(0.f, 0.f, -Dt * (bBoss ? 300.f : 150.f)));
		if (Dying <= 0.f)
		{
			GM->Enemies.Remove(this);
			Destroy();
		}
		return;
	}

	ALionelCharacter* P = GM->Player();
	if (!P) { return; }
	T += Dt;
	const FVector PL = P->GetActorLocation() / JW::M;
	const FVector2D ToP(PL.X - Pos.X, PL.Y - Pos.Y);
	const float D = FMath::Max((float)ToP.Size(), 0.001f);
	const FVector2D N = ToP / D;

	FVector2D Mv = FVector2D::ZeroVector;
	if (bFly)
	{
		const float F = D > 16.f ? 1.f : (D < 9.f ? -1.f : 0.f);
		const float Side = FMath::Sin(T * 0.7f + Hr * 9.f) > 0.f ? 1.f : -1.f;
		Mv = N * F + FVector2D(-N.Y, N.X) * Side * 0.7f;
	}
	else if (D > Range * 0.75f)
	{
		Mv = N;
	}
	for (const AJungleEnemy* O : GM->Enemies)
	{
		if (O == this || !O || !O->bAlive) { continue; }
		const FVector2D S = Pos - O->Pos;
		const float RR = Rad + O->Rad, D2 = S.SizeSquared();
		if (D2 < RR * RR && D2 > 1e-4f) { Mv += S / FMath::Sqrt(D2) * 1.5f; }
	}

	float Sp = Speed;
	if (!bFly && JW::Hgt(Pos.X, Pos.Y) < JW::Water - 0.2f) { Sp *= 0.6f; }
	const FVector2D Old = Pos;
	Pos += (Mv * Sp + Knock) * Dt;
	Knock *= FMath::Exp(-Dt * 6.f);
	if (!bFly) { GM->Collide(Pos, Rad, Z + 0.5f); }
	if (!bFly && JW::FloorH(Pos.X, Pos.Y) - Z > 1.2f) { Pos = Old; }
	Z = bFly ? FMath::Max(JW::Hgt(Pos.X, Pos.Y), JW::Water) + 2.6f + FMath::Sin(T * 2.2f) * 0.35f : JW::FloorH(Pos.X, Pos.Y);

	Lunge = FMath::Max(0.f, Lunge - Dt * 4.f);
	SetActorLocationAndRotation(FVector(Pos.X, Pos.Y, Z) * JW::M, FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(ToP.Y, ToP.X)), 0.f));
	if (bFly) { Body->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Sin(T * 1.7f) * 9.f)); }
	for (int32 k = 0; k < Legs.Num(); ++k)
	{
		Legs[k]->SetRelativeRotation(FRotator(FMath::Sin(T * Sp * 2.f + k * PI) * 40.f, 0.f, 0.f));
	}
	for (int32 k = 0; k < Arms.Num(); ++k)
	{
		Arms[k]->SetRelativeRotation(FRotator(Lunge * 100.f + FMath::Sin(T * Sp * 2.f + k * PI) * 17.f, 0.f, 0.f));
	}
	if (!bFly && Arms.Num() == 0) { Body->SetRelativeScale3D(FVector(1.f + Lunge * 0.15f)); }

	if (Flash > 0.f)
	{
		Flash -= Dt * 7.f;
		if (Flash <= 0.5f) { SetFlash(false); }
	}

	// Attacks
	const float FeetZ = (P->GetActorLocation().Z - 90.f) / JW::M;
	Atk -= Dt;
	if (!bFly && D < Range && Atk <= 0.f && FMath::Abs(FeetZ - Z) < 2.6f)
	{
		Atk = Cd;
		Lunge = 1.f;
		P->Hurt(Dmg);
	}
	if (bFly)
	{
		Shoot -= Dt;
		if (Shoot <= 0.f && D < 42.f)
		{
			Shoot = JW::Rand(1.9f, 3.f);
			FireOrb(FVector2D::ZeroVector);
		}
	}
	if (bBoss)
	{
		Shoot -= Dt;
		if (Shoot <= 0.f && D < 55.f)
		{
			Shoot = 2.6f;
			for (int32 k = -2; k <= 2; ++k)
			{
				FireOrb(FVector2D(-N.Y, N.X) * (k * 2.2f));
			}
		}
		Summon -= Dt;
		if (Summon <= 0.f)
		{
			Summon = 10.f;
			for (int32 k = 0; k < 3; ++k) { GM->SpawnEnemy(EEnemyType::Thorn, &Pos); }
			GM->Sfx(ESfx::Roar);
		}
	}
}
