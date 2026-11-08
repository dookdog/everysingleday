#include "JungleGameMode.h"
#include "LionelCharacter.h"
#include "LionelPlayerController.h"
#include "JungleHUD.h"
#include "JungleEnemy.h"
#include "JungleActors.h"
#include "JungleAudio.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

static const TCHAR* GFlavor[] = {
	TEXT("Thornlings in the ferns"), TEXT("Mud is moving"), TEXT("Masks in the canopy"), TEXT("The ground is humming"), TEXT(""),
	TEXT("Night monkeys went quiet"), TEXT("Something big is breathing"), TEXT("Roots are cracking"), TEXT("Hold the steps"), TEXT("")
};

AJungleGameMode::AJungleGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ALionelCharacter::StaticClass();
	PlayerControllerClass = ALionelPlayerController::StaticClass();
	HUDClass = AJungleHUD::StaticClass();
}

AJungleGameMode* AJungleGameMode::Get(const UObject* WorldContext)
{
	return Cast<AJungleGameMode>(UGameplayStatics::GetGameMode(WorldContext));
}

ALionelCharacter* AJungleGameMode::Player() const
{
	return Cast<ALionelCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

int32 AJungleGameMode::AliveCount() const
{
	int32 N = 0;
	for (const AJungleEnemy* E : Enemies) { if (E && E->bAlive) { ++N; } }
	return N;
}

void AJungleGameMode::StartPlay()
{
	LoadSettings();
	BuildWorld();
	Super::StartPlay();
}

void AJungleGameMode::LoadSettings()
{
	if (!GConfig) { return; }
	GConfig->GetInt(TEXT("Lionel"), TEXT("Best"), Best, GGameUserSettingsIni);
	GConfig->GetFloat(TEXT("Lionel"), TEXT("Sensitivity"), Sensitivity, GGameUserSettingsIni);
	GConfig->GetBool(TEXT("Lionel"), TEXT("Sound"), bSoundOn, GGameUserSettingsIni);
	GConfig->GetBool(TEXT("Lionel"), TEXT("Music"), bMusicOn, GGameUserSettingsIni);
	Sensitivity = FMath::Clamp(Sensitivity, 0.4f, 2.2f);
}

void AJungleGameMode::SaveSettings()
{
	if (!GConfig) { return; }
	GConfig->SetInt(TEXT("Lionel"), TEXT("Best"), Best, GGameUserSettingsIni);
	GConfig->SetFloat(TEXT("Lionel"), TEXT("Sensitivity"), Sensitivity, GGameUserSettingsIni);
	GConfig->SetBool(TEXT("Lionel"), TEXT("Sound"), bSoundOn, GGameUserSettingsIni);
	GConfig->SetBool(TEXT("Lionel"), TEXT("Music"), bMusicOn, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

// ============================================================================================
// World building
// ============================================================================================

void AJungleGameMode::BuildWorld()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	WorldActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SP);
	USceneComponent* Root = NewObject<USceneComponent>(WorldActor, TEXT("JungleRoot"));
	Root->SetMobility(EComponentMobility::Movable);
	WorldActor->SetRootComponent(Root);
	Root->RegisterComponent();

	const bool bMobile = (PLATFORM_IOS || PLATFORM_ANDROID) != 0;

	// Sun through the canopy, with light shafts.
	UDirectionalLightComponent* Sun = NewObject<UDirectionalLightComponent>(WorldActor);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetupAttachment(Root);
	Sun->SetRelativeRotation(FRotator(-38.f, 35.f, 0.f));
	Sun->bAtmosphereSunLight = true;
	Sun->bEnableLightShaftBloom = true;
	Sun->BloomScale = 0.25f;
	Sun->RegisterComponent();
	Sun->SetIntensity(7.f);
	Sun->SetLightColor(FLinearColor(1.f, 0.88f, 0.66f));

	// Soft green bounce from the opposite side, no shadows.
	UDirectionalLightComponent* Fill = NewObject<UDirectionalLightComponent>(WorldActor);
	Fill->SetMobility(EComponentMobility::Movable);
	Fill->SetupAttachment(Root);
	Fill->SetRelativeRotation(FRotator(-55.f, 215.f, 0.f));
	Fill->RegisterComponent();
	Fill->SetIntensity(1.2f);
	Fill->SetCastShadows(false);
	Fill->SetLightColor(FLinearColor(0.55f, 0.8f, 0.5f));

	USkyAtmosphereComponent* Sky = NewObject<USkyAtmosphereComponent>(WorldActor);
	Sky->SetupAttachment(Root);
	Sky->RegisterComponent();

	USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(WorldActor);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetupAttachment(Root);
	SkyLight->bRealTimeCapture = !bMobile;
	SkyLight->RegisterComponent();
	SkyLight->SetIntensity(1.3f);
	SkyLight->RecaptureSky();

	// Thick green jungle haze. Raise FogDensity for a murkier jungle, lower it to see further.
	UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(WorldActor);
	Fog->SetupAttachment(Root);
	Fog->RegisterComponent();
	Fog->SetFogDensity(0.12f);
	Fog->SetFogHeightFalloff(0.08f);
	Fog->SetFogInscatteringColor(FLinearColor(0.22f, 0.36f, 0.18f));
	Fog->SetStartDistance(400.f);

	BuildTerrain(Root);
	BuildTemple(Root);
	BuildJungle(Root);

	FX = World->SpawnActor<AJungleFX>(AJungleFX::StaticClass(), FTransform::Identity, SP);
	Audio = World->SpawnActor<AJungleAudio>(AJungleAudio::StaticClass(), FTransform::Identity, SP);
}

void AJungleGameMode::BuildTerrain(USceneComponent* Root)
{
	UProceduralMeshComponent* T = NewObject<UProceduralMeshComponent>(WorldActor, TEXT("Terrain"));
	T->SetMobility(EComponentMobility::Movable);
	T->SetupAttachment(Root);
	T->bUseComplexAsSimpleCollision = true;
	T->RegisterComponent();
	WorldActor->AddInstanceComponent(T);

	// Four flat-shaded sections: dark leaf, light leaf, riverbank mud, temple paving.
	constexpr int32 NB = 4;
	TArray<FVector> V[NB];
	TArray<int32> I[NB];
	TArray<FVector> N[NB];
	const FLinearColor Cols[NB] = { JW::Hex(0x2b4d20), JW::Hex(0x4f7429), JW::Hex(0x5b4a2c), JW::Hex(0x6f6b55) };

	constexpr int32 Cells = 120;
	constexpr float Size = 270.f;
	const float Step = Size / Cells;
	auto Pt = [](float X, float Y) { return FVector(X, Y, JW::GroundH(X, Y)); };

	auto AddTri = [&](FVector A, FVector B, FVector C)
	{
		FVector Nrm = FVector::CrossProduct(B - C, A - C).GetSafeNormal();
		if (Nrm.Z < 0.f) { Swap(B, C); Nrm = -Nrm; }
		const FVector Ctr = (A + B + C) / 3.f;
		const float R = FMath::Sqrt(Ctr.X * Ctr.X + Ctr.Y * Ctr.Y);
		const float Nz = 0.5f + 0.5f * FMath::Sin(Ctr.X * 0.31f + Ctr.Y * 0.17f) * FMath::Cos(Ctr.Y * 0.23f - Ctr.X * 0.11f);
		int32 Bk = Nz < 0.5f ? 0 : 1;
		if (Ctr.Z < JW::Water + 0.5f) { Bk = 2; }
		if (R < 11.5f) { Bk = 3; }
		const int32 Base = V[Bk].Num();
		V[Bk].Add(A * JW::M); V[Bk].Add(B * JW::M); V[Bk].Add(C * JW::M);
		N[Bk].Add(Nrm); N[Bk].Add(Nrm); N[Bk].Add(Nrm);
		I[Bk].Add(Base); I[Bk].Add(Base + 1); I[Bk].Add(Base + 2);
	};

	for (int32 Gx = 0; Gx < Cells; ++Gx)
	{
		for (int32 Gy = 0; Gy < Cells; ++Gy)
		{
			const float X0 = -Size / 2 + Gx * Step, X1 = X0 + Step;
			const float Y0 = -Size / 2 + Gy * Step, Y1 = Y0 + Step;
			const FVector A = Pt(X0, Y0), B = Pt(X1, Y0), C = Pt(X0, Y1), D = Pt(X1, Y1);
			AddTri(A, C, B);
			AddTri(B, C, D);
		}
	}

	UJungleAssets* Assets = UJungleAssets::Get(this);
	for (int32 S = 0; S < NB; ++S)
	{
		TArray<FVector2D> UV;
		UV.SetNumZeroed(V[S].Num());
		T->CreateMeshSection_LinearColor(S, V[S], I[S], N[S], UV, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
		T->SetMaterial(S, Assets->Mat(Cols[S]));
	}

	// Water surface you can wade across. Bullets pass through it.
	UStaticMeshComponent* Water = JW::Part(WorldActor, Root, EShape::Cube, JW::Hex(0x2a6a5c), FVector(270.f, 270.f, 0.2f), FVector(0.f, 0.f, JW::Water - 0.1f), FRotator::ZeroRotator, true);
	Water->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Water->SetCastShadow(false);
}

void AJungleGameMode::BuildTemple(USceneComponent* Root)
{
	const FLinearColor Stone[4] = { JW::Hex(0x7d7a62), JW::Hex(0x878369), JW::Hex(0x928d70), JW::Hex(0x9c9677) };
	const FLinearColor Moss = JW::Hex(0x3f6a2a), Gold = JW::Hex(0xd9a634), Jade = JW::Hex(0x7dffb8);

	for (int32 i = 0; i < 4; ++i)
	{
		const float S = JW::Pyr[i];
		JW::Part(WorldActor, Root, EShape::Cube, Stone[i], FVector(2 * S, 2 * S, JW::Tier), FVector(0, 0, JW::Tier * i + JW::Tier / 2), FRotator::ZeroRotator, true);
	}
	for (int32 i = 0; i < 34; ++i)
	{
		const int32 Ti = FMath::RandRange(0, 3);
		const float S = JW::Pyr[Ti];
		const int32 Side = FMath::RandRange(0, 3);
		const float U = JW::Rand(-1.f, 1.f) * (S - 0.6f);
		const float X = Side < 2 ? U : (Side == 2 ? S - 0.3f : -S + 0.3f);
		const float Y = Side < 2 ? (Side == 0 ? S - 0.3f : -S + 0.3f) : U;
		JW::Part(WorldActor, Root, EShape::Cube, Moss, FVector(JW::Rand(0.4f, 1.f), JW::Rand(0.5f, 1.6f), 0.08f), FVector(X, Y, JW::Tier * (Ti + 1) + 0.03f), FRotator(0, JW::Rand(0, 180), 0));
	}

	// The Golden Jaguar on its pedestal, looking out over the jungle.
	const float Top = JW::Tier * 4;
	JW::Part(WorldActor, Root, EShape::Cube, Stone[3], FVector(1.5f, 1.5f, 0.6f), FVector(0, 0, Top + 0.3f), FRotator::ZeroRotator, true);
	JW::Part(WorldActor, Root, EShape::Cube, Gold, FVector(1.3f, 0.7f, 0.6f), FVector(0, 0, Top + 0.95f));
	JW::Part(WorldActor, Root, EShape::Cube, Gold, FVector(0.6f, 0.62f, 0.56f), FVector(0.62f, 0, Top + 1.45f));
	JW::Part(WorldActor, Root, EShape::Cone, Gold, FVector(0.24f, 0.24f, 0.22f), FVector(0.62f, 0.2f, Top + 1.82f));
	JW::Part(WorldActor, Root, EShape::Cone, Gold, FVector(0.24f, 0.24f, 0.22f), FVector(0.62f, -0.2f, Top + 1.82f));
	JW::Part(WorldActor, Root, EShape::Cube, Jade, FVector(0.04f, 0.1f, 0.07f), FVector(0.93f, 0.15f, Top + 1.52f));
	JW::Part(WorldActor, Root, EShape::Cube, Jade, FVector(0.04f, 0.1f, 0.07f), FVector(0.93f, -0.15f, Top + 1.52f));
	JW::Part(WorldActor, Root, EShape::Cube, Gold, FVector(0.9f, 0.1f, 0.1f), FVector(-0.95f, 0, Top + 1.1f), FRotator(35.f, 0, 0));
	Obstacles.Add({ 0.f, 0.f, 1.15f, Top + 2.f });

	// Braziers on the four top corners.
	const float Corners[4][2] = { { 2.4f, 2.4f }, { -2.4f, 2.4f }, { 2.4f, -2.4f }, { -2.4f, -2.4f } };
	for (const auto& C : Corners)
	{
		JW::Part(WorldActor, Root, EShape::Cylinder, Stone[0], FVector(0.6f, 0.6f, 0.7f), FVector(C[0], C[1], Top + 0.35f), FRotator::ZeroRotator, true);
		JW::Part(WorldActor, Root, EShape::Cylinder, JW::Hex(0x3b3a30), FVector(0.84f, 0.84f, 0.18f), FVector(C[0], C[1], Top + 0.78f));
		UStaticMeshComponent* F1 = JW::Part(WorldActor, Root, EShape::Cone, JW::Hex(0xff8a2a), FVector(0.6f, 0.6f, 0.7f), FVector(C[0], C[1], Top + 1.15f));
		UStaticMeshComponent* F2 = JW::Part(WorldActor, Root, EShape::Cone, JW::Hex(0xffe070), FVector(0.32f, 0.32f, 0.45f), FVector(C[0], C[1], Top + 1.05f));
		F1->SetCastShadow(false);
		F2->SetCastShadow(false);
		Flames.Add(F1);
		Flames.Add(F2);
		Obstacles.Add({ C[0], C[1], 0.5f, Top + 1.2f });
	}

	// Ring of broken pillars.
	for (int32 i = 0; i < 12; ++i)
	{
		const float A = i / 12.f * 2 * PI + 0.13f, R = JW::Rand(15.5f, 17.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.2f) { continue; }
		const float H = FMath::FRand() < 0.4f ? JW::Rand(1.2f, 2.6f) : JW::Rand(4.f, 6.5f);
		JW::Part(WorldActor, Root, EShape::Cylinder, Stone[1 + i % 3], FVector(1.7f, 1.7f, H), FVector(X, Y, G + H / 2 - 0.2f), FRotator(JW::Rand(-3, 3), 0, JW::Rand(-3, 3)), true);
		if (H > 3.5f)
		{
			JW::Part(WorldActor, Root, EShape::Cube, Stone[2], FVector(2.1f, 2.1f, 0.5f), FVector(X, Y, G + H - 0.2f), FRotator(0, JW::Rand(0, 90), 0), true);
		}
		JW::Part(WorldActor, Root, EShape::Cube, Moss, FVector(1.9f, 1.9f, 0.12f), FVector(X, Y, G + 0.05f), FRotator(0, JW::Rand(0, 90), 0));
		Obstacles.Add({ X, Y, 1.f, G + H });
	}
}

static UHierarchicalInstancedStaticMeshComponent* MakeHISM(AActor* Owner, USceneComponent* Root, EShape Shape, const FLinearColor& Color, bool bCollide)
{
	UJungleAssets* A = UJungleAssets::Get(Owner);
	UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);
	H->SetMobility(EComponentMobility::Movable);
	H->SetStaticMesh(A->Mesh(Shape));
	H->SetMaterial(0, A->Mat(Color));
	H->SetupAttachment(Root);
	if (bCollide)
	{
		H->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	H->RegisterComponent();
	Owner->AddInstanceComponent(H);
	return H;
}

void AJungleGameMode::BuildJungle(USceneComponent* Root)
{
	auto* KapokTrunks = MakeHISM(WorldActor, Root, EShape::Cylinder, JW::Hex(0x5d4630), true);
	auto* PalmTrunks = MakeHISM(WorldActor, Root, EShape::Cylinder, JW::Hex(0x8a6a44), true);
	UHierarchicalInstancedStaticMeshComponent* Canopy[3] = {
		MakeHISM(WorldActor, Root, EShape::Sphere, JW::Hex(0x2f5a23), false),
		MakeHISM(WorldActor, Root, EShape::Sphere, JW::Hex(0x41702a), false),
		MakeHISM(WorldActor, Root, EShape::Sphere, JW::Hex(0x5a8a2e), false) };
	auto* Crowns = MakeHISM(WorldActor, Root, EShape::Cone, JW::Hex(0x5f9a2f), false);
	auto* Vines = MakeHISM(WorldActor, Root, EShape::Cylinder, JW::Hex(0x3c5e22), false);
	UHierarchicalInstancedStaticMeshComponent* Ferns[2] = {
		MakeHISM(WorldActor, Root, EShape::Cone, JW::Hex(0x3d7a2a), false),
		MakeHISM(WorldActor, Root, EShape::Cone, JW::Hex(0x5e9a34), false) };
	auto* Rocks = MakeHISM(WorldActor, Root, EShape::Sphere, JW::Hex(0x6c6a5a), true);
	Vines->SetCastShadow(false);
	Ferns[0]->SetCastShadow(false);
	Ferns[1]->SetCastShadow(false);

	auto Clear = [this](float X, float Y, float MinD2)
	{
		for (const FObstacle& O : Obstacles)
		{
			const float Dx = O.X - X, Dy = O.Y - Y;
			if (Dx * Dx + Dy * Dy < MinD2) { return false; }
		}
		return true;
	};

	int32 Trees = 0;
	for (int32 Tries = 0; Tries < 5000 && Trees < 300; ++Tries)
	{
		const float A = JW::Rand(0, 2 * PI), R = 17.f + FMath::Pow(FMath::FRand(), 0.8f) * 100.f;
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.25f || !Clear(X, Y, 12.f)) { continue; }
		const bool bPalm = FMath::FRand() < 0.35f;
		const float H = bPalm ? JW::Rand(6.f, 9.5f) : JW::Rand(10.f, 17.f);
		const float S = bPalm ? JW::Rand(0.55f, 0.75f) : JW::Rand(1.f, 1.7f);
		const FRotator Tilt(JW::Rand(-3, 3), JW::Rand(0, 360), JW::Rand(-3, 3));
		(bPalm ? PalmTrunks : KapokTrunks)->AddInstance(FTransform(Tilt, FVector(X, Y, G - 0.3f + H / 2) * JW::M, FVector(0.95f * S, 0.95f * S, H)));
		Obstacles.Add({ X, Y, 0.5f * S + 0.12f, G + H });
		++Trees;

		if (bPalm)
		{
			const float CS = JW::Rand(6.f, 8.f);
			Crowns->AddInstance(FTransform(FRotator(JW::Rand(-8, 8), JW::Rand(0, 360), JW::Rand(-8, 8)), FVector(X, Y, G + H - 0.2f) * JW::M, FVector(CS, CS, 1.1f)));
			continue;
		}
		const int32 Blobs = FMath::RandRange(2, 3);
		for (int32 b = 0; b < Blobs; ++b)
		{
			const float BS = JW::Rand(3.f, 5.2f);
			const FVector BP(X + JW::Rand(-2.4f, 2.4f), Y + JW::Rand(-2.4f, 2.4f), G + H + JW::Rand(-1.f, 1.6f));
			Canopy[FMath::RandRange(0, 2)]->AddInstance(FTransform(FRotator(0, JW::Rand(0, 360), 0), BP * JW::M, FVector(2 * BS, 2 * BS, 1.24f * BS)));
			for (int32 v = 0; v < 3; ++v)
			{
				if (FMath::FRand() < 0.3f) { continue; }
				const float L = JW::Rand(3.f, FMath::Max(3.5f, FMath::Min(9.f, (float)BP.Z - G - 1.f)));
				const FVector VP(BP.X + JW::Rand(-BS * 0.7f, BS * 0.7f), BP.Y + JW::Rand(-BS * 0.7f, BS * 0.7f), BP.Z - BS * 0.3f - L / 2);
				Vines->AddInstance(FTransform(FRotator::ZeroRotator, VP * JW::M, FVector(0.08f, 0.08f, L)));
			}
		}
	}

	for (int32 i = 0, Placed = 0; i < 1500 && Placed < 750; ++i)
	{
		const float A = JW::Rand(0, 2 * PI), R = JW::Rand(11.f, 116.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.05f) { continue; }
		const float S = JW::Rand(0.6f, 1.5f);
		Ferns[i % 2]->AddInstance(FTransform(FRotator(JW::Rand(-10, 10), JW::Rand(0, 360), JW::Rand(-10, 10)), FVector(X, Y, G + S * 0.35f) * JW::M, FVector(2 * S, 2 * S, S * JW::Rand(0.6f, 1.1f))));
		++Placed;
	}

	for (int32 i = 0, Placed = 0; i < 300 && Placed < 70; ++i)
	{
		const float A = JW::Rand(0, 2 * PI), R = JW::Rand(18.f, 98.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (!Clear(X, Y, 9.f)) { continue; }
		const float S = JW::Rand(0.6f, 2.f);
		Rocks->AddInstance(FTransform(FRotator(JW::Rand(0, 360), JW::Rand(0, 360), JW::Rand(0, 360)), FVector(X, Y, G + 0.3f * S) * JW::M, FVector(2.4f * S, 2.f * S, 1.6f * S)));
		Obstacles.Add({ X, Y, S * 0.95f, G + S * 1.1f });
		++Placed;
	}

	// Fireflies drifting through the undergrowth.
	for (int32 i = 0; i < 60; ++i)
	{
		const float A = JW::Rand(0, 2 * PI), R = JW::Rand(6.f, 60.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R;
		const float Z = FMath::Max(JW::GroundH(X, Y), JW::Water) + JW::Rand(0.5f, 4.f);
		UStaticMeshComponent* F = JW::Part(WorldActor, Root, EShape::Sphere, JW::Hex(0xe4ff8a), FVector(0.1f), FVector(X, Y, Z));
		F->SetCastShadow(false);
		Fireflies.Add(F);
		FireflyBase.Add({ X, Y, Z, JW::Rand(0.f, 10.f) });
	}
}

// ============================================================================================
// Match flow
// ============================================================================================

void AJungleGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;

	for (int32 i = 0; i < Flames.Num(); ++i)
	{
		const float S = 1.f + FMath::Sin(Time * (13 + i) + i * 2) * 0.18f;
		const FVector Base = (i % 2) ? FVector(0.32f, 0.32f, 0.45f) : FVector(0.6f, 0.6f, 0.7f);
		Flames[i]->SetRelativeScale3D(FVector(Base.X, Base.Y, Base.Z * S));
		Flames[i]->SetRelativeRotation(FRotator(0, Time * 120 + i * 40, 0));
	}
	for (int32 i = 0; i < Fireflies.Num(); ++i)
	{
		const FFirefly& B = FireflyBase[i];
		const float T = Time * 0.6f + B.Phase;
		Fireflies[i]->SetRelativeLocation(FVector(B.X + FMath::Sin(T * 1.3f) * 1.2f, B.Y + FMath::Cos(T) * 1.2f, B.Z + FMath::Sin(T * 2.1f) * 0.5f) * JW::M);
	}

	BannerTime = FMath::Max(0.f, BannerTime - Dt);
	PopTime = FMath::Max(0.f, PopTime - Dt);
	HitTime = FMath::Max(0.f, HitTime - Dt);
	FreshTime = FMath::Max(0.f, FreshTime - Dt);
	HurtFlash = FMath::Max(0.f, HurtFlash - Dt * 1.4f);

	if (State == EJState::Playing)
	{
		UpdateWaves(Dt);
	}
}

void AJungleGameMode::StartGame()
{
	if (UGameplayStatics::IsGamePaused(this))
	{
		UGameplayStatics::SetGamePaused(this, false);
	}
	for (AJungleEnemy* E : Enemies) { if (E) { E->Destroy(); } }
	Enemies.Empty();
	Boss = nullptr;
	for (AJunglePickup* P : Pickups) { if (P) { P->Destroy(); } }
	Pickups.Empty();
	for (TActorIterator<AJungleProjectile> It(GetWorld()); It; ++It) { It->Destroy(); }

	Score = 0;
	Kills = 0;
	Wave = 0;
	bFight = false;
	WaveTimer = 2.2f;
	Queue.Empty();
	HurtFlash = 0.f;

	if (ALionelCharacter* P = Player()) { P->ResetForGame(); }
	State = EJState::Playing;
	Banner(TEXT("HOLD THE TEMPLE"), TEXT("They come from every side"), 2.2f);
}

void AJungleGameMode::TogglePause()
{
	if (State == EJState::Playing)
	{
		State = EJState::Paused;
		UGameplayStatics::SetGamePaused(this, true);
	}
	else if (State == EJState::Paused)
	{
		State = EJState::Playing;
		UGameplayStatics::SetGamePaused(this, false);
	}
}

void AJungleGameMode::GameOver()
{
	if (State != EJState::Playing) { return; }
	State = EJState::Over;
	Sfx(ESfx::Death);
	if (Score > Best)
	{
		Best = Score;
		SaveSettings();
	}
}

void AJungleGameMode::Banner(const FString& Title, const FString& Sub, float Duration)
{
	BannerTitle = Title;
	BannerSub = Sub;
	BannerTime = Duration;
}

void AJungleGameMode::Pop(const FString& Text)
{
	PopText = Text;
	PopTime = 0.8f;
}

void AJungleGameMode::HitMarker(bool bKill)
{
	bHitKill = bKill;
	HitTime = bKill ? 0.22f : 0.09f;
}

void AJungleGameMode::Sfx(ESfx S)
{
	if (Audio) { Audio->Play(S); }
}

void AJungleGameMode::StartWave(int32 N)
{
	Wave = N;
	Queue.Empty();
	if (N % 5 == 0)
	{
		Queue.Add(EEnemyType::Boss);
		for (int32 i = 0; i < 4 + N; ++i) { Queue.Add(FMath::FRand() < 0.4f ? EEnemyType::Mask : EEnemyType::Thorn); }
		Banner(TEXT("BOSS WAVE"), TEXT("The Idol Colossus awakens"), 2.6f);
		Sfx(ESfx::Roar);
	}
	else
	{
		const int32 Count = 5 + N * 3;
		for (int32 i = 0; i < Count; ++i)
		{
			const float R = FMath::FRand();
			Queue.Add(N >= 3 && R < 0.22f ? EEnemyType::Mask : (N >= 2 && R < 0.42f ? EEnemyType::Brute : EEnemyType::Thorn));
		}
		const FString Flavor = GFlavor[(N - 1) % UE_ARRAY_COUNT(GFlavor)];
		Banner(FString::Printf(TEXT("WAVE %d"), N), Flavor.IsEmpty() ? TEXT("Keep the temple") : Flavor, 2.2f);
	}
	SpawnT = 1.f;
	bFight = true;
}

void AJungleGameMode::WaveClear()
{
	bFight = false;
	WaveTimer = 5.5f;
	ALionelCharacter* P = Player();
	if (!P) { return; }
	P->Health = FMath::Min(100.f, P->Health + 20.f);
	P->Resupply(2.f);
	const int32 Unlock = Wave + 2;
	if (Unlock < NumWeapons && !P->Gun[Unlock].bOwned)
	{
		P->Gun[Unlock].bOwned = true;
		P->Select(Unlock);
		FreshSlot = Unlock;
		FreshTime = 3.f;
		Sfx(ESfx::Unlock);
		Banner(FString::Printf(TEXT("WAVE %d CLEARED"), Wave), FString(TEXT("New weapon: ")) + GWeapons[Unlock].Full, 3.f);
	}
	else
	{
		Banner(FString::Printf(TEXT("WAVE %d CLEARED"), Wave), TEXT("+20 health, ammo resupplied"), 2.6f);
	}
}

void AJungleGameMode::UpdateWaves(float Dt)
{
	if (bFight)
	{
		const int32 Alive = AliveCount();
		SpawnT -= Dt;
		if (Queue.Num() > 0 && SpawnT <= 0.f && Alive < 10 + FMath::Min(Wave, 8))
		{
			const EEnemyType T = Queue[0];
			Queue.RemoveAt(0);
			SpawnEnemy(T);
			SpawnT = JW::Rand(0.5f, 1.4f) * FMath::Max(0.45f, 1.f - Wave * 0.04f);
		}
		if (Queue.Num() == 0 && Alive == 0)
		{
			WaveClear();
		}
	}
	else
	{
		WaveTimer -= Dt;
		if (WaveTimer <= 0.f) { StartWave(Wave + 1); }
	}
}

void AJungleGameMode::SpawnEnemy(EEnemyType Type, const FVector2D* Near)
{
	ALionelCharacter* P = Player();
	if (!P) { return; }
	const FVector PL = P->GetActorLocation() / JW::M;
	FVector2D At;
	if (Near)
	{
		At = *Near + FVector2D(JW::Rand(-4, 4), JW::Rand(-4, 4));
	}
	else
	{
		const float A = JW::Rand(0, 2 * PI), R = JW::Rand(36.f, 56.f);
		At = FVector2D(PL.X + FMath::Cos(A) * R, PL.Y + FMath::Sin(A) * R);
	}
	const float Len = At.Size();
	if (Len > JW::Arena - 4.f) { At *= (JW::Arena - 4.f) / Len; }

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Loc(At.X * JW::M, At.Y * JW::M, JW::FloorH(At.X, At.Y) * JW::M);
	AJungleEnemy* E = GetWorld()->SpawnActor<AJungleEnemy>(AJungleEnemy::StaticClass(), Loc, FRotator::ZeroRotator, SP);
	if (!E) { return; }
	E->Init(Type, FMath::Max(1, Wave));
	Enemies.Add(E);
	if (Type == EEnemyType::Boss) { Boss = E; }
	if (FX) { FX->Burst(Loc + FVector(0, 0, 100), 10, JW::Hex(0x2a3a1a), 400.f, 25.f, 0.6f, 600.f, 100.f); }
}

void AJungleGameMode::EnemyKilled(AJungleEnemy* E)
{
	++Kills;
	Score += E->Value;
	Pop(FString::Printf(TEXT("+%d"), E->Value));
	HitMarker(true);
	Sfx(ESfx::Kill);
	const FVector C = E->Center();
	if (FX) { FX->Burst(C, E->bBoss ? 80 : 18, E->Col, E->bBoss ? 1200.f : 700.f, E->bBoss ? 50.f : 22.f, 0.9f, 1400.f, 200.f); }

	const FVector2D At(E->GetActorLocation().X / JW::M, E->GetActorLocation().Y / JW::M);
	if (E->bBoss)
	{
		Explode(C, 0.f, 0.f, true);
		Boss = nullptr;
		Banner(TEXT("COLOSSUS DOWN"), TEXT("+5,000"), 2.4f);
		for (int32 i = 0; i < 4; ++i) { Drop(At + FVector2D(JW::Rand(-3, 3), JW::Rand(-3, 3)), i % 2 == 1); }
	}
	else
	{
		const float Chance = E->Type == EEnemyType::Brute ? 0.65f : (E->Type == EEnemyType::Mask ? 0.4f : 0.22f);
		if (FMath::FRand() < Chance) { Drop(At, FMath::FRand() < 0.62f); }
	}
}

void AJungleGameMode::Drop(const FVector2D& AtM, bool bAmmo)
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Loc(AtM.X * JW::M, AtM.Y * JW::M, (JW::FloorH(AtM.X, AtM.Y) + 0.6f) * JW::M);
	if (AJunglePickup* P = GetWorld()->SpawnActor<AJunglePickup>(AJunglePickup::StaticClass(), Loc, FRotator::ZeroRotator, SP))
	{
		P->Init(bAmmo);
		Pickups.Add(P);
	}
}

void AJungleGameMode::Explode(const FVector& At, float RadiusM, float Damage, bool bBig)
{
	if (RadiusM > 0.f)
	{
		const TArray<AJungleEnemy*> Snapshot = Enemies;
		for (AJungleEnemy* E : Snapshot)
		{
			if (!E || !E->bAlive) { continue; }
			const FVector C = E->Center();
			const float D = FVector::Dist(C, At) / JW::M;
			if (D < RadiusM + E->Hr)
			{
				const float F = 1.f - 0.55f * FMath::Clamp(D / RadiusM, 0.f, 1.f);
				FVector Dir = C - At;
				Dir.Z = 0;
				E->TakeHit(Damage * F, Dir.GetSafeNormal(), 6.f);
			}
		}
	}
	if (FX)
	{
		FX->Burst(At, bBig ? 60 : 30, JW::Hex(0xffa030), bBig ? 1400.f : 1000.f, bBig ? 50.f : 32.f, 0.6f, 600.f, 300.f);
		FX->Burst(At, bBig ? 30 : 14, JW::Hex(0x3a3528), bBig ? 500.f : 350.f, bBig ? 90.f : 60.f, 1.4f, -150.f, 150.f);
		FX->Burst(At, 10, JW::Hex(0x5b3a1e), 900.f, 14.f, 1.f, 1600.f, 400.f);
		FX->Flash(At + FVector(0, 0, 100), JW::Hex(0xffb050), bBig ? 60000.f : 40000.f, 0.35f);
	}
	if (ALionelCharacter* P = Player())
	{
		const float D = FVector::Dist2D(P->GetActorLocation(), At) / JW::M;
		P->AddShake(FMath::Max(0.f, 0.9f - D / 25.f));
	}
	Sfx(ESfx::Boom);
}

void AJungleGameMode::Collide(FVector2D& P, float R, float Z) const
{
	for (const FObstacle& O : Obstacles)
	{
		const float Dx = P.X - O.X, Dy = P.Y - O.Y, RR = O.R + R, D2 = Dx * Dx + Dy * Dy;
		if (D2 < RR * RR && Z < O.H - 0.2f)
		{
			const float D = FMath::Max(FMath::Sqrt(D2), 0.001f);
			P.X = O.X + Dx / D * RR;
			P.Y = O.Y + Dy / D * RR;
		}
	}
	const float Len = P.Size();
	if (Len > JW::Arena) { P *= JW::Arena / Len; }
}
