// Minimal stand-ins for the Unreal Engine 5 API used by Lily Leap, for syntax/type checking only.
#pragma once
#include <cstdint>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <utility>
typedef int32_t int32; typedef uint32_t uint32; typedef uint8_t uint8; typedef int64_t int64; typedef wchar_t TCHAR;
#define TEXT(x) L##x
#define UCLASS(...)
#define USTRUCT(...)
#define UPROPERTY(...)
#define UFUNCTION(...)
#define LILYLEAP_API
#define GENERATED_BODY() public: static class UClass* StaticClass() { return nullptr; } private:
#define IMPLEMENT_PRIMARY_GAME_MODULE(...)
#define check(x) ((void)(x))
#define UE_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define PI 3.14159265358979323846f
#define PLATFORM_IOS 0
#define PLATFORM_ANDROID 0

template<typename T> struct TArray {
  std::vector<T> V;
  TArray() {}
  TArray(std::initializer_list<T> L) : V(L) {}
  int32 Num() const { return (int32)V.size(); }
  int32 Add(const T& X) { V.push_back(X); return Num() - 1; }
  T& AddDefaulted_GetRef() { V.emplace_back(); return V.back(); }
  void Reset() { V.clear(); }
  void Reserve(int32 N) { V.reserve(N); }
  void RemoveAt(int32 I) { V.erase(V.begin() + I); }
  void RemoveAtSwap(int32 I) { V[I] = V.back(); V.pop_back(); }
  template<typename P> int32 RemoveAll(P Pred) { auto n = V.size(); V.erase(std::remove_if(V.begin(), V.end(), Pred), V.end()); return int32(n - V.size()); }
  template<typename P> int32 RemoveAllSwap(P Pred) { return RemoveAll(Pred); }
  bool IsValidIndex(int32 I) const { return I >= 0 && I < Num(); }
  T& Last() { return V.back(); }
  void Swap(int32 A, int32 B) { std::swap(V[A], V[B]); }
  T& operator[](int32 I) { return V[I]; }
  const T& operator[](int32 I) const { return V[I]; }
  auto begin() { return V.begin(); } auto end() { return V.end(); }
  auto begin() const { return V.begin(); } auto end() const { return V.end(); }
};
template<typename K, typename Vv> struct TPair { K Key; Vv Value; };
template<typename K, typename Vv> struct TMap {
  std::vector<TPair<K, Vv>> V;
  void Add(const K& k, const Vv& v) { for (auto& p : V) if (p.Key == k) { p.Value = v; return; } V.push_back({k, v}); }
  Vv* Find(const K& k) { for (auto& p : V) if (p.Key == k) return &p.Value; return nullptr; }
  int32 Remove(const K& k) { auto n = V.size(); V.erase(std::remove_if(V.begin(), V.end(), [&](auto& p) { return p.Key == k; }), V.end()); return int32(n - V.size()); }
  auto begin() const { return V.begin(); } auto end() const { return V.end(); }
};
template<typename F> using TUniqueFunction = std::function<F>;

struct FString {
  std::wstring S;
  FString() {} FString(const TCHAR* s) : S(s) {}
  static FString Printf(const TCHAR* Fmt, ...) { return FString(Fmt); }
  const TCHAR* operator*() const { return S.c_str(); }
  bool IsEmpty() const { return S.empty(); }
  FString ToLower() const { return *this; }
};
enum EName : int;
struct FName {
  std::wstring S;
  FName() {} FName(const TCHAR* s) : S(s) {} FName(enum EName) {}
  bool operator==(const FName& o) const { return S == o.S; }
  bool operator!=(const FName& o) const { return S != o.S; }
  bool operator==(const TCHAR* o) const { return S == o; }
};
enum EName : int { NAME_None };
inline bool operator==(const FName& a, EName) { return a.S.empty(); }
inline bool operator!=(const FName& a, EName) { return !a.S.empty(); }

