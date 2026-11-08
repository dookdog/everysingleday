#include "JungleAudio.h"
#include "JungleGameMode.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"

AJungleAudio::AJungleAudio()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	RootComponent = AudioComp;
	AudioComp->bAutoActivate = false;
	AudioComp->bIsUISound = true;          // keeps playing while the game is paused
	AudioComp->bAllowSpatialization = false;
}

void AJungleAudio::BeginPlay()
{
	Super::BeginPlay();
	Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	AudioComp->SetSound(Wave);
	AudioComp->Play();
}

void AJungleAudio::Play(ESfx Sound)
{
	AJungleGameMode* GM = AJungleGameMode::Get(this);
	if (GM && !GM->bSoundOn) { return; }
	Synth.Play(static_cast<int>(Sound));
}

void AJungleAudio::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Wave) { return; }

	if (AJungleGameMode* GM = AJungleGameMode::Get(this))
	{
		int32 Level = 0;
		if (GM->State == EJState::Playing || GM->State == EJState::Paused)
		{
			Level = GM->Boss ? 3 : (GM->bFight ? 2 : 1);
		}
		Synth.SetMusicLevel(Level);
		const float Vol = !GM->bMusicOn ? 0.f : (GM->State == EJState::Paused ? 0.12f : (GM->State == EJState::Over ? 0.25f : 0.42f));
		Synth.SetMusicVolume(Vol);
		Synth.SetSfxVolume(GM->bSoundOn ? 1.f : 0.f);

		ChirpT -= Dt;
		if (ChirpT <= 0.f)
		{
			ChirpT = 1.4f;
			if ((GM->State == EJState::Playing || GM->State == EJState::Title) && FMath::FRand() < 0.35f && GM->bSoundOn)
			{
				Synth.Play(FJungleSynthCore::Chirp);
			}
		}
	}

	// Keep about 90 ms of audio queued ahead of the mixer.
	const int32 Target = SampleRate * 9 / 100;
	const int32 Queued = Wave->GetAvailableAudioByteCount() / (int32)sizeof(int16);
	if (Queued < Target)
	{
		const int32 N = Target + SampleRate / 50 - Queued;
		Buffer.SetNumUninitialized(N);
		Synth.Render(reinterpret_cast<int16_t*>(Buffer.GetData()), N);
		Wave->QueueAudio(reinterpret_cast<const uint8*>(Buffer.GetData()), N * (int32)sizeof(int16));
	}
}
