#include "JungleWorld.h"
#include "JungleGameMode.h"
#include "LionelCharacter.h"

#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	FQuat Rot(float P, float Y, float R) { return FRotator(P, Y, R).Quaternion(); }
	FVector Polar(float AngleRad, float Radius, float Z = 0.f) { return FVector(FMath::Cos(AngleRad) * Radius, FMath::Sin(AngleRad) * Radius, Z); }
	const float TwoPi = 2.f * PI;
}

AJungleWorld::AJungleWorld()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

FVector AJungleWorld::PlayerM() const
{
	ALionelCharacter* P = GM ? GM->Player() : nullptr;
	return P ? P->GetActorLocation() / JW::M : FVector(2.2f, 0.f, 4.f);
}

bool AJungleWorld::Clear(float X, float Y, float MinDist) const
{
	for (const AJungleGameMode::FObstacle& O : GM->Obstacles)
	{
		const float Dx = O.X - X, Dy = O.Y - Y, R = MinDist + O.R;
		if (Dx * Dx + Dy * Dy < R * R) { return false; }
	}
	return true;
}

void AJungleWorld::Build(AJungleGameMode* InGM)
{
	GM = InGM;
	bMobile = (PLATFORM_IOS || PLATFORM_ANDROID) != 0;
	Density = bMobile ? 0.35f : 1.f;

	BuildSky();
	BuildTerrain();
	BuildTemple();
	BuildJungle();
	BuildFlowers();
	Flush();
	BuildProps();
	BuildLife();
}

// ============================================================================================
// Instancing helpers
// ============================================================================================

