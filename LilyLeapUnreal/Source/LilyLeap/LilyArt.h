#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LilyArt.generated.h"

class UStaticMeshComponent;

/** The engine's built-in shapes. Every model in Lily Leap is assembled from these. */
enum class ELilyShape : uint8 { Cube, Sphere, Cylinder, Cone };

/**
 * A plain movable actor with an empty root. Platforms, flies, the frog and scenery are all
 * ALilyProp actors with shape components added at runtime.
 */
UCLASS()
class LILYLEAP_API ALilyProp : public AActor
{
	GENERATED_BODY()

public:
	ALilyProp();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
};

struct LILYLEAP_API FLilyArt
{
	/**
	 * Adds a coloured engine shape to Owner, attached to Parent.
	 * SizeCm is the shape's full size in centimetres (the built-in shapes are 100 cm across).
	 * The engine's cube, sphere, cylinder and cone are all pivoted at their centre, so Loc is the shape's centre.
	 */
	static UStaticMeshComponent* Add(AActor* Owner, USceneComponent* Parent, ELilyShape Shape,
		const FVector& Loc, const FVector& SizeCm, const FLinearColor& Color,
		const FRotator& Rot = FRotator::ZeroRotator, bool bCastShadow = false);

	/** Recolours a shape created by Add. */
	static void SetColor(UStaticMeshComponent* Comp, const FLinearColor& Color);
};
