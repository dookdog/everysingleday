#include "LilySynth.h"

namespace
{
	float MidiToFreq(int32 Midi) { return 440.f * FMath::Pow(2.f, (Midi - 69) / 12.f); }

	// [step (16ths), midi note, length in 16ths]
	struct FMelodyNote { int32 Step, Midi, Len; };

	// In-game tune: 8 bars of 16 sixteenths in C major.
	const TArray<TArray<FMelodyNote>>& GameMelody()
	{
		static const TArray<TArray<FMelodyNote>> M = {
			{ {0,72,2},{2,76,2},{4,79,3},{8,76,2},{10,72,2},{12,74,4} },
			{ {0,76,2},{2,72,2},{4,69,3},{8,72,2},{10,69,2},{12,67,4} },
			{ {0,69,2},{2,72,2},{4,77,3},{8,76,2},{10,74,2},{12,72,4} },
			{ {0,74,3},{3,71,1},{4,67,2},{6,71,2},{8,74,4},{12,79,4} },
			{ {0,79,2},{2,76,2},{4,72,2},{6,76,2},{8,79,3},{11,81,1},{12,79,4} },
			{ {0,81,2},{2,79,2},{4,76,4},{8,72,2},{10,74,2},{12,76,4} },
			{ {0,77,2},{2,76,2},{4,74,2},{6,72,2},{8,74,2},{10,71,2},{12,67,4} },
			{ {0,72,6},{8,67,2},{10,69,2},{12,71,2},{14,74,2} },
		};
		return M;
	}
	struct FChord { int32 Root; int32 Tones[3]; };
	const FChord C_ = { 48, {60,64,67} }, Am = { 45, {57,60,64} }, F_ = { 41, {57,60,65} }, G_ = { 43, {59,62,67} };
	// Chord per half bar
	const FChord* GameChords[8][2] = { {&C_,&C_},{&Am,&Am},{&F_,&F_},{&G_,&G_},{&C_,&C_},{&Am,&Am},{&F_,&G_},{&C_,&C_} };

	// Title theme "Morning on the Pond": a 3/4 waltz in F, 8 bars of 12 sixteenths.
	const TArray<TArray<FMelodyNote>>& TitleMelody()
	{
		static const TArray<TArray<FMelodyNote>> M = {
			{ {0,81,6},{6,79,2},{8,77,4} },
			{ {0,74,4},{4,77,4},{8,81,4} },
			{ {0,82,6},{6,81,2},{8,79,4} },
			{ {0,76,8},{8,72,4} },
			{ {0,77,4},{4,81,4},{8,84,4} },
			{ {0,84,6},{6,81,2},{8,76,4} },
			{ {0,79,4},{4,77,4},{8,74,4} },
			{ {0,79,8},{8,76,4} },
		};
		return M;
	}
	const FChord TitleChords[8] = {
		{41,{65,69,72}}, {38,{62,65,69}}, {46,{62,65,70}}, {36,{64,67,72}},
		{41,{65,69,72}}, {45,{64,69,72}}, {46,{62,65,70}}, {36,{64,70,72}},
	};

	constexpr int32 TitleSteps = 96;
	constexpr int32 GameSteps = 128;
	constexpr int32 MaxVoices = 160;
}

ULilySynth::ULilySynth(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool ULilySynth::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateF = (float)SampleRate;
	Voices.Reserve(MaxVoices + 16);
	return true;
}

void ULilySynth::PlaySfx(ELilySfx Sfx)
{
	SynthCommand([this, Sfx]() { DoSfx(Sfx); });
}

void ULilySynth::SetMusic(ELilySong Song, float Bpm, float Level)
{
	WantSong.store((int32)Song);
	WantBpm.store(FMath::Max(30.f, Bpm));
	WantMusic.store(FMath::Max(0.f, Level));
}

void ULilySynth::SetMasterOn(bool bOn)
{
	WantMaster.store(bOn ? 0.9f : 0.f);
}

