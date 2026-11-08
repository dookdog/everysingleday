#include "LionelPlayerController.h"
#include "LionelCharacter.h"
#include "JungleGameMode.h"
#include "JungleHUD.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"

ALionelPlayerController::ALionelPlayerController()
{
	bUsedTouch = (PLATFORM_IOS || PLATFORM_ANDROID) != 0;
	bShowMouseCursor = !bUsedTouch;
	bEnableClickEvents = false;
	bEnableTouchEvents = false;
}

void ALionelPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ActivateTouchInterface(nullptr);   // we draw our own touch controls
}

ALionelCharacter* ALionelPlayerController::Lionel() const
{
	return Cast<ALionelCharacter>(GetPawn());
}

float ALionelPlayerController::UiScale() const
{
	int32 W = 0, H = 0;
	GetViewportSize(W, H);
	return FMath::Max(1.f, FMath::Min(W, H) / 400.f);
}

void ALionelPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	InputComponent->BindAxis(TEXT("MoveForward"), this, &ALionelPlayerController::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &ALionelPlayerController::MoveRight);
	InputComponent->BindAxis(TEXT("Turn"), this, &ALionelPlayerController::Turn);
	InputComponent->BindAxis(TEXT("LookUp"), this, &ALionelPlayerController::LookUp);
	InputComponent->BindAxis(TEXT("TurnRate"), this, &ALionelPlayerController::TurnRate);
	InputComponent->BindAxis(TEXT("LookUpRate"), this, &ALionelPlayerController::LookUpRate);

	auto Act = [this](const TCHAR* Name, EInputEvent Ev, void (ALionelPlayerController::*Fn)())
	{
		InputComponent->BindAction(Name, Ev, this, Fn).bExecuteWhenPaused = true;
	};
	Act(TEXT("Fire"), IE_Pressed, &ALionelPlayerController::FirePressed);
	Act(TEXT("Fire"), IE_Released, &ALionelPlayerController::FireReleased);
	Act(TEXT("Jump"), IE_Pressed, &ALionelPlayerController::JumpPressed);
	Act(TEXT("Reload"), IE_Pressed, &ALionelPlayerController::ReloadPressed);
	Act(TEXT("Pause"), IE_Pressed, &ALionelPlayerController::PausePressed);
	Act(TEXT("NextWeapon"), IE_Pressed, &ALionelPlayerController::NextWeapon);
	Act(TEXT("PrevWeapon"), IE_Pressed, &ALionelPlayerController::PrevWeapon);
	Act(TEXT("Weapon1"), IE_Pressed, &ALionelPlayerController::Weapon1);
	Act(TEXT("Weapon2"), IE_Pressed, &ALionelPlayerController::Weapon2);
	Act(TEXT("Weapon3"), IE_Pressed, &ALionelPlayerController::Weapon3);
	Act(TEXT("Weapon4"), IE_Pressed, &ALionelPlayerController::Weapon4);
	Act(TEXT("Weapon5"), IE_Pressed, &ALionelPlayerController::Weapon5);
	Act(TEXT("Weapon6"), IE_Pressed, &ALionelPlayerController::Weapon6);
	Act(TEXT("Weapon7"), IE_Pressed, &ALionelPlayerController::Weapon7);

	InputComponent->BindTouch(IE_Pressed, this, &ALionelPlayerController::OnTouchPressed).bExecuteWhenPaused = true;
	InputComponent->BindTouch(IE_Repeat, this, &ALionelPlayerController::OnTouchMoved).bExecuteWhenPaused = true;
	InputComponent->BindTouch(IE_Released, this, &ALionelPlayerController::OnTouchReleased).bExecuteWhenPaused = true;
}

void ALionelPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (!GM) { return; }

	if (!bModeSet || GM->State != LastState)
	{
		RefreshInputMode(GM->State);
		if (GM->State != EJState::Playing)
		{
			// Drop any held input when a menu opens.
			for (FTouch& T : Touches) { T = FTouch(); }
			FireHeld = 0;
			bMouseFire = false;
			bJoy = false;
		}
		LastState = GM->State;
		bModeSet = true;
	}

	if (ALionelCharacter* P = Lionel())
	{
		const bool bPlay = GM->State == EJState::Playing;
		FVector2D Joy = FVector2D::ZeroVector;
		if (bJoy)
		{
			const float R = 50.f * UiScale();
			Joy = FVector2D(JoyKnob.X / R, -JoyKnob.Y / R);
		}
		P->MoveInput = bPlay ? FVector2D(KeyRight, KeyFwd) + Joy : FVector2D::ZeroVector;
		P->bFiring = bPlay && (FireHeld > 0 || bMouseFire);
	}
}

void ALionelPlayerController::RefreshInputMode(EJState State)
{
	if (State == EJState::Playing)
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
	else
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		bShowMouseCursor = !bUsedTouch;
	}
}

void ALionelPlayerController::Action(FName Id)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* P = Lionel();
	if (!GM) { return; }
	const FString S = Id.ToString();

	if (S == TEXT("start") || S == TEXT("restart") || S == TEXT("again")) { GM->StartGame(); }
	else if (S == TEXT("resume") || S == TEXT("pause")) { GM->TogglePause(); RefreshInputMode(GM->State); LastState = GM->State; }
	else if (S == TEXT("sound")) { GM->bSoundOn = !GM->bSoundOn; GM->SaveSettings(); }
	else if (S == TEXT("music")) { GM->bMusicOn = !GM->bMusicOn; GM->SaveSettings(); }
	else if (S == TEXT("sensdown")) { GM->Sensitivity = FMath::Max(0.4f, GM->Sensitivity - 0.1f); GM->SaveSettings(); }
	else if (S == TEXT("sensup")) { GM->Sensitivity = FMath::Min(2.2f, GM->Sensitivity + 0.1f); GM->SaveSettings(); }
	else if (!P || GM->State != EJState::Playing) { return; }
	else if (S == TEXT("jump")) { P->DoJump(); }
	else if (S == TEXT("reload")) { P->DoReload(); }
	else if (S.Len() == 2 && S[0] == TEXT('w')) { P->Select(S[1] - TEXT('0')); }
}

// ---- touch ----

void ALionelPlayerController::OnTouchPressed(ETouchIndex::Type Finger, FVector Location)
{
	bUsedTouch = true;
	const int32 i = (int32)Finger;
	if (i < 0 || i >= 20) { return; }
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	AJungleHUD* Hud = Cast<AJungleHUD>(GetHUD());
	if (!GM || !Hud) { return; }
	const FVector2D P(Location.X, Location.Y);
	const FName Id = Hud->HitTest(P);

	if (GM->State != EJState::Playing)
	{
		if (!Id.IsNone()) { Action(Id); }
		return;
	}

	FTouch& T = Touches[i];
	T = FTouch();
	T.bActive = true;
	T.Last = P;
	int32 W = 0, H = 0;
	GetViewportSize(W, H);

	if (Id == TEXT("fire"))
	{
		T.Kind = 2;
		T.bFire = true;
		++FireHeld;
	}
	else if (!Id.IsNone())
	{
		T.Kind = 3;
		Action(Id);
	}
	else if (P.X < W * 0.42f && !bJoy)
	{
		T.Kind = 1;
		bJoy = true;
		JoyOrigin = P;
		JoyKnob = FVector2D::ZeroVector;
	}
	else
	{
		T.Kind = 2;
	}
}

