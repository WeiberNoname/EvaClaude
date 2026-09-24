#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SaveGame.h"
#include "EvaRules.h"
#include "EvaGame.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UCapsuleComponent;
class ACameraActor;
class UStaticMesh;
class UMaterialInstanceDynamic;
class SEvaComms;
class AEvaWingman;
class UInstancedStaticMeshComponent;
class UAudioComponent;
class USoundAttenuation;
class ADirectionalLight;

UCLASS()
class UEvaWorldSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 SurveyMask = 0;
    UPROPERTY() int32 Contracts = 0;
};

UCLASS()
class AEvaPawn : public APawn
{
    GENERATED_BODY()
public:
    AEvaPawn();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    UPROPERTY() UCapsuleComponent* Capsule;
    UPROPERTY() USpringArmComponent* Boom;
    UPROPERTY() UCameraComponent* Camera;
    UPROPERTY() TArray<UStaticMeshComponent*> Limbs;
    UPROPERTY() TArray<USceneComponent*> Knees;
    UPROPERTY() TArray<UStaticMeshComponent*> BodyMeshes;
    UPROPERTY() USceneComponent* KnifeRoot;
    UPROPERTY() USceneComponent* CannonRoot;
    UPROPERTY() UStaticMeshComponent* ShoulderHatch;
    FEvaLoadout Loadout;
    bool bHuman = false;
    float DrawTime = 0.f;
    float PickupTime = 0.f;
    FVector PickupStart;
    float CameraYaw = 90.f;
    float CameraPitch = -18.f;
    float WalkTime = 0.f;
    float DodgeTime = 0.f;
    float DodgeCooldown = 0.f;
    float AttackTime = 0.f;
    FVector DodgeDirection = FVector::ForwardVector;
    bool bGuard = false;
    bool bLock = true;
    void Melee();
    void StopFire();
    void AimPressed();
    void AimReleased();
    void ReloadWeapon();
    bool bFireHeld = false;
    bool bAiming = false;
    float ReloadTime = 0;
    float RecoilPitch = 0;
    float HitMarkerTime = 0;
    FVector MoveVelocity = FVector::ZeroVector;
    FEvaMotion Motion;
    void Jump();
    void TogglePerspective();
    void SwapShoulder();
    void ToggleHelp();
    void CallAsuka();
    void BuildCockpit();
    void UpdatePerspective();
    void MoveEva(FVector Direction,float Speed,float Dt);
    FVector ReadMovement() const;
    bool bFirstPerson=false;
    bool bGrounded=true;
    bool bHelpWasPaused=false;
    float ShoulderSide=1;
    float VerticalSpeed=0;
    float LandingKick=0;
    float DashBuffer=0;
    float OcclusionTimer=0;
    FVector TestMoveInput=FVector::ZeroVector;
    UPROPERTY() USceneComponent* EvaRig;
    UPROPERTY() USceneComponent* CockpitRoot;
    void Lance();
    void Dodge();
    void Connect();
    void ToggleLock();
    void Confirm();
    void PauseMission();
    void Restart();
    void DrawKnife();
    void EquipCannon();
    void ReleaseCable();
    void SkipToBattle();
    void ImpactCinema();
    void ReleaseCharge();
    void Overdrive();
    void AntiField();
    bool bCharging = false;
    float CannonCharge = 0.f;
    void SetHumanMode(bool bEnabled);
    void BuildEquipment();
    void BuildEvaVisuals();
    void OpenWorld();
    void ToggleMap();
    void NextWaypoint();
    void ReturnToTitle();
    void UpdateCameraOcclusion();
    UPROPERTY() TArray<UStaticMeshComponent*> CameraOccluders;
    float ComboTime = 0;
    int32 KnifeCombo = 0;
    bool bSprinting = false;
    void AnimateEquipment(float Dt);
};

