#include "LilyArt.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ALilyProp::ALilyProp()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

static UStaticMesh* LoadShape(ELilyShape Shape)
{
	const TCHAR* Path = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	switch (Shape)
	{
	case ELilyShape::Cube:     Path = TEXT("/Engine/BasicShapes/Cube.Cube"); break;
	case ELilyShape::Sphere:   Path = TEXT("/Engine/BasicShapes/Sphere.Sphere"); break;
	case ELilyShape::Cylinder: Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder"); break;
	case ELilyShape::Cone:     Path = TEXT("/Engine/BasicShapes/Cone.Cone"); break;
	}
	return LoadObject<UStaticMesh>(nullptr, Path);
}

static UMaterialInterface* LoadBaseMaterial()
{
	// The engine's shape material exposes a "Color" vector parameter.
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

UStaticMeshComponent* FLilyArt::Add(AActor* Owner, USceneComponent* Parent, ELilyShape Shape,
	const FVector& Loc, const FVector& SizeCm, const FLinearColor& Color, const FRotator& Rot, bool bCastShadow)
{
	check(Owner && Parent);
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetStaticMesh(LoadShape(Shape));
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetGenerateOverlapEvents(false);
	Comp->SetCastShadow(bCastShadow);
	Comp->SetupAttachment(Parent);
	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	Comp->SetRelativeLocationAndRotation(Loc, Rot);
	Comp->SetRelativeScale3D(SizeCm / 100.f);

	if (UMaterialInterface* Base = LoadBaseMaterial())
	{
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, Comp);
		Mat->SetVectorParameterValue(TEXT("Color"), Color);
		Comp->SetMaterial(0, Mat);
	}
	return Comp;
}

void FLilyArt::SetColor(UStaticMeshComponent* Comp, const FLinearColor& Color)
{
	if (!Comp) return;
	if (UMaterialInstanceDynamic* Mat = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0)))
	{
		Mat->SetVectorParameterValue(TEXT("Color"), Color);
	}
}
