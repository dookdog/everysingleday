// Headless runtime test of Lily Leap's game logic against the stand-in engine.
#define private public
#define protected public
#include "LilyPond.h"
#include "LilyHUD.h"
#include "LilyGameMode.h"
#include "LilySynth.h"
#undef private
#undef protected
#include <cstdio>
UWorld* GWorldMock = nullptr;
static int Fails = 0;
#define EXPECT(c, ...) do { if (!(c)) { Fails++; std::printf("FAIL: " __VA_ARGS__); std::printf("  [%s]\n", #c); } } while (0)

static void Run(ALilyPond* P, float Seconds) { for (float t = 0; t < Seconds; t += 1.f / 60.f) P->Tick(1.f / 60.f); }

int main()
{
	GWorldMock = new UWorld();
	ALilyGameMode* GM = new ALilyGameMode();
	GWorldMock->Mode = GM;
	GM->StartPlay();
	ALilyPond* P = GM->Pond;
	EXPECT(P != nullptr, "pond spawned\n");
	P->BeginPlay();
	EXPECT(P->State == ELilyState::Title, "starts on title\n");
	Run(P, 5.f);   // attract mode

	ALilyHUD* Hud = new ALilyHUD();
	Hud->Canvas = new UCanvas(); Hud->Canvas->ClipX = 2532; Hud->Canvas->ClipY = 1170;
	Hud->DrawHUD();
	EXPECT(Hud->Buttons.Num() >= 3, "title has buttons (got %d)\n", Hud->Buttons.Num());

	// Generate and play ponds 1..8
	for (int32 n = 1; n <= 8; n++)
	{
		P->Score = 0; P->Hearts = 3;
		P->LoadLevel(n);
		P->SetState(ELilyState::Play);
		const int32 Dragons = [&] { int32 c = 0; for (auto& F : P->Flies) c += F.bDragon; return c; }();
		const int32 ExpectWasps = n < 2 ? 0 : std::min(6, n - 1 + (n - 2) / 2);
		EXPECT(P->Platforms.Num() > 8, "pond %d platforms %d\n", n, P->Platforms.Num());
		EXPECT(P->Total == 8 + n * 3, "pond %d total %d\n", n, P->Total);
		EXPECT(Dragons == 1, "pond %d dragonflies %d\n", n, Dragons);
		EXPECT(P->Wasps.Num() == ExpectWasps, "pond %d wasps %d (want %d)\n", n, P->Wasps.Num(), ExpectWasps);
		EXPECT(P->SkyIndex == (n - 1) % 4, "pond %d sky %d\n", n, P->SkyIndex);
		// gaps between platforms must be jumpable
		for (int32 i = 1; i < P->Platforms.Num(); i++)
		{
			const auto& A = P->Platforms[i - 1]; const auto& B = P->Platforms[i];
			const float Gap = (B.X0 - B.Amp) - (A.X0 + A.W + A.Amp);
			EXPECT(Gap <= 210.f, "pond %d gap %d is %.0f\n", n, i, Gap);
		}
		Run(P, 1.f);
		Hud->DrawHUD();
		// Keep wasps away while eating flies, then eat each fly by touching it
		for (auto& W : P->Wasps) W.HZ = 5000;
		P->Frog.Inv = 0;
		for (int32 i = 0; i < P->Flies.Num(); i++)
		{
			auto& F = P->Flies[i];
			if (!F.bAlive) continue;
			P->Frog.X = float(F.P.X); P->Frog.Z = float(F.P.Y) - 18.f; P->Frog.Ground = -1; P->Frog.VZ = 0;
			F.RX = F.RZ = 0; F.HX = P->Frog.X; F.HZ = P->Frog.Z + 18.f;
			P->Tick(1.f / 60.f);
			if (F.bDragon) EXPECT(P->Frog.Power > 11.f, "pond %d dragonfly gives power\n", n);
			P->Frog.X = P->Platforms[0].X + 100; P->Frog.Z = P->Platforms[0].Top; P->Frog.Ground = 0;
		}
		EXPECT(P->Left == 0, "pond %d flies left %d\n", n, P->Left);
		Run(P, 2.f);
		EXPECT(P->State == ELilyState::Clear, "pond %d reaches cleared screen (state %d)\n", n, int(P->State));
		Hud->DrawHUD();
		EXPECT(Hud->Buttons.Num() >= 1, "cleared screen has a button\n");
	}

	// Wasp sting costs one heart; three falls into water end the game
	P->Hearts = 3; P->LoadLevel(3); P->SetState(ELilyState::Play); Run(P, 0.5f);
	P->Frog.Inv = 0;
	{ auto& W = P->Wasps[0]; W.RX = 0; W.HX = P->Frog.X; W.HZ = P->Frog.Z + 18.f; }
	P->Tick(1.f / 60.f);
	EXPECT(P->Hearts == 2, "sting costs a heart (hearts %d)\n", P->Hearts);
	for (auto& W : P->Wasps) W.HZ = 5000;
	for (int k = 0; k < 2; k++) { P->Frog.Inv = 0; P->Frog.Ground = -1; P->Frog.Z = -100; P->Tick(1.f / 60.f); Run(P, 2.f); }
	EXPECT(P->State == ELilyState::Over, "game over after losing all hearts (state %d hearts %d)\n", int(P->State), P->Hearts);
	Run(P, 2.f);
	Hud->DrawHUD();
	P->HandleButton(TEXT("again"));
	EXPECT(P->State == ELilyState::Play && P->Hearts == 3 && P->Level == 1, "try again restarts\n");

	// Double jump while powered
	P->Frog.Power = 5; P->Frog.Ground = -1; P->Frog.Z = 200; P->Frog.VZ = -100; P->Frog.bAirJumped = false; P->Frog.Buffer = 0.13f;
	P->UpdateFrog(1.f / 60.f, false, false, true);
	EXPECT(P->Frog.bAirJumped && P->Frog.VZ > 700, "double jump (vz %.0f)\n", P->Frog.VZ);

	// Pause / resume / quit, music and sound toggles
	P->HandlePauseKey(); EXPECT(P->State == ELilyState::Paused, "pause\n"); Hud->DrawHUD();
	P->HandleConfirm();  EXPECT(P->State == ELilyState::Play, "resume\n");
	P->HandleButton(TEXT("music")); P->HandleButton(TEXT("sound"));
	P->HandleButton(TEXT("pause")); P->HandleButton(TEXT("quit")); EXPECT(P->State == ELilyState::Title, "quit to title\n");

	// Synth renders finite, non-silent audio for both songs and every sound effect
	ULilySynth* S = new ULilySynth(FObjectInitializer());
	int32 SR = 48000; S->Init(SR);
	std::vector<float> Buf(1024);
	double Peak = 0; bool bFinite = true;
	auto Render = [&](float Sec) { for (int b = 0; b < int(Sec * SR / 1024); b++) { S->OnGenerateAudio(Buf.data(), 1024); for (float v : Buf) { bFinite &= std::isfinite(v); Peak = std::max(Peak, (double)std::fabs(v)); } } };
	S->SetMasterOn(true);
	S->SetMusic(ELilySong::Title, 84, 0.42f); Render(8.f);
	EXPECT(Peak > 0.05, "title music audible (peak %.3f)\n", Peak);
	Peak = 0; S->SetMusic(ELilySong::Game, 120, 0.6f); Render(8.f);
	EXPECT(Peak > 0.05, "game music audible (peak %.3f)\n", Peak);
	for (int e = 0; e <= int(ELilySfx::Hop2); e++) S->DoSfx(ELilySfx(e));
	Render(2.f);
	EXPECT(bFinite, "audio is finite\n");
	EXPECT(S->Voices.Num() < 160, "voices cleaned up (%d)\n", S->Voices.Num());

	std::printf(Fails ? "\n%d check(s) FAILED\n" : "\nALL CHECKS PASSED\n", Fails);
	return Fails ? 1 : 0;
}