struct FEvaBuilding
{
    UStaticMeshComponent* Mesh = nullptr;
    TArray<UStaticMeshComponent*> Windows;
    FVector Center;
    bool bDestroyed = false;
    USceneComponent* DistrictRoot = nullptr;
    UStaticMeshComponent* Rubble = nullptr;
    int32 DamageStage = 0;
    float CollapseTime = 0;
    FLinearColor FacadeColor;
};

struct FEvaEffect
{
    UStaticMeshComponent* Mesh = nullptr;
    float Remaining = 0.f;
    float Duration = 0.f;
    FVector Growth = FVector::ZeroVector;
};

enum class EEvaOrder : uint8 { Follow, Hold, Assault, Regroup };

UCLASS()
class AEvaWingman : public AActor
{
    GENERATED_BODY()
public:
    AEvaWingman();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    void Deploy(FVector Position);
    FString OrderName() const;
    UPROPERTY() UCapsuleComponent* Capsule;
    UPROPERTY() USceneComponent* Rig;
    UPROPERTY() USceneComponent* Rifle;
    UPROPERTY() TArray<UStaticMeshComponent*> Parts;
    UPROPERTY() TArray<UStaticMeshComponent*> Limbs;
    UPROPERTY() TArray<USceneComponent*> Knees;
    UPROPERTY() UStaticMeshComponent* Hatch;
    EEvaOrder Order=EEvaOrder::Follow;
    FVector HoldPosition=FVector::ZeroVector;
    FVector Velocity=FVector::ZeroVector;
    float Integrity=100, FireCooldown=0, WalkPhase=0, EvadeTime=0, PreviousTelegraph=0, RecoverTime=0;
    int32 ShotsFired=0;
    bool bDeployed=false;
};

