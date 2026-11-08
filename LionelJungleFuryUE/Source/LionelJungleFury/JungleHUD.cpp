#include "JungleHUD.h"
#include "JungleGameMode.h"
#include "JungleEnemy.h"
#include "JungleActors.h"
#include "LionelCharacter.h"
#include "LionelPlayerController.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "RenderUtils.h"

namespace
{
	const FLinearColor Gold = JW::Hex(0xf2b632);
	const FLinearColor Bone = JW::Hex(0xefe6cf);
	const FLinearColor Jade = JW::Hex(0x6fe3a8);
	const FLinearColor Blood = JW::Hex(0xe5483a);
	const FLinearColor Mist = JW::Hex(0xefe6cf).CopyWithNewOpacity(0.6f);
	const FLinearColor Ink = FLinearColor(0.003f, 0.01f, 0.004f, 0.62f);
	const TCHAR* Dot = TEXT(" · ");
}

// ============================================================================================
// Primitives
// ============================================================================================

void AJungleHUD::Txt(const FString& S, float X, float Y, float Size, const FLinearColor& C, float Align, bool bShadow)
{
	const float Sc = Size * U / FontH;
	float TW = 0.f, TH = 0.f;
	GetTextSize(S, TW, TH, Font, Sc);
	const float X0 = X - TW * Align;
	if (bShadow)
	{
		DrawText(S, FLinearColor(0.f, 0.f, 0.f, 0.55f * C.A), X0 + 1.5f * U, Y + 2.f * U, Font, Sc);
	}
	DrawText(S, C, X0, Y, Font, Sc);
}

float AJungleHUD::TxtW(const FString& S, float Size)
{
	float TW = 0.f, TH = 0.f;
	GetTextSize(S, TW, TH, Font, Size * U / FontH);
	return TW;
}

void AJungleHUD::Box(float X, float Y, float BW, float BH, const FLinearColor& C)
{
	DrawRect(C, X, Y, BW, BH);
}

void AJungleHUD::Frame(float X, float Y, float BW, float BH, const FLinearColor& C, float T)
{
	DrawRect(C, X, Y, BW, T);
	DrawRect(C, X, Y + BH - T, BW, T);
	DrawRect(C, X, Y + T, T, BH - 2 * T);
	DrawRect(C, X + BW - T, Y + T, T, BH - 2 * T);
}

