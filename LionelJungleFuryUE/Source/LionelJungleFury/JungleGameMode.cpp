#include "JungleGameMode.h"
#include "LionelCharacter.h"
#include "LionelPlayerController.h"
#include "JungleHUD.h"
#include "JungleEnemy.h"
#include "JungleActors.h"
#include "JungleAudio.h"
#include "JungleWorld.h"
#include "Engine/StaticMeshActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

static const TCHAR* GFlavor[] = {
	TEXT("Thornlings in the ferns"), TEXT("Mud is moving"), TEXT("Masks in the canopy"), TEXT("The ground is humming"), TEXT(""),
	TEXT("Night monkeys went quiet"), TEXT("Something big is breathing"), TEXT("Roots are cracking"), TEXT("Hold the steps"), TEXT("")
};

AJungleGameMode::AJungleGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ALionelCharacter::StaticClass();
	PlayerControllerClass = ALionelPlayerController::StaticClass();
	HUDClass = AJungleHUD::StaticClass();
}

AJungleGameMode* AJungleGameMode::Get(const UObject* WorldContext)
{
	return Cast<AJungleGameMode>(UGameplayStatics::GetGameMode(WorldContext));
}

ALionelCharacter* AJungleGameMode::Player() const
{
	return Cast<ALionelCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

int32 AJungleGameMode::AliveCount() const
{
	int32 N = 0;
	for (const AJungleEnemy* E : Enemies) { if (E && E->bAlive) { ++N; } }
	return N;
}

void AJungleGameMode::StartPlay()
{
	LoadSettings();
	BuildWorld();
	Super::StartPlay();
}

void AJungleGameMode::LoadSettings()
{
	if (!GConfig) { return; }
	GConfig->GetInt(TEXT("Lionel"), TEXT("Best"), Best, GGameUserSettingsIni);
	GConfig->GetFloat(TEXT("Lionel"), TEXT("Sensitivity"), Sensitivity, GGameUserSettingsIni);
	GConfig->GetBool(TEXT("Lionel"), TEXT("Sound"), bSoundOn, GGameUserSettingsIni);
	GConfig->GetBool(TEXT("Lionel"), TEXT("Music"), bMusicOn, GGameUserSettingsIni);
	Sensitivity = FMath::Clamp(Sensitivity, 0.4f, 2.2f);
}

void AJungleGameMode::SaveSettings()
{
	if (!GConfig) { return; }
	GConfig->SetInt(TEXT("Lionel"), TEXT("Best"), Best, GGameUserSettingsIni);
	GConfig->SetFloat(TEXT("Lionel"), TEXT("Sensitivity"), Sensitivity, GGameUserSettingsIni);
	GConfig->SetBool(TEXT("Lionel"), TEXT("Sound"), bSoundOn, GGameUserSettingsIni);
	GConfig->SetBool(TEXT("Lionel"), TEXT("Music"), bMusicOn, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

// ============================================================================================
// World building
// ============================================================================================

void AJungleGameMode::BuildWorld()
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	JungleWorld = GetWorld()->SpawnActor<AJungleWorld>(AJungleWorld::StaticClass(), FTransform::Identity, SP);
	if (JungleWorld) { JungleWorld->Build(this); }
	FX = GetWorld()->SpawnActor<AJungleFX>(AJungleFX::StaticClass(), FTransform::Identity, SP);
	Audio = GetWorld()->SpawnActor<AJungleAudio>(AJungleAudio::StaticClass(), FTransform::Identity, SP);
}

// ============================================================================================
// Real-time physics bodies: props, enemy debris, shell casings
// ============================================================================================

AStaticMeshActor* AJungleGameMode::SpawnProp(UStaticMesh* Mesh, UMaterialInterface* Mat, const FVector& Scale, const FVector& LocCm,
	const FRotator& Rot, float MassKg, float LifeSpan, bool bDebris)
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), LocCm, Rot, SP);
	if (!A) { return nullptr; }
	A->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* C = A->GetStaticMeshComponent();
	C->SetStaticMesh(Mesh);
	C->SetMaterial(0, Mat);
	A->SetActorScale3D(Scale);
	C->SetCollisionProfileName(TEXT("PhysicsActor"));
	if (bDebris) { C->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); }
	C->SetSimulatePhysics(true);
	C->SetMassOverrideInKg(NAME_None, MassKg, true);
	C->SetLinearDamping(0.15f);
	C->SetAngularDamping(0.45f);
	if (LifeSpan > 0.f) { A->SetLifeSpan(LifeSpan); }
	if (!bDebris) { Props.Add(A); }
	return A;
}

