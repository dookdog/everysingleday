#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LilyPlayerController.generated.h"

class ALilyPond;

/**
 * Keyboard and touch input. Touches on a HUD button (menus, pause, music, sound) go to that button;
 * other touches are held controls: the left half of the screen walks, the right half jumps or flicks the tongue.
 * With bUseMouseForTouch (DefaultInput.ini) a mouse click counts as a touch, so it can all be tried on a computer.
 */
UCLASS()
class LILYLEAP_API ALilyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALilyPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Combined keyboard + touch controls currently held. */
	void GetHeld(bool& bLeft, bool& bRight, bool& bJump, bool& bTongue) const;

private:
	ALilyPond* GetPond() const;

	void TouchPressed(ETouchIndex::Type Finger, FVector Location);
	void TouchMoved(ETouchIndex::Type Finger, FVector Location);
	void TouchReleased(ETouchIndex::Type Finger, FVector Location);
	FName ClassifyTouch(const FVector2D& P) const;

	void LeftDown()    { bKeyLeft = true; }
	void LeftUp()      { bKeyLeft = false; }
	void RightDown()   { bKeyRight = true; }
	void RightUp()     { bKeyRight = false; }
	void JumpDown();
	void JumpUp()      { bKeyJump = false; }
	void TongueDown()  { bKeyTongue = true; }
	void TongueUp()    { bKeyTongue = false; }
	void ConfirmDown();
	void PauseDown();

	bool bKeyLeft = false, bKeyRight = false, bKeyJump = false, bKeyTongue = false;
	TMap<int32, FVector2D> Touches;
};