void AJungleHUD::Disc(const FVector2D& C, float R, const FLinearColor& Col, int32 Sides)
{
	FCanvasNGonItem Item(C, FVector2D(R, R), Sides, GWhiteTexture, Col);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AJungleHUD::Ring(const FVector2D& C, float R, const FLinearColor& Col, float T)
{
	constexpr int32 Segs = 40;
	for (int32 i = 0; i < Segs; ++i)
	{
		const float A0 = i * 2.f * PI / Segs, A1 = (i + 1) * 2.f * PI / Segs;
		Canvas->K2_DrawLine(C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R, C + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R, T, Col);
	}
}

void AJungleHUD::Button(FName Id, float X, float Y, float BW, float BH, const FString& Label, bool bPrimary, float TextSize)
{
	if (bPrimary)
	{
		Box(X, Y + 4.f * U, BW, BH, JW::Hex(0x8a5a0c));
		Box(X, Y, BW, BH, Gold);
		Txt(Label, X + BW / 2, Y + (BH - TextSize * U) / 2 - 1.f * U, TextSize, JW::Hex(0x1a1004), 0.5f, false);
	}
	else
	{
		Box(X, Y, BW, BH, Ink);
		Frame(X, Y, BW, BH, Bone.CopyWithNewOpacity(0.4f), FMath::Max(1.f, 1.f * U));
		Txt(Label, X + BW / 2, Y + (BH - TextSize * U) / 2 - 1.f * U, TextSize, Bone, 0.5f, false);
	}
	Btns.Add({ FVector2D(X, Y), FVector2D(X + BW, Y + BH), Id });
}

void AJungleHUD::RoundButton(FName Id, const FVector2D& C, float R, const FString& Label, bool bOn, const FLinearColor& Fill, float TextSize)
{
	Disc(C, R, bOn ? Gold.CopyWithNewOpacity(0.38f) : Fill);
	Ring(C, R, bOn ? Gold : Bone.CopyWithNewOpacity(0.4f), FMath::Max(1.f, 1.5f * U));
	Txt(Label, C.X, C.Y - TextSize * U * 0.55f, TextSize, Bone, 0.5f, false);
	Btns.Add({ C - FVector2D(R, R), C + FVector2D(R, R), Id });
}

void AJungleHUD::Overlay(float Alpha)
{
	Box(0.f, 0.f, W, H, FLinearColor(0.002f, 0.008f, 0.004f, Alpha));
}

FName AJungleHUD::HitTest(const FVector2D& P) const
{
	for (int32 i = Btns.Num() - 1; i >= 0; --i)
	{
		const FBtn& B = Btns[i];
		if (P.X >= B.Min.X && P.X <= B.Max.X && P.Y >= B.Min.Y && P.Y <= B.Max.Y) { return B.Id; }
	}
	return NAME_None;
}

// ============================================================================================
// Frame
// ============================================================================================

void AJungleHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) { return; }
	Btns.Reset();

	if (!Font)
	{
		Font = GEngine->GetLargeFont();
		FontH = FMath::Max(1.f, (float)Font->GetMaxCharHeight());
	}
	W = Canvas->SizeX;
	H = Canvas->SizeY;
	U = FMath::Max(1.f, FMath::Min(W, H) / 400.f);
	const bool bPhone = (PLATFORM_IOS || PLATFORM_ANDROID) != 0;
	Mx = (bPhone ? 46.f : 16.f) * U;     // clears the iPhone notch in landscape
	My = (bPhone ? 14.f : 10.f) * U;

	AJungleGameMode* GM = AJungleGameMode::Get(this);
	ALionelPlayerController* PC = Cast<ALionelPlayerController>(GetOwningPlayerController());
	ALionelCharacter* P = GM ? GM->Player() : nullptr;
	if (!GM || !PC) { return; }

	switch (GM->State)
	{
	case EJState::Title: DrawTitle(GM, PC); break;
	case EJState::Playing: if (P) { DrawPlay(GM, P, PC); } break;
	case EJState::Paused: if (P) { DrawPlay(GM, P, PC); } Btns.Reset(); DrawPause(GM); break;
	case EJState::Over: DrawOver(GM); break;
	}
}

void AJungleHUD::DrawTitle(AJungleGameMode* GM, ALionelPlayerController* PC)
{
	Overlay(0.5f);
	const float Cx = W / 2;
	float Y = FMath::Max(My, H * 0.08f);
	Txt(TEXT("T E M P L E   O F   T H E   G O L D E N   J A G U A R"), Cx, Y, 11.f, Jade, 0.5f);
	Y += 20.f * U;
	Txt(TEXT("LIONEL"), Cx + 3.f * U, Y + 4.f * U, 84.f, JW::Hex(0x6b3d08), 0.5f, false);
	Txt(TEXT("LIONEL"), Cx, Y, 84.f, Gold, 0.5f);
	Y += 92.f * U;
	Txt(TEXT("J U N G L E    F U R Y"), Cx, Y, 22.f, Bone, 0.5f);
	Y += 38.f * U;
	Txt(TEXT("MACHETE  /  JAGUAR .45  /  MONSOON SMG  /  RHINO BOOMSTICK"), Cx, Y, 11.f, Bone, 0.5f);
	Y += 17.f * U;
	Txt(TEXT("VIPER CROSSBOW  /  COCONUT LAUNCHER  /  STORM TOTEM"), Cx, Y, 11.f, Bone, 0.5f);
	Y += 28.f * U;

	const float BW = 250.f * U, MW = 130.f * U, BH = 50.f * U, Gap = 12.f * U;
	const float X0 = Cx - (BW + Gap + MW) / 2;
	Button(TEXT("start"), X0, Y, BW, BH, TEXT("ENTER THE JUNGLE"), true, 18.f);
	Button(TEXT("music"), X0 + BW + Gap, Y, MW, BH, GM->bMusicOn ? TEXT("MUSIC ON") : TEXT("MUSIC OFF"), false, 13.f);
	Y += BH + 18.f * U;

	Txt(PC->bUsedTouch
		? TEXT("Left stick moves; push past the ring to sprint. Drag right to look. FIRE with either thumb. AIM zooms the sights.")
		: TEXT("WASD move  /  Shift sprint  /  Mouse aim  /  Click fire  /  Right-click sights  /  1-7 swap  /  R reload  /  P pause"),
		Cx, Y, 12.f, Mist, 0.5f, false);
	Y += 22.f * U;
	Txt(FString::Printf(TEXT("BEST SCORE  %d"), GM->Best), Cx, Y, 12.f, Jade, 0.5f, false);
}

