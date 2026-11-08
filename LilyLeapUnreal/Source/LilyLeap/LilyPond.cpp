#include "LilyPond.h"

#include "LilyArt.h"
#include "LilyPlayerController.h"
#include "LilySave.h"
#include "LilySynth.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"

namespace
{
	const TCHAR* SaveSlot = TEXT("LilyLeap");

	// Colours
	const FLinearColor FrogBody   = LilyHex(0x5DBB4C);
	const FLinearColor FrogShade  = LilyHex(0x4AA23C);
	const FLinearColor FrogDark   = LilyHex(0x2F6E2C);
	const FLinearColor FrogBelly  = LilyHex(0xD6EF9A);
	const FLinearColor StemGreen  = LilyHex(0x3E8E41);
	const FLinearColor LeafGreen  = LilyHex(0x4FA548);
	const FLinearColor PadGreen   = LilyHex(0x4F9D47);
	const FLinearColor PadMoving  = LilyHex(0x6DBB5A);
	const FLinearColor Grass      = LilyHex(0x6CC04A);
	const FLinearColor Earth      = LilyHex(0x8A5A3B);
	const FLinearColor BloomPurple= LilyHex(0x8E5BD6);
	const FLinearColor BloomDark  = LilyHex(0x4A2589);
	const FLinearColor Water      = LilyHex(0x3F93B0);
	const FLinearColor Sky        = LilyHex(0xBFE6F5);
	const FLinearColor Horizon    = LilyHex(0xFDE3C8);
	const FLinearColor Cream      = LilyHex(0xFFF8EC);
	const FLinearColor Pink       = LilyHex(0xF28DB2);
	const FLinearColor Gold       = LilyHex(0xF6C445);

	float MaxGap(float Rise) { return Rise > 0.f ? 188.f - Rise * 0.6f : 205.f; }

}

ALilyPond::ALilyPond()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Synth = CreateDefaultSubobject<ULilySynth>(TEXT("Synth"));
	Synth->SetupAttachment(Root);
}

void ALilyPond::BeginPlay()
{
	Super::BeginPlay();

	LoadProgress();
	BuildLightingAndCamera();
	BuildScenery();
	BuildFrog();
	BuildFx();

#if PLATFORM_IOS || PLATFORM_ANDROID
	bTouchUI = true;
#endif

	Synth->Start();
	Synth->SetMasterOn(bSoundOn);

	LoadLevel(1);
	UpdateCamera(0.f, true);
	SetState(ELilyState::Title);
}

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void ALilyPond::BuildLightingAndCamera()
{
	UWorld* World = GetWorld();

	auto MakeLight = [World](const FRotator& Rot, float Lux, const FLinearColor& Color, bool bShadows)
	{
		ADirectionalLight* Light = World->SpawnActorDeferred<ADirectionalLight>(ADirectionalLight::StaticClass(), FTransform(Rot));
		if (!Light) return;
		if (ULightComponent* LC = Light->GetLightComponent())
		{
			LC->SetMobility(EComponentMobility::Movable);
			LC->SetIntensity(Lux);
			LC->SetLightColor(Color);
			LC->SetCastShadows(bShadows);
		}
		Light->FinishSpawning(FTransform(Rot));
	};
	// Warm morning sun from front-left and above; a cool fill from front-right so shadows are not black.
	MakeLight(FRotator(-45.f, 70.f, 0.f), LilyTune::SunLux, LilyHex(0xFFF1D6), true);
	MakeLight(FRotator(-20.f, 120.f, 0.f), LilyTune::FillLux, LilyHex(0xCFE8FF), false);

	// Fixed exposure so the bright sky does not darken the whole scene.
	if (APostProcessVolume* PPV = World->SpawnActor<APostProcessVolume>())
	{
		PPV->bUnbound = true;
		FPostProcessSettings& S = PPV->Settings;
		S.bOverride_AutoExposureMethod = true;
		S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		S.AutoExposureApplyPhysicalCameraExposure = 0;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = LilyTune::ExposureBias;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 0.3f;
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 0.25f;
	}

	Camera = World->SpawnActor<ACameraActor>(FVector(0.f, -LilyTune::CamDistance, LilyTune::CamRestZ), FRotator(LilyTune::CamPitch, 90.f, 0.f));
	if (Camera)
	{
		UCameraComponent* Cam = Camera->GetCameraComponent();
		Cam->SetFieldOfView(LilyTune::CamFov);
		Cam->SetConstraintAspectRatio(false);
	}
}

void ALilyPond::BuildScenery()
{
	UWorld* World = GetWorld();
	FRandomStream Rng(4242);

	// Sky backdrop, horizon glow and sun: they follow the camera sideways so they never run out.
	SkyActor = World->SpawnActor<ALilyProp>();
	FLilyArt::Add(SkyActor, SkyActor->Root, ELilyShape::Cube, FVector(0, 14000, 4000), FVector(80000, 100, 30000), Sky);
	FLilyArt::Add(SkyActor, SkyActor->Root, ELilyShape::Cube, FVector(0, 13800, 250), FVector(80000, 100, 1600), Horizon);
	FLilyArt::Add(SkyActor, SkyActor->Root, ELilyShape::Sphere, FVector(3200, 13000, 3300), FVector(1300, 100, 1300), LilyHex(0xFFE9A3));

	// Rolling hills in two layers; real depth gives natural parallax.
	ALilyProp* Hills = World->SpawnActor<ALilyProp>();
	for (float X = -4000.f; X < 16000.f; X += Rng.FRandRange(800.f, 1100.f))
	{
		FLilyArt::Add(Hills, Hills->Root, ELilyShape::Sphere, FVector(X, 3000, -150), FVector(Rng.FRandRange(1500, 2200), 700, Rng.FRandRange(600, 800)), LilyHex(0x82C389));
		// meadow flowers dotted on the near hills
		for (int32 k = 0; k < 3; k++)
		{
			const FLinearColor C = LilyFlowerPalette(Rng.RandRange(0, LilyNumPalettes - 1)).Petal;
			FLilyArt::Add(Hills, Hills->Root, ELilyShape::Sphere, FVector(X + Rng.FRandRange(-500, 500), 2620, Rng.FRandRange(40, 160)), FVector(22, 22, 22), C);
		}
	}
	for (float X = -6000.f; X < 18000.f; X += Rng.FRandRange(1500.f, 2000.f))
	{
		FLilyArt::Add(Hills, Hills->Root, ELilyShape::Sphere, FVector(X, 6500, -300), FVector(Rng.FRandRange(2800, 3800), 1200, Rng.FRandRange(1200, 1500)), LilyHex(0xA7D6B0));
	}

	// The pond itself.
	ALilyProp* Pond = World->SpawnActor<ALilyProp>();
	FLilyArt::Add(Pond, Pond->Root, ELilyShape::Cube, FVector(6000, 3500, -200), FVector(30000, 11000, 400), Water);

	// Clouds drift and wrap around the camera.
	for (int32 i = 0; i < 9; i++)
	{
		ALilyProp* Cloud = World->SpawnActor<ALilyProp>(FVector(i * 1500.f - 4000.f, 9000.f, Rng.FRandRange(1300.f, 2400.f)), FRotator::ZeroRotator);
		const float S = Rng.FRandRange(0.8f, 1.4f);
		FLilyArt::Add(Cloud, Cloud->Root, ELilyShape::Sphere, FVector(0, 0, 0), FVector(520, 200, 170) * S, FLinearColor::White);
		FLilyArt::Add(Cloud, Cloud->Root, ELilyShape::Sphere, FVector(-260 * S, 0, -20 * S), FVector(300, 180, 150) * S, FLinearColor::White);
		FLilyArt::Add(Cloud, Cloud->Root, ELilyShape::Sphere, FVector(200 * S, 0, 90 * S), FVector(340, 200, 220) * S, FLinearColor::White);
		Clouds.Add(Cloud);
	}
}

