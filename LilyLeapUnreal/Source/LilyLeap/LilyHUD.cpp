#include "LilyHUD.h"

#include "LilyGameMode.h"
#include "LilyPlayerController.h"
#include "LilyPond.h"
#include "LilyTypes.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"

namespace
{
	const FLinearColor Ink   = LilyHex(0x173A2A);
	const FLinearColor Leaf  = LilyHex(0x3F9B3A);
	const FLinearColor Petal = LilyHex(0xE9679A);
	const FLinearColor Cream = LilyHex(0xFFF8EC);

	FLinearColor WithAlpha(FLinearColor C, float A) { C.A = A; return C; }
}

ALilyPond* ALilyHUD::GetPond() const
{
	const ALilyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALilyGameMode>() : nullptr;
	return GM ? GM->Pond : nullptr;
}

/* ---------------- primitives ---------------- */

void ALilyHUD::Disc(const FVector2D& C, float R, const FLinearColor& Col)
{
	if (R <= 0.f) return;
	const float Step = FMath::Max(1.5f, R / 22.f);
	for (float Y = -R; Y < R; Y += Step)
	{
		const float Yc = FMath::Min(Y + Step * 0.5f, R);
		const float Hw = FMath::Sqrt(FMath::Max(0.f, R * R - Yc * Yc));
		DrawRect(Col, C.X - Hw, C.Y + Y, Hw * 2.f, FMath::Min(Step, R - Y));
	}
}

void ALilyHUD::RoundRect(float X, float Y, float W, float H, float R, const FLinearColor& Col)
{
	R = FMath::Min(R, FMath::Min(W, H) * 0.5f);
	const float Step = FMath::Max(1.5f, R / 14.f);
	// rounded top and bottom bands, straight middle
	for (float Dy = 0.f; Dy < R; Dy += Step)
	{
		const float D = R - (Dy + Step * 0.5f);
		const float Inset = R - FMath::Sqrt(FMath::Max(0.f, R * R - D * D));
		const float Hh = FMath::Min(Step, R - Dy);
		DrawRect(Col, X + Inset, Y + Dy, W - 2.f * Inset, Hh);
		DrawRect(Col, X + Inset, Y + H - Dy - Hh, W - 2.f * Inset, Hh);
	}
	DrawRect(Col, X, Y + R, W, H - 2.f * R);
}

void ALilyHUD::Heart(const FVector2D& C, float Size, const FLinearColor& Col)
{
	const float R = Size * 0.28f;
	Disc(C + FVector2D(-Size * 0.22f, 0.f), R, Col);
	Disc(C + FVector2D(Size * 0.22f, 0.f), R, Col);
	const float Step = FMath::Max(1.f, Size / 30.f);
	for (float Y = 0.f; Y < Size * 0.62f; Y += Step)
	{
		const float Hw = Size * 0.5f * (1.f - Y / (Size * 0.62f));
		DrawRect(Col, C.X - Hw, C.Y + Y + R * 0.2f, Hw * 2.f, Step);
	}
}

void ALilyHUD::Arrow(const FVector2D& C, float Size, int32 Dir, const FLinearColor& Col)
{
	const float Step = FMath::Max(1.f, Size / 24.f);
	if (Dir == 0)
	{
		for (float Y = -Size; Y < Size * 0.6f; Y += Step)
		{
			const float Hw = Size * 0.9f * ((Y + Size) / (Size * 1.6f));
			DrawRect(Col, C.X - Hw, C.Y + Y, Hw * 2.f, Step);
		}
		return;
	}
	for (float Y = -Size; Y < Size; Y += Step)
	{
		const float Len = Size * 1.5f * (1.f - FMath::Abs(Y + Step * 0.5f) / Size);
		const float X0 = Dir > 0 ? C.X - Size * 0.55f : C.X + Size * 0.55f - Len;
		DrawRect(Col, X0, C.Y + Y, Len, Step);
	}
}

