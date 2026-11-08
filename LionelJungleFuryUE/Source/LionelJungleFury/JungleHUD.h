// Canvas HUD: title, pause and game-over screens, health, ammo, radar, weapon bar and touch controls.
// Drawn with engine fonts and primitives, so the project needs no UI assets.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "JungleHUD.generated.h"

class AJungleGameMode;
class ALionelCharacter;
class ALionelPlayerController;
class UFont;

UCLASS()
class AJungleHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Returns the id of the button under a screen point (pixels), or NAME_None. */
	FName HitTest(const FVector2D& P) const;

private:
	struct FBtn { FVector2D Min; FVector2D Max; FName Id; };
	TArray<FBtn> Btns;

	float U = 1.f;      // UI scale: one layout unit in pixels
	float W = 0.f;
	float H = 0.f;
	float Mx = 16.f;    // safe-area side margin
	float My = 10.f;    // safe-area top/bottom margin
	UPROPERTY() UFont* Font = nullptr;
	float FontH = 24.f;

	void DrawTitle(AJungleGameMode* GM, ALionelPlayerController* PC);
	void DrawPlay(AJungleGameMode* GM, ALionelCharacter* P, ALionelPlayerController* PC);
	void DrawPause(AJungleGameMode* GM);
	void DrawOver(AJungleGameMode* GM);
	void DrawRadar(AJungleGameMode* GM, ALionelCharacter* P, const FVector2D& C, float R);

	void Txt(const FString& S, float X, float Y, float Size, const FLinearColor& C, float Align = 0.f, bool bShadow = true);
	float TxtW(const FString& S, float Size);
	void Box(float X, float Y, float BW, float BH, const FLinearColor& C);
	void Frame(float X, float Y, float BW, float BH, const FLinearColor& C, float T);
	void Disc(const FVector2D& C, float R, const FLinearColor& Col, int32 Sides = 32);
	void Ring(const FVector2D& C, float R, const FLinearColor& Col, float T);
	void Button(FName Id, float X, float Y, float BW, float BH, const FString& Label, bool bPrimary, float TextSize = 15.f);
	void RoundButton(FName Id, const FVector2D& C, float R, const FString& Label, bool bOn, const FLinearColor& Fill, float TextSize);
	void Overlay(float Alpha);
};
