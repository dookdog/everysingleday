#include "LilyGameMode.h"

#include "LilyHUD.h"
#include "LilyPlayerController.h"
#include "LilyPond.h"
#include "Engine/World.h"

ALilyGameMode::ALilyGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ALilyPlayerController::StaticClass();
	HUDClass = ALilyHUD::StaticClass();
}

void ALilyGameMode::StartPlay()
{
	Super::StartPlay();
	if (!Pond)
	{
		Pond = GetWorld()->SpawnActor<ALilyPond>();
	}
}
