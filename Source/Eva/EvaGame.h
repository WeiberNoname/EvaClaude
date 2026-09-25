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
class UTextRenderComponent;

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
    bool bTestGuard=false;
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
    UPROPERTY() TArray<USceneComponent*> CameraOccluders;
    float ComboTime = 0;
    int32 KnifeCombo = 0;
    bool bSprinting = false;
    void AnimateEquipment(float Dt);
};

enum class EEvaArch : uint8 { Office, Slab, Hall, Tank, Stack, Cooling, Sphere };

struct FEvaArchSpec
{
    EEvaArch Kind = EEvaArch::Office;
    float Width = 2200.f, Depth = 2200.f, Height = 4000.f;
    int32 Style = 0;
    int32 District = -1;
    FLinearColor Facade = FLinearColor(.29f,.32f,.31f);
    FLinearColor Accent = FLinearColor(.08f,.12f,.14f);
    FString Label;
};

struct FEvaRubblePiece
{
    int32 Batch = 0;
    int32 Instance = 0;
    FTransform Pose;
};

// Every city structure is assembled from the kit: a base and a crown that can break apart.
struct FEvaBuilding
{
    UStaticMeshComponent* Mesh = nullptr;
    UStaticMeshComponent* CrownMesh = nullptr;
    TArray<UStaticMeshComponent*> Windows;
    TArray<UStaticMeshComponent*> Lights;
    UInstancedStaticMeshComponent* Scars[2] = {nullptr, nullptr};
    UMaterialInterface* Skin[2] = {nullptr, nullptr};
    USceneComponent* DistrictRoot = nullptr;
    USceneComponent* Crown = nullptr;
    FVector Center = FVector::ZeroVector;
    FVector Base = FVector::ZeroVector;
    FVector2D Extent = FVector2D::ZeroVector;
    float Height = 0, Split = 0, MaxTilt = 0;
    bool bDestroyed = false;
    int32 DamageStage = 0;
    int32 District = -1;
    float CollapseTime = 0, SmokeClock = 0, Smolder = 0;
    EEvaArch Kind = EEvaArch::Office;
    FVector FallDirection = FVector::ForwardVector;
    FVector SmokePoint = FVector::ZeroVector;
    FLinearColor FacadeColor;
    TArray<FEvaRubblePiece> Rubble;
    float CollapseDuration() const { return Kind==EEvaArch::Hall ? 2.2f : Kind==EEvaArch::Tank || Kind==EEvaArch::Sphere ? 2.f : Kind==EEvaArch::Slab ? 2.8f : 3.2f; }
    FBox Bounds() const { return FBox(Base-FVector(Extent.X,Extent.Y,0),Base+FVector(Extent.X,Extent.Y,Height)); }
};

// Pooled CPU particles drawn through one instanced batch each: debris, dust and smoke, fire.
struct FEvaParticle
{
    FVector Position = FVector::ZeroVector, Velocity = FVector::ZeroVector, Scale = FVector(1);
    FRotator Rotation = FRotator::ZeroRotator, Spin = FRotator::ZeroRotator;
    float Age = 0, Life = 1, Gravity = 0, Drag = 0, Growth = 0, Alpha = 1, Bright = 1;
    int32 Slot = INDEX_NONE;
};

struct FEvaParticleBatch
{
    UInstancedStaticMeshComponent* Mesh = nullptr;
    TArray<FEvaParticle> Live;
    TArray<int32> Free;
    bool bFades = false;
};

// Street-scale details the Evas can knock over: trees, houses, cars, lights, poles and containers.
enum class EEvaBreak : uint8 { Flatten, Topple, Hide, Drop, Scatter };

struct FEvaPropPart
{
    UInstancedStaticMeshComponent* Batch = nullptr;
    int32 Instance = 0;
    FTransform Rest;
    EEvaBreak Mode = EEvaBreak::Topple;
};

struct FEvaProp
{
    FVector Position = FVector::ZeroVector;
    float Radius = 300;
    bool bBroken = false;
    TArray<FEvaPropPart> Parts;
};

// A.T. field rings attached to a field root, and pooled octagonal ripples in world space.
struct FEvaFieldRings
{
    USceneComponent* Root = nullptr;
    UInstancedStaticMeshComponent* Rings = nullptr;
    FLinearColor Color = FLinearColor(1,.3f,.06f);
};

