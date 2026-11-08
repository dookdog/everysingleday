// Streams the procedural synthesizer into Unreal through a USoundWaveProcedural.
// Ticks even while paused so the menu music keeps playing.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JungleTypes.h"
#include "JungleSynthCore.h"
#include "JungleAudio.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

UCLASS()
class AJungleAudio : public AActor
{
	GENERATED_BODY()

public:
	AJungleAudio();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void Play(ESfx Sound);

private:
	static constexpr int32 SampleRate = 32000;

	UPROPERTY() UAudioComponent* AudioComp = nullptr;
	UPROPERTY() USoundWaveProcedural* Wave = nullptr;

	FJungleSynthCore Synth;   // defaults to 32 kHz, matching SampleRate
	TArray<int16> Buffer;
	float ChirpT = 1.4f;
};