struct FVector {
  double X = 0, Y = 0, Z = 0;
  FVector() {} FVector(double x, double y, double z) : X(x), Y(y), Z(z) {} explicit FVector(double s) : X(s), Y(s), Z(s) {}
  static const FVector ZeroVector;
  FVector operator+(const FVector& o) const { return {X + o.X, Y + o.Y, Z + o.Z}; }
  FVector operator-(const FVector& o) const { return {X - o.X, Y - o.Y, Z - o.Z}; }
  FVector operator*(double s) const { return {X * s, Y * s, Z * s}; }
  FVector operator/(double s) const { return {X / s, Y / s, Z / s}; }
  FVector& operator+=(const FVector& o) { X += o.X; Y += o.Y; Z += o.Z; return *this; }
  FVector& operator*=(double s) { X *= s; Y *= s; Z *= s; return *this; }
};
inline const FVector FVector::ZeroVector{};
struct FVector2D {
  double X = 0, Y = 0;
  FVector2D() {} FVector2D(double x, double y) : X(x), Y(y) {}
  static const FVector2D ZeroVector;
  static double Distance(const FVector2D& a, const FVector2D& b) { return std::hypot(a.X - b.X, a.Y - b.Y); }
  double Size() const { return std::hypot(X, Y); }
  FVector2D GetSafeNormal() const { double s = Size(); return s > 0 ? FVector2D(X / s, Y / s) : FVector2D(); }
  FVector2D operator+(const FVector2D& o) const { return {X + o.X, Y + o.Y}; }
  FVector2D operator-(const FVector2D& o) const { return {X - o.X, Y - o.Y}; }
  FVector2D operator-() const { return {-X, -Y}; }
  FVector2D operator*(double s) const { return {X * s, Y * s}; }
};
inline const FVector2D FVector2D::ZeroVector{};
struct FVector4 { double X = 0, Y = 0, Z = 0, W = 0; FVector4() {} FVector4(double x, double y, double z, double w) : X(x), Y(y), Z(z), W(w) {} };
struct FRotator {
  double Pitch = 0, Yaw = 0, Roll = 0;
  FRotator() {} FRotator(double p, double y, double r) : Pitch(p), Yaw(y), Roll(r) {}
  static const FRotator ZeroRotator;
  FVector Vector() const { return FVector(std::cos(Yaw), std::sin(Yaw), 0); }
};
inline const FRotator FRotator::ZeroRotator{};
struct FTransform { FTransform() {} explicit FTransform(const FRotator&) {} };
struct FMatrixMock { FRotator Rotator() const { return {}; } };
struct FRotationMatrix { static FMatrixMock MakeFromZ(const FVector&) { return {}; } };
struct FColor { uint8 R, G, B, A; FColor(uint8 r, uint8 g, uint8 b, uint8 a = 255) : R(r), G(g), B(b), A(a) {} };
struct FLinearColor {
  float R = 0, G = 0, B = 0, A = 1;
  FLinearColor() {} FLinearColor(float r, float g, float b, float a = 1.f) : R(r), G(g), B(b), A(a) {}
  explicit FLinearColor(const FColor& c) : R(c.R / 255.f), G(c.G / 255.f), B(c.B / 255.f), A(1) {}
  static const FLinearColor White, Black;
  FLinearColor operator*(float s) const { return {R * s, G * s, B * s, A * s}; }
};
inline const FLinearColor FLinearColor::White{1, 1, 1, 1};
inline const FLinearColor FLinearColor::Black{0, 0, 0, 1};

struct FMath {
  template<typename T> static T Min(T a, T b) { return a < b ? a : b; }
  template<typename T> static T Max(T a, T b) { return a > b ? a : b; }
  static double Min(float a, double b) { return a < b ? a : b; } static double Min(double a, float b) { return a < b ? a : b; }
  static double Max(float a, double b) { return a > b ? a : b; } static double Max(double a, float b) { return a > b ? a : b; }
  template<typename T> static T Clamp(T v, T a, T b) { return v < a ? a : v > b ? b : v; }
  template<typename T> static T Abs(T v) { return v < 0 ? -v : v; }
  template<typename T> static T Sign(T v) { return v > 0 ? T(1) : v < 0 ? T(-1) : T(0); }
  static float Sin(float v) { return std::sin(v); } static float Cos(float v) { return std::cos(v); } static float Tan(float v) { return std::tan(v); }
  static double Sin(double v) { return std::sin(v); } static double Cos(double v) { return std::cos(v); }
  static float Atan2(float y, float x) { return std::atan2(y, x); } static double Atan2(double y, double x) { return std::atan2(y, x); }
  static float Sqrt(float v) { return std::sqrt(v); } static float Pow(float a, float b) { return std::pow(a, b); }
  static float Exp(float v) { return std::exp(v); } static float Loge(float v) { return std::log(v); }
  static int32 FloorToInt(float v) { return (int32)std::floor(v); } static float FloorToFloat(float v) { return std::floor(v); }
  static int32 CeilToInt(float v) { return (int32)std::ceil(v); } static int32 RoundToInt(float v) { return (int32)std::round(v); }
  static float DegreesToRadians(float v) { return v * PI / 180.f; }
  static float FRandRange(float a, float b) { return a + (b - a) * (std::rand() / float(RAND_MAX)); }
};
struct FRandomStream { mutable uint32 Seed; explicit FRandomStream(int32 s) : Seed(uint32(s) * 2654435761u + 1u) {}
  float FRand() const { Seed = Seed * 1664525u + 1013904223u; return (Seed >> 8) / 16777216.f; }
  float FRandRange(float a, float b) const { return a + (b - a) * FRand(); }
  int32 RandRange(int32 a, int32 b) const { return a + int32(FRand() * (b - a + 1)) > b ? b : a + int32(FRand() * (b - a + 1)); } };

