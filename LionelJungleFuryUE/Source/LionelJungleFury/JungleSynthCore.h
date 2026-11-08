// Engine-independent synthesizer for every sound in the game: weapon and creature effects,
// jungle ambience, the composed intro theme and the adaptive combat score.
// Mono, float internally, rendered to 16-bit PCM. Plain C++ so it can be tested outside Unreal.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

class FJungleSynthCore
{
public:
	enum EWave { Sine, Tri, Saw, Square, Noise };
	enum EFilter { None, Low, High, Band };

	// Must match ESfx order in JungleTypes.h
	enum ESound
	{
		Slash, Pistol, Smg, Shotgun, Crossbow, Launcher, Boom, Zap, Hit, Kill, Hurt,
		Pickup, Reload, Empty, EnemyShot, Roar, Unlock, Death, Chirp, Tink
	};

	explicit FJungleSynthCore(int InSampleRate = 32000)
		: SR(InSampleRate), Rng(1108u)
	{
		GenPhrase();
		NextStep = 0.0;
	}

	/** 0 = intro theme, 1 = between waves, 2 = combat, 3 = boss. */
	void SetMusicLevel(int InLevel) { WantLevel = InLevel; }
	void SetMusicVolume(float V) { MusTarget = V; }
	void SetSfxVolume(float V) { SfxVol = V; }

	void Play(int Sound)
	{
		switch (Sound)
		{
		case Slash: Nz(0.2f, 2000, Band, 0.5f); Tn(300, 120, 0.12f, Tri, 0.08f); break;
		case Pistol: Nz(0.2f, 2600, Low, 0.7f); Tn(170, 45, 0.14f, Square, 0.22f); break;
		case Smg: Nz(0.09f, 3200, Low, 0.42f); Tn(210, 70, 0.07f, Square, 0.12f); break;
		case Shotgun: Nz(0.45f, 1500, Low, 1.f); Tn(95, 32, 0.32f, Sine, 0.7f); Nz(0.12f, 400, Band, 0.4f, 0.45f); break;
		case Crossbow: Nz(0.16f, 3000, Band, 0.4f); Tn(700, 180, 0.16f, Tri, 0.2f); break;
		case Launcher: Tn(140, 55, 0.22f, Sine, 0.6f); Nz(0.18f, 700, Low, 0.45f); break;
		case Boom: Nz(1.1f, 500, Low, 1.f); Tn(75, 24, 0.9f, Sine, 0.9f); break;
		case Zap:
			for (int i = 0; i < 3; ++i) { Tn(Uni(600, 1600), Uni(80, 200), 0.12f, Saw, 0.09f, i * 0.03f); }
			Nz(0.25f, 4000, Band, 0.35f);
			break;
		case Hit: Tn(1200, 900, 0.04f, Square, 0.06f); break;
		case Kill: Tn(520, 110, 0.22f, Square, 0.12f); Nz(0.15f, 1800, Band, 0.2f); break;
		case Hurt: Tn(120, 55, 0.25f, Square, 0.3f); Nz(0.15f, 500, Low, 0.4f); break;
		case Pickup: Tn(660, 1320, 0.14f, Tri, 0.25f); Tn(990, 1760, 0.12f, Tri, 0.18f, 0.07f); break;
		case Reload: Tn(900, 700, 0.03f, Square, 0.12f); Tn(500, 420, 0.04f, Square, 0.12f, 0.22f); Tn(1100, 900, 0.03f, Square, 0.14f, 0.45f); break;
		case Empty: Tn(1400, 1300, 0.03f, Square, 0.08f); break;
		case EnemyShot: Tn(280, 760, 0.25f, Sine, 0.14f); break;
		case Roar: Nz(1.6f, 300, Low, 0.9f); Tn(62, 38, 1.4f, Saw, 0.35f); break;
		case Unlock: { const float F[4] = { 523, 659, 784, 1047 }; for (int i = 0; i < 4; ++i) { Tn(F[i], F[i] * 1.01f, 0.2f, Tri, 0.2f, i * 0.09f); } break; }
		case Death: Tn(300, 40, 1.2f, Saw, 0.3f); Nz(1.f, 400, Low, 0.6f); break;
		case Chirp:
		{
			const float B = Uni(2200, 3600);
			const int N = 2 + (int)Uni(0, 2.99f);
			for (int i = 0; i < N; ++i) { Tn(B, B + Uni(500, 1200), 0.08f, Sine, 0.045f, i * 0.11f); }
			break;
		}
		case Tink: Tn(Uni(3800, 5200), Uni(3000, 4200), 0.06f, Tri, 0.05f); Tn(Uni(6500, 8200), Uni(5600, 7000), 0.04f, Sine, 0.028f, 0.004f); break;
		default: break;
		}
	}

