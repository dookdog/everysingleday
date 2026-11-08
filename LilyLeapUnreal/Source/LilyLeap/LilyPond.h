#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LilyTypes.h"
#include "LilyPond.generated.h"

class ALilyProp;
class ACameraActor;
class ULilySynth;
class UStaticMeshComponent;

/** One thing the frog can stand on. */
struct FLilyPlatform
{
	ELilyPlat Type = ELilyPlat::Island;
	float X = 0.f, X0 = 0.f, W = 100.f;
	float Top = 0.f, BaseTop = 0.f;
	float Amp = 0.f, Spd = 0.f, Phase = 0.f;   // drifting lily pads
	float DX = 0.f;                            // how far it moved this step
	float Squish = 0.f;                        // bellflower bounce animation
	int32 Palette = 0;
	bool bLeaves = false;
	bool bBlossom = false;
	ALilyProp* Actor = nullptr;
	USceneComponent* Cup = nullptr;
};

struct FLilyFly
{
	float HX = 0.f, HZ = 0.f;   // home position
	float RX = 0.f, RZ = 0.f;   // loop size
	float Sp = 1.f, Ph = 0.f;
	bool bGold = false;
	bool bAlive = true;
	bool bCaught = false;
	FVector2D P = FVector2D::ZeroVector;
	ALilyProp* Actor = nullptr;
	USceneComponent* WingL = nullptr;
	USceneComponent* WingR = nullptr;
};

struct FLilyParticle
{
	FVector P = FVector::ZeroVector, V = FVector::ZeroVector;
	float Life = 0.f, Max = 1.f, Size = 4.f;
	bool bDrop = false;   // water drops fall; sparks float
	int32 Comp = -1;
};

/** The frog's model parts, repositioned every frame for squash, leg pose and eye direction. */
struct FLilyFrogRig
{
	USceneComponent* Body = nullptr;
	UStaticMeshComponent* Thigh[2] = { nullptr, nullptr };
	UStaticMeshComponent* Foot[2] = { nullptr, nullptr };
	UStaticMeshComponent* Pupil[2] = { nullptr, nullptr };
	UStaticMeshComponent* Throat = nullptr;
	UStaticMeshComponent* Tongue = nullptr;
	UStaticMeshComponent* TongueTip = nullptr;
};

/**
 * The whole game: builds the pond from code, runs the frog, flies, scoring and menus,
 * and drives the camera, lights and music. Spawned by ALilyGameMode.
 */
UCLASS()
class LILYLEAP_API ALilyPond : public AActor
{
	GENERATED_BODY()

public:
	ALilyPond();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---- Called by the player controller ----
	void HandleButton(FName Id);
	void HandleConfirm();      // Enter / Space on a menu
	void HandlePauseKey();     // Escape / P
	bool IsMenuOpen() const;
	void NotifyTouchUsed() { bTouchUI = true; }

	// ---- Read by the HUD ----
	ELilyState State = ELilyState::Title;
	int32 Score = 0, Hearts = 3, Best = 0, Combo = 0, Level = 1, Total = 0, Left = 0, ClearBonus = 0;
	float GameTime = 0.f, LastEat = -9.f, LevelTime = 0.f;
	bool bMusicOn = true, bSoundOn = true, bTouchUI = false;
	TArray<FLilyText> Texts;
	FVector FrogWorld() const { return FVector(Frog.X, 0.f, Frog.Z + 18.f); }
	bool NearestFlyWorld(FVector& Out, bool& bGold) const;

private:
	struct FFrogState
	{
		float X = 90.f, Z = 90.f, VX = 0.f, VZ = 0.f, Face = 1.f;
		int32 Ground = -1, Safe = 0;
		float Coyote = 0.f, Buffer = 0.f, Air = 0.f, Land = 0.f, Inv = 0.f, Gulp = 0.f, Croak = 4.f;
		bool bCut = true;
	};
	struct FTongueState
	{
		int32 Phase = 0;      // 0 idle, 1 shooting out, 2 reeling in
		float Len = 0.f, Max = 0.f, Ang = 0.f, Cooldown = 0.f;
		int32 Target = -1, Caught = -1;
	};

	// Setup
	void BuildLightingAndCamera();
	void BuildScenery();
	void BuildFrog();
	void BuildFx();
	void LoadLevel(int32 N);
	void ClearLevel();
	void BuildPlatformVisual(FLilyPlatform& P);
	void BuildFlyVisual(FLilyFly& F);
	void AddDecor(ALilyProp* Owner, float X, float Top, FRandomStream& Rng);

	// Game flow
	void StartGame();
	void SetState(ELilyState NewState);
	void SyncMusic();
	void SaveProgress();
	void LoadProgress();

	// Simulation
	void Step(float Dt);
	void UpdatePlatforms(float Dt);
	void UpdateFlies();
	void UpdateFrog(float Dt, bool bLeft, bool bRight, bool bJumpHeld);
	void UpdateTongue(float Dt);
	void FireTongue();
	void Eat(int32 FlyIndex);
	void LandOn(int32 PlatformIndex);
	void Hurt();
	void UpdateFx(float Dt);
	FVector2D Mouth() const { return FVector2D(Frog.X + Frog.Face * 19.f, Frog.Z + 17.f); }

	// Presentation
	void UpdateVisuals(float Dt);
	void UpdateCamera(float Dt, bool bSnap);
	void Burst(const FVector& At, int32 Count, const TArray<FLinearColor>& Colors, float Speed, bool bDrops);
	void FloatText(const FVector& At, const FString& Text, const FLinearColor& Color, float Size);
	void Sfx(ELilySfx S);

	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<ULilySynth> Synth;
	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TObjectPtr<ALilyProp> FrogActor;
	UPROPERTY() TObjectPtr<ALilyProp> FxActor;
	UPROPERTY() TObjectPtr<ALilyProp> SkyActor;
	UPROPERTY() TArray<TObjectPtr<ALilyProp>> LevelActors;
	UPROPERTY() TArray<TObjectPtr<ALilyProp>> Clouds;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> FxPool;

	TArray<FLilyPlatform> Platforms;
	TArray<FLilyFly> Flies;
	TArray<FLilyParticle> Particles;
	FLilyFrogRig Rig;
	FFrogState Frog;
	FTongueState Tongue;
	float LevelEnd = 3000.f;
	float ClearTimer = 0.f;
	float OverTimer = -1.f;
	float LevelBpm = 100.f;
	int32 NextFx = 0;
	FVector CamPos = FVector::ZeroVector;
	bool bPrevJump = false, bPrevTongue = false;
};