void ALilyPond::BuildFrog()
{
	FrogActor = GetWorld()->SpawnActor<ALilyProp>();
	ALilyProp* A = FrogActor;
	USceneComponent* Body = NewObject<USceneComponent>(A);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetupAttachment(A->Root);
	Body->RegisterComponent();
	A->AddInstanceComponent(Body);
	Rig.Body = Body;

	// Model faces +X; origin at the bottom of the feet.
	FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(0, 0, 18), FVector(46, 40, 32), FrogBody, FRotator(-6, 0, 0), true);
	FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(8, 0, 11), FVector(28, 30, 16), FrogBelly);
	for (int32 s = 0; s < 2; s++)
	{
		const float Y = s == 0 ? -1.f : 1.f;
		Rig.Thigh[s] = FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(-12, 14 * Y, 10), FVector(24, 12, 18), FrogShade, FRotator::ZeroRotator, true);
		Rig.Foot[s]  = FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(-6, 16 * Y, 2), FVector(26, 10, 5), FrogDark);
		FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(16, 10 * Y, 2), FVector(14, 8, 5), FrogDark);        // front foot
		FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(6, 9 * Y, 31), FVector(16, 16, 16), FrogBody);       // eye bump
		FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(9, 10 * Y, 33), FVector(12, 12, 12), FLinearColor(0.98f, 0.98f, 0.93f));
		Rig.Pupil[s] = FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(13, 10.5f * Y, 33.5f), FVector(6, 6, 6), FLinearColor(0.01f, 0.01f, 0.01f));
		FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(14, 13 * Y, 17), FVector(5, 3, 5), Pink);             // blush
		FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(-6, 12 * Y, 27), FVector(6, 3, 6), FrogDark);        // spot
	}
	Rig.Throat = FLilyArt::Add(A, Body, ELilyShape::Sphere, FVector(17, 0, 10), FVector(8, 14, 6), LilyHex(0xE8F8BE));

	// Tongue lives on the actor root so it can point anywhere regardless of facing.
	Rig.Tongue = FLilyArt::Add(A, A->Root, ELilyShape::Cylinder, FVector::ZeroVector, FVector(6, 6, 10), LilyHex(0xEF6F90));
	Rig.TongueTip = FLilyArt::Add(A, A->Root, ELilyShape::Sphere, FVector::ZeroVector, FVector(13, 13, 13), LilyHex(0xEF6F90));
	Rig.Tongue->SetVisibility(false);
	Rig.TongueTip->SetVisibility(false);
}

void ALilyPond::BuildFx()
{
	FxActor = GetWorld()->SpawnActor<ALilyProp>();
	for (int32 i = 0; i < 64; i++)
	{
		UStaticMeshComponent* C = FLilyArt::Add(FxActor, FxActor->Root, ELilyShape::Sphere, FVector::ZeroVector, FVector(6, 6, 6), FLinearColor::White);
		C->SetVisibility(false);
		FxPool.Add(C);
	}
}

/* ------------------------------------------------------------------ */
/* Level generation (same rules as the web version)                    */
/* ------------------------------------------------------------------ */

void ALilyPond::ClearLevel()
{
	for (ALilyProp* A : LevelActors) if (A) A->Destroy();
	LevelActors.Reset();
	Platforms.Reset();
	Flies.Reset();
}