	void Render(int16_t* Out, int Count)
	{
		const float TwoPi = 6.28318530718f;
		const float InvSR = 1.f / SR;
		for (int n = 0; n < Count; ++n)
		{
			if (Clock >= NextStep)
			{
				MusicStep();
				NextStep += SamplesPerStep();
			}
			MusVol += (MusTarget - MusVol) * 0.0002f;

			float Mus = 0.f, Sfx = 0.f;
			for (Voice& V : Voices)
			{
				if (V.Dead) { continue; }
				if (V.Delay > 0.f) { V.Delay -= 1.f; continue; }
				const float T = V.T;
				float G;
				if (T < V.Att)
				{
					G = V.Peak * (T / V.Att);
				}
				else if (V.Sustain)
				{
					const float Rel = T - (V.Att + V.Dur);
					G = Rel < 0.f ? V.Peak : V.Peak * std::exp(-Rel * 18.f);
					if (Rel > 0.35f) { V.Dead = true; continue; }
				}
				else
				{
					const float D = (T - V.Att) / V.Dur;
					if (D > 1.f) { V.Dead = true; continue; }
					G = V.Peak * std::exp(-6.9f * D);
				}
				float F = V.F0;
				if (V.LogR != 0.f) { F *= std::exp(V.LogR * std::fmin(1.f, T / (V.Att + V.Dur))); }
				if (V.Vib > 0.f) { F *= 1.f + V.Vib * std::sin(TwoPi * 5.2f * T) * std::fmin(1.f, T / 0.5f); }

				float X;
				switch (V.Wave)
				{
				case Sine: X = std::sin(TwoPi * V.Phase); break;
				case Tri: X = 4.f * std::fabs(V.Phase - 0.5f) - 1.f; break;
				case Saw: X = 2.f * V.Phase - 1.f; break;
				case Square: X = V.Phase < 0.5f ? 1.f : -1.f; break;
				default: X = White(); break;
				}
				V.Phase += F * InvSR;
				V.Phase -= std::floor(V.Phase);

				if (V.HpA > 0.f) { V.Hp += V.HpA * (X - V.Hp); X -= V.Hp; }
				if (V.LpA > 0.f) { V.Lp += V.LpA * (X - V.Lp); X = V.Lp; }
				if (V.Mix2 > 0.f) { X += V.Mix2 * (4.f * std::fabs(V.Phase - 0.5f) - 1.f); }

				(V.Music ? Mus : Sfx) += X * G;
				V.T += InvSR;
			}

			// Insects: high band of noise, pulsing at 7 Hz.
			const float W = White();
			AmbHp += 0.55f * (W - AmbHp);
			AmbLp += 0.6f * ((W - AmbHp) - AmbLp);
			const float Amb = AmbLp * (0.02f + 0.015f * std::sin(TwoPi * 7.f * (float)(Clock * InvSR)));

			float Y = Sfx * SfxVol * 0.6f + Mus * MusVol + Amb * SfxVol;
			Y = std::tanh(Y * 1.6f);
			Out[n] = (int16_t)(Y * 30000.f);
			Clock += 1.0;
		}
		Compact();
	}

