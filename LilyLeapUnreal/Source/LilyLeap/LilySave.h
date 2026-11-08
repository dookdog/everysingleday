#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LilySave.generated.h"

/** Best score and the music/sound switches, kept between sessions. */
UCLASS()
class LILYLEAP_API ULilySave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Best = 0;
	UPROPERTY() bool bMusicOn = true;
	UPROPERTY() bool bSoundOn = true;
};