void ALilyHUD::FlyIcon(const FVector2D& C, float Size, bool bGold)
{
	Disc(C + FVector2D(-Size * 0.3f, -Size * 0.45f), Size * 0.35f, FLinearColor(0.85f, 0.95f, 1.f, 0.85f));
	Disc(C + FVector2D(Size * 0.15f, -Size * 0.5f), Size * 0.35f, FLinearColor(0.85f, 0.95f, 1.f, 0.85f));
	Disc(C, Size * 0.5f, bGold ? LilyHex(0xE3A91F) : LilyHex(0x2B2A33));
	Disc(C + FVector2D(Size * 0.5f, -Size * 0.05f), Size * 0.3f, bGold ? LilyHex(0xC48812) : LilyHex(0x1C1B22));
	Disc(C + FVector2D(Size * 0.62f, -Size * 0.15f), Size * 0.12f, LilyHex(0xD6455A));
}

FVector2D ALilyHUD::Measure(const FString& Text, float Scale)
{
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, Font, Scale);
	return FVector2D(W, H);
}

void ALilyHUD::Label(const FString& Text, float X, float Y, float Scale, const FLinearColor& Col, bool bCenter, bool bShadow)
{
	if (bCenter) X -= Measure(Text, Scale).X * 0.5f;
	if (bShadow) DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Col.A * 0.45f), X + 2.f * S, Y + 2.f * S, Font, Scale);
	DrawText(Text, Col, X, Y, Font, Scale);
}

void ALilyHUD::AddButton(FName Id, float X, float Y, float W, float H)
{
	Buttons.Add({ Id, FVector2D(X, Y), FVector2D(X + W, Y + H) });
}

bool ALilyHUD::HitTest(const FVector2D& P, FName& OutId) const
{
	for (int32 i = Buttons.Num() - 1; i >= 0; i--)   // last drawn is on top
	{
		const FButtonBox& B = Buttons[i];
		if (P.X >= B.Min.X && P.X <= B.Max.X && P.Y >= B.Min.Y && P.Y <= B.Max.Y) { OutId = B.Id; return true; }
	}
	return false;
}

/* ---------------- frame ---------------- */

void ALilyHUD::DrawHUD()
{
	Super::DrawHUD();
	Buttons.Reset();
	if (!Canvas) return;
	ALilyPond* Pond = GetPond();
	if (!Pond) return;

	Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	S = Canvas->ClipY / 720.f;
	Margin = FMath::Max(24.f * S, Canvas->ClipX * 0.045f);

	DrawNightGlow(Pond);
	DrawWorldTexts(Pond);
	if (Pond->State != ELilyState::Title)
	{
		DrawStatus(Pond);
		if (Pond->State == ELilyState::Play) { DrawFlyArrow(Pond); DrawBanner(Pond); }
	}
	DrawControls(Pond);
	if (Pond->IsMenuOpen()) DrawMenu(Pond);
	DrawTopButtons(Pond);   // last, so they sit above the menu dimmer and stay tappable
}

void ALilyHUD::DrawWorldTexts(ALilyPond* Pond)
{
	for (const FLilyText& T : Pond->Texts)
	{
		const FVector Sc = Project(T.Pos);
		if (Sc.Z <= 0.f) continue;
		const float A = FMath::Clamp(T.Life / 0.5f, 0.f, 1.f);
		Label(T.Text, Sc.X, Sc.Y, 1.1f * T.Size * S, WithAlpha(T.Color, A), true, true);
	}
}