	int ActiveVoices() const { int N = 0; for (const Voice& V : Voices) { N += V.Dead ? 0 : 1; } return N; }

private:
	struct Voice
	{
		int Wave = Sine;
		float F0 = 440.f, LogR = 0.f, Dur = 0.1f, Att = 0.004f, Peak = 0.1f, T = 0.f, Delay = 0.f;
		float LpA = 0.f, HpA = 0.f, Lp = 0.f, Hp = 0.f, Vib = 0.f, Phase = 0.f, Mix2 = 0.f;
		bool Sustain = false, Music = false, Dead = false;
	};

	int SR;
	std::mt19937 Rng;
	std::vector<Voice> Voices;
	double Clock = 0.0, NextStep = 0.0;
	int Step = 0, Bar = 0, Level = 0, WantLevel = 0;
	float MusVol = 0.f, MusTarget = 0.42f, SfxVol = 1.f;
	float AmbHp = 0.f, AmbLp = 0.f;
	int Phrase[4][16] = {};

	float White() { return std::uniform_real_distribution<float>(-1.f, 1.f)(Rng); }
	float Uni(float A, float B) { return std::uniform_real_distribution<float>(A, B)(Rng); }
	static float Mtof(float M) { return 440.f * std::pow(2.f, (M - 69.f) / 12.f); }
	float Coef(float Fc) const { return 1.f - std::exp(-6.28318530718f * Fc / SR); }

	double SamplesPerStep() const
	{
		const double Bpm = Level >= 3 ? 124.0 : (Level == 0 ? 84.0 : 108.0);
		return SR * 60.0 / (Bpm * 4.0);
	}
	float StepSec() const { return (float)(SamplesPerStep() / SR); }

	Voice& Add(bool bMusic)
	{
		if (Voices.size() > 160)
		{
			for (Voice& V : Voices) { if (!V.Dead) { V.Dead = true; break; } }
		}
		Voices.emplace_back();
		Voices.back().Music = bMusic;
		return Voices.back();
	}

	void Compact()
	{
		size_t W = 0;
		for (size_t R = 0; R < Voices.size(); ++R) { if (!Voices[R].Dead) { Voices[W++] = Voices[R]; } }
		Voices.resize(W);
	}

	void Tn(float F, float F2, float Dur, int Wave, float G, float Delay = 0.f, bool bMusic = false,
		float Att = 0.004f, float Lp = 0.f, float Hp = 0.f, float Vib = 0.f, bool bSustain = false)
	{
		Voice& V = Add(bMusic);
		V.Wave = Wave; V.F0 = F; V.LogR = (F2 > 0.f && F2 != F) ? std::log(F2 / F) : 0.f;
		V.Dur = Dur; V.Att = Att; V.Peak = G; V.Delay = Delay * SR;
		V.LpA = Lp > 0.f ? Coef(Lp) : 0.f; V.HpA = Hp > 0.f ? Coef(Hp) : 0.f; V.Vib = Vib; V.Sustain = bSustain;
	}

	void Nz(float Dur, float F, int Filter, float G, float Delay = 0.f, bool bMusic = false)
	{
		Voice& V = Add(bMusic);
		V.Wave = Noise; V.Dur = Dur; V.Att = 0.002f; V.Peak = G; V.Delay = Delay * SR;
		if (Filter == Low) { V.LpA = Coef(F); }
		else if (Filter == High) { V.HpA = Coef(F); }
		else if (Filter == Band) { V.HpA = Coef(F / 1.5f); V.LpA = Coef(F * 1.5f); }
	}