UCLASS()
class AEvaGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AEvaGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    FEvaRules Rules;
    FEvaSystems Systems;
    bool bCableWarning = false;
    void UseOverdrive();
    void UseAntiField();
    void TickSystems(float Dt);
    EEvaChapter Chapter = EEvaChapter::Title;
    float ChapterTime = 0.f;
    int32 DialogueIndex = 0;
    bool bPhoneUsed = false;
    bool bStreetAttack = false;
    bool bBerserk = false;
    bool bChapterAuto = false;
    bool bChapterSmoke = false;
    int32 AutoStep = 0;
    bool bTestCharged = false;
    bool bTestCannon = false;
    bool bTestKnife = false;
    bool bTestBattleDamage = false;
    int32 ShotStage = -1;
    float ShotWait = 0.f;
    FVector PhonePosition = FVector(250,-7800,0);
    FVector CarPosition = FVector(-650,-6500,0);
    FVector DepotPosition = FVector(1800,-2600,0);
    FVector DepotApproach = FVector(1800,-3900,780);
    FVector HangarPosition = FVector(40000,0,0);
    FVector HospitalPosition = FVector(80000,0,0);
    TArray<FVector> Chargers;
    int32 ActiveCharger = 0;
    float DepotOpen = 0.f;
    bool bDepotOpening = false;
    bool bCannonTaken = false;
    UPROPERTY() ACameraActor* StoryCamera;
    UPROPERTY() USceneComponent* CarRoot;
    UPROPERTY() USceneComponent* HangarRoot;
    UPROPERTY() USceneComponent* HospitalRoot;
    UPROPERTY() USceneComponent* DepotGunRoot;
    UPROPERTY() UStaticMeshComponent* DepotLeft;
    UPROPERTY() UStaticMeshComponent* DepotRight;
    UPROPERTY() UStaticMeshComponent* DepotLift;
    UPROPERTY() TArray<UStaticMeshComponent*> ChargerLamps;
    UPROPERTY() TArray<UStaticMeshComponent*> AngelLimbs;
    bool bStarted = false;
    bool bPaused = false;
    bool bHelp = false;
    bool bEnded = false;
    bool bVictory = false;
    float MissionTime = 0.f;
    float MeleeCooldown = 0.f;
    float LanceCooldown = 0.f;
    float EnemyClock = 4.f;
    float Telegraph = 0.f;
    float VulnerableTime = 0.f;
    float HitFlash = 0.f;
    float NoticeTime = 0.f;
    int32 AttackCount = 0;
    int32 BuildingsLost = 0;
    FString Notice;
    FVector EnemyPosition = FVector(0, 4300, 1500);
    FVector ThreatPosition;
    const FVector Anchor = FVector(-1000, -6600, 100);
    UPROPERTY() USceneComponent* Scene;
    UPROPERTY() AActor* VisualWorld;
    bool bSmokeCaptured = false;
    UPROPERTY() USceneComponent* AngelRoot;
    UPROPERTY() UStaticMeshComponent* AngelCore;
    UPROPERTY() USceneComponent* EnemyShield;
    UPROPERTY() USceneComponent* PlayerShield;
    UPROPERTY() UStaticMeshComponent* ThreatRing;
    UPROPERTY() UStaticMeshComponent* Cable;
    UPROPERTY() UMaterialInterface* Surface;
    UPROPERTY() TArray<UStaticMeshComponent*> Meshes;
    UPROPERTY() TMap<FString,UStaticMesh*> MeshCache;
    UPROPERTY() TMap<FString,UMaterialInstanceDynamic*> MaterialCache;
    UPROPERTY() TArray<UStaticMeshComponent*> EffectPool;
    TArray<FEvaBuilding> Buildings;
    TArray<FEvaEffect> Effects;
    UStaticMeshComponent* Shape(const FString& Kind, FVector Position, FVector Scale, FLinearColor Color, float Glow = 0.f, USceneComponent* Parent = nullptr, bool Collision = false);
    void ApplySurface(UStaticMeshComponent* Mesh,const FString& Kind,FLinearColor Color,float Glow);
    void BuildUnit(USceneComponent* Root,bool Red,TArray<UStaticMeshComponent*>& Parts,TArray<UStaticMeshComponent*>& Limbs,TArray<USceneComponent*>& Knees,UStaticMeshComponent*& Hatch);
    UPROPERTY() AEvaWingman* Wingman;
    void DeployWingman();
    void ResolveWorldVictory();
    void ToggleComms();
    void CloseComms();
    FString ReplyAsuka(const FString& Message);
    void SubmitComms(const FString& Message);
    TSharedPtr<SEvaComms> CommsPanel;
    TArray<FString> CommsHistory;
    bool bCommsOpen=false, bCallWasPaused=false;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool bCompanionTest=false, bCompanionDone=false, bCompanionOK=true;
    int32 CompanionStep=0;
    float CompanionTime=0, CompanionMissionStamp=0;
    FVector CompanionStart=FVector::ZeroVector;
    void TickCompanionTest(float Dt);
    void BuildCity();
    void BuildCentralDistrict();
    void BuildCentralTower(FVector Base,float Height,int32 Style);
    void TickDistrict(float Dt);
    void SetDistrictLighting(bool Enabled);
    void DamageDistrictBuilding(FEvaBuilding& Building);
    void ResetDistrict();
    bool DistrictInteract();
    FString DistrictPrompt() const;
    UMaterialInstanceDynamic* DistrictMaterial(FLinearColor Color,int32 Type,float Glow=0);
    UStaticMeshComponent* CityBox(FVector Position,FVector Scale,FLinearColor Color,int32 Type=0,USceneComponent* Parent=nullptr,bool Collision=false,float Glow=0);
    UInstancedStaticMeshComponent* CityInstances(USceneComponent* Parent,FLinearColor Color,int32 Type=0,float Glow=0);
    void CitySign(USceneComponent* Parent,FVector Position,const FString& Text,float Size,FColor Color,FRotator Rotation=FRotator(0,-90,0));
    void DistrictSound(const TCHAR* Name,FVector Position,float Volume=1);
    void DistrictDust(FVector Position);
    UPROPERTY() UMaterialInterface* CitySurface;
    UPROPERTY() UMaterialInterface* DustSurface;
    UPROPERTY() TMap<FString,UMaterialInstanceDynamic*> CityMaterials;
    UPROPERTY() USoundAttenuation* CityAttenuation;
    UPROPERTY() UAudioComponent* CityAmbience;
    UPROPERTY() UAudioComponent* CityHum;
    UPROPERTY() ADirectionalLight* WorldSun;
    UPROPERTY() ADirectionalLight* WorldFill;
    UPROPERTY() USceneComponent* CityArmoryRoot;
    UPROPERTY() USceneComponent* CityGunRoot;
    UPROPERTY() UStaticMeshComponent* CityDoorLeft;
    UPROPERTY() UStaticMeshComponent* CityDoorRight;
    UPROPERTY() TArray<UStaticMeshComponent*> SupplyLamps;
    TArray<FVector> SupplyPositions;
    FVector CityArmoryPosition=FVector::ZeroVector;
    float CityArmoryOpen=0, CityArmoryStock=0, CityStepClock=0;
    bool bCityArmoryOpening=false;
    int32 SupplyMask=0;
    bool bDistrictTest=false, bDistrictDone=false, bDistrictOK=true;
    int32 DistrictTestStep=0, DistrictTestBuilding=INDEX_NONE;
    float DistrictTestTime=0, DistrictCollapseStamp=0;
    int32 DistrictMeshCount=0;
    double DistrictFrameStamp=0;
    TArray<double> DistrictFrames;
    void TickDistrictTest(float Dt);
    void StartOpenWorld();
    void BuildOpenWorld();
    void TickOpenWorld(float Dt);
    bool WorldInteract();
    FString WorldPrompt() const;
    void BeginWorldEncounter();
    void SaveWorldProgress();
    FVector WorldCenter = FVector(-150000,0,0);
    TArray<FVector> Districts;
    TArray<FString> DistrictNames;
    bool bOpenWorld = false;
    bool bWorldEncounter = false;
    bool bWorldMap = false;
    bool bWorldBuilt = false;
    bool bWorldTest = false;
    bool bWorldTestDone = false;
    int32 SurveyMask = 0;
    int32 CompletedContracts = 0;
    int32 SelectedDistrict = 0;
    int32 EncounterDistrict = 0;
    int32 WorldTestStep = 0;
    float WorldTestTime = 0;
    bool bWorldChecksOK = true;
    bool HasCombatTarget() const { return !bOpenWorld || bWorldEncounter; }
    void BuildAngel();
    void BuildShamshel();
    void SelectAngel(EEvaAngel Kind);
    void BuildRamiel();
    void TickRamiel(float Dt);
    void TickDynamicTest(float Dt);
    void ClearAngelHazards();
    bool bRamiel=false;
    UPROPERTY() USceneComponent* RamielRoot;
    UPROPERTY() USceneComponent* RamielField;
    UPROPERTY() UStaticMeshComponent* RamielCore;
    UPROPERTY() UStaticMeshComponent* RamielCrystal;
    UPROPERTY() TArray<UStaticMeshComponent*> RamielPetals;
    UPROPERTY() TArray<UStaticMeshComponent*> BeamWarnings;
    UPROPERTY() TArray<UStaticMeshComponent*> BeamShots;
    float BeamTime=0;
    FVector BeamAim;
    bool bDynamicTest=false;
    int32 DynamicStep=0;
    float DynamicTime=0;
    bool bDynamicOK=true;
    bool bDynamicDone=false;
    FVector DynamicStart;
    float DynamicPeak=0;
    float DynamicMissionStamp=0;
    UPROPERTY() UStaticMeshComponent* DynamicCover;
    TArray<double> FrameSamples;
    double LastFrameStamp=0;
    void TickShamshel(float Dt);
    void TickShooterTest(float Dt);
    FVector EnemyAimPoint() const;
    UPROPERTY() USceneComponent* SachielRoot;
    UPROPERTY() USceneComponent* SachielField;
    UPROPERTY() UStaticMeshComponent* SachielCore;
    UPROPERTY() USceneComponent* ShamshelRoot;
    UPROPERTY() USceneComponent* ShamshelField;
    UPROPERTY() UStaticMeshComponent* ShamshelCore;
    UPROPERTY() TArray<UStaticMeshComponent*> WhipSegments;
    UPROPERTY() TArray<UStaticMeshComponent*> WhipTelegraphs;
    bool bShamshel = false;
    float EnemyMaxHealth = 1000;
    float WhipStrikeTime = 0;
    FVector WhipOrigin;
    bool bShooterTest = false;
    int32 ShooterTestStep = 0;
    float ShooterTestTime = 0;
    bool bShooterChecksOK = true;
    bool bShooterTestDone = false;
    USceneComponent* BuildField(FLinearColor Color,USceneComponent* Parent);
    void BuildChapterScenes();
    void StartImpact();
    void TickImpact(float Dt);
    UPROPERTY() USceneComponent* ImpactRoot;
    UPROPERTY() USceneComponent* FallenEva;
    UPROPERTY() USceneComponent* RisingEva;
    UPROPERTY() USceneComponent* ImpactFigure;
    UPROPERTY() TArray<UStaticMeshComponent*> BerserkArms;
    UPROPERTY() UStaticMeshComponent* ImpactOcean;
    UPROPERTY() TArray<USceneComponent*> ImpactBirds;
    UPROPERTY() TArray<UStaticMeshComponent*> ImpactWings;
    UPROPERTY() TArray<USceneComponent*> ImpactPeople;
    UPROPERTY() TArray<UStaticMeshComponent*> ImpactSouls;
    UPROPERTY() TArray<UStaticMeshComponent*> ImpactRings;
    UPROPERTY() TArray<UStaticMeshComponent*> ImpactTowers;
    float ImpactTime = 0.f;
    int32 ImpactStage = -1;
    int32 ImpactShots = 0;
    bool bImpactTest = false;
    void BuildDepot();
    void BuildCannon(USceneComponent* Root);
    USceneComponent* NewSceneRoot(FVector Position);
    void SetChapter(EEvaChapter Next);
    void AdvanceStory();
    void Interact();
    void StartBattle();
    void TickChapter(float Dt);
    void TickChapterAutomation(float Dt);
    void TickDepot(float Dt);
    FString InteractionPrompt() const;
    FString Objective() const;
    FString StorySpeaker() const;
    FString StoryLine() const;
    FString StoryLineTwo() const;
    FVector CableAnchor() const;
    void StoryView(FVector Location,FVector Target,float Blend = .6f);
    void Attack(bool bRanged,float Charge = 0.f,bool PlayerAim = false);
    void StartMission();
    void SetNotice(const FString& Message);
    void Pulse(FVector Position, FLinearColor Color, float Size = 1.f);
    UStaticMeshComponent* EffectShape(const FString& Kind,FVector Position,FVector Scale,FLinearColor Color,float Glow,float Duration,FVector Growth=FVector::ZeroVector);
    void DestroyNearby(FVector Position, float Radius);
    AEvaPawn* Pilot() const;
    bool IsActive() const { return Chapter == EEvaChapter::Battle && !bPaused && !bEnded; }
    bool CanWalk() const { return !bPaused && !bEnded && !bWorldMap && (Chapter == EEvaChapter::Battle || Chapter == EEvaChapter::Street); }
};

UCLASS()
class AEvaHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    void Label(const FString& Text, float X, float Y, FLinearColor Color, float Size = 1.f);
    void Box(float X, float Y, float W, float H, FLinearColor Color);
    void Meter(float X, float Y, float W, float Fraction, FLinearColor Color);
    void DrawChapter(AEvaGameMode* G);
    void DrawWorld(AEvaGameMode* G);
    void DrawControls();
    void DrawCockpit(AEvaGameMode* G);
    void WorldMarker(FVector Position,const FString& Text,FLinearColor Color);
    float SX = 1.f;
    float SY = 1.f;
};