void AJungleHUD::DrawPlay(AJungleGameMode* GM, ALionelCharacter* P, ALionelPlayerController* PC)
{
	const FVector2D Ctr(W / 2, H / 2);
	const bool bTouch = PC->bUsedTouch;

	// Damage vignette, pulsing when health is low.
	const float Low = P->Health < 30.f ? 0.25f + FMath::Sin(GM->Time * 5.f) * 0.12f : 0.f;
	const float V = FMath::Max(GM->HurtFlash, Low);
	if (V > 0.01f)
	{
		const FLinearColor R(0.6f, 0.02f, 0.01f, 0.35f * V);
		Box(0, 0, W, H, FLinearColor(0.6f, 0.02f, 0.01f, 0.12f * V));
		Box(0, 0, W * 0.12f, H, R);
		Box(W * 0.88f, 0, W * 0.12f, H, R);
		Box(W * 0.12f, 0, W * 0.76f, H * 0.12f, R);
		Box(W * 0.12f, H * 0.88f, W * 0.76f, H * 0.12f, R);
	}

	// Crosshair
	const float CL = 8.f * U, CG = 6.f * U, CT = FMath::Max(2.f, 2.f * U);
	const FLinearColor CrossC = Bone.CopyWithNewOpacity(P->bAds ? 0.25f : 1.f);
	Box(Ctr.X - CT / 2, Ctr.Y - CG - CL, CT, CL, CrossC);
	Box(Ctr.X - CT / 2, Ctr.Y + CG, CT, CL, CrossC);
	Box(Ctr.X - CG - CL, Ctr.Y - CT / 2, CL, CT, CrossC);
	Box(Ctr.X + CG, Ctr.Y - CT / 2, CL, CT, CrossC);
	Box(Ctr.X - CT * 0.75f, Ctr.Y - CT * 0.75f, CT * 1.5f, CT * 1.5f, Gold);
	if (GM->HitTime > 0.f)
	{
		const FLinearColor HC = GM->bHitKill ? Blood : FLinearColor::White;
		for (int32 k = 0; k < 4; ++k)
		{
			const FVector2D D(k % 2 ? 1.f : -1.f, k < 2 ? 1.f : -1.f);
			Canvas->K2_DrawLine(Ctr + D * 6.f * U, Ctr + D * 13.f * U, FMath::Max(2.f, 2.f * U), HC);
		}
	}
	if (GM->PopTime > 0.f)
	{
		const float A = GM->PopTime / 0.8f;
		Txt(GM->PopText, Ctr.X, Ctr.Y + 22.f * U - (1.f - A) * 26.f * U, 16.f, Gold.CopyWithNewOpacity(A), 0.5f);
	}

	// Banner
	if (GM->BannerTime > 0.f)
	{
		const float A = FMath::Min(1.f, GM->BannerTime * 3.f);
		Txt(GM->BannerTitle, Ctr.X, H * 0.24f, 44.f, Bone.CopyWithNewOpacity(A), 0.5f);
		Txt(GM->BannerSub.ToUpper(), Ctr.X, H * 0.24f + 54.f * U, 13.f, Gold.CopyWithNewOpacity(A), 0.5f);
	}

	// Top left: Lionel, health, wave, radar
	const float X0 = Mx, Y0 = My;
	Txt(TEXT("LIONEL"), X0, Y0, 16.f, Gold);
	Txt(FString::FromInt(FMath::CeilToInt(P->Health)), X0 + TxtW(TEXT("LIONEL"), 16.f) + 10.f * U, Y0 + 2.f * U, 14.f, Bone);
	const float BarW = 150.f * U, BarH = 9.f * U, BarY = Y0 + 24.f * U;
	Box(X0, BarY, BarW, BarH, FLinearColor(0, 0, 0, 0.5f));
	Box(X0, BarY, BarW * FMath::Clamp(P->Health / 100.f, 0.f, 1.f), BarH, P->Health < 30.f ? Blood : Jade);
	Frame(X0, BarY, BarW, BarH, Bone.CopyWithNewOpacity(0.3f), FMath::Max(1.f, U));
	Txt(FString::Printf(TEXT("WAVE %d%s%d LEFT"), GM->Wave, Dot, GM->RemainingCount()), X0, BarY + 16.f * U, 12.f, Mist);
	DrawRadar(GM, P, FVector2D(X0 + 42.f * U, BarY + 80.f * U), 42.f * U);

	// Top center: score and boss health
	Txt(FString::FromInt(GM->Score), Ctr.X, Y0, 26.f, Bone, 0.5f);
	if (GM->Boss && GM->Boss->bAlive)
	{
		const float TW = FMath::Min(W * 0.46f, 320.f * U);
		Txt(TEXT("I D O L   C O L O S S U S"), Ctr.X, Y0 + 34.f * U, 11.f, Blood, 0.5f);
		Box(Ctr.X - TW / 2, Y0 + 52.f * U, TW, 8.f * U, FLinearColor(0, 0, 0, 0.55f));
		Box(Ctr.X - TW / 2, Y0 + 52.f * U, TW * FMath::Clamp(GM->Boss->HP / GM->Boss->MaxHP, 0.f, 1.f), 8.f * U, Blood);
	}

	// Top right: pause
	Button(TEXT("pause"), W - Mx - 44.f * U, Y0, 44.f * U, 44.f * U, TEXT("II"), false, 14.f);

	// Bottom center: weapon slots
	const float SW = 48.f * U, SH = 42.f * U, SG = 4.f * U;
	const float SX = Ctr.X - (NumWeapons * SW + (NumWeapons - 1) * SG) / 2, SY = H - My - SH;
	for (int32 i = 0; i < NumWeapons; ++i)
	{
		const float X = SX + i * (SW + SG);
		const bool bOwned = P->Gun[i].bOwned, bCur = i == P->Cur;
		const bool bFresh = GM->FreshSlot == i && GM->FreshTime > 0.f && FMath::Sin(GM->Time * 12.f) > 0.f;
		const float A = bOwned ? 1.f : 0.35f;
		Box(X, SY, SW, SH, bFresh ? Jade.CopyWithNewOpacity(0.45f) : (bCur ? Gold.CopyWithNewOpacity(0.25f) : Ink));
		Frame(X, SY, SW, SH, (bCur ? Gold : Bone.CopyWithNewOpacity(0.3f)).CopyWithNewOpacity(A), FMath::Max(1.f, U));
		Txt(FString::FromInt(i + 1), X + SW / 2, SY + 4.f * U, 13.f, Bone.CopyWithNewOpacity(A), 0.5f, false);
		Txt(GWeapons[i].Short, X + SW / 2, SY + 23.f * U, 8.5f, Mist.CopyWithNewOpacity(0.6f * A), 0.5f, false);
		Btns.Add({ FVector2D(X, SY), FVector2D(X + SW, SY + SH), FName(*FString::Printf(TEXT("w%d"), i)) });
	}

	// Bottom right: ammo and touch buttons
	const FWeaponDef& Wd = GWeapons[P->Cur];
	const ALionelCharacter::FGunState& G = P->Gun[P->Cur];
	FString Ammo;
	if (Wd.Kind == EWeaponKind::Melee) { Ammo = TEXT("--"); }
	else if (Wd.Regen > 0.f) { Ammo = FString::Printf(TEXT("%d / %d"), FMath::FloorToInt(G.Mag), Wd.Mag); }
	else { Ammo = FString::Printf(TEXT("%d / %s"), FMath::FloorToInt(G.Mag), G.Res < 0 ? TEXT("∞") : *FString::FromInt(G.Res)); }
	const float AY = bTouch ? H - My - 236.f * U : H - My - 62.f * U;
	Txt(Wd.Full, W - Mx, AY, 11.f, Gold, 1.f);
	Txt(Ammo, W - Mx, AY + 15.f * U, 26.f, Bone, 1.f);
	if (P->IsReloading()) { Txt(TEXT("RELOADING"), W - Mx, AY + 46.f * U, 11.f, Jade, 1.f); }
	else if (Wd.Kind != EWeaponKind::Melee && Wd.Regen <= 0.f && G.Mag < 1.f && G.Res == 0) { Txt(TEXT("NO AMMO"), W - Mx, AY + 46.f * U, 11.f, Blood, 1.f); }

	if (bTouch)
	{
		RoundButton(TEXT("fire"), FVector2D(W - Mx - 50.f * U, H - My - 54.f * U), 46.f * U, TEXT("FIRE"), PC->FireHeld > 0, FLinearColor(0.2f, 0.02f, 0.005f, 0.45f), 14.f);
		RoundButton(TEXT("jump"), FVector2D(W - Mx - 140.f * U, H - My - 30.f * U), 30.f * U, TEXT("JUMP"), false, Ink, 11.f);
		RoundButton(TEXT("reload"), FVector2D(W - Mx - 128.f * U, H - My - 100.f * U), 27.f * U, TEXT("RELOAD"), false, Ink, 9.f);

		RoundButton(TEXT("ads"), FVector2D(W - Mx - 48.f * U, H - My - 142.f * U), 30.f * U, TEXT("AIM"), P->bAds, Ink, 11.f);
		// Left-thumb fire, up and to the right of the stick.
		RoundButton(TEXT("fire"), FVector2D(Mx + 219.f * U, H - My - 179.f * U), 29.f * U, TEXT("FIRE"), PC->FireHeld > 0, FLinearColor(0.2f, 0.02f, 0.005f, 0.4f), 11.f);

		const float SR = PC->StickRadius();
		const FVector2D Base = PC->bJoy ? PC->JoyOrigin : FVector2D(Mx + 100.f * U, H - My - 100.f * U);
		const float JA = PC->bJoy ? 1.f : 0.45f;
		Disc(Base, 60.f * U, FLinearColor(0.003f, 0.01f, 0.004f, 0.25f * JA));
		Ring(Base, 60.f * U, (PC->bSprint ? Gold : Bone.CopyWithNewOpacity(0.3f * JA)), FMath::Max(1.f, 1.5f * U));
		if (PC->bJoy) { Ring(Base, SR * 1.35f, Gold.CopyWithNewOpacity(PC->bSprint ? 0.8f : 0.25f), FMath::Max(1.f, U)); }
		if (PC->bSprint) { Txt(TEXT("S P R I N T"), Base.X, Base.Y - SR * 1.35f - 18.f * U, 10.f, Gold, 0.5f); }
		Disc(Base + PC->JoyKnob, 26.f * U, PC->bSprint ? Gold.CopyWithNewOpacity(0.6f) : Bone.CopyWithNewOpacity(0.35f * JA));
	}
}