// ---- object model ----
class UClass;
struct FObjectInitializer {};
extern class UWorld* GWorldMock;
class UObject { public: virtual ~UObject() {} template<typename T> T* CreateDefaultSubobject(const TCHAR*) { if constexpr (std::is_constructible_v<T, const FObjectInitializer&>) return new T(FObjectInitializer()); else return new T(); } class UWorld* GetWorld() const { return GWorldMock; } };
template<typename T> struct TObjectPtr {
  T* P = nullptr;
  TObjectPtr() {} TObjectPtr(T* p) : P(p) {}
  operator T*() const { return P; } T* operator->() const { return P; } T* Get() const { return P; }
  explicit operator bool() const { return P != nullptr; }
};
template<typename T> T* NewObject(UObject* Outer) { return new T(); }
template<typename T> T* LoadObject(UObject* Outer, const TCHAR* Path) { return nullptr; }
template<typename T, typename U> T* Cast(U* p) { return dynamic_cast<T*>(p); }
template<typename T, typename U> const T* Cast(const U* p) { return dynamic_cast<const T*>(p); }
template<typename T, typename U> T* Cast(const TObjectPtr<U>& p) { return dynamic_cast<T*>(p.P); }

enum class EComponentMobility { Static, Stationary, Movable };
namespace ECollisionEnabled { enum Type { NoCollision, QueryAndPhysics }; }
enum ETickingGroup { TG_PrePhysics };
struct FActorTickFunction { bool bCanEverTick = false; ETickingGroup TickGroup = TG_PrePhysics; };

class UActorComponent : public UObject { public: bool bAutoActivate = true; void RegisterComponent() {} };
class USceneComponent : public UActorComponent {
 public:
  void SetMobility(EComponentMobility) {}
  void SetupAttachment(USceneComponent*) {}
  void SetRelativeLocation(const FVector&) {}
  void SetRelativeRotation(const FRotator&) {}
  void SetRelativeLocationAndRotation(const FVector&, const FRotator&) {}
  void SetRelativeScale3D(const FVector&) {}
  void SetWorldLocation(const FVector&) {}
  void SetWorldScale3D(const FVector&) {}
  void SetVisibility(bool) {}
};
class UMaterialInterface : public UObject {};
class UMaterialInstanceDynamic : public UMaterialInterface { public: static UMaterialInstanceDynamic* Create(UMaterialInterface*, UObject*) { return new UMaterialInstanceDynamic(); } void SetVectorParameterValue(FName, const FLinearColor&) {} };
class UStaticMesh : public UObject {};
class UPrimitiveComponent : public USceneComponent { public: void SetCollisionEnabled(ECollisionEnabled::Type) {} void SetGenerateOverlapEvents(bool) {} void SetCastShadow(bool) {} };
class UStaticMeshComponent : public UPrimitiveComponent { public: UMaterialInterface* Mat = nullptr; bool SetStaticMesh(UStaticMesh*) { return true; } void SetMaterial(int32, UMaterialInterface* m) { Mat = m; } UMaterialInterface* GetMaterial(int32) const { return Mat; } };

class AActor : public UObject {
 public:
  FActorTickFunction PrimaryActorTick;
  TObjectPtr<USceneComponent> RootComponent;
  virtual void BeginPlay() {}
  virtual void Tick(float) {}
  void AddInstanceComponent(UActorComponent*) {}
  FVector Loc; bool bHiddenMock = false;
  bool SetActorLocation(const FVector& l) { Loc = l; return true; }
  bool SetActorRotation(const FRotator&) { return true; }
  void SetActorScale3D(const FVector&) {}
  void SetActorHiddenInGame(bool b) { bHiddenMock = b; }
  FVector GetActorLocation() const { return Loc; }
  void Destroy() {}
  void FinishSpawning(const FTransform&) {}
};
struct FActorSpawnParameters {};
class AGameModeBase;
class UWorld : public UObject {
 public:
  template<typename T> T* SpawnActor() { return new T(); }
  template<typename T> T* SpawnActor(const FVector& l, const FRotator&, const FActorSpawnParameters& = FActorSpawnParameters()) { T* a = new T(); a->SetActorLocation(l); return a; }
  template<typename T> T* SpawnActorDeferred(UClass*, const FTransform&) { return new T(); }
  AGameModeBase* Mode = nullptr;
  template<typename T> T* GetAuthGameMode() const { return dynamic_cast<T*>((AActor*)Mode); }
};