UHierarchicalInstancedStaticMeshComponent* AJungleWorld::Hism(EShape Shape, uint32 Hex, bool bCollide, bool bShadow, int32 CullEndCm)
{
	UJungleAssets* A = UJungleAssets::Get(this);
	UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	H->SetMobility(EComponentMobility::Movable);
	H->SetStaticMesh(A->Mesh(Shape));
	H->SetMaterial(0, A->Mat(JW::Hex(Hex)));
	H->SetupAttachment(Root);
	if (bCollide) { H->SetCollisionProfileName(TEXT("BlockAll")); }
	else { H->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	H->SetCastShadow(bShadow);
	if (CullEndCm > 0) { H->SetCullDistances(CullEndCm * 4 / 5, CullEndCm); }
	H->RegisterComponent();
	AddInstanceComponent(H);
	Pending.Add(H);
	return H;
}

void AJungleWorld::Inst(UHierarchicalInstancedStaticMeshComponent* H, const FQuat& Q, const FVector& PosM, const FVector& SizeM)
{
	Pending.FindOrAdd(H).Add(FTransform(Q, PosM * JW::M, SizeM));
}

void AJungleWorld::Flush()
{
	for (TPair<UHierarchicalInstancedStaticMeshComponent*, TArray<FTransform>>& KV : Pending)
	{
		if (KV.Value.Num() > 0) { KV.Key->AddInstances(KV.Value, false); }
	}
	Pending.Empty();
}

void AJungleWorld::Bloom(UHierarchicalInstancedStaticMeshComponent* PetalH, UHierarchicalInstancedStaticMeshComponent* CenterH,
	const FVector& PosM, const FVector& Dir, int32 N, float Len, float Wid, float TiltDeg, float CenterSize)
{
	const FQuat Face = FRotationMatrix::MakeFromZ(Dir.GetSafeNormal()).ToQuat();
	const float Spin = JW::Rand(0.f, 360.f);
	for (int32 k = 0; k < N; ++k)
	{
		const float A = Spin + k * 360.f / N;
		const FQuat Local = Rot(TiltDeg, A, 0.f);
		const FVector Off = Face.RotateVector(Polar(FMath::DegreesToRadians(A), Len * 0.48f));
		Inst(PetalH, Face * Local, PosM + Off, FVector(Len, Wid, 0.016f));
	}
	if (CenterH)
	{
		Inst(CenterH, Face, PosM + Dir.GetSafeNormal() * 0.01f, FVector(CenterSize, CenterSize, CenterSize * 0.6f));
	}
}

// ============================================================================================
// Sky, light and fog: golden hour through a misty canopy
// ============================================================================================

void AJungleWorld::BuildSky()
{
	UDirectionalLightComponent* Sun = NewObject<UDirectionalLightComponent>(this);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetupAttachment(Root);
	Sun->SetRelativeRotation(FRotator(-24.f, 55.f, 0.f));
	Sun->bAtmosphereSunLight = true;
	Sun->bEnableLightShaftBloom = true;
	Sun->BloomScale = 0.3f;
	Sun->VolumetricScatteringIntensity = 2.5f;   // god rays through the volumetric fog
	Sun->RegisterComponent();
	Sun->SetIntensity(9.f);
	Sun->SetLightColor(FLinearColor(1.f, 0.78f, 0.52f));

	UDirectionalLightComponent* Fill = NewObject<UDirectionalLightComponent>(this);
	Fill->SetMobility(EComponentMobility::Movable);
	Fill->SetupAttachment(Root);
	Fill->SetRelativeRotation(FRotator(-55.f, 235.f, 0.f));
	Fill->VolumetricScatteringIntensity = 0.f;
	Fill->RegisterComponent();
	Fill->SetIntensity(1.1f);
	Fill->SetCastShadows(false);
	Fill->SetLightColor(FLinearColor(0.5f, 0.75f, 0.6f));

	USkyAtmosphereComponent* Sky = NewObject<USkyAtmosphereComponent>(this);
	Sky->SetupAttachment(Root);
	Sky->RegisterComponent();

	USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(this);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetupAttachment(Root);
	SkyLight->bRealTimeCapture = !bMobile;
	SkyLight->RegisterComponent();
	SkyLight->SetIntensity(1.4f);
	SkyLight->RecaptureSky();

	// Two fog layers: a green haze through the trees, and a low mist that pools on the ground and river.
	UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(this);
	Fog->SetupAttachment(Root);
	Fog->SecondFogData.FogDensity = 0.35f;
	Fog->SecondFogData.FogHeightFalloff = 1.2f;
	Fog->SecondFogData.FogHeightOffset = -150.f;
	if (!bMobile)
	{
		Fog->bEnableVolumetricFog = true;
		Fog->VolumetricFogScatteringDistribution = 0.65f;
		Fog->VolumetricFogAlbedo = FColor(235, 255, 220);
		Fog->VolumetricFogDistance = 9000.f;
	}
	Fog->RegisterComponent();
	Fog->SetFogDensity(0.06f);
	Fog->SetFogHeightFalloff(0.1f);
	Fog->SetFogInscatteringColor(FLinearColor(0.3f, 0.42f, 0.24f));
	Fog->SetStartDistance(300.f);
}

// ============================================================================================
// Terrain: flat-shaded sections in six tones
// ============================================================================================

void AJungleWorld::BuildTerrain()
{
	UProceduralMeshComponent* T = NewObject<UProceduralMeshComponent>(this, TEXT("Terrain"));
	T->SetMobility(EComponentMobility::Movable);
	T->SetupAttachment(Root);
	T->bUseComplexAsSimpleCollision = true;
	T->RegisterComponent();
	AddInstanceComponent(T);

	constexpr int32 NB = 6;
	TArray<FVector> V[NB];
	TArray<int32> I[NB];
	TArray<FVector> N[NB];
	const uint32 Cols[NB] = { 0x2b4d20, 0x3f6b25, 0x6a8a2e, 0x5b4a2c, 0x6f6b55, 0x9a8a5a };

	constexpr int32 Cells = 130;
	constexpr float Size = 280.f;
	const float Step = Size / Cells;
	auto Pt = [](float X, float Y) { return FVector(X, Y, JW::GroundH(X, Y)); };
	auto AddTri = [&](FVector A, FVector B, FVector C)
	{
		FVector Nrm = FVector::CrossProduct(B - C, A - C).GetSafeNormal();
		if (Nrm.Z < 0.f) { Swap(B, C); Nrm = -Nrm; }
		const FVector Ctr = (A + B + C) / 3.f;
		const float R = FMath::Sqrt(Ctr.X * Ctr.X + Ctr.Y * Ctr.Y);
		const float Nz = 0.5f + 0.5f * FMath::Sin(Ctr.X * 0.31f + Ctr.Y * 0.17f) * FMath::Cos(Ctr.Y * 0.23f - Ctr.X * 0.11f);
		int32 Bk = Nz < 0.38f ? 0 : (Nz < 0.72f ? 1 : 2);
		if (Ctr.Z < JW::Water - 0.1f) { Bk = 3; }
		else if (Ctr.Z < JW::Water + 0.45f) { Bk = 5; }
		if (R < 11.5f) { Bk = 4; }
		const int32 Base = V[Bk].Num();
		V[Bk].Add(A * JW::M); V[Bk].Add(B * JW::M); V[Bk].Add(C * JW::M);
		N[Bk].Add(Nrm); N[Bk].Add(Nrm); N[Bk].Add(Nrm);
		I[Bk].Add(Base); I[Bk].Add(Base + 1); I[Bk].Add(Base + 2);
	};
	for (int32 Gx = 0; Gx < Cells; ++Gx)
	{
		for (int32 Gy = 0; Gy < Cells; ++Gy)
		{
			const float X0 = -Size / 2 + Gx * Step, X1 = X0 + Step, Y0 = -Size / 2 + Gy * Step, Y1 = Y0 + Step;
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
		T->SetMaterial(S, Assets->Mat(JW::Hex(Cols[S])));
	}

	UStaticMeshComponent* Water = JW::Part(this, Root, EShape::Cube, JW::Hex(0x2a6a5c), FVector(280.f, 280.f, 0.2f), FVector(0.f, 0.f, JW::Water - 0.1f), FRotator::ZeroRotator, true);
	Water->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Water->SetCastShadow(false);
}

// ============================================================================================
// Temple: stepped pyramid, golden jaguar, braziers with real light, broken pillars
// ============================================================================================

void AJungleWorld::BuildTemple()
{
	const FLinearColor Stone[4] = { JW::Hex(0x7d7a62), JW::Hex(0x878369), JW::Hex(0x928d70), JW::Hex(0x9c9677) };
	const FLinearColor Moss = JW::Hex(0x3f6a2a), Gold = JW::Hex(0xd9a634), Jade = JW::Hex(0x7dffb8);

	for (int32 i = 0; i < 4; ++i)
	{
		const float S = JW::Pyr[i];
		JW::Part(this, Root, EShape::Cube, Stone[i], FVector(2 * S, 2 * S, JW::Tier), FVector(0, 0, JW::Tier * i + JW::Tier / 2), FRotator::ZeroRotator, true);
	}
	// Moss and creepers spilling over the step edges.
	for (int32 i = 0; i < 46; ++i)
	{
		const int32 Ti = FMath::RandRange(0, 3);
		const float S = JW::Pyr[Ti];
		const int32 Side = FMath::RandRange(0, 3);
		const float U = JW::Rand(-1.f, 1.f) * (S - 0.6f);
		const float X = Side < 2 ? U : (Side == 2 ? S - 0.3f : -S + 0.3f);
		const float Y = Side < 2 ? (Side == 0 ? S - 0.3f : -S + 0.3f) : U;
		JW::Part(this, Root, EShape::Cube, Moss, FVector(JW::Rand(0.4f, 1.f), JW::Rand(0.5f, 1.6f), 0.08f), FVector(X, Y, JW::Tier * (Ti + 1) + 0.03f), FRotator(0, JW::Rand(0, 180), 0));
		if (i % 2 == 0)
		{
			const float HangX = Side < 2 ? U : (Side == 2 ? S + 0.02f : -S - 0.02f);
			const float HangY = Side < 2 ? (Side == 0 ? S + 0.02f : -S - 0.02f) : U;
			JW::Part(this, Root, EShape::Cube, JW::Hex(0x355e22), FVector(0.06f, 0.06f, JW::Rand(0.4f, 0.7f)), FVector(HangX, HangY, JW::Tier * (Ti + 1) - 0.25f));
		}
	}

	const float Top = JW::Tier * 4;
	JW::Part(this, Root, EShape::Cube, Stone[3], FVector(1.5f, 1.5f, 0.6f), FVector(0, 0, Top + 0.3f), FRotator::ZeroRotator, true);
	JW::Part(this, Root, EShape::Cube, Gold, FVector(1.3f, 0.7f, 0.6f), FVector(0, 0, Top + 0.95f));
	JW::Part(this, Root, EShape::Cube, Gold, FVector(0.6f, 0.62f, 0.56f), FVector(0.62f, 0, Top + 1.45f));
	JW::Part(this, Root, EShape::Cone, Gold, FVector(0.24f, 0.24f, 0.22f), FVector(0.62f, 0.2f, Top + 1.82f));
	JW::Part(this, Root, EShape::Cone, Gold, FVector(0.24f, 0.24f, 0.22f), FVector(0.62f, -0.2f, Top + 1.82f));
	JW::Part(this, Root, EShape::Cube, Jade, FVector(0.04f, 0.1f, 0.07f), FVector(0.93f, 0.15f, Top + 1.52f));
	JW::Part(this, Root, EShape::Cube, Jade, FVector(0.04f, 0.1f, 0.07f), FVector(0.93f, -0.15f, Top + 1.52f));
	JW::Part(this, Root, EShape::Cube, Gold, FVector(0.9f, 0.1f, 0.1f), FVector(-0.95f, 0, Top + 1.1f), FRotator(35.f, 0, 0));
	GM->Obstacles.Add({ 0.f, 0.f, 1.15f, Top + 2.f });

	const float Corners[4][2] = { { 2.4f, 2.4f }, { -2.4f, 2.4f }, { 2.4f, -2.4f }, { -2.4f, -2.4f } };
	for (const auto& C : Corners)
	{
		JW::Part(this, Root, EShape::Cylinder, Stone[0], FVector(0.6f, 0.6f, 0.7f), FVector(C[0], C[1], Top + 0.35f), FRotator::ZeroRotator, true);
		JW::Part(this, Root, EShape::Cylinder, JW::Hex(0x3b3a30), FVector(0.84f, 0.84f, 0.18f), FVector(C[0], C[1], Top + 0.78f));
		UStaticMeshComponent* F1 = JW::Part(this, Root, EShape::Cone, JW::Hex(0xff8a2a), FVector(0.6f, 0.6f, 0.7f), FVector(C[0], C[1], Top + 1.15f));
		UStaticMeshComponent* F2 = JW::Part(this, Root, EShape::Cone, JW::Hex(0xffe070), FVector(0.32f, 0.32f, 0.45f), FVector(C[0], C[1], Top + 1.05f));
		F1->SetCastShadow(false);
		F2->SetCastShadow(false);
		Flames.Add(F1);
		Flames.Add(F2);

		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(Root);
		L->SetRelativeLocation(FVector(C[0], C[1], Top + 1.6f) * JW::M);
		L->SetIntensityUnits(ELightUnits::Unitless);
		L->VolumetricScatteringIntensity = 3.f;     // glowing halos in the mist
		L->RegisterComponent();
		L->SetIntensity(6000.f);
		L->SetAttenuationRadius(1400.f);
		L->SetLightColor(FLinearColor(1.f, 0.55f, 0.2f));
		L->SetCastShadows(false);
		FireLights.Add(L);
		GM->Obstacles.Add({ C[0], C[1], 0.5f, Top + 1.2f });
	}

	for (int32 i = 0; i < 12; ++i)
	{
		const float A = i / 12.f * TwoPi + 0.13f, R = JW::Rand(15.5f, 17.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.2f) { continue; }
		const float H = FMath::FRand() < 0.4f ? JW::Rand(1.2f, 2.6f) : JW::Rand(4.f, 6.5f);
		JW::Part(this, Root, EShape::Cylinder, Stone[1 + i % 3], FVector(1.7f, 1.7f, H), FVector(X, Y, G + H / 2 - 0.2f), FRotator(JW::Rand(-3, 3), 0, JW::Rand(-3, 3)), true);
		if (H > 3.5f)
		{
			JW::Part(this, Root, EShape::Cube, Stone[2], FVector(2.1f, 2.1f, 0.5f), FVector(X, Y, G + H - 0.2f), FRotator(0, JW::Rand(0, 90), 0), true);
		}
		JW::Part(this, Root, EShape::Cube, Moss, FVector(1.9f, 1.9f, 0.12f), FVector(X, Y, G + 0.05f), FRotator(0, JW::Rand(0, 90), 0));
		GM->Obstacles.Add({ X, Y, 1.f, G + H });
	}
}

// ============================================================================================
// Trees, vines, grass, rocks. Some kapok trees are in full blossom.
// ============================================================================================

void AJungleWorld::BuildJungle()
{
	auto* KapokTrunks = Hism(EShape::Cylinder, 0x5d4630, true, true);
	auto* PalmTrunks = Hism(EShape::Cylinder, 0x8a6a44, true, true);
	UHierarchicalInstancedStaticMeshComponent* Canopy[3] = { Hism(EShape::Sphere, 0x2f5a23, false, true), Hism(EShape::Sphere, 0x41702a, false, true), Hism(EShape::Sphere, 0x5a8a2e, false, true) };
	UHierarchicalInstancedStaticMeshComponent* Blossom[3] = { Hism(EShape::Sphere, 0xf28cb8, false, true), Hism(EShape::Sphere, 0xb07ad8, false, true), Hism(EShape::Sphere, 0xff9a4a, false, true) };
	UHierarchicalInstancedStaticMeshComponent* Carpet[3] = { Hism(EShape::Cylinder, 0xf7a8c8, false, false, 6000), Hism(EShape::Cylinder, 0xc9a0ea, false, false, 6000), Hism(EShape::Cylinder, 0xffb878, false, false, 6000) };
	auto* Crowns = Hism(EShape::Cone, 0x5f9a2f, false, true);
	auto* Vines = Hism(EShape::Cylinder, 0x3c5e22, false, false);
	UHierarchicalInstancedStaticMeshComponent* Ferns[2] = { Hism(EShape::Cone, 0x3d7a2a, false, false, 9000), Hism(EShape::Cone, 0x5e9a34, false, false, 9000) };
	UHierarchicalInstancedStaticMeshComponent* Grass[2] = { Hism(EShape::Cone, 0x4b8a2c, false, false, 3500), Hism(EShape::Cone, 0x7aa83a, false, false, 3500) };
	auto* Rocks = Hism(EShape::Sphere, 0x6c6a5a, true, true);

	int32 Count = 0;
	for (int32 Tries = 0; Tries < 6000 && Count < 300; ++Tries)
	{
		const float A = JW::Rand(0, TwoPi), R = 17.f + FMath::Pow(FMath::FRand(), 0.8f) * 100.f;
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.25f || !Clear(X, Y, 3.f)) { continue; }
		const bool bPalm = FMath::FRand() < 0.35f;
		const float H = bPalm ? JW::Rand(6.f, 9.5f) : JW::Rand(10.f, 17.f);
		const float S = bPalm ? JW::Rand(0.55f, 0.75f) : JW::Rand(1.f, 1.7f);
		Inst(bPalm ? PalmTrunks : KapokTrunks, Rot(JW::Rand(-3, 3), JW::Rand(0, 360), JW::Rand(-3, 3)), FVector(X, Y, G - 0.3f + H / 2), FVector(0.95f * S, 0.95f * S, H));
		GM->Obstacles.Add({ X, Y, 0.5f * S + 0.12f, G + H });
		Trees.Add({ X, Y, G, H, S, bPalm });
		++Count;

		if (bPalm)
		{
			const float CS = JW::Rand(6.f, 8.f);
			Inst(Crowns, Rot(JW::Rand(-8, 8), JW::Rand(0, 360), JW::Rand(-8, 8)), FVector(X, Y, G + H - 0.2f), FVector(CS, CS, 1.1f));
			continue;
		}
		const bool bBlossom = FMath::FRand() < 0.18f;
		const int32 Tone = FMath::RandRange(0, 2);
		const int32 Blobs = FMath::RandRange(2, 3);
		for (int32 b = 0; b < Blobs; ++b)
		{
			const float BS = JW::Rand(3.f, 5.2f);
			const FVector BP(X + JW::Rand(-2.4f, 2.4f), Y + JW::Rand(-2.4f, 2.4f), G + H + JW::Rand(-1.f, 1.6f));
			Inst(bBlossom ? Blossom[Tone] : Canopy[FMath::RandRange(0, 2)], Rot(0, JW::Rand(0, 360), 0), BP, FVector(2 * BS, 2 * BS, 1.24f * BS));
			for (int32 v = 0; v < 3; ++v)
			{
				if (FMath::FRand() < 0.3f) { continue; }
				const float L = JW::Rand(3.f, FMath::Max(3.5f, FMath::Min(9.f, (float)BP.Z - G - 1.f)));
				Inst(Vines, FQuat::Identity, FVector(BP.X + JW::Rand(-BS * 0.7f, BS * 0.7f), BP.Y + JW::Rand(-BS * 0.7f, BS * 0.7f), BP.Z - BS * 0.3f - L / 2), FVector(0.08f, 0.08f, L));
			}
		}
		if (bBlossom)
		{
			// Fallen petals carpeting the ground beneath.
			const int32 NP = FMath::RoundToInt(45 * Density);
			for (int32 p = 0; p < NP; ++p)
			{
				const float PA = JW::Rand(0, TwoPi), PR = FMath::Sqrt(FMath::FRand()) * 5.f;
				const float PX = X + FMath::Cos(PA) * PR, PY = Y + FMath::Sin(PA) * PR, PG = JW::Hgt(PX, PY);
				if (PG < JW::Water + 0.05f) { continue; }
				Inst(Carpet[Tone], Rot(JW::Rand(-6, 6), JW::Rand(0, 360), JW::Rand(-6, 6)), FVector(PX, PY, PG + 0.02f), FVector(0.14f, 0.1f, 0.01f));
			}
		}
	}

	for (int32 i = 0, Placed = 0; i < 1600 && Placed < FMath::RoundToInt(750 * Density); ++i)
	{
		const float A = JW::Rand(0, TwoPi), R = JW::Rand(11.f, 116.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.05f) { continue; }
		const float S = JW::Rand(0.6f, 1.5f);
		Inst(Ferns[i % 2], Rot(JW::Rand(-10, 10), JW::Rand(0, 360), JW::Rand(-10, 10)), FVector(X, Y, G + S * 0.35f), FVector(2 * S, 2 * S, S * JW::Rand(0.6f, 1.1f)));
		++Placed;
	}

	const int32 GrassCount = FMath::RoundToInt(15000 * Density);
	for (int32 i = 0, Placed = 0; i < GrassCount * 2 && Placed < GrassCount; ++i)
	{
		const float A = JW::Rand(0, TwoPi), R = JW::Rand(9.5f, 105.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.15f) { continue; }
		const float H = JW::Rand(0.35f, 0.8f);
		Inst(Grass[i % 2], Rot(JW::Rand(-14, 14), JW::Rand(0, 360), JW::Rand(-14, 14)), FVector(X, Y, G + H * 0.4f), FVector(0.07f, 0.05f, H));
		++Placed;
	}

	for (int32 i = 0, Placed = 0; i < 300 && Placed < 70; ++i)
	{
		const float A = JW::Rand(0, TwoPi), R = JW::Rand(18.f, 98.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (!Clear(X, Y, 1.5f)) { continue; }
		const float S = JW::Rand(0.6f, 2.f);
		Inst(Rocks, Rot(JW::Rand(0, 360), JW::Rand(0, 360), JW::Rand(0, 360)), FVector(X, Y, G + 0.3f * S), FVector(2.4f * S, 2.f * S, 1.6f * S));
		GM->Obstacles.Add({ X, Y, S * 0.95f, G + S * 1.1f });
		++Placed;
	}
}

// ============================================================================================
// Flowers everywhere
// ============================================================================================

void AJungleWorld::BuildFlowers()
{
	constexpr int32 Far = 7000;   // cm; small flowers fade out beyond this
	auto* Stem = Hism(EShape::Cylinder, 0x4a8a2a, false, false, Far);
	auto* Leaf = Hism(EShape::Sphere, 0x3f8a2c, false, true, 9000);
	UHierarchicalInstancedStaticMeshComponent* Daisy[5] = {
		Hism(EShape::Sphere, 0xfff6ea, false, false, Far),   // white
		Hism(EShape::Sphere, 0xff8fc0, false, false, Far),   // pink
		Hism(EShape::Sphere, 0xffd23f, false, false, Far),   // yellow
		Hism(EShape::Sphere, 0xa77bff, false, false, Far),   // violet
		Hism(EShape::Sphere, 0xff6a3d, false, false, Far) }; // coral
	auto* Eye = Hism(EShape::Sphere, 0xffb020, false, false, Far);
	auto* DarkEye = Hism(EShape::Sphere, 0x5a2a10, false, false, Far);
	UHierarchicalInstancedStaticMeshComponent* Bract[2] = { Hism(EShape::Cone, 0xe8302a, false, true, 9000), Hism(EShape::Cone, 0xff9a1e, false, true, 9000) };
	auto* BractTip = Hism(EShape::Cone, 0xffe14a, false, false, 9000);
	auto* Bush = Hism(EShape::Sphere, 0x2c5e22, false, true, 11000);
	UHierarchicalInstancedStaticMeshComponent* Hibiscus[3] = { Hism(EShape::Sphere, 0xe0203a, false, false, 9000), Hism(EShape::Sphere, 0xff7aa8, false, false, 9000), Hism(EShape::Sphere, 0xffc83a, false, false, 9000) };
	auto* Stamen = Hism(EShape::Cylinder, 0xffe680, false, false, 9000);
	UHierarchicalInstancedStaticMeshComponent* Pad[2] = { Hism(EShape::Cylinder, 0x3f8a3a, false, false, 9000), Hism(EShape::Cylinder, 0x5aa24a, false, false, 9000) };
	auto* LotusOut = Hism(EShape::Sphere, 0xf59ac0, false, false, 9000);
	auto* LotusIn = Hism(EShape::Sphere, 0xffd8e8, false, false, 9000);
	UHierarchicalInstancedStaticMeshComponent* Orchid[2] = { Hism(EShape::Sphere, 0xc86bff, false, false, Far), Hism(EShape::Sphere, 0xffffff, false, false, Far) };
	auto* GlowPetal = Hism(EShape::Sphere, 0x7fe8ff, false, false, 0);
	auto* GlowEye = Hism(EShape::Sphere, 0xe8ffff, false, false, 0);

	auto Daisies = [&](float X, float Y, int32 Tone)
	{
		const float G = JW::Hgt(X, Y);
		if (G < JW::Water + 0.1f) { return; }
		const float H = JW::Rand(0.22f, 0.55f), Sz = JW::Rand(0.8f, 1.35f);
		Inst(Stem, Rot(JW::Rand(-8, 8), 0, JW::Rand(-8, 8)), FVector(X, Y, G + H / 2), FVector(0.025f, 0.025f, H));
		if (FMath::FRand() < 0.5f) { Inst(Leaf, Rot(25, JW::Rand(0, 360), 0), FVector(X, Y, G + 0.06f), FVector(0.22f, 0.08f, 0.015f)); }
		Bloom(Daisy[Tone], Tone == 2 ? DarkEye : Eye, FVector(X, Y, G + H), FVector(JW::Rand(-0.2f, 0.2f), JW::Rand(-0.2f, 0.2f), 1.f), FMath::RandRange(5, 8), 0.15f * Sz, 0.06f * Sz, 14.f, 0.07f * Sz);
	};

	// Meadows of jungle daisies, each patch mostly one color with a few strays.
	const int32 Patches = FMath::RoundToInt(150 * Density);
	for (int32 i = 0, Made = 0; i < Patches * 4 && Made < Patches; ++i)
	{
		const float A = JW::Rand(0, TwoPi), R = JW::Rand(10.f, 95.f);
		const float Cx = FMath::Cos(A) * R, Cy = FMath::Sin(A) * R;
		if (JW::GroundH(Cx, Cy) < JW::Water + 0.3f || !Clear(Cx, Cy, 1.f)) { continue; }
		Meadows.Add(FVector2D(Cx, Cy));
		++Made;
		const int32 Tone = FMath::RandRange(0, 4);
		const int32 N = FMath::RandRange(10, 22);
		const float Spread = JW::Rand(1.5f, 3.5f);
		for (int32 f = 0; f < N; ++f)
		{
			const float FA = JW::Rand(0, TwoPi), FR = FMath::Sqrt(FMath::FRand()) * Spread;
			Daisies(Cx + FMath::Cos(FA) * FR, Cy + FMath::Sin(FA) * FR, FMath::FRand() < 0.85f ? Tone : FMath::RandRange(0, 4));
		}
	}

	// Flowers overgrowing the temple steps and pillar bases.
	for (int32 i = 0; i < FMath::RoundToInt(120 * Density); ++i)
	{
		const int32 Ti = FMath::RandRange(0, 3);
		const float S = JW::Pyr[Ti] - JW::Rand(0.15f, 0.6f);
		const float U = JW::Rand(-S, S);
		const int32 Side = FMath::RandRange(0, 3);
		const float X = Side < 2 ? U : (Side == 2 ? S : -S);
		const float Y = Side < 2 ? (Side == 0 ? S : -S) : U;
		if (FMath::Max(FMath::Abs(X), FMath::Abs(Y)) < 3.2f) { continue; }   // keep the summit clear
		Daisies(X, Y, FMath::RandRange(0, 4));
	}

	// Heliconia: tall stalks with stacked red and orange bracts.
	for (int32 i = 0, Made = 0; i < 1200 && Made < FMath::RoundToInt(280 * Density); ++i)
	{
		const float A = JW::Rand(0, TwoPi), R = JW::Rand(13.f, 100.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.2f || !Clear(X, Y, 0.8f)) { continue; }
		++Made;
		const float H = JW::Rand(1.0f, 1.7f);
		const int32 Tone = FMath::RandRange(0, 1);
		Inst(Stem, FQuat::Identity, FVector(X, Y, G + H / 2), FVector(0.045f, 0.045f, H));
		for (int32 l = 0; l < 3; ++l)
		{
			Inst(Leaf, Rot(JW::Rand(35, 55), JW::Rand(0, 360), 0), FVector(X, Y, G + 0.4f + l * 0.15f), FVector(0.9f, 0.22f, 0.02f));
		}
		const float Yaw = JW::Rand(0, TwoPi);
		for (int32 b = 0; b < 5; ++b)
		{
			const float Side = (b % 2 == 0) ? 1.f : -1.f;
			const FVector Dir = FVector(FMath::Cos(Yaw) * Side, FMath::Sin(Yaw) * Side, 0.35f).GetSafeNormal();
			const FVector P(X, Y, G + H * (0.5f + b * 0.1f));
			const FQuat Q = FRotationMatrix::MakeFromZ(Dir).ToQuat();
			Inst(Bract[Tone], Q, P + Dir * 0.13f, FVector(0.11f, 0.08f, 0.28f));
			Inst(BractTip, Q, P + Dir * 0.27f, FVector(0.05f, 0.04f, 0.08f));
		}
	}

	// Hibiscus bushes studded with big flowers.
	for (int32 i = 0, Made = 0; i < 800 && Made < FMath::RoundToInt(110 * Density); ++i)
	{
		const float A = JW::Rand(0, TwoPi), R = JW::Rand(12.f, 98.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::GroundH(X, Y);
		if (G < JW::Water + 0.2f || !Clear(X, Y, 1.5f)) { continue; }
		++Made;
		const float D = JW::Rand(1.5f, 2.6f);
		const FVector C(X, Y, G + D * 0.3f);
		Inst(Bush, Rot(0, JW::Rand(0, 360), 0), C, FVector(D, D * JW::Rand(0.85f, 1.1f), D * 0.75f));
		const int32 Tone = FMath::RandRange(0, 2);
		for (int32 f = 0; f < 7; ++f)
		{
			const float Az = JW::Rand(0, TwoPi), El = FMath::DegreesToRadians(JW::Rand(15.f, 70.f));
			const FVector Dir(FMath::Cos(Az) * FMath::Cos(El), FMath::Sin(Az) * FMath::Cos(El), FMath::Sin(El));
			const FVector P = C + FVector(Dir.X * D * 0.5f, Dir.Y * D * 0.5f, Dir.Z * D * 0.375f);
			Bloom(Hibiscus[Tone], nullptr, P, Dir, 5, 0.26f, 0.15f, 28.f, 0.f);
			Inst(Stamen, FRotationMatrix::MakeFromZ(Dir).ToQuat(), P + Dir * 0.09f, FVector(0.02f, 0.02f, 0.18f));
		}
	}

	// Lily pads and lotus flowers floating on the river.
	for (int32 i = 0, Made = 0; i < 6000 && Made < FMath::RoundToInt(460 * Density); ++i)
	{
		const float X = JW::Rand(-120.f, 120.f), Y = JW::Rand(-120.f, 120.f);
		if (JW::GroundH(X, Y) > JW::Water - 0.35f) { continue; }
		++Made;
		const float D = JW::Rand(0.5f, 1.2f);
		Inst(Pad[i % 2], Rot(0, JW::Rand(0, 360), 0), FVector(X, Y, JW::Water + 0.03f), FVector(D, D, 0.02f));
		if (FMath::FRand() < 0.3f)
		{
			const FVector P(X + JW::Rand(-0.2f, 0.2f), Y + JW::Rand(-0.2f, 0.2f), JW::Water + 0.12f);
			Bloom(LotusOut, nullptr, P, FVector::UpVector, 8, 0.26f, 0.11f, 48.f, 0.f);
			Bloom(LotusIn, Eye, P + FVector(0, 0, 0.04f), FVector::UpVector, 6, 0.18f, 0.08f, 70.f, 0.08f);
		}
	}

	// Orchids clinging to the kapok trunks.
	for (const FTree& T : Trees)
	{
		if (T.bPalm) { continue; }
		const int32 N = FMath::RoundToInt(4 * Density + 0.5f);
		for (int32 k = 0; k < N; ++k)
		{
			const float A = JW::Rand(0, TwoPi);
			const FVector Out(FMath::Cos(A), FMath::Sin(A), 0.25f);
			const FVector P = FVector(T.X, T.Y, T.G + JW::Rand(1.5f, 5.5f)) + FVector(Out.X, Out.Y, 0.f) * (0.48f * T.S + 0.05f);
			Bloom(Orchid[k % 2], Eye, P, Out, 5, 0.12f, 0.07f, 10.f, 0.04f);
		}
	}

	// Glowing blue flowers around the temple, each with its own light.
	const int32 Glows = bMobile ? 0 : 12;
	for (int32 i = 0; i < Glows; ++i)
	{
		const float A = i / float(Glows) * TwoPi + 0.3f, R = JW::Rand(10.5f, 13.5f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R, G = JW::Hgt(X, Y);
		if (G < JW::Water + 0.1f) { continue; }
		for (int32 f = 0; f < 5; ++f)
		{
			const FVector P(X + JW::Rand(-0.7f, 0.7f), Y + JW::Rand(-0.7f, 0.7f), G + JW::Rand(0.3f, 0.6f));
			Inst(Stem, FQuat::Identity, FVector(P.X, P.Y, (G + P.Z) / 2), FVector(0.025f, 0.025f, P.Z - G));
			Bloom(GlowPetal, GlowEye, P, FVector::UpVector, 6, 0.2f, 0.08f, 22.f, 0.08f);
		}
		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(Root);
		L->SetRelativeLocation(FVector(X, Y, G + 0.8f) * JW::M);
		L->SetIntensityUnits(ELightUnits::Unitless);
		L->VolumetricScatteringIntensity = 4.f;
		L->RegisterComponent();
		L->SetIntensity(1800.f);
		L->SetAttenuationRadius(520.f);
		L->SetLightColor(FLinearColor(0.35f, 0.85f, 1.f));
		L->SetCastShadows(false);
		GlowLights.Add(L);
	}
}

// ============================================================================================
// Physics props: crates, clay urns, fallen stone blocks, coconuts. All simulate in real time.
// ============================================================================================

void AJungleWorld::BuildProps()
{
	auto Ground = [](float X, float Y) { return JW::Hgt(X, Y); };

	// Supply crates stacked near the ruins.
	for (int32 s = 0; s < 7; ++s)
	{
		const float A = s / 7.f * TwoPi + 0.5f, R = JW::Rand(11.f, 13.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R;
		if (Ground(X, Y) < JW::Water + 0.2f) { continue; }
		const float Yaw = JW::Rand(0, 90);
		const FVector Rt = Polar(FMath::DegreesToRadians(Yaw + 90.f), 1.f);
		const int32 Layers = FMath::RandRange(2, 3);
		for (int32 L = 0; L < Layers; ++L)
		{
			const int32 PerRow = 3 - L;
			for (int32 c = 0; c < PerRow; ++c)
			{
				const float Off = (c - (PerRow - 1) * 0.5f) * 0.74f;
				const float CX = X + Rt.X * Off, CY = Y + Rt.Y * Off;
				const FVector P(CX, CY, Ground(X, Y) + 0.36f + L * 0.71f);
				GM->SpawnProp(EShape::Cube, JW::Hex(L % 2 ? 0x9a6a33 : 0x8a5a2b), FVector(0.7f), P * JW::M, FRotator(0, Yaw + JW::Rand(-4, 4), 0), 25.f, 0.f, false);
			}
		}
	}

	// Terracotta urns around the base of the pyramid.
	for (int32 i = 0; i < 22; ++i)
	{
		const float A = JW::Rand(0, TwoPi);
		const float R = 9.6f + JW::Rand(0.f, 0.8f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R;
		GM->SpawnProp(EShape::Cylinder, JW::Hex(i % 3 ? 0xb8643a : 0xc9804a), FVector(0.5f, 0.5f, 0.75f), FVector(X, Y, Ground(X, Y) + 0.42f) * JW::M, FRotator::ZeroRotator, 12.f, 0.f, false);
	}

	// Heavy carved blocks that have fallen from the pillars.
	for (int32 i = 0; i < 12; ++i)
	{
		const float A = JW::Rand(0, TwoPi), R = JW::Rand(14.f, 19.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R;
		if (Ground(X, Y) < JW::Water + 0.2f || !Clear(X, Y, 0.8f)) { continue; }
		GM->SpawnProp(EShape::Cube, JW::Hex(0x8a8668), FVector(0.95f, 0.95f, 0.8f), FVector(X, Y, Ground(X, Y) + 0.5f) * JW::M, FRotator(0, JW::Rand(0, 90), 0), 350.f, 0.f, false);
	}

	// Coconuts under the palms.
	int32 Nuts = 0;
	for (const FTree& T : Trees)
	{
		if (!T.bPalm || Nuts > 50) { continue; }
		for (int32 k = 0; k < 2; ++k)
		{
			const FVector P = FVector(T.X, T.Y, 0.f) + Polar(JW::Rand(0, TwoPi), JW::Rand(0.8f, 2.f));
			GM->SpawnProp(EShape::Sphere, JW::Hex(0x5b3a1e), FVector(0.26f), FVector(P.X, P.Y, Ground(P.X, P.Y) + 0.2f) * JW::M, FRotator::ZeroRotator, 1.5f, 0.f, false);
			++Nuts;
		}
	}
}

// ============================================================================================
// Ambient life: fireflies, drifting petals, butterflies
// ============================================================================================

void AJungleWorld::BuildLife()
{
	for (int32 i = 0; i < 70; ++i)
	{
		const float A = JW::Rand(0, TwoPi), R = JW::Rand(6.f, 60.f);
		const float X = FMath::Cos(A) * R, Y = FMath::Sin(A) * R;
		const float Z = FMath::Max(JW::GroundH(X, Y), JW::Water) + JW::Rand(0.5f, 4.f);
		UStaticMeshComponent* F = JW::Part(this, Root, EShape::Sphere, JW::Hex(0xe4ff8a), FVector(0.1f), FVector(X, Y, Z));
		F->SetCastShadow(false);
		Fireflies.Add(F);
		FireflyState.Add({ FVector(X, Y, Z), FVector::ZeroVector, JW::Rand(0, 10) });
	}

	const uint32 PetalCols[4] = { 0xff9cc8, 0xfff0f5, 0xffc0a0, 0xd8b0ff };
	const FVector Me = PlayerM();
	for (int32 i = 0; i < FMath::RoundToInt(140 * FMath::Max(Density, 0.5f)); ++i)
	{
		const FVector P = Me + FVector(JW::Rand(-22, 22), JW::Rand(-22, 22), JW::Rand(1, 10));
		UStaticMeshComponent* C = JW::Part(this, Root, EShape::Cube, JW::Hex(PetalCols[i % 4]), FVector(0.06f, 0.045f, 0.006f), P);
		C->SetCastShadow(false);
		Petals.Add(C);
		PetalState.Add({ P, FVector(JW::Rand(0.6f, 1.2f), JW::Rand(0.2f, 0.6f), -JW::Rand(0.35f, 0.7f)), JW::Rand(0, 10) });
	}

	// Butterflies: blue morpho, monarch, sulphur, white, scarlet.
	const uint32 Fore[5] = { 0x2f7cff, 0xff8a1e, 0xffe14a, 0xf5f2e8, 0xe0402a };
	const uint32 Hind[5] = { 0x1a4fc0, 0x2a1408, 0xf0c020, 0xe8e0d0, 0x2a0a08 };
	for (int32 i = 0; i < 28; ++i)
	{
		const int32 Kind = i % 5;
		const FVector P = Me + FVector(JW::Rand(-18, 18), JW::Rand(-18, 18), 0.f);
		const FVector Start(P.X, P.Y, FMath::Max(JW::Hgt(P.X, P.Y), JW::Water) + JW::Rand(0.8f, 2.4f));
		USceneComponent* B = JW::Group(this, Root, Start);
		B->SetRelativeScale3D(FVector(1.5f));
		JW::Part(this, B, EShape::Cylinder, JW::Hex(0x1a1410), FVector(0.025f, 0.025f, 0.15f), FVector::ZeroVector, FRotator(90.f, 0.f, 0.f));
		for (float Side : { 1.f, -1.f })
		{
			USceneComponent* W = JW::Group(this, B, FVector(0.f, 0.01f * Side, 0.f));
			JW::Part(this, W, EShape::Sphere, JW::Hex(Fore[Kind]), FVector(0.1f, 0.13f, 0.008f), FVector(0.025f, 0.065f * Side, 0.f));
			JW::Part(this, W, EShape::Sphere, JW::Hex(Hind[Kind]), FVector(0.075f, 0.09f, 0.008f), FVector(-0.045f, 0.05f * Side, 0.f));
			Wings.Add(W);
		}
		Flies.Add(B);
		FlyState.Add({ Start, FVector::ZeroVector, JW::Rand(0, 10) });
		FlyTarget.Add(Start);
	}
	TArray<UStaticMeshComponent*> All;
	GetComponents<UStaticMeshComponent>(All);
	for (UStaticMeshComponent* C : All)
	{
		if (C->GetAttachParent() != Root) { C->SetCastShadow(false); }   // butterfly parts
	}
}

// ============================================================================================
// Animation
// ============================================================================================

void AJungleWorld::Tick(float Dt)
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
	for (int32 i = 0; i < FireLights.Num(); ++i)
	{
		FireLights[i]->SetIntensity(6000.f * (0.85f + 0.15f * FMath::Sin(Time * 17.f + i * 3.f) * FMath::Sin(Time * 7.3f + i)));
	}
	for (int32 i = 0; i < GlowLights.Num(); ++i)
	{
		GlowLights[i]->SetIntensity(1800.f * (0.75f + 0.25f * FMath::Sin(Time * 1.3f + i)));
	}

	for (int32 i = 0; i < Fireflies.Num(); ++i)
	{
		const FMote& F = FireflyState[i];
		const float T = Time * 0.6f + F.Phase;
		Fireflies[i]->SetRelativeLocation(FVector(F.P.X + FMath::Sin(T * 1.3f) * 1.2f, F.P.Y + FMath::Cos(T) * 1.2f, F.P.Z + FMath::Sin(T * 2.1f) * 0.5f) * JW::M);
	}

	const FVector Me = PlayerM();

	// Petals drift on the breeze around Lionel and are recycled when they land or blow away.
	for (int32 i = 0; i < Petals.Num(); ++i)
	{
		FMote& S = PetalState[i];
		S.P += FVector(S.V.X + FMath::Sin(Time * 1.7f + S.Phase) * 0.6f, S.V.Y + FMath::Cos(Time * 1.3f + S.Phase) * 0.4f, S.V.Z) * Dt;
		const bool bLanded = S.P.Z < FMath::Max(JW::Hgt(S.P.X, S.P.Y), JW::Water) + 0.05f;
		if (bLanded || FMath::Abs(S.P.X - Me.X) > 24.f || FMath::Abs(S.P.Y - Me.Y) > 24.f)
		{
			S.P = Me + FVector(JW::Rand(-24, 10), JW::Rand(-22, 22), 0.f);
			S.P.Z = FMath::Max(JW::Hgt(S.P.X, S.P.Y), JW::Water) + JW::Rand(4.f, 11.f);
		}
		Petals[i]->SetRelativeLocationAndRotation(S.P * JW::M, FRotator(Time * 140.f + S.Phase * 50.f, Time * 60.f + S.Phase * 30.f, Time * 95.f + S.Phase * 70.f));
	}

	// Butterflies wander between flowers near Lionel, flapping.
	for (int32 i = 0; i < Flies.Num(); ++i)
	{
		FMote& S = FlyState[i];
		FVector& Tgt = FlyTarget[i];
		FVector To = Tgt - S.P;
		if (To.Size() < 0.4f || FVector::Dist2D(S.P, Me) > 30.f)
		{
			FVector2D Pick(Me.X + JW::Rand(-16, 16), Me.Y + JW::Rand(-16, 16));
			if (Meadows.Num() > 0 && FMath::FRand() < 0.5f)
			{
				const FVector2D M = Meadows[FMath::RandRange(0, Meadows.Num() - 1)];
				if (FVector2D(M.X - Me.X, M.Y - Me.Y).Size() < 30.f) { Pick = M + FVector2D(JW::Rand(-2, 2), JW::Rand(-2, 2)); }
			}
			Tgt = FVector(Pick.X, Pick.Y, FMath::Max(JW::Hgt(Pick.X, Pick.Y), JW::Water) + JW::Rand(0.5f, 2.6f));
			if (FVector::Dist2D(S.P, Me) > 30.f) { S.P = Tgt + FVector(JW::Rand(-3, 3), JW::Rand(-3, 3), 1.f); }
			To = Tgt - S.P;
		}
		const FVector Dir = To.GetSafeNormal();
		S.V = FMath::Lerp(S.V, Dir * 2.2f, FMath::Min(1.f, Dt * 2.f));
		S.P += (S.V + FVector(0.f, 0.f, FMath::Sin(Time * 6.f + S.Phase) * 0.6f)) * Dt;
		Flies[i]->SetRelativeLocationAndRotation(S.P * JW::M, FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(S.V.Y, S.V.X)), 0.f));
		const float Flap = 15.f + 55.f * (0.5f + 0.5f * FMath::Sin(Time * 15.f + S.Phase * 7.f));
		Wings[i * 2]->SetRelativeRotation(FRotator(0.f, 0.f, -Flap));
		Wings[i * 2 + 1]->SetRelativeRotation(FRotator(0.f, 0.f, Flap));
	}
}