void ALilyHUD::DrawNightGlow(ALilyPond* Pond)
{
	const float T = Pond->GameTime;
	int32 i = 0;
	for (const FVector& P : Pond->Stars)
	{
		const FVector Sc = Project(P);
		if (Sc.Z <= 0.f) continue;
		const float A = 0.35f + 0.45f * FMath::Abs(FMath::Sin(T * (0.8f + (i % 7) * 0.1f) + i));
		DrawRect(FLinearColor(1.f, 1.f, 0.94f, A), Sc.X, Sc.Y, 2.5f * S, 2.5f * S);
		i++;
	}
	for (const FVector& P : Pond->NightFlies)
	{
		const FVector Sc = Project(P);
		if (Sc.Z > 0.f) Disc(FVector2D(Sc.X, Sc.Y), 20.f * S, FLinearColor(0.86f, 0.92f, 1.f, 0.16f));
	}
	i = 0;
	for (const FVector& P : Pond->Fireflies)
	{
		const FVector Sc = Project(P);
		if (Sc.Z <= 0.f) continue;
		const float A = 0.35f + 0.65f * FMath::Max(0.f, FMath::Sin(T * 2.2f + i * 1.7f));
		const FVector2D C(Sc.X, Sc.Y);
		Disc(C, 12.f * S, FLinearColor(0.94f, 1.f, 0.6f, 0.12f * A));
		Disc(C, 6.f * S, FLinearColor(0.94f, 1.f, 0.6f, 0.3f * A));
		Disc(C, 2.5f * S, FLinearColor(1.f, 1.f, 0.8f, 0.9f * A));
		i++;
	}
}

void ALilyHUD::DrawBanner(ALilyPond* Pond)
{
	if (Pond->BannerLife <= 0.f || Pond->BannerText.IsEmpty()) return;
	const float A = FMath::Clamp(FMath::Min(Pond->BannerLife / 0.5f, (2.4f - Pond->BannerLife) / 0.3f), 0.f, 1.f);
	Label(Pond->BannerText, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.22f, 2.f * S, WithAlpha(Cream, A), true, true);
}

void ALilyHUD::DrawStatus(ALilyPond* Pond)
{
	const float X = Margin, Y = 14.f * S;
	const float W = 300.f * S, H = 64.f * S;
	RoundRect(X, Y, W, H, H * 0.5f, WithAlpha(Cream, 0.9f));
	FlyIcon(FVector2D(X + 34.f * S, Y + H * 0.55f), 16.f * S, false);
	Label(FString::Printf(TEXT("%d/%d"), Pond->Total - Pond->Left, Pond->Total), X + 60.f * S, Y + 14.f * S, 1.1f * S, Ink, false);
	for (int32 i = 0; i < 3; i++)
	{
		Heart(FVector2D(X + 190.f * S + i * 36.f * S, Y + H * 0.42f), 26.f * S, i < Pond->Hearts ? Petal : WithAlpha(Ink, 0.18f));
	}
	const float Y2 = Y + H + 8.f * S, H2 = 40.f * S;
	RoundRect(X, Y2, W, H2, H2 * 0.5f, WithAlpha(Cream, 0.9f));
	Label(FString::Printf(TEXT("Pond %d"), Pond->Level), X + 20.f * S, Y2 + 7.f * S, 0.85f * S, Ink, false);
	const FString ScoreText = FString::Printf(TEXT("%05d"), Pond->Score);
	Label(ScoreText, X + W - 20.f * S - Measure(ScoreText, 0.95f * S).X, Y2 + 6.f * S, 0.95f * S, Ink, false);

	float Below = Y2 + H2 + 8.f * S;
	if (Pond->Combo > 1 && Pond->GameTime - Pond->LastEat < 2.5f)
	{
		Label(FString::Printf(TEXT("%dx combo"), Pond->Combo), X + 6.f * S, Below, 1.1f * S, Cream, false, true);
		Below += 40.f * S;
	}
	const float Power = Pond->SuperHopLeft();
	if (Power > 0.f)
	{
		const float PW = 210.f * S, PH = 40.f * S;
		RoundRect(X, Below, PW, PH, PH * 0.5f, FLinearColor(0.08f, 0.27f, 0.31f, 0.8f));
		RoundRect(X + 4.f * S, Below + 4.f * S, (PW - 8.f * S) * FMath::Clamp(Power / 12.f, 0.05f, 1.f), PH - 8.f * S, (PH - 8.f * S) * 0.5f, LilyHex(0xB9F3FF));
		Label(FString::Printf(TEXT("Super hop %ds"), FMath::CeilToInt(Power)), X + 18.f * S, Below + 7.f * S, 0.75f * S, LilyHex(0x0D3A40), false);
	}
}