	void GenPhrase()
	{
		int Deg = 5 + (int)Uni(0, 2.99f);
		for (int b = 0; b < 4; ++b)
		{
			for (int i = 0; i < 16; ++i)
			{
				const float P = i % 4 == 0 ? 0.55f : (i % 2 == 0 ? 0.32f : 0.12f);
				if (Uni(0, 1) < P)
				{
					Deg = std::max(0, std::min(11, Deg + (int)std::floor(Uni(-2.f, 2.999f))));
					Phrase[b][i] = Deg;
				}
				else
				{
					Phrase[b][i] = -1;
				}
			}
		}
	}

	void MusicStep()
	{
		if (Step == 0 || (Step == 8 && WantLevel != 0 && Level != 0))
		{
			if (WantLevel == 0 && Level != 0) { Bar = 0; }
			Level = WantLevel;
		}
		if (Level == 0) { IntroStep(Step); } else { GameStep(Step); }
		Step = (Step + 1) % 16;
		if (Step == 0)
		{
			++Bar;
			if (Bar % 8 == 0) { GenPhrase(); }
		}
	}

	// ---- The intro theme: Am | F | G | Em | Am | F | G | E, flute over harp arpeggios ----
	void IntroStep(int i)
	{
		static const int Ch[8][4] = { { 45, 57, 60, 64 }, { 41, 57, 60, 65 }, { 43, 55, 59, 62 }, { 40, 55, 59, 64 },
			{ 45, 57, 60, 64 }, { 41, 57, 60, 65 }, { 43, 55, 59, 62 }, { 40, 56, 59, 64 } };
		static const int Mel[][3] = { { 0, 76, 6 }, { 6, 74, 2 }, { 8, 72, 4 }, { 12, 69, 4 }, { 16, 72, 6 }, { 22, 74, 2 }, { 24, 76, 4 }, { 28, 72, 4 },
			{ 32, 74, 6 }, { 38, 71, 2 }, { 40, 67, 4 }, { 44, 71, 4 }, { 48, 76, 12 }, { 60, 79, 4 }, { 64, 81, 6 }, { 70, 79, 2 }, { 72, 76, 4 }, { 76, 72, 4 },
			{ 80, 77, 4 }, { 84, 76, 4 }, { 88, 72, 4 }, { 92, 69, 4 }, { 96, 71, 4 }, { 100, 74, 4 }, { 104, 79, 4 }, { 108, 74, 4 }, { 112, 68, 4 }, { 116, 71, 4 }, { 120, 76, 8 } };
		const float S16 = StepSec();
		const int B = Bar % 8, Abs = B * 16 + i;
		const int* C = Ch[B];
		if (i == 0)
		{
			Tn(Mtof(C[0]), 0, S16 * 15, Tri, 0.22f, 0, true, 0.05f);
			Tn(Mtof(C[0] - 12), 0, S16 * 15, Sine, 0.18f, 0, true, 0.05f);
			for (int k = 1; k < 4; ++k)
			{
				Tn(Mtof(C[k]) * 0.9965f, 0, S16 * 15, Saw, 0.016f, 0, true, 0.8f, 900.f);
				Tn(Mtof(C[k]) * 1.0035f, 0, S16 * 15, Saw, 0.016f, 0, true, 0.8f, 900.f);
			}
		}
		if (i % 2 == 0)
		{
			const int Arp[4] = { C[1] + 12, C[2] + 12, C[3] + 12, C[2] + 12 };
			const float F = Mtof(Arp[(i / 2) % 4]);
			Tn(F, 0, 0.6f, Sine, 0.07f, 0, true, 0.002f);
			Tn(F * 2, 0, 0.15f, Sine, 0.02f, 0, true, 0.001f);
		}
		if (i == 0 || i == 10) { Tn(i ? 150.f : 105.f, i ? 95.f : 60.f, 0.3f, Sine, 0.3f, 0, true, 0.002f); }
		if (i == 6 || i == 14) { NzM(0.06f, 6500, High, 0.025f); }
		if (B == 7 && i >= 12) { Tn(i % 2 ? 160.f : 120.f, 80, 0.15f, Sine, 0.25f, 0, true, 0.002f); }
		for (const auto& N : Mel)
		{
			if (N[0] == Abs) { Flute(Mtof((float)N[1]), N[2] * S16); }
		}
	}