void ALilyPond::LoadLevel(int32 N)
{
	ClearLevel();
	Level = N;
	FRandomStream Rng(1000 + N * 7919);

	auto Add = [this, &Rng](ELilyPlat Type, float X, float W, float Top) -> FLilyPlatform&
	{
		FLilyPlatform& P = Platforms.AddDefaulted_GetRef();
		P.Type = Type; P.X = P.X0 = X; P.W = W; P.Top = P.BaseTop = Top;
		P.Phase = Rng.FRandRange(0.f, 2.f * PI);
		P.Palette = Rng.RandRange(0, LilyNumPalettes - 1);
		P.bLeaves = Rng.FRand() < 0.6f;
		return P;
	};

	Add(ELilyPlat::Island, -220.f, 760.f, 90.f);
	float LastTop = 90.f, X = 540.f;
	ELilyPlat LastType = ELilyPlat::Island;
	const float Length = 3000.f + N * 650.f;

	while (X < Length)
	{
		ELilyPlat Type; float Top, W, Gap, Amp = 0.f, Spd = 0.f;
		const float R = Rng.FRand();
		if (LastType == ELilyPlat::Bloom)
		{
			Type = ELilyPlat::Flower;
			Top = FMath::Clamp(LastTop + Rng.FRandRange(170.f, 250.f), 180.f, 400.f);
			W = Rng.FRandRange(115.f, 150.f);
			Gap = Rng.FRandRange(40.f, 110.f);
		}
		else if (R < 0.12f && X > 900.f)
		{
			Type = ELilyPlat::Bloom; W = 76.f;
			Top = Rng.FRandRange(45.f, 68.f);
			Gap = Rng.FRandRange(50.f, MaxGap(Top - LastTop));
		}
		else
		{
			Top = FMath::Clamp(LastTop + Rng.FRandRange(-130.f, 110.f), 12.f, 310.f);
			if (Top < 42.f) { Type = ELilyPlat::Pad; Top = 14.f; W = Rng.FRandRange(92.f, 130.f); }
			else if (Top < 130.f && Rng.FRand() < 0.4f) { Type = ELilyPlat::Island; W = Rng.FRandRange(220.f, 420.f); }
			else { Type = ELilyPlat::Flower; W = Rng.FRandRange(100.f, 150.f); }
			Gap = Rng.FRandRange(55.f, FMath::Max(56.f, MaxGap(Top - LastTop)));
			if (Type == ELilyPlat::Pad && Rng.FRand() < 0.3f + N * 0.05f) { Amp = Rng.FRandRange(30.f, 70.f); Spd = Rng.FRandRange(0.6f, 1.15f); }
		}
		const float PX = X + Gap + Amp;
		FLilyPlatform& P = Add(Type, PX, W, Top);
		P.Amp = Amp; P.Spd = Spd;
		P.bBlossom = Type == ELilyPlat::Pad && Rng.FRand() < 0.35f;
		X = PX + W + Amp;
		LastTop = Top; LastType = Type;
	}
	const float EndX = X + 90.f;
	Add(ELilyPlat::Island, EndX, 700.f, 90.f);
	LevelEnd = EndX + 700.f;

	// Flies
	const int32 Want = 8 + N * 3;
	const int32 Golds = 1 + N / 2;
	TArray<int32> Hosts;
	for (int32 i = 1; i < Platforms.Num(); i++) if (Platforms[i].Type != ELilyPlat::Bloom) Hosts.Add(i);
	for (int32 i = Hosts.Num() - 1; i > 0; i--) Hosts.Swap(i, Rng.RandRange(0, i));
	const float SpeedUp = 1.f + N * 0.12f;
	for (int32 k = 0; k < Want && Hosts.Num() > 0; k++)
	{
		const FLilyPlatform& P = Platforms[Hosts[k % Hosts.Num()]];
		FLilyFly& F = Flies.AddDefaulted_GetRef();
		F.bGold = k < Golds;
		F.HX = P.X0 + P.W * 0.5f + Rng.FRandRange(-40.f, 40.f);
		F.HZ = P.Top + (F.bGold ? Rng.FRandRange(125.f, 175.f) : Rng.FRandRange(75.f, 160.f));
		F.RX = F.bGold ? Rng.FRandRange(80.f, 120.f) : Rng.FRandRange(28.f, 75.f);
		F.RZ = F.bGold ? Rng.FRandRange(30.f, 50.f) : Rng.FRandRange(14.f, 38.f);
		F.Sp = Rng.FRandRange(0.8f, 1.35f) * SpeedUp * (F.bGold ? 1.45f : 1.f);
		F.Ph = Rng.FRandRange(0.f, 2.f * PI);
	}
	Total = Left = Flies.Num();

	// Visuals
	for (FLilyPlatform& P : Platforms) BuildPlatformVisual(P);
	for (FLilyFly& F : Flies) BuildFlyVisual(F);

	// Frog and per-level state
	Frog = FFrogState();
	Frog.Ground = 0; Frog.Safe = 0;
	Tongue = FTongueState();
	Particles.Reset();
	for (UStaticMeshComponent* C : FxPool) C->SetVisibility(false);
	Texts.Reset();
	LevelTime = 0.f; Combo = 0;
	LevelBpm = 100.f + FMath::Min(N - 1, 6) * 4.f;
	UpdateFlies();
	UpdateCamera(0.f, true);
}

void ALilyPond::BuildPlatformVisual(FLilyPlatform& P)
{
	ALilyProp* A = GetWorld()->SpawnActor<ALilyProp>(FVector(P.X, 0.f, 0.f), FRotator::ZeroRotator);
	LevelActors.Add(A);
	P.Actor = A;
	USceneComponent* R = A->Root;
	const float CX = P.W * 0.5f;   // local centre
	FRandomStream Rng(int32(P.X0 * 13.f) + int32(P.Top));

	switch (P.Type)
	{
	case ELilyPlat::Island:
	{
		const float Bottom = -70.f;
		FLilyArt::Add(A, R, ELilyShape::Cube, FVector(CX, 0, (P.Top - 6.f + Bottom) * 0.5f), FVector(P.W, 300, P.Top - 6.f - Bottom), Earth, FRotator::ZeroRotator, true);
		FLilyArt::Add(A, R, ELilyShape::Cube, FVector(CX, 0, P.Top - 6.f), FVector(P.W + 8, 308, 12), Grass, FRotator::ZeroRotator, true);
		for (int32 i = 0; i < int32(P.W / 46.f); i++) AddDecor(A, Rng.FRandRange(18.f, P.W - 18.f), P.Top, Rng);
		// cattails at the water's edge
		for (float EX : { 4.f, P.W - 4.f })
		{
			const float H = Rng.FRandRange(60.f, 90.f);
			FLilyArt::Add(A, R, ELilyShape::Cylinder, FVector(EX, -150, H * 0.5f), FVector(3, 3, H), LilyHex(0x5C8F3A));
			FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(EX, -150, H - 12.f), FVector(10, 10, 26), LilyHex(0x7A4A2A));
		}
		break;
	}
	case ELilyPlat::Flower:
	{
		const FLilyPalette& Pal = LilyFlowerPalette(P.Palette);
		const float StemH = P.Top + 32.f;
		FLilyArt::Add(A, R, ELilyShape::Cylinder, FVector(CX, 0, P.Top - 8.f - StemH * 0.5f + 8.f), FVector(8, 8, StemH), StemGreen, FRotator::ZeroRotator, true);
		if (P.bLeaves)
		{
			FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(CX + 22, 0, P.Top * 0.45f), FVector(46, 14, 6), LeafGreen, FRotator(28, 0, 0));
			FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(CX - 18, 0, P.Top * 0.3f), FVector(38, 12, 5), LeafGreen, FRotator(-30, 0, 0));
		}
		const float Rad = P.W * 0.5f;
		FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(CX, 0, P.Top - 10.f), FVector(Rad * 0.6f, Rad * 0.6f, 12), StemGreen);   // sepals
		for (int32 i = 0; i < 10; i++)
		{
			const float Ang = i * 36.f + 8.f;
			const FVector Dir = FRotator(0, Ang, 0).Vector();
			FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(CX, 0, P.Top - 4.f) + Dir * Rad * 0.52f, FVector(Rad * 0.95f, Rad * 0.4f, 7), Pal.Petal, FRotator(0, Ang, 0), true);
		}
		for (int32 i = 0; i < 10; i++)
		{
			const float Ang = i * 36.f + 26.f;
			const FVector Dir = FRotator(0, Ang, 0).Vector();
			FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(CX, 0, P.Top - 1.f) + Dir * Rad * 0.32f, FVector(Rad * 0.6f, Rad * 0.28f, 6), Pal.Inner, FRotator(0, Ang, 0));
		}
		FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(CX, 0, P.Top + 1.f), FVector(Rad * 0.42f, Rad * 0.42f, 9), Pal.Center);
		break;
	}
	case ELilyPlat::Pad:
	{
		FLilyArt::Add(A, R, ELilyShape::Cylinder, FVector(CX, 0, P.Top - 2.f), FVector(P.W, P.W * 0.92f, 4), P.Amp > 0.f ? PadMoving : PadGreen, FRotator::ZeroRotator, true);
		if (P.bBlossom)
		{
			const FLinearColor B = Rng.FRand() < 0.5f ? LilyHex(0xF7A8C4) : LilyHex(0xFFF4F8);
			for (int32 i = 0; i < 6; i++)
			{
				const float Ang = i * 60.f;
				FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(CX - P.W * 0.25f, 10, P.Top + 6.f) + FRotator(0, Ang, 0).Vector() * 6.f, FVector(14, 7, 12), B, FRotator(0, Ang, 0));
			}
			FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(CX - P.W * 0.25f, 10, P.Top + 8.f), FVector(7, 7, 7), Gold);
		}
		break;
	}
	case ELilyPlat::Bloom:
	{
		const float StemH = P.Top + 40.f;
		FLilyArt::Add(A, R, ELilyShape::Cylinder, FVector(CX, 0, P.Top - 36.f - StemH * 0.5f + 36.f), FVector(8, 8, StemH), StemGreen);
		USceneComponent* Cup = NewObject<USceneComponent>(A);
		Cup->SetMobility(EComponentMobility::Movable);
		Cup->SetupAttachment(R);
		Cup->RegisterComponent();
		A->AddInstanceComponent(Cup);
		Cup->SetRelativeLocation(FVector(CX, 0, P.Top - 50.f));   // pivot at the bottom of the bell, for squash
		FLilyArt::Add(A, Cup, ELilyShape::Cone, FVector(0, 0, 25), FVector(P.W, P.W, 50), BloomPurple, FRotator(180, 0, 0), true);
		FLilyArt::Add(A, Cup, ELilyShape::Cylinder, FVector(0, 0, 48), FVector(P.W * 0.82f, P.W * 0.82f, 3), BloomDark);
		for (int32 i = 0; i < 5; i++)
		{
			const FVector D = FRotator(0, i * 72.f, 0).Vector();
			FLilyArt::Add(A, Cup, ELilyShape::Sphere, FVector(0, 0, 52) + D * P.W * 0.45f, FVector(22, 16, 12), LilyHex(0xB48AF0), FRotator(0, i * 72.f, 0));
		}
		P.Cup = Cup;
		break;
	}
	}
}

