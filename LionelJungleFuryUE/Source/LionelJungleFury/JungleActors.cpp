#include "JungleActors.h"
#include "JungleGameMode.h"
#include "LionelCharacter.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"

// ============================================================================================
// Projectile
// ============================================================================================

AJungleProjectile::AJungleProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AJungleProjectile::Init(const FVector& Velocity, bool bCoconut, bool bBossOrb, float Damage, float SplashM)
{
	Vel = Velocity;
	bNut = bCoconut;
	bBoss = bBossOrb;
	Dmg = Damage;
	Splash = SplashM;
	const FLinearColor C = bNut ? JW::Hex(0x5b3a1e) : (bBoss ? JW::Hex(0xff6a22) : JW::Hex(0xd36bff));
	Mesh = JW::Part(this, Root, EShape::Sphere, C, FVector(bNut ? 0.4f : (bBoss ? 0.84f : 0.6f)), FVector::ZeroVector);
	Mesh->SetCastShadow(false);
	bReady = true;
}

void AJungleProjectile::Tick(float Dt)
{
	Super::Tick(Dt);
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (!bReady || !GM || GM->State != EJState::Playing) { return; }
	Age += Dt;
	const FVector P = GetActorLocation();

	if (bNut)
	{
		Vel.Z -= 1600.f * Dt;
		const FVector Next = P + Vel * Dt;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(Coconut), false, this);
		Params.AddIgnoredActor(GM->Player());
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, P, Next, ECC_Visibility, Params);
		const bool bGround = Next.Z / JW::M < JW::Hgt(Next.X / JW::M, Next.Y / JW::M) + 0.15f;
		if (bHit || bGround || Age > 4.f)
		{
			GM->Explode(bHit ? FVector(Hit.ImpactPoint) : Next, Splash, Dmg, false);
			Destroy();
			return;
		}
		SetActorLocation(Next);
		Mesh->AddLocalRotation(FRotator(Dt * 600.f, 0.f, 0.f));
		return;
	}

	const FVector Next = P + Vel * Dt;
	SetActorLocation(Next);
	if (FMath::FRand() < 0.5f && GM->FX)
	{
		GM->FX->Burst(Next, 1, bBoss ? JW::Hex(0xff8a3a) : JW::Hex(0xd36bff), 50.f, 12.f, 0.35f, 0.f, 0.f);
	}
	if (ALionelCharacter* Pl = GM->Player())
	{
		if (FVector::Dist(Next, Pl->EyeLocation() - FVector(0.f, 0.f, 40.f)) < 85.f)
		{
			Pl->Hurt(Dmg);
			Destroy();
			return;
		}
	}
	if (Age > 5.f || Next.Z / JW::M < JW::Hgt(Next.X / JW::M, Next.Y / JW::M))
	{
		if (GM->FX) { GM->FX->Burst(Next, 6, bBoss ? JW::Hex(0xff8a3a) : JW::Hex(0xd36bff), 300.f, 12.f, 0.4f, 800.f, 100.f); }
		Destroy();
	}
}

// ============================================================================================
// Pickup
// ============================================================================================

AJunglePickup::AJunglePickup()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AJunglePickup::Init(bool bInAmmo)
{
	bAmmo = bInAmmo;
	T = JW::Rand(0.f, 5.f);
	if (bAmmo)
	{
		JW::Part(this, Root, EShape::Cube, JW::Hex(0x6b4a26), FVector(0.7f, 0.5f, 0.45f), FVector::ZeroVector);
		JW::Part(this, Root, EShape::Cube, JW::Hex(0xf2b632), FVector(0.72f, 0.52f, 0.12f), FVector::ZeroVector);
	}
	else
	{
		JW::Part(this, Root, EShape::Sphere, JW::Hex(0xf08a24), FVector(0.56f, 0.56f, 0.68f), FVector::ZeroVector);
		JW::Part(this, Root, EShape::Cone, JW::Hex(0x4a9a2a), FVector(0.16f, 0.06f, 0.25f), FVector(0.f, 0.08f, 0.38f), FRotator(0.f, 0.f, 30.f));
	}
}