AStaticMeshActor* AJungleGameMode::SpawnProp(EShape Shape, const FLinearColor& Color, const FVector& Scale, const FVector& LocCm,
	const FRotator& Rot, float MassKg, float LifeSpan, bool bDebris)
{
	UJungleAssets* A = UJungleAssets::Get(this);
	return SpawnProp(A->Mesh(Shape), A->Mat(Color), Scale, LocCm, Rot, MassKg, LifeSpan, bDebris);
}

void AJungleGameMode::PushProps(const FVector& FromCm, float RadiusCm, float Speed)
{
	for (const TWeakObjectPtr<AStaticMeshActor>& W : Props)
	{
		AStaticMeshActor* A = W.Get();
		if (!A) { continue; }
		const FVector D = A->GetActorLocation() - FromCm;
		if (FMath::Abs(D.Z) > 250.f || D.SizeSquared() > RadiusCm * RadiusCm) { continue; }
		const FVector Dir = FVector(D.X, D.Y, 0.f).GetSafeNormal() + FVector(0.f, 0.f, 0.35f);
		A->GetStaticMeshComponent()->AddImpulse(Dir * Speed, NAME_None, true);
	}
}

// ============================================================================================
// Match flow
// ============================================================================================

void AJungleGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;

	BannerTime = FMath::Max(0.f, BannerTime - Dt);
	PopTime = FMath::Max(0.f, PopTime - Dt);
	HitTime = FMath::Max(0.f, HitTime - Dt);
	FreshTime = FMath::Max(0.f, FreshTime - Dt);
	HurtFlash = FMath::Max(0.f, HurtFlash - Dt * 1.4f);

	if (State == EJState::Playing)
	{
		UpdateWaves(Dt);
	}
}

void AJungleGameMode::StartGame()
{
	if (UGameplayStatics::IsGamePaused(this))
	{
		UGameplayStatics::SetGamePaused(this, false);
	}
	for (AJungleEnemy* E : Enemies) { if (E) { E->Destroy(); } }
	Enemies.Empty();
	Boss = nullptr;
	for (AJunglePickup* P : Pickups) { if (P) { P->Destroy(); } }
	Pickups.Empty();
	for (TActorIterator<AJungleProjectile> It(GetWorld()); It; ++It) { It->Destroy(); }

	Score = 0;
	Kills = 0;
	Wave = 0;
	bFight = false;
	WaveTimer = 2.2f;
	Queue.Empty();
	HurtFlash = 0.f;

	if (ALionelCharacter* P = Player()) { P->ResetForGame(); }
	State = EJState::Playing;
	Banner(TEXT("HOLD THE TEMPLE"), TEXT("They come from every side"), 2.2f);
}

void AJungleGameMode::TogglePause()
{
	if (State == EJState::Playing)
	{
		State = EJState::Paused;
		UGameplayStatics::SetGamePaused(this, true);
	}
	else if (State == EJState::Paused)
	{
		State = EJState::Playing;
		UGameplayStatics::SetGamePaused(this, false);
	}
}

void AJungleGameMode::GameOver()
{
	if (State != EJState::Playing) { return; }
	State = EJState::Over;
	Sfx(ESfx::Death);
	if (Score > Best)
	{
		Best = Score;
		SaveSettings();
	}
}

void AJungleGameMode::Banner(const FString& Title, const FString& Sub, float Duration)
{
	BannerTitle = Title;
	BannerSub = Sub;
	BannerTime = Duration;
}

void AJungleGameMode::Pop(const FString& Text)
{
	PopText = Text;
	PopTime = 0.8f;
}

void AJungleGameMode::HitMarker(bool bKill)
{
	bHitKill = bKill;
	HitTime = bKill ? 0.22f : 0.09f;
}

void AJungleGameMode::Sfx(ESfx S)
{
	if (Audio) { Audio->Play(S); }
}