void ALilyPond::AddDecor(ALilyProp* Owner, float X, float Top, FRandomStream& Rng)
{
	USceneComponent* R = Owner->Root;
	const float Y = Rng.FRandRange(-130.f, 60.f);
	const int32 Kind = Rng.RandRange(0, 3);
	const FLilyPalette& Pal = LilyFlowerPalette(Rng.RandRange(0, LilyNumPalettes - 1));
	if (Kind == 0)
	{
		// grass tuft
		for (int32 i = -1; i <= 1; i++)
			FLilyArt::Add(Owner, R, ELilyShape::Cone, FVector(X + i * 4.f, Y, Top + 8.f), FVector(5, 5, 18), LeafGreen, FRotator(0, 0, i * 14.f));
		return;
	}
	const float H = Rng.FRandRange(14.f, 34.f);
	FLilyArt::Add(Owner, R, ELilyShape::Cylinder, FVector(X, Y, Top + H * 0.5f), FVector(2.5f, 2.5f, H), StemGreen);
	if (Kind == 1)
	{
		for (int32 i = 0; i < 6; i++)
			FLilyArt::Add(Owner, R, ELilyShape::Sphere, FVector(X, Y, Top + H) + FRotator(0, i * 60.f, 0).Vector() * 5.f, FVector(9, 4, 3), Pal.Petal, FRotator(0, i * 60.f, 0));
		FLilyArt::Add(Owner, R, ELilyShape::Sphere, FVector(X, Y, Top + H + 1.f), FVector(5, 5, 5), Pal.Center);
	}
	else if (Kind == 2)
	{
		FLilyArt::Add(Owner, R, ELilyShape::Cone, FVector(X, Y, Top + H + 3.f), FVector(11, 11, 13), Pal.Petal, FRotator(180, 0, 0));   // tulip
	}
	else
	{
		FLilyArt::Add(Owner, R, ELilyShape::Sphere, FVector(X, Y, Top + H), FVector(10, 10, 10), Pal.Petal);
	}
}

void ALilyPond::BuildFlyVisual(FLilyFly& F)
{
	ALilyProp* A = GetWorld()->SpawnActor<ALilyProp>();
	LevelActors.Add(A);
	F.Actor = A;
	USceneComponent* R = A->Root;
	const FLinearColor BodyC = F.bGold ? LilyHex(0xE3A91F) : LilyHex(0x2B2A33);
	const FLinearColor HeadC = F.bGold ? LilyHex(0xC48812) : LilyHex(0x1C1B22);
	FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(0, 0, 0), FVector(13, 10, 9), BodyC);
	FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(6, 0, 0.5f), FVector(7, 7, 7), HeadC);
	FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(8, -2.5f, 1.8f), FVector(3, 3, 3), LilyHex(0xD6455A));
	FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(8, 2.5f, 1.8f), FVector(3, 3, 3), LilyHex(0xD6455A));
	const FLinearColor WingC(0.85f, 0.95f, 1.f);
	F.WingL = FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(-1, -5, 4), FVector(7, 13, 1.5f), WingC);
	F.WingR = FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(-1, 5, 4), FVector(7, 13, 1.5f), WingC);
	if (F.bGold)
	{
		A->SetActorScale3D(FVector(1.35f));
		FLilyArt::Add(A, R, ELilyShape::Sphere, FVector(0, 0, 0), FVector(30, 30, 30), LilyHex(0xFFE27A) * 0.6f);   // halo hint
	}
}

/* ------------------------------------------------------------------ */
/* Game flow                                                           */
/* ------------------------------------------------------------------ */

bool ALilyPond::IsMenuOpen() const
{
	return State == ELilyState::Title || State == ELilyState::Paused || State == ELilyState::Clear || State == ELilyState::Over;
}

void ALilyPond::SetState(ELilyState NewState)
{
	State = NewState;
	SyncMusic();
}

void ALilyPond::SyncMusic()
{
	if (!Synth) return;
	const bool bMenu = IsMenuOpen();
	const ELilySong Song = State == ELilyState::Title ? ELilySong::Title : ELilySong::Game;
	const float Bpm = State == ELilyState::Title ? 84.f : (bMenu ? 90.f : LevelBpm);
	const float Vol = !bMusicOn ? 0.f : (bMenu ? 0.42f : 0.6f);
	Synth->SetMusic(Song, Bpm, Vol);
	Synth->SetMasterOn(bSoundOn);
}

void ALilyPond::StartGame()
{
	Score = 0; Hearts = 3;
	LoadLevel(1);
	SetState(ELilyState::Play);
	Sfx(ELilySfx::Croak);
}

