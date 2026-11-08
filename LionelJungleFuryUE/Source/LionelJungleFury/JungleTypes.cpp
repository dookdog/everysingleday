#include "JungleTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"

//                    Short       Full                  Kind               Dmg   Rate  Spread Mag Res  Max  Reload Kick   Sfx              Tracer    Thick  Pel Pierce Range Arc  Splash Regen
const FWeaponDef GWeapons[NumWeapons] = {
	{ TEXT("MACHETE"), TEXT("MACHETE"),          EWeaponKind::Melee, 65.f, 0.42f, 0.f,   0,  0,   0,   0.f,   0.01f, ESfx::Slash,    0xffffff, 0.f,   1,  false, 3.2f, 0.95f, 0.f,  0.f },
	{ TEXT("JAGUAR"),  TEXT("JAGUAR .45"),       EWeaponKind::Ray,   30.f, 0.20f, 0.006f,12, -1,  -1,  1.0f,  0.025f,ESfx::Pistol,   0xffe08a, 0.026f,1,  false, 0.f,  0.f,   0.f,  0.f },
	{ TEXT("MONSOON"), TEXT("MONSOON SMG"),      EWeaponKind::Ray,   15.f, 0.08f, 0.03f, 40, 160, 320, 1.4f,  0.012f,ESfx::Smg,      0xfff0b0, 0.026f,1,  false, 0.f,  0.f,   0.f,  0.f },
	{ TEXT("RHINO"),   TEXT("RHINO BOOMSTICK"),  EWeaponKind::Ray,   13.f, 0.75f, 0.085f,6,  24,  48,  1.8f,  0.09f, ESfx::Shotgun,  0xffd27a, 0.018f,10, false, 0.f,  0.f,   0.f,  0.f },
	{ TEXT("VIPER"),   TEXT("VIPER CROSSBOW"),   EWeaponKind::Ray,   115.f,0.85f, 0.f,   5,  20,  40,  1.5f,  0.03f, ESfx::Crossbow, 0x9dff7a, 0.07f, 1,  true,  0.f,  0.f,   0.f,  0.f },
	{ TEXT("COCO"),    TEXT("COCONUT LAUNCHER"), EWeaponKind::Proj,  150.f,0.85f, 0.f,   4,  12,  24,  2.0f,  0.07f, ESfx::Launcher, 0x5b3a1e, 0.f,   1,  false, 0.f,  0.f,   5.5f, 0.f },
	{ TEXT("STORM"),   TEXT("STORM TOTEM"),      EWeaponKind::Chain, 48.f, 0.30f, 0.f,   30, 0,   0,   0.f,   0.02f, ESfx::Zap,      0x8ff8ff, 0.06f, 1,  false, 0.f,  0.f,   0.f,  2.5f },
};

namespace JW
{
	const float Pyr[4] = { 9.f, 7.f, 5.f, 3.f };

	static float SmoothStep(float E0, float E1, float X)
	{
		X = FMath::Clamp((X - E0) / (E1 - E0), 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}

	float BaseH(float X, float Y)
	{
		return FMath::Sin(X * 0.045f) * 1.9f + FMath::Cos(Y * 0.052f) * 1.7f
			+ FMath::Sin((X + Y) * 0.1f) * 0.75f + FMath::Sin(X * 0.19f - Y * 0.23f) * 0.3f;
	}

	float GroundH(float X, float Y)
	{
		return BaseH(X, Y) * SmoothStep(11.f, 24.f, FMath::Sqrt(X * X + Y * Y));
	}

	float Hgt(float X, float Y)
	{
		float H = GroundH(X, Y);
		const float Mx = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
		if (Mx < Pyr[0])
		{
			int32 N = 0;
			for (float S : Pyr) { if (Mx < S) { ++N; } }
			H = FMath::Max(H, N * Tier);
		}
		return H;
	}

	float FloorH(float X, float Y) { return FMath::Max(Hgt(X, Y), Water - 0.9f); }

	float Rand(float A, float B) { return FMath::FRandRange(A, B); }

	FLinearColor Hex(uint32 C)
	{
		return FLinearColor(FColor((C >> 16) & 255, (C >> 8) & 255, C & 255, 255));
	}

	UStaticMeshComponent* Part(AActor* Owner, USceneComponent* Parent, EShape Shape, const FLinearColor& Color,
		const FVector& SizeM, const FVector& PosM, const FRotator& Rot, bool bCollide)
	{
		UJungleAssets* A = UJungleAssets::Get(Owner);
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(A->Mesh(Shape));
		C->SetMaterial(0, A->Mat(Color));
		C->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
		C->SetRelativeLocation(PosM * M);
		C->SetRelativeRotation(Rot);
		C->SetRelativeScale3D(SizeM);
		if (bCollide)
		{
			C->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetGenerateOverlapEvents(false);
		}
		C->RegisterComponent();
		Owner->AddInstanceComponent(C);
		return C;
	}

	USceneComponent* Group(AActor* Owner, USceneComponent* Parent, const FVector& PosM, const FRotator& Rot)
	{
		USceneComponent* G = NewObject<USceneComponent>(Owner);
		G->SetMobility(EComponentMobility::Movable);
		G->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
		G->SetRelativeLocation(PosM * M);
		G->SetRelativeRotation(Rot);
		G->RegisterComponent();
		Owner->AddInstanceComponent(G);
		return G;
	}
}

void UJungleAssets::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

UJungleAssets* UJungleAssets::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UJungleAssets>() : nullptr;
}

UStaticMesh* UJungleAssets::Mesh(EShape Shape) const
{
	switch (Shape)
	{
	case EShape::Sphere: return Sphere;
	case EShape::Cylinder: return Cylinder;
	case EShape::Cone: return Cone;
	default: return Cube;
	}
}

UMaterialInstanceDynamic* UJungleAssets::Mat(const FLinearColor& Color)
{
	const uint32 Key = Color.ToFColor(false).DWColor();
	if (UMaterialInstanceDynamic** Found = Mats.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	Mats.Add(Key, MID);
	return MID;
}
