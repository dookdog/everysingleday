// Input: touch twin-stick for iPhone (left thumb moves, right thumb aims, FIRE aims while held),
// plus keyboard, mouse and gamepad. Also routes menu taps and clicks to HUD buttons.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JungleTypes.h"
#include "LionelPlayerController.generated.h"

UCLASS()
class ALionelPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALionelPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Runs a HUD button by id: start, resume, restart, pause, sound, music, sensdown, sensup, jump, reload, w0..w6. */
	void Action(FName Id);

	// Read by the HUD to draw the touch controls.
	bool bUsedTouch = false;
	bool bJoy = false;
	FVector2D JoyOrigin = FVector2D::ZeroVector;
	FVector2D JoyKnob = FVector2D::ZeroVector;
	int32 FireHeld = 0;
	bool bMouseFire = false;

private:
	struct FTouch
	{
		bool bActive = false;
		uint8 Kind = 0;          // 1 joystick, 2 look, 3 button
		bool bFire = false;
		FVector2D Last = FVector2D::ZeroVector;
	};
	FTouch Touches[20];

	float KeyFwd = 0.f;
	float KeyRight = 0.f;
	EJState LastState = EJState::Title;
	bool bModeSet = false;

	void OnTouchPressed(ETouchIndex::Type Finger, FVector Location);
	void OnTouchMoved(ETouchIndex::Type Finger, FVector Location);
	void OnTouchReleased(ETouchIndex::Type Finger, FVector Location);

	void MoveForward(float V) { KeyFwd = V; }
	void MoveRight(float V) { KeyRight = V; }
	void Turn(float V);
	void LookUp(float V);
	void TurnRate(float V);
	void LookUpRate(float V);
	void FirePressed();
	void FireReleased();
	void JumpPressed();
	void ReloadPressed();
	void PausePressed();
	void NextWeapon();
	void PrevWeapon();
	void Weapon1() { Action(TEXT("w0")); }
	void Weapon2() { Action(TEXT("w1")); }
	void Weapon3() { Action(TEXT("w2")); }
	void Weapon4() { Action(TEXT("w3")); }
	void Weapon5() { Action(TEXT("w4")); }
	void Weapon6() { Action(TEXT("w5")); }
	void Weapon7() { Action(TEXT("w6")); }

	void RefreshInputMode(EJState State);
	float UiScale() const;
	class ALionelCharacter* Lionel() const;
};