	void Flute(float F, float Dur)
	{
		Voice& V = Add(true);
		V.Wave = Sine; V.F0 = F; V.Dur = Dur; V.Att = 0.07f; V.Peak = 0.12f; V.Sustain = true;
		V.Vib = 0.008f; V.LpA = Coef(2600.f); V.Mix2 = 0.35f;
		NzM(0.08f, 3000, Band, 0.025f);
	}

	void NzM(float Dur, float F, int Filter, float G) { Nz(Dur, F, Filter, G, 0.f, true); }

	// ---- Adaptive combat score ----
	void GameStep(int i)
	{
		static const int Kick[16] = { 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0 };
		static const int Tom[16] = { 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 1, 1, 0, 0, 2, 0 };
		static const int Bass[16] = { 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0 };
		static const int Prog[4] = { 0, 0, -4, -2 };
		static const int Penta[5] = { 0, 3, 5, 7, 10 };
		const int L = Level;
		const float S16 = StepSec();
		const int Root = 45 + Prog[Bar % 4];

		if (i == 0)
		{
			const float Bar16 = S16 * 16.f;
			for (int M : { Root + 12, Root + 19 })
			{
				Tn(Mtof((float)M) * 0.9965f, 0, Bar16 - 0.4f, Saw, L >= 3 ? 0.03f : 0.022f, 0, true, 0.5f, L >= 3 ? 1100.f : 650.f);
				Tn(Mtof((float)M) * 1.004f, 0, Bar16 - 0.4f, Saw, 0.018f, 0, true, 0.5f, 650.f);
			}
		}
		NzM(0.045f, 7500, High, (i % 4 == 2 ? 0.06f : 0.03f));
		if (L >= 2 && Kick[i]) { Tn(150, 40, 0.3f, Sine, 0.85f, 0, true, 0.002f); }
		else if (L == 1 && (i == 0 || i == 8)) { Tn(130, 40, 0.28f, Sine, 0.45f, 0, true, 0.002f); }
		if (L >= 2 && Tom[i])
		{
			const float F = Tom[i] == 1 ? 118.f : 176.f;
			Tn(F, F * 0.62f, 0.22f, Sine, 0.4f, 0, true, 0.002f);
			NzM(0.03f, 2500, Band, 0.12f);
		}
		if (L >= 3 && i % 2 == 0)
		{
			Tn(78, 48, 0.25f, Sine, 0.5f, 0, true, 0.002f);
			if (i % 8 == 4) { NzM(0.18f, 1200, Band, 0.3f); }
		}
		if (Bass[i])
		{
			const float F = Mtof((float)(Root - 12 + (i == 14 ? 7 : 0)));
			Tn(F, 0, S16 * 3, Tri, 0.32f, 0, true, 0.008f);
			Tn(F, 0, S16 * 2.5f, Saw, 0.06f, 0, true, 0.008f, 380.f);
		}
		if (L >= 3 && (i == 0 || i == 6 || i == 10))
		{
			for (int O : { 12, 15, 19 }) { Tn(Mtof((float)(Root + O)), 0, S16 * 1.6f, Saw, 0.05f, 0, true, 0.01f, 1400.f); }
		}
		const int D = Phrase[Bar % 4][i];
		if (D >= 0)
		{
			const float F = Mtof((float)(57 + 12 * (D / 5) + Penta[D % 5]));
			Tn(F, 0, 0.45f, Sine, L >= 2 ? 0.13f : 0.17f, 0, true, 0.003f);
			Tn(F * 4, 0, 0.07f, Sine, 0.035f, 0, true, 0.001f);
		}
	}
};