// ---- lights, camera, post ----
class ULightComponentBase : public USceneComponent { public: void SetCastShadows(bool) {} };
class ULightComponent : public ULightComponentBase { public: void SetIntensity(float) {} void SetLightColor(FLinearColor, bool bSRGB = true) {} };
class UDirectionalLightComponent : public ULightComponent {};
class ALight : public AActor { public: ULightComponent LC; ULightComponent* GetLightComponent() const { return const_cast<ULightComponent*>(&LC); } };
class ADirectionalLight : public ALight { GENERATED_BODY() };
class UCameraComponent : public USceneComponent { public: void SetFieldOfView(float) {} void SetConstraintAspectRatio(bool) {} };
class ACameraActor : public AActor { public: UCameraComponent CC; UCameraComponent* GetCameraComponent() const { return const_cast<UCameraComponent*>(&CC); } };
enum class EAutoExposureMethod { AEM_Histogram, AEM_Basic, AEM_Manual };
struct FPostProcessSettings {
  uint32 bOverride_AutoExposureMethod : 1; uint32 bOverride_AutoExposureApplyPhysicalCameraExposure : 1; uint32 bOverride_AutoExposureBias : 1;
  uint32 bOverride_BloomIntensity : 1; uint32 bOverride_VignetteIntensity : 1; uint32 AutoExposureApplyPhysicalCameraExposure : 1;
  EAutoExposureMethod AutoExposureMethod; float AutoExposureBias; float BloomIntensity; float VignetteIntensity;
};
class APostProcessVolume : public AActor { public: FPostProcessSettings Settings; uint32 bUnbound : 1; };

// ---- save, game mode, controller, input, hud ----
class USaveGame : public UObject {};
class AController : public AActor {};
class AHUD;
class UTouchInterface;
namespace ETouchIndex { enum Type { Touch1, Touch2 }; }
enum EInputEvent { IE_Pressed, IE_Released, IE_Repeat };
struct FKey { int id; };
struct EKeys { static const FKey Left, Right, A, D, SpaceBar, Up, W, Z, X, J, K, LeftShift, RightShift, Enter, Escape, P; };
class UInputComponent : public UActorComponent {
 public:
  template<typename C> void BindKey(const FKey, EInputEvent, C*, void (C::*)()) {}
  template<typename C> void BindTouch(EInputEvent, C*, void (C::*)(ETouchIndex::Type, FVector)) {}
};
struct FInputModeGameOnly {};
class APlayerController : public AController {
 public:
  TObjectPtr<UInputComponent> InputComponent;
  bool bShowMouseCursor = false, bEnableTouchEvents = true, bEnableClickEvents = false, bAutoManageActiveCameraTarget = true;
  virtual void SetupInputComponent() {}
  void ActivateTouchInterface(UTouchInterface*) {}
  void SetInputMode(const FInputModeGameOnly&) {}
  AHUD* GetHUD() const { return nullptr; }
  AActor* GetViewTarget() const { return nullptr; }
  void SetViewTarget(AActor*) {}
};
class AGameModeBase : public AActor {
 public:
  UClass* DefaultPawnClass = nullptr; UClass* PlayerControllerClass = nullptr; UClass* HUDClass = nullptr;
  virtual void StartPlay() {}
  virtual void RestartPlayer(AController*) {}
};
class UFont : public UObject {};
class UCanvas : public UObject { public: float ClipX = 0, ClipY = 0; };
class AHUD : public AActor {
 public:
  TObjectPtr<UCanvas> Canvas;
  TObjectPtr<APlayerController> PlayerOwner;
  virtual void DrawHUD() {}
  void DrawRect(FLinearColor, float, float, float, float) {}
  void DrawLine(float, float, float, float, FLinearColor, float = 0.f) {}
  void DrawText(const FString&, FLinearColor, float, float, UFont* = nullptr, float = 1.f, bool = false) {}
  void GetTextSize(const FString&, float&, float&, UFont* = nullptr, float = 1.f) const {}
  FVector Project(FVector, bool = true) const { return {}; }
};
class UEngine : public UObject { public: UFont* GetLargeFont() { return nullptr; } };
extern UEngine* GEngine;
struct UGameplayStatics {
  static APlayerController* GetPlayerController(const UObject*, int32) { return nullptr; }
  static USaveGame* LoadGameFromSlot(const FString&, int32) { return nullptr; }
  static USaveGame* CreateSaveGameObject(UClass*) { return nullptr; }
  static bool SaveGameToSlot(USaveGame*, const FString&, int32) { return true; }
};

// ---- audio ----
class USynthComponent : public USceneComponent {
 public:
  USynthComponent(const FObjectInitializer&) {}
  void Start() {}
 protected:
  int32 NumChannels = 1;
  virtual bool Init(int32& SampleRate) { return true; }
  virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) = 0;
  void SynthCommand(TUniqueFunction<void()> Command) {}
};