void ALilyPond::HandleButton(FName Id)
{
	if (Id == TEXT("start") || Id == TEXT("again")) StartGame();
	else if (Id == TEXT("next")) { Hearts = FMath::Min(3, Hearts + 1); LoadLevel(Level + 1); SetState(ELilyState::Play); }
	else if (Id == TEXT("resume")) SetState(ELilyState::Play);
	else if (Id == TEXT("quit")) { SaveProgress(); LoadLevel(1); SetState(ELilyState::Title); }
	else if (Id == TEXT("pause")) { if (State == ELilyState::Play) SetState(ELilyState::Paused); }
	else if (Id == TEXT("music")) { bMusicOn = !bMusicOn; if (bMusicOn) bSoundOn = true; SyncMusic(); SaveProgress(); }
	else if (Id == TEXT("sound")) { bSoundOn = !bSoundOn; SyncMusic(); SaveProgress(); }
}

void ALilyPond::HandleConfirm()
{
	switch (State)
	{
	case ELilyState::Title:  HandleButton(TEXT("start")); break;
	case ELilyState::Paused: HandleButton(TEXT("resume")); break;
	case ELilyState::Clear:  HandleButton(TEXT("next")); break;
	case ELilyState::Over:   HandleButton(TEXT("again")); break;
	default: break;
	}
}

void ALilyPond::HandlePauseKey()
{
	if (State == ELilyState::Play) SetState(ELilyState::Paused);
	else if (State == ELilyState::Paused) SetState(ELilyState::Play);
}

void ALilyPond::LoadProgress()
{
	if (ULilySave* Save = Cast<ULilySave>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0)))
	{
		Best = Save->Best;
		bMusicOn = Save->bMusicOn;
		bSoundOn = Save->bSoundOn;
	}
}

void ALilyPond::SaveProgress()
{
	Best = FMath::Max(Best, Score);
	ULilySave* Save = Cast<ULilySave>(UGameplayStatics::CreateSaveGameObject(ULilySave::StaticClass()));
	if (!Save) return;
	Save->Best = Best;
	Save->bMusicOn = bMusicOn;
	Save->bSoundOn = bSoundOn;
	UGameplayStatics::SaveGameToSlot(Save, SaveSlot, 0);
}

void ALilyPond::Sfx(ELilySfx S)
{
	if (Synth) Synth->PlaySfx(S);
}

/* ------------------------------------------------------------------ */
/* Tick                                                                */
/* ------------------------------------------------------------------ */

void ALilyPond::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The camera is ours; make sure the player is looking through it.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (Camera && PC->GetViewTarget() != Camera) PC->SetViewTarget(Camera);
	}

	const float Dt = FMath::Min(DeltaSeconds, 1.f / 30.f);
	if (State != ELilyState::Paused && State != ELilyState::Clear)
	{
		const int32 Steps = Dt > 1.f / 90.f ? 2 : 1;
		for (int32 i = 0; i < Steps; i++) Step(Dt / Steps);
	}
	UpdateVisuals(Dt);
}

void ALilyPond::Step(float Dt)
{
	GameTime += Dt;

	bool bLeft = false, bRight = false, bJump = false, bTongue = false;
	if (ALilyPlayerController* PC = Cast<ALilyPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->GetHeld(bLeft, bRight, bJump, bTongue);
	}

	UpdatePlatforms(Dt);
	UpdateFlies();

	if (State == ELilyState::Play || State == ELilyState::Clearing)
	{
		if (State == ELilyState::Play)
		{
			LevelTime += Dt;
			if (bJump && !bPrevJump) Frog.Buffer = 0.13f;
			if (bTongue && !bPrevTongue) FireTongue();
		}
		else
		{
			bLeft = bRight = bJump = false;
		}
		UpdateFrog(Dt, bLeft, bRight, bJump);
		UpdateTongue(Dt);

		if (State == ELilyState::Play && Left == 0 && Tongue.Phase == 0)
		{
			State = ELilyState::Clearing;
			ClearTimer = 1.1f;
			Sfx(ELilySfx::Clear);
			Burst(FrogWorld() + FVector(0, 0, 30), 30, { Pink, Gold, LilyHex(0xA98BE0), Cream, LilyHex(0x6FB6E8) }, 380.f, false);
		}
		if (State == ELilyState::Clearing)
		{
			ClearTimer -= Dt;
			if (ClearTimer <= 0.f)
			{
				ClearBonus = FMath::Max(0, FMath::RoundToInt((Total * 12.f - LevelTime) * 5.f));
				Score += ClearBonus;
				SaveProgress();
				SetState(ELilyState::Clear);
			}
		}
	}
	else if (State == ELilyState::Title)
	{
		// Attract mode: the frog sits on the first island and now and then snaps at a fly.
		Frog.Ground = 0; Frog.Z = Platforms[0].Top;
		if (FMath::Sin(GameTime * 0.9f) > 0.995f && Tongue.Phase == 0) FireTongue();
		UpdateTongue(Dt);
		for (FLilyFly& F : Flies) F.bAlive = true;
		Left = Total; Score = 0; Combo = 0;
	}
	else if (State == ELilyState::Over && OverTimer > 0.f)
	{
		OverTimer -= Dt;
		if (OverTimer <= 0.f) SetState(ELilyState::Over);   // re-sync music once the jingle has played
	}

	bPrevJump = bJump; bPrevTongue = bTongue;
	UpdateFx(Dt);
}

void ALilyPond::UpdatePlatforms(float Dt)
{
	for (FLilyPlatform& P : Platforms)
	{
		if (P.Amp > 0.f)
		{
			const float NX = P.X0 + FMath::Sin(GameTime * P.Spd + P.Phase) * P.Amp;
			P.DX = NX - P.X; P.X = NX;
		}
		else P.DX = 0.f;
		if (P.Type == ELilyPlat::Pad) P.Top = P.BaseTop + FMath::Sin(GameTime * 2.f + P.Phase) * 1.5f;
		if (P.Squish > 0.f) P.Squish = FMath::Max(0.f, P.Squish - Dt * 3.f);
	}
}

void ALilyPond::UpdateFlies()
{
	for (FLilyFly& F : Flies)
	{
		if (!F.bAlive || F.bCaught) continue;
		F.P.X = F.HX + FMath::Sin(GameTime * F.Sp * 1.3f + F.Ph) * F.RX + FMath::Sin(GameTime * F.Sp * 3.7f + F.Ph * 2.f) * 7.f;
		F.P.Y = F.HZ + FMath::Sin(GameTime * F.Sp * 2.1f + F.Ph) * F.RZ + FMath::Cos(GameTime * 9.f + F.Ph) * 3.f;
	}
}