struct FEvaRipple
{
    int32 Slot = INDEX_NONE;
    FVector Center = FVector::ZeroVector, Drift = FVector::ZeroVector;
    FQuat Rotation = FQuat::Identity;
    float Age = 0, Delay = 0, Life = .5f, From = 0, To = 0, Peak = 1;
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
    void TickDistrict(float Dt);
    void SetDistrictLighting(bool Enabled);
    void ResetDistrict();
    bool DistrictInteract();
    FString DistrictPrompt() const;
    UMaterialInstanceDynamic* DistrictMaterial(FLinearColor Color,int32 Type,float Glow=0);
    UStaticMeshComponent* CityBox(FVector Position,FVector Scale,FLinearColor Color,int32 Type=0,USceneComponent* Parent=nullptr,bool Collision=false,float Glow=0,const FString& Kind=TEXT("Cube"));
    UInstancedStaticMeshComponent* CityInstances(USceneComponent* Parent,FLinearColor Color,int32 Type=0,float Glow=0,const TCHAR* MeshKind=TEXT("Cube"));
    UTextRenderComponent* CitySign(USceneComponent* Parent,FVector Position,const FString& Text,float Size,FColor Color,FRotator Rotation=FRotator(0,-90,0));
    void DistrictSound(const TCHAR* Name,FVector Position,float Volume=1);
    void DistrictDust(FVector Position,float Size=1);
    UStaticMesh* KitMesh(const FString& Kind);
    // City kit (EvaArchitecture.cpp), district layouts (EvaCityDistricts.cpp), destruction (EvaDestruction.cpp).
    int32 BuildArchitecture(FVector Base,const FEvaArchSpec& Spec);
    void BuildStreetGrid(int32 District,FVector Base,int32 Blocks,bool bTrees,bool bPoles);
    void BuildOpenBlock(int32 District,FVector Center,bool bPark);
    void BuildHarborDistrict();
    void BuildUplandDistrict();
    void BuildIndustrialDistrict();
    void BuildWorldInfrastructure();
    UInstancedStaticMeshComponent* PropBatch(int32 District,FLinearColor Color,int32 Type,float Glow=0,const TCHAR* MeshKind=TEXT("Cube"),bool bShadow=false);
    int32 AddProp(FVector Position,float Radius);
    void AddPropPart(int32 Prop,UInstancedStaticMeshComponent* Batch,const FTransform& Pose,EEvaBreak Mode,FLinearColor Tint=FLinearColor::White);
    void AddTree(int32 District,FVector Position,float Scale=1);
    void AddHouse(int32 District,FVector Position,float Width,float Depth,bool bTurned,FLinearColor Wall,FLinearColor Roof);
    void AddCar(int32 District,FVector Position,float Yaw,FLinearColor Paint);
    void AddStreetLight(int32 District,FVector Position,float Yaw);
    void AddUtilityLine(int32 District,FVector From,FVector To,int32 Poles);
    void AddVending(int32 District,FVector Position,float Yaw,FLinearColor Face);
    void AddContainerStack(int32 District,FVector Position,int32 Rows,int32 Tiers,bool bTurned);
    void BreakProp(int32 Index,FVector From);
    void BreakPropsNear(FVector Position,float Radius);
    void DamageBuilding(FEvaBuilding& Building,FVector Impact,int32 Stages=1);
    void CollapseRubble(FEvaBuilding& Building);
    void TickDestruction(float Dt);
    void TickCollapse(FEvaBuilding& Building,float Dt);
    void ResetCity();
    UInstancedStaticMeshComponent* ParticleMesh(int32 Batch);
    void SpawnParticle(int32 Batch,FVector Position,FVector Velocity,FVector Scale,float Life,float Gravity=0,float Growth=0,float Bright=1,FLinearColor Tint=FLinearColor::White,float Alpha=1);
    void DebrisBurst(FVector Position,FVector Direction,int32 Count,float Speed,FLinearColor Tint);
    void SmokePuff(FVector Position,float Size,float Darkness);
    TMap<UPrimitiveComponent*,int32> BuildingByComponent;
    UPROPERTY() TArray<UInstancedStaticMeshComponent*> RubbleBatches;
    TArray<FEvaParticleBatch> ParticleBatches;
    TArray<FEvaProp> Props;
    TMap<FIntPoint,TArray<int32>> PropGrid;
    TMap<FString,UInstancedStaticMeshComponent*> PropBatches;
    UPROPERTY() UMaterialInstanceDynamic* AviationMaterial;
    UPROPERTY() TArray<UAudioComponent*> DistrictLoops;
    TArray<float> DistrictLoopVolumes;
    float CityClock=0, CrumbleClock=0, SteamClock=0;
    int32 PropsBroken=0;
    TArray<int32> SteamVents;
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
    int32 DistrictTestStep=0, DistrictTestBuilding=INDEX_NONE, DistrictTestAux=INDEX_NONE, DistrictTestProp=INDEX_NONE;
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
    // Octagonal A.T. fields (EvaField.cpp): flowing rings at rest, ripples on contact, shatter, dissolve and restore.
    UMaterialInstanceDynamic* FieldMaterial(FLinearColor Color);
    FLinearColor FieldColor(const USceneComponent* Field) const;
    void BuildFieldEffects();
    void TickFields(float Dt);
    FVector FieldImpact(USceneComponent* Field,FVector From,FVector To,float Strength=1);
    void FieldBurst(USceneComponent* Field,int32 Kind,FVector Point);
    void GuardRipple(FVector Source);
    void SpawnRipple(FVector Center,FQuat Rotation,FLinearColor Color,float From,float To,float Life,float Delay,float Peak,FVector Drift=FVector::ZeroVector);
    TArray<FEvaFieldRings> FieldRings;
    TArray<FEvaRipple> Ripples;
    TArray<int32> RippleFree;
    UPROPERTY() UInstancedStaticMeshComponent* RippleBatch;
    UPROPERTY() UMaterialInterface* FieldSurface;
    UPROPERTY() USceneComponent* WatchedShield;
    float WatchedField=100, FieldClock=0;
    int32 FieldBursts[3]={0,0,0};
    int32 GuardRipples=0;
    bool bFieldTest=false, bFieldDone=false, bFieldOK=true;
    int32 FieldTestStep=0, FieldTestMeshes=0, FieldTestRipples=0;
    float FieldTestTime=0, FieldTestIntegrity=0;
    void TickFieldTest(float Dt);
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