void AJungleHUD::DrawRadar(AJungleGameMode* GM, ALionelCharacter* P, const FVector2D& C, float R)
{
	Disc(C, R, FLinearColor(0.003f, 0.01f, 0.004f, 0.55f), 40);
	Ring(C, R, Jade.CopyWithNewOpacity(0.35f), FMath::Max(1.f, U));
	Ring(C, R / 2, Jade.CopyWithNewOpacity(0.25f), FMath::Max(1.f, U));

	const float Yaw = FMath::DegreesToRadians(P->GetControlRotation().Yaw);
	const float Cy = FMath::Cos(Yaw), Sy = FMath::Sin(Yaw);
	const FVector Me = P->GetActorLocation() / JW::M;
	const float Range = 48.f, Sc = (R - 6.f * U) / Range;
	auto Plot = [&](const FVector& WorldCm, const FLinearColor& Col, float Size)
	{
		const float Dx = WorldCm.X / JW::M - Me.X, Dy = WorldCm.Y / JW::M - Me.Y;
		float Fw = Dx * Cy + Dy * Sy, Rt = -Dx * Sy + Dy * Cy;
		const float L = FMath::Sqrt(Fw * Fw + Rt * Rt);
		if (L > Range) { Fw *= Range / L; Rt *= Range / L; }
		Disc(C + FVector2D(Rt * Sc, -Fw * Sc), Size * U, Col, 12);
	};
	Plot(FVector::ZeroVector, Gold, 3.f);
	for (const AJunglePickup* K : GM->Pickups) { if (K) { Plot(K->GetActorLocation(), Bone, 2.f); } }
	for (const AJungleEnemy* E : GM->Enemies)
	{
		if (!E || !E->bAlive) { continue; }
		const FLinearColor Col = E->bBoss ? JW::Hex(0xff3322) : (E->Type == EEnemyType::Mask ? JW::Hex(0xd36bff) : (E->Type == EEnemyType::Brute ? JW::Hex(0xff8a1e) : Blood));
		Plot(E->GetActorLocation(), Col, E->bBoss ? 5.f : 2.8f);
	}
	Canvas->K2_DrawLine(C + FVector2D(0.f, -6.f * U), C + FVector2D(4.f * U, 4.f * U), FMath::Max(1.5f, 1.5f * U), Bone);
	Canvas->K2_DrawLine(C + FVector2D(0.f, -6.f * U), C + FVector2D(-4.f * U, 4.f * U), FMath::Max(1.5f, 1.5f * U), Bone);
}