void ALionelPlayerController::OnTouchMoved(ETouchIndex::Type Finger, FVector Location)
{
	const int32 i = (int32)Finger;
	if (i < 0 || i >= 20 || !Touches[i].bActive) { return; }
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	FTouch& T = Touches[i];
	const FVector2D P(Location.X, Location.Y);
	const float U = UiScale();

	if (T.Kind == 2)
	{
		const FVector2D D = P - T.Last;
		if (ALionelCharacter* L = Lionel())
		{
			const float K = 0.32f * (GM ? GM->Sensitivity : 1.f) / U;
			L->Look(D.X * K, -D.Y * K);
		}
	}
	else if (T.Kind == 1)
	{
		FVector2D D = P - JoyOrigin;
		const float R = 50.f * U;
		if (D.Size() > R) { D = D.GetSafeNormal() * R; }
		JoyKnob = D;
	}
	T.Last = P;
}

void ALionelPlayerController::OnTouchReleased(ETouchIndex::Type Finger, FVector Location)
{
	const int32 i = (int32)Finger;
	if (i < 0 || i >= 20 || !Touches[i].bActive) { return; }
	FTouch& T = Touches[i];
	if (T.bFire) { FireHeld = FMath::Max(0, FireHeld - 1); }
	if (T.Kind == 1) { bJoy = false; JoyKnob = FVector2D::ZeroVector; }
	T = FTouch();
}

// ---- mouse, keyboard, gamepad ----

void ALionelPlayerController::Turn(float V)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (V != 0.f && GM && GM->State == EJState::Playing)
	{
		if (ALionelCharacter* L = Lionel()) { L->Look(V * 2.5f * GM->Sensitivity, 0.f); }
	}
}

void ALionelPlayerController::LookUp(float V)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (V != 0.f && GM && GM->State == EJState::Playing)
	{
		if (ALionelCharacter* L = Lionel()) { L->Look(0.f, V * 2.5f * GM->Sensitivity); }
	}
}

void ALionelPlayerController::TurnRate(float V)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (V != 0.f && GM && GM->State == EJState::Playing)
	{
		if (ALionelCharacter* L = Lionel()) { L->Look(V * 140.f * GetWorld()->GetDeltaSeconds() * GM->Sensitivity, 0.f); }
	}
}

void ALionelPlayerController::LookUpRate(float V)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (V != 0.f && GM && GM->State == EJState::Playing)
	{
		if (ALionelCharacter* L = Lionel()) { L->Look(0.f, V * 100.f * GetWorld()->GetDeltaSeconds() * GM->Sensitivity); }
	}
}

void ALionelPlayerController::FirePressed()
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (!GM) { return; }
	if (GM->State == EJState::Playing)
	{
		bMouseFire = true;
		return;
	}
	// In menus a click presses the button under the cursor; a gamepad trigger starts the game.
	float X = 0.f, Y = 0.f;
	FName Id;
	if (GetMousePosition(X, Y))
	{
		if (AJungleHUD* Hud = Cast<AJungleHUD>(GetHUD())) { Id = Hud->HitTest(FVector2D(X, Y)); }
	}
	if (!Id.IsNone()) { Action(Id); }
	else if (!bShowMouseCursor && (GM->State == EJState::Title || GM->State == EJState::Over)) { GM->StartGame(); }
}

void ALionelPlayerController::FireReleased() { bMouseFire = false; }

void ALionelPlayerController::JumpPressed()
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (GM && (GM->State == EJState::Title || GM->State == EJState::Over)) { GM->StartGame(); return; }
	Action(TEXT("jump"));
}

void ALionelPlayerController::ReloadPressed() { Action(TEXT("reload")); }

void ALionelPlayerController::PausePressed()
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (GM && (GM->State == EJState::Playing || GM->State == EJState::Paused))
	{
		GM->TogglePause();
		RefreshInputMode(GM->State);
		LastState = GM->State;
	}
}

void ALionelPlayerController::NextWeapon()
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* P = Lionel();
	if (GM && P && GM->State == EJState::Playing) { P->Cycle(1); }
}

void ALionelPlayerController::PrevWeapon()
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* P = Lionel();
	if (GM && P && GM->State == EJState::Playing) { P->Cycle(-1); }
}
