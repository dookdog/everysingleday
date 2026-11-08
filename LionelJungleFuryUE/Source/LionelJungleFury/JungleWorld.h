// The living jungle: golden-hour sky and volumetric fog, terrain and river, the temple, trees,
// flowers everywhere, physics props, and ambient life (butterflies, drifting petals, fireflies).
// Everything is assembled from engine BasicShapes at startup.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JungleTypes.h"
#include "JungleWorld.generated.h"

class AJungleGameMode;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class AJungleWorld : public AActor
{
	GENERATED_BODY()

public:
	AJungleWorld();

	/** Builds the whole world. Fills GM->Obstacles and spawns the physics props. */
	void Build(AJungleGameMode* InGM);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() USceneComponent* Root = nullptr;
	UPROPERTY() TArray<UStaticMeshComponent*> Flames;
	UPROPERTY() TArray<UPointLightComponent*> FireLights;
	UPROPERTY() TArray<UPointLightComponent*> GlowLights;
	UPROPERTY() TArray<UStaticMeshComponent*> Fireflies;
	UPROPERTY() TArray<UStaticMeshComponent*> Petals;
	UPROPERTY() TArray<USceneComponent*> Flies;
	UPROPERTY() TArray<USceneComponent*> Wings;   // two per butterfly: left, right

	struct FMote { FVector P; FVector V; float Phase; };
	TArray<FMote> FireflyState;
	TArray<FMote> PetalState;
	TArray<FMote> FlyState;
	TArray<FVector> FlyTarget;

	struct FTree { float X; float Y; float G; float H; float S; bool bPalm; };
	TArray<FTree> Trees;
	TArray<FVector2D> Meadows;
	TMap<UHierarchicalInstancedStaticMeshComponent*, TArray<FTransform>> Pending;

	AJungleGameMode* GM = nullptr;
	float Density = 1.f;
	bool bMobile = false;
	float Time = 0.f;

	void BuildSky();
	void BuildTerrain();
	void BuildTemple();
	void BuildJungle();
	void BuildFlowers();
	void BuildProps();
	void BuildLife();

	UHierarchicalInstancedStaticMeshComponent* Hism(EShape Shape, uint32 Hex, bool bCollide, bool bShadow, int32 CullEndCm = 0);
	void Inst(UHierarchicalInstancedStaticMeshComponent* H, const FQuat& Q, const FVector& PosM, const FVector& SizeM);
	void Flush();

	/** One flower head: N petals around Dir, a center bead. Positions and sizes in meters. */
	void Bloom(UHierarchicalInstancedStaticMeshComponent* PetalH, UHierarchicalInstancedStaticMeshComponent* CenterH,
		const FVector& PosM, const FVector& Dir, int32 N, float Len, float Wid, float TiltDeg, float CenterSize);

	bool Clear(float X, float Y, float MinDist) const;
	FVector PlayerM() const;
};