void ALilyPond::UpdateFrog(float Dt, bool bLeft, bool bRight, bool bJumpHeld)
{
	FFrogState& F = Frog;
	const float Ax = (bRight ? 1.f : 0.f) - (bLeft ? 1.f : 0.f);
	if (Ax != 0.f)
	{
		if (Tongue.Phase == 0) F.Face = Ax;
		F.VX = FMath::Clamp(F.VX + Ax * (F.Ground >= 0 ? LilyTune::Accel : LilyTune::AirAccel) * Dt, -LilyTune::Run, LilyTune::Run);
	}
	else if (F.Ground >= 0)
	{
		const float Fr = LilyTune::Friction * Dt;
		F.VX = FMath::Abs(F.VX) <= Fr ? 0.f : F.VX - FMath::Sign(F.VX) * Fr;
	}
	else F.VX *= 1.f - 1.2f * Dt;

	if (F.Buffer > 0.f) F.Buffer -= Dt;
	if (F.Buffer > 0.f && (F.Ground >= 0 || F.Coyote > 0.f))
	{
		F.VZ = LilyTune::JumpV; F.Ground = -1; F.Coyote = 0.f; F.Buffer = 0.f; F.bCut = false; F.Air = 0.f;
		Burst(FVector(F.X, 0, F.Z + 2), 5, { FLinearColor(1, 1, 1) }, 80.f, false);
		Sfx(ELilySfx::Jump);
	}

	if (F.Ground >= 0) F.X += Platforms[F.Ground].DX;
	F.X += F.VX * Dt;
	F.X = FMath::Max(F.X, -180.f);

	if (F.Ground >= 0)
	{
		const FLilyPlatform& P = Platforms[F.Ground];
		if (F.X + LilyTune::Foot < P.X || F.X - LilyTune::Foot > P.X + P.W)
		{
			F.Ground = -1; F.Coyote = 0.1f; F.bCut = true; F.Air = 0.f;
		}
		else { F.Z = P.Top; F.VZ = 0.f; }
	}
	if (F.Ground < 0)
	{
		F.Coyote -= Dt; F.Air += Dt;
		F.VZ = FMath::Max(F.VZ - LilyTune::Gravity * Dt, -LilyTune::MaxFall);
		// Let go of jump early for a shorter hop.
		if (!F.bCut && !bJumpHeld && F.Air > 0.06f && F.VZ > 250.f) { F.VZ *= 0.5f; F.bCut = true; }
		const float PrevZ = F.Z;
		F.Z += F.VZ * Dt;
		if (F.VZ <= 0.f)
		{
			for (int32 i = 0; i < Platforms.Num(); i++)
			{
				const FLilyPlatform& P = Platforms[i];
				if (F.X + LilyTune::Foot > P.X && F.X - LilyTune::Foot < P.X + P.W && PrevZ >= P.Top - 8.f && F.Z <= P.Top)
				{
					LandOn(i);
					break;
				}
			}
		}
		if (F.Z < -28.f && State == ELilyState::Play) Hurt();
	}
	if (F.Land > 0.f) F.Land = FMath::Max(0.f, F.Land - Dt * 5.f);
	if (F.Gulp > 0.f) F.Gulp = FMath::Max(0.f, F.Gulp - Dt * 2.5f);
	if (F.Inv > 0.f) F.Inv -= Dt;

	// Bumping into a fly eats it too.
	const FVector2D C(F.X, F.Z + 18.f);
	for (int32 i = 0; i < Flies.Num(); i++)
	{
		const FLilyFly& Fl = Flies[i];
		if (Fl.bAlive && !Fl.bCaught && FVector2D::Distance(Fl.P, C) < 27.f) Eat(i);
	}

	// Idle croak
	if (F.Ground >= 0 && FMath::Abs(F.VX) < 1.f)
	{
		F.Croak -= Dt;
		if (F.Croak <= 0.f) { Sfx(ELilySfx::Croak); F.Gulp = 0.8f; F.Croak = FMath::FRandRange(5.f, 10.f); }
	}
	else F.Croak = FMath::Max(F.Croak, 3.f);
}

void ALilyPond::LandOn(int32 Index)
{
	FLilyPlatform& P = Platforms[Index];
	if (P.Type == ELilyPlat::Bloom)
	{
		Frog.VZ = LilyTune::BounceV; Frog.bCut = true; Frog.Air = 0.f; Frog.Z = P.Top; P.Squish = 1.f;
		Burst(FVector(P.X + P.W * 0.5f, 0, P.Top), 14, { LilyHex(0xE7D6FF), LilyHex(0xB08AE8), Cream }, 200.f, false);
		Sfx(ELilySfx::Bounce);
		return;
	}
	const float Impact = FMath::Clamp(-Frog.VZ / 1100.f, 0.25f, 1.f);
	Frog.Z = P.Top; Frog.VZ = 0.f; Frog.Ground = Index; Frog.Land = Impact; Frog.Safe = Index;
	if (P.Type == ELilyPlat::Pad) Burst(FVector(Frog.X, 0, P.Top), 6, { LilyHex(0xBFE9F5), FLinearColor::White }, 120.f, true);
	Sfx(ELilySfx::Land);
}

void ALilyPond::Hurt()
{
	Hearts--;
	Sfx(ELilySfx::Splash);
	Burst(FVector(Frog.X, 0, 0), 24, { LilyHex(0xBFE9F5), LilyHex(0xE6F7FB), LilyHex(0x7CC3DC) }, 320.f, true);
	if (Tongue.Caught >= 0) { Flies[Tongue.Caught].bCaught = false; Tongue.Caught = -1; }
	Tongue.Phase = 0; Tongue.Len = 0.f;

	if (Hearts <= 0)
	{
		SaveProgress();
		State = ELilyState::Over;      // music re-syncs after the jingle (OverTimer)
		if (Synth) Synth->SetMusic(ELilySong::Game, LevelBpm, 0.f);
		OverTimer = 0.9f;
		Sfx(ELilySfx::Over);
		Frog.Z = -400.f; Frog.VZ = 0.f;
		return;
	}
	const FLilyPlatform& P = Platforms[FMath::Clamp(Frog.Safe, 0, Platforms.Num() - 1)];
	Frog.X = P.X + P.W * 0.5f; Frog.Z = P.Top + 140.f;
	Frog.VX = Frog.VZ = 0.f; Frog.Ground = -1; Frog.Inv = 1.6f; Frog.bCut = true;
}

void ALilyPond::FireTongue()
{
	if (Tongue.Phase != 0 || Tongue.Cooldown > 0.f) return;
	const FVector2D M = Mouth();
	int32 BestFly = -1; float BestScore = 1e9f;
	for (int32 i = 0; i < Flies.Num(); i++)
	{
		const FLilyFly& F = Flies[i];
		if (!F.bAlive || F.bCaught) continue;
		const FVector2D D = F.P - M;
		const float Dist = D.Size();
		if (Dist > LilyTune::Reach) continue;
		const float S = Dist - (FMath::Sign(D.X) == Frog.Face ? 60.f : 0.f);
		if (S < BestScore) { BestScore = S; BestFly = i; }
	}
	Tongue.Target = BestFly; Tongue.Len = 0.f; Tongue.Phase = 1;
	if (BestFly >= 0)
	{
		const FLilyFly& F = Flies[BestFly];
		if (FMath::Abs(F.P.X - Frog.X) > 6.f) Frog.Face = FMath::Sign(F.P.X - Frog.X);
		const FVector2D M2 = Mouth();
		Tongue.Ang = FMath::Atan2(F.P.Y - M2.Y, F.P.X - M2.X);
		Tongue.Max = LilyTune::Reach + 40.f;
	}
	else
	{
		Tongue.Ang = Frog.Face > 0.f ? 0.32f : PI - 0.32f;
		Tongue.Max = 150.f;
	}
	Sfx(ELilySfx::Tongue);
}