void ULilySynth::AddTone(float F0, float F1, float Dur, EWave Wave, float Vol, float Delay, float Attack, bool bMusic, float Sweep)
{
	if (Voices.Num() >= MaxVoices) Voices.RemoveAt(0);
	FVoice V;
	V.Wave = Wave;
	V.Freq = F0;
	const float SweepTime = Sweep > 0.f ? Sweep : Dur;
	V.SweepLeft = (F1 != F0) ? SweepTime : 0.f;
	V.FreqMul = (F1 != F0) ? FMath::Pow(F1 / F0, 1.f / (SweepTime * SampleRateF)) : 1.f;
	V.T = -Delay;
	V.Dur = Dur;
	V.Vol = Vol;
	V.Attack = FMath::Max(Attack, 0.001f);
	// Exponential decay from Vol to 0.0001 over the rest of the note (matches the web version's envelopes).
	const float DecayTime = FMath::Max(Dur - V.Attack, 0.005f);
	V.DecayMul = FMath::Exp(-FMath::Loge(FMath::Max(Vol, 0.0002f) / 0.0001f) / (DecayTime * SampleRateF));
	V.bMusic = bMusic;
	Voices.Add(V);
}

void ULilySynth::AddNoise(float Dur, float Vol, float Cutoff, bool bHighpass, float Delay, bool bMusic)
{
	AddTone(0.f, 0.f, Dur, bHighpass ? NoiseHigh : NoiseLow, Vol, Delay, 0.002f, bMusic);
	FVoice& V = Voices.Last();
	const float Dt = 1.f / SampleRateF;
	if (bHighpass)
	{
		const float RC = 1.f / (2.f * PI * Cutoff);
		V.Alpha = RC / (RC + Dt);
	}
	else
	{
		V.Alpha = 1.f - FMath::Exp(-2.f * PI * Cutoff * Dt);
	}
}

void ULilySynth::Note(int32 Midi, float Dur, EWave Wave, float Vol, float Attack, float Delay)
{
	const float F = MidiToFreq(Midi);
	AddTone(F, F, Dur, Wave, Vol, Delay, Attack, true);
}

void ULilySynth::DoSfx(ELilySfx Sfx)
{
	switch (Sfx)
	{
	case ELilySfx::Jump:   AddTone(280, 640, 0.15f, Sine, 0.14f); break;
	case ELilySfx::Land:   AddTone(150, 90, 0.07f, Sine, 0.08f); break;
	case ELilySfx::Tongue: AddTone(900, 1700, 0.07f, Triangle, 0.05f); break;
	case ELilySfx::Gulp:
		AddTone(300, 85, 0.17f, Sine, 0.28f);
		AddTone(600, 200, 0.10f, Triangle, 0.07f, 0.04f);
		break;
	case ELilySfx::Gold:
	{
		const float Fs[] = { 784, 988, 1175, 1568 };
		for (int32 i = 0; i < 4; i++) AddTone(Fs[i], Fs[i] * 1.01f, 0.12f, Triangle, 0.09f, i * 0.06f);
		break;
	}
	case ELilySfx::Bounce:
		AddTone(170, 760, 0.28f, Sine, 0.2f);
		AddTone(340, 1500, 0.22f, Triangle, 0.05f);
		break;
	case ELilySfx::Splash:
		AddNoise(0.4f, 0.35f, 1100.f, false);
		AddTone(420, 120, 0.3f, Sine, 0.08f);
		break;
	case ELilySfx::Clear:
	{
		const float Fs[] = { 523, 659, 784, 1047, 1319 };
		for (int32 i = 0; i < 5; i++) AddTone(Fs[i], Fs[i], 0.2f, Triangle, 0.1f, i * 0.1f);
		break;
	}
	case ELilySfx::Croak:
		AddTone(190, 150, 0.09f, Square, 0.035f);
		AddTone(200, 140, 0.11f, Square, 0.035f, 0.13f);
		break;
	case ELilySfx::Sting:
		AddTone(900, 380, 0.16f, Square, 0.04f);
		AddNoise(0.12f, 0.12f, 2500.f, true);
		break;
	case ELilySfx::Power:
	{
		const float Fs[] = { 659, 784, 988, 1319, 1568 };
		for (int32 i = 0; i < 5; i++) AddTone(Fs[i], Fs[i] * 1.02f, 0.14f, Triangle, 0.08f, i * 0.05f);
		break;
	}
	case ELilySfx::Hop2:
		AddTone(520, 1040, 0.14f, Sine, 0.12f);
		AddTone(1040, 1560, 0.10f, Triangle, 0.04f, 0.03f);
		break;
	case ELilySfx::Over:
	{
		const float Fs[] = { 392, 330, 262, 196 };
		for (int32 i = 0; i < 4; i++) AddTone(Fs[i], Fs[i] * 0.98f, 0.25f, Triangle, 0.09f, i * 0.16f);
		break;
	}
	}
}

