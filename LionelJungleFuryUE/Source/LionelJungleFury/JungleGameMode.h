// Builds the jungle at startup and runs the match: waves, score, unlocks, pause and game over.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JungleTypes.h"
#include "JungleGameMode.generated.h"

class AJungleEnemy;
class AJunglePickup;
class AJungleFX;
class AJungleAudio;
class ALionelCharacter;
class UStaticMeshComponent;

UCLASS()
class AJungleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJungleGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static AJungleGameMode* Get(const UObject* WorldContext);

	// ---- match state ----
	EJState State = EJState::Title;
	int32 Wave = 0;
	int32 Score = 0;
	int32 Kills = 0;
	int32 Best = 0;
	bool bFight = false;

	UPROPERTY() TArray<AJungleEnemy*> Enemies;
	UPROPERTY() TArray<AJunglePickup*> Pickups;
	UPROPERTY() AJungleEnemy* Boss = nullptr;
	UPROPERTY() AJungleFX* FX = nullptr;
	UPROPERTY() AJungleAudio* Audio = nullptr;

	/** Tree trunks, pillars, rocks and the idol, as circles in meters, for enemy steering. */
	struct FObstacle { float X; float Y; float R; float H; };
	TArray<FObstacle> Obstacles;

	// ---- settings (saved) ----
	float Sensitivity = 1.f;
	bool bSoundOn = true;
	bool bMusicOn = true;

	// ---- feedback read by the HUD ----
	FString BannerTitle;
	FString BannerSub;
	float BannerTime = 0.f;
	FString PopText;
	float PopTime = 0.f;
	float HitTime = 0.f;
	bool bHitKill = false;
	float HurtFlash = 0.f;
	float Time = 0.f;
	int32 FreshSlot = -1;
	float FreshTime = 0.f;

	ALionelCharacter* Player() const;
	int32 AliveCount() const;
	int32 RemainingCount() const { return Queue.Num() + AliveCount(); }

	void StartGame();
	void TogglePause();
	void GameOver();
	void Banner(const FString& Title, const FString& Sub, float Duration);
	void Pop(const FString& Text);
	void HitMarker(bool bKill);
	void Sfx(ESfx S);
	void SaveSettings();

	void EnemyKilled(AJungleEnemy* E);
	void SpawnEnemy(EEnemyType Type, const FVector2D* Near = nullptr);
	void Explode(const FVector& At, float RadiusM, float Damage, bool bBig);
	void Drop(const FVector2D& AtM, bool bAmmo);

	/** Pushes a circle (meters) out of obstacles and keeps it inside the arena. */
	void Collide(FVector2D& P, float R, float Z) const;

private:
	TArray<EEnemyType> Queue;
	float SpawnT = 0.f;
	float WaveTimer = 0.f;

	UPROPERTY() AActor* WorldActor = nullptr;
	UPROPERTY() TArray<UStaticMeshComponent*> Flames;
	UPROPERTY() TArray<UStaticMeshComponent*> Fireflies;
	struct FFirefly { float X; float Y; float Z; float Phase; };
	TArray<FFirefly> FireflyBase;

	void BuildWorld();
	void BuildTerrain(USceneComponent* Root);
	void BuildTemple(USceneComponent* Root);
	void BuildJungle(USceneComponent* Root);
	void UpdateWaves(float Dt);
	void StartWave(int32 N);
	void WaveClear();
	void LoadSettings();
};
