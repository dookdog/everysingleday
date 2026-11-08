#include "LionelPlayerController.h"
#include "LionelCharacter.h"
#include "JungleGameMode.h"
#include "JungleEnemy.h"
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
	Act(TEXT("Aim"), IE_Pressed, &ALionelPlayerController::AimPressed);
	Act(TEXT("Aim"), IE_Released, &ALionelPlayerController::AimReleased);
	Act(TEXT("Sprint"), IE_Pressed, &ALionelPlayerController::SprintPressed);
	Act(TEXT("Sprint"), IE_Released, &ALionelPlayerController::SprintReleased);
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
			bSprint = false;
			JoyMove = JoyKnob = LookAcc = LookPrev = FVector2D::ZeroVector;
			if (ALionelCharacter* P = Lionel()) { P->SetAds(false); }
		}
		LastState = GM->State;
		bModeSet = true;
	}

	ALionelCharacter* P = Lionel();
	if (!P) { return; }
	const bool bPlay = GM->State == EJState::Playing;
	P->MoveInput = bPlay ? FVector2D(KeyRight, KeyFwd) + JoyMove : FVector2D::ZeroVector;
	P->bFiring = bPlay && (FireHeld > 0 || bMouseFire);
	P->bSprintInput = bPlay && (bSprint || bSprintKey);
	if (bPlay) { ApplyTouchLook(DeltaTime, P, GM); }
}

void ALionelPlayerController::ApplyTouchLook(float Dt, ALionelCharacter* P, AJungleGameMode* GM)
{
	// Average this frame's drag with the last one: smooths touch jitter without losing distance.
	const FVector2D D = (LookAcc + LookPrev) * 0.5f;
	LookPrev = LookAcc;
	LookAcc = FVector2D::ZeroVector;
	const float Assist = bUsedTouch ? AimAssist(Dt, P, GM) : 1.f;
	if (D.IsNearlyZero()) { return; }

	const float U = UiScale();
	const float Speed = (float)D.Size() / U / FMath::Max(Dt, 1.f / 120.f);           // layout px per second
	const float Accel = 1.f + FMath::Clamp((Speed - 300.f) / 1500.f, 0.f, 1.f) * 0.8f; // fast flicks turn further
	const float K = 0.30f / U * Accel * (P->bAds ? 0.55f : 1.f) * Assist * GM->Sensitivity;
	P->Look((float)D.X * K, -(float)D.Y * K * 0.85f);
}

float ALionelPlayerController::AimAssist(float Dt, ALionelCharacter* P, AJungleGameMode* GM)
{
	// Slow the crosshair over an enemy; gently pull toward it while shooting or aiming down sights.
	const FVector Eye = P->EyeLocation();
	const FRotator Ctrl = GetControlRotation();
	float BestA = 4.3f, BestYaw = 0.f, BestPitch = 0.f;
	bool bFound = false;
	for (const AJungleEnemy* E : GM->Enemies)
	{
		if (!E || !E->bAlive) { continue; }
		const FVector To = E->Center() - Eye;
		if (To.SizeSquared() > 4800.f * 4800.f) { continue; }
		const FRotator R = To.Rotation();
		const float DYaw = FRotator::NormalizeAxis(R.Yaw - Ctrl.Yaw);
		const float DPitch = FRotator::NormalizeAxis(R.Pitch - FRotator::NormalizeAxis(Ctrl.Pitch));
		const float A = FMath::Sqrt(DYaw * DYaw + DPitch * DPitch);
		if (A < BestA) { BestA = A; BestYaw = DYaw; BestPitch = DPitch; bFound = true; }
	}
	if (!bFound) { return 1.f; }
	if (FireHeld > 0 || P->bAds)
	{
		const float Pull = FMath::Min(1.f, Dt * (P->bAds ? 3.2f : 2.2f)) * 0.5f;
		P->Look(BestYaw * Pull, BestPitch * Pull);
	}
	return 0.55f;
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
	else if (S == TEXT("ads")) { P->SetAds(!P->bAds); }
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
		// Both fire buttons also aim while held and dragged.
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
		JoyKnob = JoyMove = FVector2D::ZeroVector;
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
	FTouch& T = Touches[i];
	const FVector2D P(Location.X, Location.Y);

	if (T.Kind == 2)
	{
		LookAcc += P - T.Last;
	}
	else if (T.Kind == 1)
	{
		const float R = StickRadius();
		FVector2D D = P - JoyOrigin;
		float L = (float)D.Size();
		// The base follows a thumb that wanders well past the sprint ring.
		if (L > R * 1.9f)
		{
			JoyOrigin += D / L * (L - R * 1.9f);
			D = P - JoyOrigin;
			L = (float)D.Size();
		}
		const FVector2D Dir = L > 0.01f ? D / L : FVector2D::ZeroVector;
		JoyKnob = Dir * FMath::Min(L, R);
		// Analog response: dead zone, then a soft curve for fine steps.
		const float M = FMath::Min(L / R, 1.f);
		const float MM = M < 0.12f ? 0.f : (M - 0.12f) / 0.88f;
		const float C = MM * MM * 0.35f + MM * 0.65f;
		JoyMove = FVector2D(Dir.X * C, -Dir.Y * C);
		bSprint = L > R * 1.35f && -Dir.Y > 0.7f;
	}
	T.Last = P;
}

void ALionelPlayerController::OnTouchReleased(ETouchIndex::Type Finger, FVector Location)
{
	const int32 i = (int32)Finger;
	if (i < 0 || i >= 20 || !Touches[i].bActive) { return; }
	FTouch& T = Touches[i];
	if (T.bFire) { FireHeld = FMath::Max(0, FireHeld - 1); }
	if (T.Kind == 1)
	{
		bJoy = false;
		bSprint = false;
		JoyKnob = JoyMove = FVector2D::ZeroVector;
	}
	T = FTouch();
}

// ---- mouse, keyboard, gamepad ----

void ALionelPlayerController::Turn(float V)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* L = Lionel();
	if (V != 0.f && GM && L && GM->State == EJState::Playing) { L->Look(V * 2.5f * GM->Sensitivity * (L->bAds ? 0.6f : 1.f), 0.f); }
}

void ALionelPlayerController::LookUp(float V)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* L = Lionel();
	if (V != 0.f && GM && L && GM->State == EJState::Playing) { L->Look(0.f, V * 2.5f * GM->Sensitivity * (L->bAds ? 0.6f : 1.f)); }
}

void ALionelPlayerController::TurnRate(float V)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* L = Lionel();
	if (V != 0.f && GM && L && GM->State == EJState::Playing) { L->Look(V * 140.f * GetWorld()->GetDeltaSeconds() * GM->Sensitivity * (L->bAds ? 0.55f : 1.f), 0.f); }
}

void ALionelPlayerController::LookUpRate(float V)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelCharacter* L = Lionel();
	if (V != 0.f && GM && L && GM->State == EJState::Playing) { L->Look(0.f, V * 100.f * GetWorld()->GetDeltaSeconds() * GM->Sensitivity * (L->bAds ? 0.55f : 1.f)); }
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

void ALionelPlayerController::AimPressed()
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (ALionelCharacter* P = Lionel()) { if (GM && GM->State == EJState::Playing) { P->SetAds(true); } }
}

void ALionelPlayerController::AimReleased()
{
	if (ALionelCharacter* P = Lionel()) { P->SetAds(false); }
}

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