void AJungleHUD::DrawPause(AJungleGameMode* GM)
{
	Overlay(0.72f);
	const float Cx = W / 2;
	float Y = H * 0.16f;
	Txt(TEXT("P A U S E D"), Cx, Y, 11.f, Jade, 0.5f);
	Y += 20.f * U;
	Txt(TEXT("BREATHE"), Cx, Y, 64.f, Gold, 0.5f);
	Y += 84.f * U;

	const float BH = 46.f * U, G = 10.f * U;
	const float Ws[4] = { 150.f * U, 130.f * U, 130.f * U, 130.f * U };
	const float Total = Ws[0] + Ws[1] + Ws[2] + Ws[3] + 3 * G;
	float X = Cx - Total / 2;
	Button(TEXT("resume"), X, Y, Ws[0], BH, TEXT("RESUME"), true, 17.f);       X += Ws[0] + G;
	Button(TEXT("restart"), X, Y, Ws[1], BH, TEXT("RESTART"), false, 13.f);    X += Ws[1] + G;
	Button(TEXT("sound"), X, Y, Ws[2], BH, GM->bSoundOn ? TEXT("SOUND ON") : TEXT("SOUND OFF"), false, 13.f); X += Ws[2] + G;
	Button(TEXT("music"), X, Y, Ws[3], BH, GM->bMusicOn ? TEXT("MUSIC ON") : TEXT("MUSIC OFF"), false, 13.f);
	Y += BH + 22.f * U;

	const float SB = 46.f * U;
	Button(TEXT("sensdown"), Cx - 110.f * U, Y, SB, SB, TEXT("-"), false, 20.f);
	Txt(FString::Printf(TEXT("AIM SPEED %.1f"), GM->Sensitivity), Cx, Y + 14.f * U, 13.f, Bone, 0.5f, false);
	Button(TEXT("sensup"), Cx + 110.f * U - SB, Y, SB, SB, TEXT("+"), false, 20.f);
}

void AJungleHUD::DrawOver(AJungleGameMode* GM)
{
	Overlay(0.62f);
	const float Cx = W / 2;
	float Y = H * 0.14f;
	Txt(TEXT("T H E   J U N G L E   T A K E S   I T S   D U E"), Cx, Y, 11.f, Jade, 0.5f);
	Y += 22.f * U;
	Txt(TEXT("LIONEL HAS FALLEN"), Cx, Y, 50.f, Blood, 0.5f);
	Y += 74.f * U;

	const TCHAR* Labels[4] = { TEXT("WAVE"), TEXT("KILLS"), TEXT("SCORE"), TEXT("BEST") };
	const int32 Vals[4] = { GM->Wave, GM->Kills, GM->Score, GM->Best };
	const float ColW = 100.f * U;
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = Cx + (i - 1.5f) * ColW;
		Txt(Labels[i], X, Y, 10.f, Mist, 0.5f, false);
		Txt(FString::FromInt(Vals[i]), X, Y + 16.f * U, 26.f, Bone, 0.5f);
	}
	Y += 70.f * U;
	Button(TEXT("again"), Cx - 115.f * U, Y, 230.f * U, 50.f * U, TEXT("FIGHT AGAIN"), true, 18.f);
}
