#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "LilyTypes.h"
#include <atomic>
#include "LilySynth.generated.h"

/**
 * All of Lily Leap's audio, synthesized live: the "Morning on the Pond" title waltz, the in-game tune,
 * and every sound effect. No audio files are needed.
 *
 * Game-thread calls (PlaySfx, SetMusic, SetMasterOn) only post requests; voices are created and
 * rendered on the audio render thread.
 */
UCLASS(ClassGroup = Synth)
class LILYLEAP_API ULilySynth : public USynthComponent
{
	GENERATED_BODY()

public:
	ULilySynth(const FObjectInitializer& ObjectInitializer);

	void PlaySfx(ELilySfx Sfx);

	/** Level 0 fades the music out; anything above fades it in to that volume. */
	void SetMusic(ELilySong Song, float Bpm, float Level);

	void SetMasterOn(bool bOn);

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	enum EWave : uint8 { Sine, Triangle, Square, NoiseLow, NoiseHigh };

	struct FVoice
	{
		EWave Wave = Sine;
		float Freq = 440.f;
		float FreqMul = 1.f;      // per-sample pitch sweep multiplier
		float SweepLeft = 0.f;    // seconds of sweep remaining
		float T = 0.f;            // seconds since start; negative while delayed
		float Dur = 0.1f;
		float Vol = 0.1f;
		float Attack = 0.01f;
		float Env = 0.f;
		float DecayMul = 1.f;     // per-sample exponential decay after the attack
		float Phase = 0.f;
		float Alpha = 1.f;        // noise filter coefficient
		float Filt = 0.f;
		float PrevIn = 0.f;
		bool bMusic = false;
	};

	// Audio-thread only
	void AddTone(float F0, float F1, float Dur, EWave Wave, float Vol, float Delay = 0.f, float Attack = 0.012f, bool bMusic = false, float Sweep = -1.f);
	void AddNoise(float Dur, float Vol, float Cutoff, bool bHighpass, float Delay = 0.f, bool bMusic = false);
	void Note(int32 Midi, float Dur, EWave Wave, float Vol, float Attack = 0.008f, float Delay = 0.f);
	void DoSfx(ELilySfx Sfx);
	void PlayTitleStep(int32 Step, float StepDur);
	void PlayGameStep(int32 Step, float StepDur);

	TArray<FVoice> Voices;
	float SampleRateF = 48000.f;
	int64 Clock = 0;
	double NextStepAt = 0.0;
	int32 Step = 0;
	int32 CurrentSong = -1;
	float MusicGain = 0.f;
	float MasterGain = 1.f;
	uint32 NoiseSeed = 22222u;

	// Shared with the game thread
	std::atomic<int32> WantSong { 0 };
	std::atomic<float> WantBpm { 84.f };
	std::atomic<float> WantMusic { 0.f };
	std::atomic<float> WantMaster { 1.f };
};