void AJungleGameMode::StartWave(int32 N)
{
	Wave = N;
	Queue.Empty();
	if (N % 5 == 0)
	{
		Queue.Add(EEnemyType::Boss);
		for (int32 i = 0; i < 4 + N; ++i) { Queue.Add(FMath::FRand() < 0.4f ? EEnemyType::Mask : EEnemyType::Thorn); }
		Banner(TEXT("BOSS WAVE"), TEXT("The Idol Colossus awakens"), 2.6f);
		Sfx(ESfx::Roar);
	}
	else
	{
		const int32 Count = 5 + N * 3;
		for (int32 i = 0; i < Count; ++i)
		{
			const float R = FMath::FRand();
			Queue.Add(N >= 3 && R < 0.22f ? EEnemyType::Mask : (N >= 2 && R < 0.42f ? EEnemyType::Brute : EEnemyType::Thorn));
		}
		const FString Flavor = GFlavor[(N - 1) % UE_ARRAY_COUNT(GFlavor)];
		Banner(FString::Printf(TEXT("WAVE %d"), N), Flavor.IsEmpty() ? TEXT("Keep the temple") : Flavor, 2.2f);
	}
	SpawnT = 1.f;
	bFight = true;
}

void AJungleGameMode::WaveClear()
{
	bFight = false;
	WaveTimer = 5.5f;
	ALionelCharacter* P = Player();
	if (!P) { return; }
	P->Health = FMath::Min(100.f, P->Health + 20.f);
	P->Resupply(2.f);
	const int32 Unlock = Wave + 2;
	if (Unlock < NumWeapons && !P->Gun[Unlock].bOwned)
	{
		P->Gun[Unlock].bOwned = true;
		P->Select(Unlock);
		FreshSlot = Unlock;
		FreshTime = 3.f;
		Sfx(ESfx::Unlock);
		Banner(FString::Printf(TEXT("WAVE %d CLEARED"), Wave), FString(TEXT("New weapon: ")) + GWeapons[Unlock].Full, 3.f);
	}
	else
	{
		Banner(FString::Printf(TEXT("WAVE %d CLEARED"), Wave), TEXT("+20 health, ammo resupplied"), 2.6f);
	}
}

void AJungleGameMode::UpdateWaves(float Dt)
{
	if (bFight)
	{
		const int32 Alive = AliveCount();
		SpawnT -= Dt;
		if (Queue.Num() > 0 && SpawnT <= 0.f && Alive < 10 + FMath::Min(Wave, 8))
		{
			const EEnemyType T = Queue[0];
			Queue.RemoveAt(0);
			SpawnEnemy(T);
			SpawnT = JW::Rand(0.5f, 1.4f) * FMath::Max(0.45f, 1.f - Wave * 0.04f);
		}
		if (Queue.Num() == 0 && Alive == 0)
		{
			WaveClear();
		}
	}
	else
	{
		WaveTimer -= Dt;
		if (WaveTimer <= 0.f) { StartWave(Wave + 1); }
	}
}

void AJungleGameMode::SpawnEnemy(EEnemyType Type, const FVector2D* Near)
{
	ALionelCharacter* P = Player();
	if (!P) { return; }
	const FVector PL = P->GetActorLocation() / JW::M;
	FVector2D At;
	if (Near)
	{
		At = *Near + FVector2D(JW::Rand(-4, 4), JW::Rand(-4, 4));
	}
	else
	{
		const float A = JW::Rand(0, 2 * PI), R = JW::Rand(36.f, 56.f);
		At = FVector2D(PL.X + FMath::Cos(A) * R, PL.Y + FMath::Sin(A) * R);
	}
	const float Len = At.Size();
	if (Len > JW::Arena - 4.f) { At *= (JW::Arena - 4.f) / Len; }

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Loc(At.X * JW::M, At.Y * JW::M, JW::FloorH(At.X, At.Y) * JW::M);
	AJungleEnemy* E = GetWorld()->SpawnActor<AJungleEnemy>(AJungleEnemy::StaticClass(), Loc, FRotator::ZeroRotator, SP);
	if (!E) { return; }
	E->Init(Type, FMath::Max(1, Wave));
	Enemies.Add(E);
	if (Type == EEnemyType::Boss) { Boss = E; }
	if (FX) { FX->Burst(Loc + FVector(0, 0, 100), 10, JW::Hex(0x2a3a1a), 400.f, 25.f, 0.6f, 600.f, 100.f); }
}