void ALilyHUD::DrawFlyArrow(ALilyPond* Pond)
{
	FVector Fly; bool bGold = false;
	if (!Pond->NearestFlyWorld(Fly, bGold)) return;
	const FVector Sc = Project(Fly);
	const float W = Canvas->ClipX, H = Canvas->ClipY, M = 30.f * S;
	if (Sc.Z > 0.f && Sc.X > M && Sc.X < W - M && Sc.Y > M && Sc.Y < H - M) return;

	FVector2D D(Sc.X - W * 0.5f, Sc.Y - H * 0.5f);
	if (Sc.Z <= 0.f) D = -D;
	const float K = FMath::Min((W * 0.5f - Margin - 30.f * S) / FMath::Max(FMath::Abs(D.X), 1e-3f), (H * 0.5f - 70.f * S) / FMath::Max(FMath::Abs(D.Y), 1e-3f));
	const FVector2D At(W * 0.5f + D.X * K, H * 0.5f + D.Y * K);
	const float Pulse = 1.f + FMath::Sin(Pond->GameTime * 6.f) * 0.08f;
	Disc(At, 24.f * S * Pulse, WithAlpha(Cream, 0.92f));
	Disc(At + D.GetSafeNormal() * 30.f * S * Pulse, 7.f * S, Petal);
	FlyIcon(At, 13.f * S, bGold);
}

void ALilyHUD::DrawControls(ALilyPond* Pond)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float R = FMath::Clamp(FMath::Min(W, H) * 0.1f, 40.f * S, 64.f * S);
	const float JR = R * 1.22f;
	Layout.bValid = true;
	Layout.Screen = FVector2D(W, H);
	Layout.Radius = R; Layout.JumpRadius = JR;
	Layout.Left = FVector2D(Margin + R, H - 22.f * S - R);
	Layout.Right = FVector2D(Layout.Left.X + R * 2.f + 18.f * S, Layout.Left.Y);
	Layout.Jump = FVector2D(W - Margin - JR, H - 22.f * S - JR);
	Layout.Tongue = FVector2D(Layout.Jump.X - JR - R - 18.f * S, Layout.Jump.Y - R * 0.3f);

	const bool bShow = Pond->bTouchUI && (Pond->State == ELilyState::Play || Pond->State == ELilyState::Clearing);
	if (!bShow) return;

	bool bL = false, bR = false, bJ = false, bT = false;
	if (const ALilyPlayerController* PC = Cast<ALilyPlayerController>(PlayerOwner)) PC->GetHeld(bL, bR, bJ, bT);

	auto Pad = [this](const FVector2D& C, float Rad, bool bOn)
	{
		Disc(C, Rad, FLinearColor(1.f, 1.f, 1.f, bOn ? 0.6f : 0.4f));
		Disc(C, Rad * 0.9f, WithAlpha(Cream, bOn ? 0.55f : 0.28f));
	};
	Pad(Layout.Left, R, bL);   Arrow(Layout.Left, R * 0.38f, -1, WithAlpha(Ink, 0.8f));
	Pad(Layout.Right, R, bR);  Arrow(Layout.Right, R * 0.38f, 1, WithAlpha(Ink, 0.8f));
	Pad(Layout.Jump, JR, bJ);  Disc(Layout.Jump, JR * 0.62f, WithAlpha(LilyHex(0x5DBB4C), 0.95f));
	Arrow(Layout.Jump, JR * 0.3f, 0, Cream);
	Pad(Layout.Tongue, R, bT); Disc(Layout.Tongue, R * 0.62f, WithAlpha(Petal, 0.95f));
	Disc(Layout.Tongue + FVector2D(R * 0.22f, -R * 0.12f), R * 0.16f, Cream);
	DrawRect(Cream, Layout.Tongue.X - R * 0.3f, Layout.Tongue.Y - R * 0.05f, R * 0.5f, R * 0.12f);
}