void ALilyPond::UpdateTongue(float Dt)
{
	if (Tongue.Cooldown > 0.f) Tongue.Cooldown -= Dt;
	if (Tongue.Phase == 0) return;
	const FVector2D M = Mouth();
	if (Tongue.Phase == 1)
	{
		if (Flies.IsValidIndex(Tongue.Target))
		{
			const FLilyFly& F = Flies[Tongue.Target];
			if (F.bAlive && !F.bCaught) Tongue.Ang = FMath::Atan2(F.P.Y - M.Y, F.P.X - M.X);   // homing
		}
		Tongue.Len += 2100.f * Dt;
		const FVector2D Tip = M + FVector2D(FMath::Cos(Tongue.Ang), FMath::Sin(Tongue.Ang)) * Tongue.Len;
		int32 Hit = -1;
		for (int32 i = 0; i < Flies.Num(); i++)
		{
			const FLilyFly& F = Flies[i];
			if (F.bAlive && !F.bCaught && FVector2D::Distance(F.P, Tip) < (i == Tongue.Target ? 26.f : 20.f)) { Hit = i; break; }
		}
		if (Hit >= 0) { Flies[Hit].bCaught = true; Tongue.Caught = Hit; Tongue.Phase = 2; }
		else if (Tongue.Len >= Tongue.Max) Tongue.Phase = 2;
	}
	else
	{
		Tongue.Len -= 2500.f * Dt;
		if (Tongue.Caught >= 0)
		{
			Flies[Tongue.Caught].P = M + FVector2D(FMath::Cos(Tongue.Ang), FMath::Sin(Tongue.Ang)) * FMath::Max(0.f, Tongue.Len);
		}
		if (Tongue.Len <= 0.f)
		{
			Tongue.Len = 0.f; Tongue.Phase = 0; Tongue.Cooldown = 0.12f; Tongue.Target = -1;
			if (Tongue.Caught >= 0) { const int32 C = Tongue.Caught; Tongue.Caught = -1; Eat(C); }
		}
	}
}

void ALilyPond::Eat(int32 Index)
{
	FLilyFly& F = Flies[Index];
	if (!F.bAlive) return;
	F.bAlive = false; F.bCaught = false;
	Left--;
	Combo = (GameTime - LastEat < 2.5f) ? Combo + 1 : 1;
	LastEat = GameTime;
	const int32 Pts = (F.bGold ? 50 : 10) * Combo;
	Score += Pts;
	Frog.Gulp = 1.f;
	const FVector2D M = Mouth();
	const FVector At(M.X, 0.f, M.Y + 6.f);
	if (F.bGold) Burst(At, 22, { LilyHex(0xFFE27A), Gold, Cream }, 280.f, false);
	else Burst(At, 12, { Cream, Pink, Gold }, 200.f, false);
	FloatText(At + FVector(0, 0, 30), FString::Printf(TEXT("+%d"), Pts), F.bGold ? LilyHex(0xFFE27A) : Cream, F.bGold ? 1.25f : 1.f);
	if (Combo > 1) FloatText(At + FVector(0, 0, 62), FString::Printf(TEXT("%dx combo"), Combo), LilyHex(0xF9B4CF), 0.8f);
	if (F.bGold) Sfx(ELilySfx::Gold);
	Sfx(ELilySfx::Gulp);
	if (F.Actor) F.Actor->SetActorHiddenInGame(true);
}

/* ------------------------------------------------------------------ */
/* Effects                                                             */
/* ------------------------------------------------------------------ */

void ALilyPond::Burst(const FVector& At, int32 Count, const TArray<FLinearColor>& Colors, float Speed, bool bDrops)
{
	if (FxPool.Num() == 0 || Colors.Num() == 0) return;
	for (int32 i = 0; i < Count; i++)
	{
		const float A = FMath::FRandRange(0.f, 2.f * PI);
		const float V = Speed * FMath::FRandRange(0.35f, 1.f);
		FLilyParticle P;
		P.P = At + FVector(0, FMath::FRandRange(-20.f, -5.f), 0);   // slightly toward the camera
		P.V = FVector(FMath::Cos(A) * V, 0.f, FMath::Sin(A) * V + (bDrops ? 180.f : 40.f));
		P.Life = P.Max = FMath::FRandRange(0.5f, 0.95f);
		P.Size = bDrops ? FMath::FRandRange(4.f, 7.f) : FMath::FRandRange(4.f, 8.f);
		P.bDrop = bDrops;
		P.Comp = NextFx;
		NextFx = (NextFx + 1) % FxPool.Num();
		// Recycle: retire any particle already using this mesh.
		Particles.RemoveAll([&P](const FLilyParticle& Q) { return Q.Comp == P.Comp; });
		UStaticMeshComponent* C = FxPool[P.Comp];
		FLilyArt::SetColor(C, Colors[i % Colors.Num()]);
		C->SetVisibility(true);
		Particles.Add(P);
	}
}

void ALilyPond::FloatText(const FVector& At, const FString& Text, const FLinearColor& Color, float Size)
{
	FLilyText T;
	T.Pos = At; T.Text = Text; T.Color = Color; T.Size = Size; T.Life = 1.1f;
	Texts.Add(T);
}

void ALilyPond::UpdateFx(float Dt)
{
	for (FLilyParticle& P : Particles)
	{
		P.Life -= Dt;
		P.P += P.V * Dt;
		if (P.bDrop) P.V.Z -= 900.f * Dt;
		else { P.V *= 0.94f; P.V.Z -= 60.f * Dt; }
	}
	for (int32 i = Particles.Num() - 1; i >= 0; i--)
	{
		if (Particles[i].Life <= 0.f)
		{
			FxPool[Particles[i].Comp]->SetVisibility(false);
			Particles.RemoveAtSwap(i);
		}
	}
	for (FLilyText& T : Texts) { T.Life -= Dt; T.Pos.Z += 40.f * Dt; }
	Texts.RemoveAll([](const FLilyText& T) { return T.Life <= 0.f; });
}

bool ALilyPond::NearestFlyWorld(FVector& Out, bool& bGold) const
{
	float BestD = 1e9f; bool bFound = false;
	for (const FLilyFly& F : Flies)
	{
		if (!F.bAlive) continue;
		const float D = FVector2D::Distance(F.P, FVector2D(Frog.X, Frog.Z + 20.f));
		if (D < BestD) { BestD = D; Out = FVector(F.P.X, 0.f, F.P.Y); bGold = F.bGold; bFound = true; }
	}
	return bFound;
}

/* ------------------------------------------------------------------ */
/* Presentation                                                        */
/* ------------------------------------------------------------------ */

