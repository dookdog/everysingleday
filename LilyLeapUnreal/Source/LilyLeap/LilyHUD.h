#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LilyHUD.generated.h"

class ALilyPond;
class UFont;

/** Where the on-screen touch buttons are, in screen pixels. Shared with the player controller. */
struct FLilyTouchLayout
{
	bool bValid = false;
	FVector2D Screen = FVector2D(1, 1);
	FVector2D Left = FVector2D::ZeroVector, Right = FVector2D::ZeroVector;
	FVector2D Jump = FVector2D::ZeroVector, Tongue = FVector2D::ZeroVector;
	float Radius = 40.f, JumpRadius = 48.f;
};

/**
 * Everything drawn on top of the 3D pond: score panel, hearts, touch controls, the music / sound / pause
 * buttons, floating score text, the arrow to the nearest fly, and the title, pause, cleared and
 * game-over menus. Drawn with simple shapes and the engine font, so no UI assets are needed.
 */
UCLASS()
class LILYLEAP_API ALilyHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Which button (if any) is under a screen point, from the last drawn frame. */
	bool HitTest(const FVector2D& P, FName& OutId) const;

	FLilyTouchLayout Layout;

private:
	struct FButtonBox { FName Id; FVector2D Min, Max; };
	struct FCardLine
	{
		FString Text;
		float Scale = 1.f;
		FLinearColor Color = FLinearColor::Black;
		FName ButtonId = NAME_None;     // set for a button row
		bool bPrimary = true;
	};

	ALilyPond* GetPond() const;

	// Drawing primitives (all built from DrawRect strips so no textures are needed)
	void Disc(const FVector2D& C, float R, const FLinearColor& Col);
	void RoundRect(float X, float Y, float W, float H, float R, const FLinearColor& Col);
	void Heart(const FVector2D& C, float Size, const FLinearColor& Col);
	void Arrow(const FVector2D& C, float Size, int32 Dir, const FLinearColor& Col);   // Dir: -1 left, 1 right, 0 up
	void FlyIcon(const FVector2D& C, float Size, bool bGold);
	void Label(const FString& Text, float X, float Y, float Scale, const FLinearColor& Col, bool bCenter, bool bShadow = false);
	FVector2D Measure(const FString& Text, float Scale);

	void DrawNightGlow(ALilyPond* Pond);
	void DrawBanner(ALilyPond* Pond);
	void DrawStatus(ALilyPond* Pond);
	void DrawWorldTexts(ALilyPond* Pond);
	void DrawFlyArrow(ALilyPond* Pond);
	void DrawControls(ALilyPond* Pond);
	void DrawTopButtons(ALilyPond* Pond);
	void DrawMenu(ALilyPond* Pond);
	void DrawCard(const TArray<FCardLine>& Lines);
	void AddButton(FName Id, float X, float Y, float W, float H);

	TArray<FButtonBox> Buttons;
	float S = 1.f;          // UI scale for this frame
	float Margin = 24.f;    // side gutter (clears the iPhone notch in landscape)
	UFont* Font = nullptr;
};
