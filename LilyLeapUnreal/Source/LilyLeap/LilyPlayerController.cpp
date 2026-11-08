#include "LilyPlayerController.h"

#include "LilyGameMode.h"
#include "LilyHUD.h"
#include "LilyPond.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

ALilyPlayerController::ALilyPlayerController()
{
	bAutoManageActiveCameraTarget = false;   // the pond owns the camera
	bShowMouseCursor = true;
	bEnableTouchEvents = true;
	bEnableClickEvents = false;
}

void ALilyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ActivateTouchInterface(nullptr);         // no default virtual joysticks: the HUD draws Lily Leap's own buttons
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
}

ALilyPond* ALilyPlayerController::GetPond() const
{
	const ALilyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALilyGameMode>() : nullptr;
	return GM ? GM->Pond : nullptr;
}

void ALilyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	for (const FKey& K : { EKeys::Left, EKeys::A })
	{
		InputComponent->BindKey(K, IE_Pressed, this, &ALilyPlayerController::LeftDown);
		InputComponent->BindKey(K, IE_Released, this, &ALilyPlayerController::LeftUp);
	}
	for (const FKey& K : { EKeys::Right, EKeys::D })
	{
		InputComponent->BindKey(K, IE_Pressed, this, &ALilyPlayerController::RightDown);
		InputComponent->BindKey(K, IE_Released, this, &ALilyPlayerController::RightUp);
	}
	for (const FKey& K : { EKeys::SpaceBar, EKeys::Up, EKeys::W, EKeys::Z })
	{
		InputComponent->BindKey(K, IE_Pressed, this, &ALilyPlayerController::JumpDown);
		InputComponent->BindKey(K, IE_Released, this, &ALilyPlayerController::JumpUp);
	}
	for (const FKey& K : { EKeys::X, EKeys::J, EKeys::K, EKeys::LeftShift, EKeys::RightShift })
	{
		InputComponent->BindKey(K, IE_Pressed, this, &ALilyPlayerController::TongueDown);
		InputComponent->BindKey(K, IE_Released, this, &ALilyPlayerController::TongueUp);
	}
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ALilyPlayerController::ConfirmDown);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ALilyPlayerController::PauseDown);
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ALilyPlayerController::PauseDown);

	InputComponent->BindTouch(IE_Pressed, this, &ALilyPlayerController::TouchPressed);
	InputComponent->BindTouch(IE_Repeat, this, &ALilyPlayerController::TouchMoved);
	InputComponent->BindTouch(IE_Released, this, &ALilyPlayerController::TouchReleased);
}

void ALilyPlayerController::JumpDown()
{
	ALilyPond* Pond = GetPond();
	if (Pond && Pond->IsMenuOpen()) { Pond->HandleConfirm(); return; }   // Space also presses the menu's main button
	bKeyJump = true;
}

void ALilyPlayerController::ConfirmDown()
{
	if (ALilyPond* Pond = GetPond()) Pond->HandleConfirm();
}

void ALilyPlayerController::PauseDown()
{
	if (ALilyPond* Pond = GetPond()) Pond->HandlePauseKey();
}

void ALilyPlayerController::TouchPressed(ETouchIndex::Type Finger, FVector Location)
{
	const FVector2D P(Location.X, Location.Y);
	ALilyPond* Pond = GetPond();
	if (Pond) Pond->NotifyTouchUsed();

	if (ALilyHUD* Hud = Cast<ALilyHUD>(GetHUD()))
	{
		FName Button;
		if (Hud->HitTest(P, Button))
		{
			if (Pond) Pond->HandleButton(Button);
			return;   // a button press is not also a held control
		}
	}
	if (Pond && Pond->IsMenuOpen()) return;
	Touches.Add((int32)Finger, P);
}

void ALilyPlayerController::TouchMoved(ETouchIndex::Type Finger, FVector Location)
{
	if (FVector2D* P = Touches.Find((int32)Finger)) *P = FVector2D(Location.X, Location.Y);
}

void ALilyPlayerController::TouchReleased(ETouchIndex::Type Finger, FVector Location)
{
	Touches.Remove((int32)Finger);
}

FName ALilyPlayerController::ClassifyTouch(const FVector2D& P) const
{
	const ALilyHUD* Hud = Cast<ALilyHUD>(GetHUD());
	if (!Hud || !Hud->Layout.bValid) return NAME_None;
	const FLilyTouchLayout& L = Hud->Layout;
	if (P.Y < L.Screen.Y * 0.3f) return NAME_None;
	if (P.X < L.Screen.X * 0.5f)
	{
		return P.X < (L.Left.X + L.Right.X) * 0.5f ? FName(TEXT("left")) : FName(TEXT("right"));
	}
	const float DJ = FVector2D::Distance(P, L.Jump) / L.JumpRadius;
	const float DT = FVector2D::Distance(P, L.Tongue) / L.Radius;
	return DJ <= DT ? FName(TEXT("jump")) : FName(TEXT("tongue"));
}

void ALilyPlayerController::GetHeld(bool& bLeft, bool& bRight, bool& bJump, bool& bTongue) const
{
	bLeft = bKeyLeft; bRight = bKeyRight; bJump = bKeyJump; bTongue = bKeyTongue;
	for (const TPair<int32, FVector2D>& T : Touches)
	{
		const FName C = ClassifyTouch(T.Value);
		if (C == TEXT("left")) bLeft = true;
		else if (C == TEXT("right")) bRight = true;
		else if (C == TEXT("jump")) bJump = true;
		else if (C == TEXT("tongue")) bTongue = true;
	}
}