void ALilyHUD::DrawTopButtons(ALilyPond* Pond)
{
	const float R = 26.f * S;
	float X = Canvas->ClipX - Margin - R;
	const float Y = 14.f * S + R;

	auto Round = [&](FName Id, const FString& Text, bool bOn)
	{
		Disc(FVector2D(X, Y), R, WithAlpha(Cream, bOn ? 0.92f : 0.55f));
		Label(Text, X, Y - 11.f * S, 0.55f * S, WithAlpha(Ink, bOn ? 1.f : 0.5f), true);
		if (!bOn) DrawLine(X - R * 0.6f, Y - R * 0.6f, X + R * 0.6f, Y + R * 0.6f, WithAlpha(Ink, 0.7f), 3.f * S);
		AddButton(Id, X - R * 1.15f, Y - R * 1.15f, R * 2.3f, R * 2.3f);
		X -= R * 2.f + 12.f * S;
	};
	if (Pond->State == ELilyState::Play)
	{
		Disc(FVector2D(X, Y), R, WithAlpha(Cream, 0.92f));
		DrawRect(Ink, X - 8.f * S, Y - 9.f * S, 5.f * S, 18.f * S);
		DrawRect(Ink, X + 3.f * S, Y - 9.f * S, 5.f * S, 18.f * S);
		AddButton(TEXT("pause"), X - R * 1.15f, Y - R * 1.15f, R * 2.3f, R * 2.3f);
		X -= R * 2.f + 12.f * S;
	}
	Round(TEXT("sound"), TEXT("Sound"), Pond->bSoundOn);
	Round(TEXT("music"), TEXT("Music"), Pond->bMusicOn);
}

void ALilyHUD::DrawCard(const TArray<FCardLine>& Lines)
{
	const float W = FMath::Min(Canvas->ClipX * 0.86f, 860.f * S);
	const float Pad = 26.f * S, Gap = 10.f * S;
	const float BtnH = 62.f * S;

	float Total = Pad * 2.f;
	for (const FCardLine& L : Lines) Total += (L.ButtonId != NAME_None ? BtnH : Measure(TEXT("Ag"), L.Scale).Y) + Gap;
	Total -= Gap;

	const float X = (Canvas->ClipX - W) * 0.5f;
	float Y = FMath::Max(8.f * S, (Canvas->ClipY - Total) * 0.5f);
	RoundRect(X, Y + 8.f * S, W, Total, 28.f * S, FLinearColor(0.09f, 0.23f, 0.16f, 0.18f));   // soft drop
	RoundRect(X, Y, W, Total, 28.f * S, Cream);
	Y += Pad;
	const float CX = Canvas->ClipX * 0.5f;
	for (const FCardLine& L : Lines)
	{
		if (L.ButtonId != NAME_None)
		{
			const float BW = FMath::Min(W - Pad * 2.f, 420.f * S);
			if (L.bPrimary)
			{
				RoundRect(CX - BW * 0.5f, Y + 5.f * S, BW, BtnH, BtnH * 0.5f, LilyHex(0x276A24));
				RoundRect(CX - BW * 0.5f, Y, BW, BtnH, BtnH * 0.5f, Leaf);
			}
			const FVector2D TS = Measure(L.Text, L.Scale);
			Label(L.Text, CX, Y + (BtnH - TS.Y) * 0.5f, L.Scale, L.bPrimary ? Cream : Leaf, true);
			AddButton(L.ButtonId, CX - BW * 0.5f, Y, BW, BtnH);
			Y += BtnH + Gap;
		}
		else
		{
			Label(L.Text, CX, Y, L.Scale, L.Color, true);
			Y += Measure(TEXT("Ag"), L.Scale).Y + Gap;
		}
	}
}