void ULilySynth::PlayGameStep(int32 InStep, float Sd)
{
	const int32 Bar = InStep / 16, S = InStep % 16;
	for (const FMelodyNote& N : GameMelody()[Bar])
	{
		if (N.Step != S) continue;
		const float D = N.Len * Sd * 0.95f + 0.12f;
		Note(N.Midi, D, Triangle, 0.16f);           // marimba-like lead
		Note(N.Midi + 12, D * 0.5f, Sine, 0.05f);   // octave shimmer
	}
	const FChord& C = *GameChords[Bar][S < 8 ? 0 : 1];
	if (S == 0 || S == 8)
	{
		for (int32 Tone : C.Tones)
		{
			Note(Tone, Sd * 8.5f, Sine, 0.035f, 0.12f);
			const float F = MidiToFreq(Tone) * 1.004f;
			AddTone(F, F, Sd * 8.5f, Sine, 0.02f, 0.f, 0.12f, true);
		}
		Note(C.Root, Sd * 3.5f, Sine, 0.22f, 0.01f);   // bass
		AddTone(110, 110, 0.12f, Sine, 0.12f, 0.f, 0.003f, true); // soft kick
	}
	if (S == 6 || S == 14) Note(C.Root + 12, Sd * 1.6f, Sine, 0.12f);
	if (S == 4 || S == 12) Note(C.Root + 7, Sd * 1.6f, Sine, 0.10f);
	if (S % 4 == 2) AddNoise(0.12f, 0.09f, 6000.f, true, 0.f, true);
	else if (S % 2 == 1) AddNoise(0.12f, 0.035f, 6000.f, true, 0.f, true);
}

void ULilySynth::PlayTitleStep(int32 InStep, float Sd)
{
	const int32 Bar = InStep / 12, S = InStep % 12;
	for (const FMelodyNote& N : TitleMelody()[Bar])
	{
		if (N.Step != S) continue;
		Note(N.Midi, 0.9f + N.Len * Sd, Sine, 0.15f, 0.004f);   // music box
		const float F = MidiToFreq(N.Midi) * 4.f;
		AddTone(F, F, 0.25f, Sine, 0.025f, 0.f, 0.002f, true);   // its bright tine
	}
	const FChord& C = TitleChords[Bar];
	if (S == 0)
	{
		Note(C.Root, Sd * 11.f, Sine, 0.2f, 0.02f);
		for (int32 Tone : C.Tones) Note(Tone - 12, Sd * 12.f, Sine, 0.018f, 0.4f);  // soft bed
	}
	int32 Arp = -1;
	switch (S) { case 2: Arp = 0; break; case 4: Arp = 1; break; case 6: Arp = 2; break; case 8: Arp = 1; break; case 10: Arp = 2; break; default: break; }
	if (Arp >= 0) Note(C.Tones[Arp], 0.8f, Triangle, 0.05f, 0.003f);           // harp
	// birdsong
	if ((Bar == 3 || Bar == 7) && S == 9)
	{
		AddTone(2600, 3400, 0.09f, Sine, 0.03f, 0.f, 0.01f, true, 0.07f);
		AddTone(2800, 3600, 0.09f, Sine, 0.025f, 0.11f, 0.01f, true, 0.07f);
	}
	if (Bar == 5 && S == 3) AddTone(3000, 2400, 0.09f, Sine, 0.025f, 0.f, 0.01f, true, 0.07f);
}

