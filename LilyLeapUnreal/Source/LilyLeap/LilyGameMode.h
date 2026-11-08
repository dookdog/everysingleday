#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LilyGameMode.generated.h"

class ALilyPond;

/** Starts Lily Leap: spawns the pond, which builds everything else from code. */
UCLASS()
class LILYLEAP_API ALilyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALilyGameMode();

	virtual void StartPlay() override;

	/** The frog is not a possessed pawn; the pond simulates it directly. */
	virtual void RestartPlayer(AController* NewPlayer) override {}

	UPROPERTY(Transient)
	TObjectPtr<ALilyPond> Pond;
};