void ALilyHUD::DrawMenu(ALilyPond* Pond)
{
	DrawRect(FLinearColor(0.07f, 0.2f, 0.25f, 0.35f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	auto Line = [this](const FString& T, float Sc, const FLinearColor& C) { FCardLine L; L.Text = T; L.Scale = Sc * S; L.Color = C; return L; };
	auto Btn = [this](const FString& T, FName Id, bool bPrimary = true) { FCardLine L; L.Text = T; L.Scale = (bPrimary ? 1.15f : 0.95f) * S; L.ButtonId = Id; L.bPrimary = bPrimary; return L; };

	TArray<FCardLine> Lines;
	switch (Pond->State)
	{
	case ELilyState::Title:
		Lines.Add(Line(TEXT("Lily Leap"), 2.6f, Leaf));
		Lines.Add(Line(TEXT("Hop across the blooms and snap up every fly in the pond."), 0.8f, Ink));
		Lines.Add(Line(TEXT("Walk: hold the left side of the screen.   Hop: big green button (hold = higher)."), 0.6f, Ink));
		Lines.Add(Line(TEXT("Tongue: pink button snaps the nearest fly.   Golden flies are worth 5x."), 0.6f, Ink));
		Lines.Add(Line(TEXT("Eat the shimmering dragonfly for a double jump. Purple bellflowers launch you sky-high."), 0.6f, Ink));
		Lines.Add(Line(TEXT("Wasps patrol from pond 2. Stay out of the water!"), 0.6f, Petal));
		Lines.Add(Btn(TEXT("Hop in"), TEXT("start")));
		Lines.Add(Line(Pond->Best > 0 ? FString::Printf(TEXT("Best score: %d"), Pond->Best) : FString(TEXT("Keyboard: arrows or A/D, Space to hop, X to snap")), 0.55f, WithAlpha(Ink, 0.7f)));
		break;
	case ELilyState::Paused:
		Lines.Add(Line(TEXT("Resting on a lily pad"), 1.6f, Leaf));
		Lines.Add(Btn(TEXT("Keep hopping"), TEXT("resume")));
		Lines.Add(Btn(TEXT("Back to title"), TEXT("quit"), false));
		break;
	case ELilyState::Clear:
		Lines.Add(Line(FString::Printf(TEXT("Pond %d cleared"), Pond->Level), 1.8f, Leaf));
		Lines.Add(Line(FString::Printf(TEXT("Flies eaten: %d      Time: %ds"), Pond->Total, FMath::RoundToInt(Pond->LevelTime)), 0.8f, Ink));
		Lines.Add(Line(FString::Printf(TEXT("Speed bonus: +%d      Score: %d"), Pond->ClearBonus, Pond->Score), 0.8f, Ink));
		Lines.Add(Line(FString::Printf(TEXT("Next up: the %s pond, with more flies and %s. You get a heart back."),
			*FString(Pond->SkyForPond(Pond->Level + 1).Name).ToLower(), Pond->Level == 1 ? TEXT("its first wasps") : TEXT("more wasps")), 0.55f, WithAlpha(Ink, 0.7f)));
		Lines.Add(Btn(TEXT("Next pond"), TEXT("next")));
		break;
	case ELilyState::Over:
		Lines.Add(Line(TEXT("Out of hops"), 1.8f, Leaf));
		Lines.Add(Line(FString::Printf(TEXT("Score: %d      Best: %d"), Pond->Score, FMath::Max(Pond->Best, Pond->Score)), 0.9f, Ink));
		Lines.Add(Line(FString::Printf(TEXT("You reached pond %d (%s)."), Pond->Level, *FString(Pond->CurrentSky().Name).ToLower()), 0.6f, WithAlpha(Ink, 0.7f)));
		Lines.Add(Btn(TEXT("Try again"), TEXT("again")));
		break;
	default:
		return;
	}
	DrawCard(Lines);
}