void AJunglePickup::Tick(float Dt)
{
	Super::Tick(Dt);
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (!GM || GM->State != EJState::Playing) { return; }
	ALionelCharacter* P = GM->Player();
	if (!P) { return; }
	T += Dt;

	const FVector L = GetActorLocation() / JW::M;
	const FVector PL = P->GetActorLocation() / JW::M;
	FVector2D Pos(L.X, L.Y);
	const FVector2D To(PL.X - L.X, PL.Y - L.Y);
	const float D = To.Size();
	if (D < 4.5f && D > 0.01f) { Pos += To / D * Dt * 9.f; }
	SetActorLocationAndRotation(FVector(Pos.X, Pos.Y, JW::FloorH(Pos.X, Pos.Y) + 0.6f + FMath::Sin(T * 3.f) * 0.12f) * JW::M, FRotator(0.f, T * 115.f, 0.f));

	if (D < 1.3f)
	{
		if (bAmmo)
		{
			P->Resupply(0.75f);
			GM->Pop(TEXT("AMMO"));
		}
		else
		{
			if (P->Health >= 100.f) { return; }
			P->Health = FMath::Min(100.f, P->Health + 30.f);
			GM->Pop(TEXT("+30 HEALTH"));
		}
		GM->Sfx(ESfx::Pickup);
		GM->Pickups.Remove(this);
		Destroy();
		return;
	}
	if (T > 40.f)
	{
		GM->Pickups.Remove(this);
		Destroy();
	}
}

// ============================================================================================
// Effects
// ============================================================================================

AJungleFX::AJungleFX()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AJungleFX::BeginPlay()
{
	Super::BeginPlay();
	for (int32 i = 0; i < 220; ++i)
	{
		UStaticMeshComponent* C = JW::Part(this, Root, EShape::Cube, FLinearColor::White, FVector(0.1f), FVector::ZeroVector);
		C->SetCastShadow(false);
		C->SetVisibility(false);
		PComps.Add(C);
	}
	Ps.SetNum(PComps.Num());
	for (int32 i = 0; i < 90; ++i)
	{
		UStaticMeshComponent* C = JW::Part(this, Root, EShape::Cube, FLinearColor::White, FVector(0.1f), FVector::ZeroVector);
		C->SetCastShadow(false);
		C->SetVisibility(false);
		TComps.Add(C);
	}
	Ts.SetNum(TComps.Num());

	Brass = NewObject<UPhysicalMaterial>(this);
	Brass->Friction = 0.25f;
	Brass->Restitution = 0.5f;
	Brass->bOverrideRestitutionCombineMode = true;
	Brass->RestitutionCombineMode = EFrictionCombineMode::Max;

	Light = NewObject<UPointLightComponent>(this);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Root);
	Light->SetIntensityUnits(ELightUnits::Unitless);
	Light->SetAttenuationRadius(2200.f);
	Light->SetCastShadows(false);
	Light->VolumetricScatteringIntensity = 2.f;   // explosions light up the mist
	Light->RegisterComponent();
	Light->SetIntensity(0.f);
}

void AJungleFX::Burst(const FVector& At, int32 Count, const FLinearColor& Color, float Speed, float Size, float Life, float Gravity, float Up)
{
	UJungleAssets* A = UJungleAssets::Get(this);
	UMaterialInstanceDynamic* Mat = A->Mat(Color);
	for (int32 i = 0; i < Count; ++i)
	{
		FParticle& P = Ps[PNext];
		UStaticMeshComponent* C = PComps[PNext];
		PNext = (PNext + 1) % Ps.Num();
		const float S = Speed * JW::Rand(0.3f, 1.f);
		P.P = At;
		P.V = FMath::VRand() * S + FVector(0.f, 0.f, Up);
		P.Max = P.Life = Life * JW::Rand(0.6f, 1.2f);
		P.Size = Size * JW::Rand(0.6f, 1.3f);
		P.Grav = Gravity;
		P.Spin = FRotator(JW::Rand(-400, 400), JW::Rand(-400, 400), 0.f);
		C->SetMaterial(0, Mat);
		C->SetVisibility(true);
	}
}

void AJungleFX::Tracer(const FVector& A, const FVector& B, const FLinearColor& Color, float Thick, float Life)
{
	FTracer& T = Ts[TNext];
	UStaticMeshComponent* C = TComps[TNext];
	TNext = (TNext + 1) % Ts.Num();
	const FVector D = B - A;
	const float Len = FMath::Max((float)D.Size(), 1.f);
	T.Life = T.Max = Life;
	T.Thick = Thick;
	C->SetMaterial(0, UJungleAssets::Get(this)->Mat(Color));
	C->SetWorldLocationAndRotation((A + B) * 0.5f, D.Rotation());
	C->SetWorldScale3D(FVector(Len / JW::M, Thick / JW::M, Thick / JW::M));
	C->SetVisibility(true);
}