void AJungleGameMode::EnemyKilled(AJungleEnemy* E)
{
	++Kills;
	Score += E->Value;
	Pop(FString::Printf(TEXT("+%d"), E->Value));
	HitMarker(true);
	Sfx(ESfx::Kill);
	const FVector C = E->Center();
	if (FX) { FX->Burst(C, E->bBoss ? 80 : 18, E->Col, E->bBoss ? 1200.f : 700.f, E->bBoss ? 50.f : 22.f, 0.9f, 1400.f, 200.f); }

	const FVector2D At(E->GetActorLocation().X / JW::M, E->GetActorLocation().Y / JW::M);
	if (E->bBoss)
	{
		Explode(C, 0.f, 0.f, true);
		Boss = nullptr;
		Banner(TEXT("COLOSSUS DOWN"), TEXT("+5,000"), 2.4f);
		for (int32 i = 0; i < 4; ++i) { Drop(At + FVector2D(JW::Rand(-3, 3), JW::Rand(-3, 3)), i % 2 == 1); }
	}
	else
	{
		const float Chance = E->Type == EEnemyType::Brute ? 0.65f : (E->Type == EEnemyType::Mask ? 0.4f : 0.22f);
		if (FMath::FRand() < Chance) { Drop(At, FMath::FRand() < 0.62f); }
	}
}

void AJungleGameMode::Drop(const FVector2D& AtM, bool bAmmo)
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Loc(AtM.X * JW::M, AtM.Y * JW::M, (JW::FloorH(AtM.X, AtM.Y) + 0.6f) * JW::M);
	if (AJunglePickup* P = GetWorld()->SpawnActor<AJunglePickup>(AJunglePickup::StaticClass(), Loc, FRotator::ZeroRotator, SP))
	{
		P->Init(bAmmo);
		Pickups.Add(P);
	}
}

void AJungleGameMode::Explode(const FVector& At, float RadiusM, float Damage, bool bBig)
{
	if (RadiusM > 0.f)
	{
		const TArray<AJungleEnemy*> Snapshot = Enemies;
		for (AJungleEnemy* E : Snapshot)
		{
			if (!E || !E->bAlive) { continue; }
			const FVector C = E->Center();
			const float D = FVector::Dist(C, At) / JW::M;
			if (D < RadiusM + E->Hr)
			{
				const float F = 1.f - 0.55f * FMath::Clamp(D / RadiusM, 0.f, 1.f);
				FVector Dir = C - At;
				Dir.Z = 0;
				E->TakeHit(Damage * F, Dir.GetSafeNormal(), 6.f);
			}
		}
	}
	if (FX)
	{
		FX->Burst(At, bBig ? 60 : 30, JW::Hex(0xffa030), bBig ? 1400.f : 1000.f, bBig ? 50.f : 32.f, 0.6f, 600.f, 300.f);
		FX->Burst(At, bBig ? 30 : 14, JW::Hex(0x3a3528), bBig ? 500.f : 350.f, bBig ? 90.f : 60.f, 1.4f, -150.f, 150.f);
		FX->Burst(At, 10, JW::Hex(0x5b3a1e), 900.f, 14.f, 1.f, 1600.f, 400.f);
		FX->Flash(At + FVector(0, 0, 100), JW::Hex(0xffb050), bBig ? 60000.f : 40000.f, 0.35f);
	}
	// Shove every physics body in the blast: props, debris, casings.
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* C = It->GetStaticMeshComponent();
		if (C && C->IsSimulatingPhysics())
		{
			C->AddRadialImpulse(At, FMath::Max(RadiusM, 4.f) * JW::M * 1.6f, bBig ? 2200.f : 1500.f, ERadialImpulseFalloff::RIF_Linear, true);
		}
	}
	if (ALionelCharacter* P = Player())
	{
		const float D = FVector::Dist2D(P->GetActorLocation(), At) / JW::M;
		P->AddShake(FMath::Max(0.f, 0.9f - D / 25.f));
	}
	Sfx(ESfx::Boom);
}

void AJungleGameMode::Collide(FVector2D& P, float R, float Z) const
{
	for (const FObstacle& O : Obstacles)
	{
		const float Dx = P.X - O.X, Dy = P.Y - O.Y, RR = O.R + R, D2 = Dx * Dx + Dy * Dy;
		if (D2 < RR * RR && Z < O.H - 0.2f)
		{
			const float D = FMath::Max(FMath::Sqrt(D2), 0.001f);
			P.X = O.X + Dx / D * RR;
			P.Y = O.Y + Dy / D * RR;
		}
	}
	const float Len = P.Size();
	if (Len > JW::Arena) { P *= JW::Arena / Len; }
}