void ALilyPond::UpdateCamera(float Dt, bool bSnap)
{
	const float TX = Frog.X + 100.f + Frog.Face * 50.f;
	const float TZ = FMath::Max(LilyTune::CamRestZ, Frog.Z + 40.f);
	if (bSnap) { CamPos.X = TX; CamPos.Z = TZ; }
	else
	{
		CamPos.X += (TX - CamPos.X) * FMath::Min(1.f, Dt * 5.f);
		CamPos.Z += (TZ - CamPos.Z) * FMath::Min(1.f, Dt * 4.f);
	}
	CamPos.X = FMath::Clamp(CamPos.X, 490.f, FMath::Max(490.f, LevelEnd - 530.f));
	CamPos.Z = FMath::Max(CamPos.Z, LilyTune::CamRestZ);
	const float PitchLift = FMath::Tan(FMath::DegreesToRadians(-LilyTune::CamPitch)) * LilyTune::CamDistance;
	if (Camera) Camera->SetActorLocation(FVector(CamPos.X, -LilyTune::CamDistance, CamPos.Z + PitchLift));
	if (SkyActor) SkyActor->SetActorLocation(FVector(CamPos.X * 0.9f, 0.f, 0.f));
}

void ALilyPond::UpdateVisuals(float Dt)
{
	// Platforms: drifting pads, bobbing, bellflower squash
	for (FLilyPlatform& P : Platforms)
	{
		if (!P.Actor) continue;
		P.Actor->SetActorLocation(FVector(P.X, 0.f, P.Top - P.BaseTop));
		if (P.Cup)
		{
			const float Sq = FMath::Sin(P.Squish * PI) * 0.35f * P.Squish;
			P.Cup->SetRelativeScale3D(FVector(1.f + Sq * 0.6f, 1.f + Sq * 0.6f, 1.f - Sq));
		}
	}

	// Flies
	for (FLilyFly& F : Flies)
	{
		if (!F.Actor) continue;
		F.Actor->SetActorHiddenInGame(!F.bAlive);   // title-screen flies come back after the attract-mode frog eats them
		if (!F.bAlive) continue;
		F.Actor->SetActorLocation(FVector(F.P.X, 0.f, F.P.Y));
		const float Flap = FMath::Sin(GameTime * 70.f + F.Ph) * 35.f;
		if (F.WingL) F.WingL->SetRelativeRotation(FRotator(0, 0, -25.f - Flap));
		if (F.WingR) F.WingR->SetRelativeRotation(FRotator(0, 0, 25.f + Flap));
	}

	// Frog
	if (FrogActor)
	{
		const bool bVisible = !(State == ELilyState::Over && Frog.Z < -100.f);
		const bool bBlink = Frog.Inv > 0.f && FMath::FloorToInt(Frog.Inv * 14.f) % 2 == 0;
		FrogActor->SetActorHiddenInGame(!bVisible || bBlink);
		FrogActor->SetActorLocation(FVector(Frog.X, 0.f, Frog.Z));

		float SZ = 1.f, SX = 1.f;
		const bool bAir = Frog.Ground < 0;
		if (bAir) { const float S = FMath::Clamp(Frog.VZ / 2600.f, -0.15f, 0.22f); SZ = 1.f + S; SX = 1.f - S * 0.6f; }
		if (Frog.Land > 0.f) { const float S = Frog.Land * 0.26f; SZ *= 1.f - S; SX *= 1.f + S * 0.8f; }
		Rig.Body->SetRelativeScale3D(FVector(SX, SX, SZ));
		Rig.Body->SetRelativeRotation(FRotator(0.f, Frog.Face > 0.f ? 0.f : 180.f, 0.f));

		for (int32 s = 0; s < 2; s++)
		{
			const float Y = s == 0 ? -1.f : 1.f;
			Rig.Thigh[s]->SetRelativeLocationAndRotation(bAir ? FVector(-20, 14 * Y, 10) : FVector(-12, 14 * Y, 10), FRotator(bAir ? 30.f : 0.f, 0, 0));
			Rig.Foot[s]->SetRelativeLocation(bAir ? FVector(-36, 14 * Y, 2) : FVector(-6, 16 * Y, 2));
		}

		// Eyes follow the nearest fly
		FVector Fly; bool bGold;
		FVector2D Look(1.f, 0.f);
		if (NearestFlyWorld(Fly, bGold))
		{
			Look = FVector2D((Fly.X - Frog.X) * Frog.Face, Fly.Z - (Frog.Z + 33.f)).GetSafeNormal();
		}
		for (int32 s = 0; s < 2; s++)
		{
			const float Y = s == 0 ? -1.f : 1.f;
			Rig.Pupil[s]->SetRelativeLocation(FVector(11.5f + Look.X * 2.5f, 10.5f * Y, 33.5f + Look.Y * 2.5f));
		}
		const float G = Frog.Gulp;
		Rig.Throat->SetRelativeScale3D(FVector(8 + G * 8, 14 + G * 8, 6 + G * 8) / 100.f);

		// Tongue
		const bool bTongue = Tongue.Phase != 0 && Tongue.Len > 0.f;
		Rig.Tongue->SetVisibility(bTongue);
		Rig.TongueTip->SetVisibility(bTongue);
		if (bTongue)
		{
			const FVector2D M = Mouth();
			const FVector Dir(FMath::Cos(Tongue.Ang), 0.f, FMath::Sin(Tongue.Ang));
			const FVector MouthLocal(M.X - Frog.X, -2.f, M.Y - Frog.Z);
			Rig.Tongue->SetRelativeLocationAndRotation(MouthLocal + Dir * Tongue.Len * 0.5f, FRotationMatrix::MakeFromZ(Dir).Rotator());
			Rig.Tongue->SetRelativeScale3D(FVector(0.06f, 0.06f, FMath::Max(Tongue.Len, 1.f) / 100.f));
			Rig.TongueTip->SetRelativeLocation(MouthLocal + Dir * Tongue.Len);
		}
	}

	// Particles
	for (const FLilyParticle& P : Particles)
	{
		UStaticMeshComponent* C = FxPool[P.Comp];
		const float A = FMath::Clamp(P.Life / P.Max, 0.f, 1.f);
		C->SetWorldLocation(P.P);
		C->SetWorldScale3D(FVector(P.Size * (0.3f + 0.7f * A) / 100.f));
	}

	// Clouds drift and wrap around the camera
	for (int32 i = 0; i < Clouds.Num(); i++)
	{
		ALilyProp* C = Clouds[i];
		FVector L = C->GetActorLocation();
		L.X += 20.f * Dt;
		const float Span = 13500.f;
		if (L.X < CamPos.X - Span * 0.5f) L.X += Span;
		if (L.X > CamPos.X + Span * 0.5f) L.X -= Span;
		C->SetActorLocation(L);
	}

	UpdateCamera(Dt, false);
}