void AJungleFX::Flash(const FVector& At, const FLinearColor& Color, float Intensity, float Duration)
{
	Light->SetWorldLocation(At);
	Light->SetLightColor(Color);
	LightI = Intensity;
	LightT = LightDur = Duration;
	Light->SetIntensity(Intensity);
}

void AJungleFX::Casing(const FVector& At, const FVector& Velocity, bool bShotgunShell)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (!GM) { return; }
	AStaticMeshActor* A = GM->SpawnProp(EShape::Cylinder, JW::Hex(bShotgunShell ? 0xb3201a : 0xd8a43a),
		bShotgunShell ? FVector(0.04f, 0.04f, 0.07f) : FVector(0.022f, 0.022f, 0.055f), At,
		FRotator(0.f, JW::Rand(0.f, 360.f), 90.f), bShotgunShell ? 0.04f : 0.02f, 3.f, true);
	if (!A) { return; }
	UStaticMeshComponent* MC = A->GetStaticMeshComponent();
	MC->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	MC->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MC->SetUseCCD(true);
	MC->SetCastShadow(false);
	MC->SetPhysMaterialOverride(Brass);
	MC->SetNotifyRigidBodyCollision(true);
	MC->OnComponentHit.AddDynamic(this, &AJungleFX::OnCasingHit);
	MC->SetPhysicsLinearVelocity(Velocity);
	MC->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 720.f);
}

void AJungleFX::OnCasingHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// A small metallic clink for the first few real impacts of each casing near Lionel.
	if (!HitComp || Now - LastClink < 0.035f) { return; }
	const float Speed = (float)NormalImpulse.Size() / FMath::Max(HitComp->GetMass(), 0.001f);   // cm/s change along the normal
	if (Speed < 70.f) { return; }
	int32& N = Clinks.FindOrAdd(HitComp);
	if (N >= 4) { return; }
	++N;
	LastClink = Now;
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* P = GM ? GM->Player() : nullptr;
	if (P && FVector::Dist(P->GetActorLocation(), HitComp->GetComponentLocation()) < 1200.f) { GM->Sfx(ESfx::Tink); }
}

void AJungleFX::Tick(float Dt)
{
	Super::Tick(Dt);
	Now += Dt;
	if (Clinks.Num() > 64) { Clinks.Empty(); }
	for (int32 i = 0; i < Ps.Num(); ++i)
	{
		FParticle& P = Ps[i];
		if (P.Life <= 0.f) { continue; }
		UStaticMeshComponent* C = PComps[i];
		P.Life -= Dt;
		if (P.Life <= 0.f) { C->SetVisibility(false); continue; }
		P.V.Z -= P.Grav * Dt;
		P.P += P.V * Dt;
		const float Gz = JW::Hgt(P.P.X / JW::M, P.P.Y / JW::M) * JW::M;
		if (P.P.Z < Gz)
		{
			P.P.Z = Gz;
			P.V.Z *= -0.3f;
			P.V.X *= 0.6f;
			P.V.Y *= 0.6f;
		}
		const float S = FMath::Max(0.001f, P.Size * (P.Life / P.Max)) / JW::M;
		C->SetWorldLocationAndRotation(P.P, C->GetComponentRotation() + P.Spin * Dt);
		C->SetWorldScale3D(FVector(S));
	}
	for (int32 i = 0; i < Ts.Num(); ++i)
	{
		FTracer& T = Ts[i];
		if (T.Life <= 0.f) { continue; }
		UStaticMeshComponent* C = TComps[i];
		T.Life -= Dt;
		if (T.Life <= 0.f) { C->SetVisibility(false); continue; }
		const float Th = T.Thick * (T.Life / T.Max) / JW::M;
		const FVector Sc = C->GetComponentScale();
		C->SetWorldScale3D(FVector(Sc.X, Th, Th));
	}
	if (LightT > 0.f)
	{
		LightT -= Dt;
		Light->SetIntensity(LightT > 0.f ? LightI * (LightT / LightDur) : 0.f);
	}
}