int32 ULilySynth::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	const float Dt = 1.f / SampleRateF;

	const int32 Song = WantSong.load();
	if (Song != CurrentSong)
	{
		CurrentSong = Song;
		Step = 0;
		NextStepAt = (double)Clock;
	}
	const float StepDur = 60.f / WantBpm.load() / 4.f;
	const float MusicTarget = WantMusic.load();
	const float MasterTarget = WantMaster.load();
	const int32 SongSteps = CurrentSong == (int32)ELilySong::Title ? TitleSteps : GameSteps;

	for (int32 i = 0; i < NumSamples; i++)
	{
		// Sequencer: only advances while music is (or is still fading) audible.
		if (MusicTarget > 0.001f || MusicGain > 0.001f)
		{
			if ((double)Clock >= NextStepAt)
			{
				const int32 S = Step % SongSteps;
				if (CurrentSong == (int32)ELilySong::Title) PlayTitleStep(S, StepDur);
				else PlayGameStep(S, StepDur);
				Step = (Step + 1) % SongSteps;
				NextStepAt += (double)(StepDur * SampleRateF);
			}
		}
		else
		{
			NextStepAt = (double)Clock;
		}
		MusicGain += (MusicTarget - MusicGain) * 0.00015f;
		MasterGain += (MasterTarget - MasterGain) * 0.001f;

		float MusicSum = 0.f, SfxSum = 0.f;
		for (FVoice& V : Voices)
		{
			if (V.T < 0.f) { V.T += Dt; continue; }
			if (V.T >= V.Dur) continue;

			if (V.T < V.Attack) V.Env = V.Vol * (V.T / V.Attack);
			else V.Env *= V.DecayMul;
			if (V.T < V.Attack && V.T + Dt >= V.Attack) V.Env = V.Vol;

			float Smp = 0.f;
			if (V.Wave == NoiseLow || V.Wave == NoiseHigh)
			{
				NoiseSeed = NoiseSeed * 1664525u + 1013904223u;
				const float W = (float)(NoiseSeed >> 9) * (1.f / 8388608.f) * 2.f - 1.f;
				if (V.Wave == NoiseLow) { V.Filt += V.Alpha * (W - V.Filt); Smp = V.Filt; }
				else { V.Filt = V.Alpha * (V.Filt + W - V.PrevIn); V.PrevIn = W; Smp = V.Filt; }
			}
			else
			{
				if (V.SweepLeft > 0.f) { V.Freq *= V.FreqMul; V.SweepLeft -= Dt; }
				V.Phase += V.Freq * Dt;
				V.Phase -= FMath::FloorToFloat(V.Phase);
				switch (V.Wave)
				{
				case Sine:     Smp = FMath::Sin(2.f * PI * V.Phase); break;
				case Triangle: Smp = 1.f - 4.f * FMath::Abs(V.Phase - 0.5f); break;
				case Square:   Smp = V.Phase < 0.5f ? 1.f : -1.f; break;
				default: break;
				}
			}
			const float Out = Smp * V.Env;
			if (V.bMusic) MusicSum += Out; else SfxSum += Out;
			V.T += Dt;
		}
		OutAudio[i] = FMath::Clamp((MusicSum * MusicGain + SfxSum) * MasterGain, -1.f, 1.f);
		Clock++;
	}

	Voices.RemoveAllSwap([](const FVoice& V) { return V.T >= V.Dur; });
	return NumSamples;
}
