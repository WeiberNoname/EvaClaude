#include "EvaGame.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "Sound/SoundBase.h"

namespace EvaPalette
{
    const FLinearColor Lime(.52f, 1.f, .045f);
    const FLinearColor Purple(.24f, .055f, .48f);
    const FLinearColor Orange(1.f, .19f, .035f);
    const FLinearColor White(.81f, .91f, .87f);
}

static void EvaSound(UObject* Context,const TCHAR* Name,float Volume=.6f)
{
    const FString Path=FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name);
    if(auto* Sound=LoadObject<USoundBase>(nullptr,*Path)) UGameplayStatics::PlaySound2D(Context,Sound,Volume);
}

AEvaPawn::AEvaPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
    Capsule->InitCapsuleSize(145.f, 760.f);
    Capsule->SetCollisionProfileName(TEXT("Pawn"));
    RootComponent = Capsule;
    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    Boom->SetupAttachment(RootComponent);
    Boom->TargetArmLength = 2850.f;
    Boom->TargetOffset = FVector(0, 0, 650);
    Boom->SetUsingAbsoluteRotation(true);
    Boom->bDoCollisionTest = true;
    Boom->ProbeSize = 60.f;
    Boom->bEnableCameraLag=true; Boom->CameraLagSpeed=22;
    Boom->bUseCameraLagSubstepping=true; Boom->CameraLagMaxTimeStep=1.f/120.f;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Boom);
    Camera->FieldOfView = 75.f;
    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AEvaPawn::BeginPlay()
{
    Super::BeginPlay();
    auto* G = Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if (!G) return;
    BuildEvaVisuals();
    BuildEquipment();
    // GameMode may enter the street before this pawn receives BeginPlay in an automated run.
    SetHumanMode(bHuman);
}

void AEvaPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AEvaPawn::Melee);
    Input->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AEvaPawn::StopFire);
    Input->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AEvaPawn::AimPressed);
    Input->BindKey(EKeys::RightMouseButton, IE_Released, this, &AEvaPawn::AimReleased);
    Input->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AEvaPawn::Lance);
    Input->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AEvaPawn::ReleaseCharge);
    Input->BindKey(EKeys::Z, IE_Pressed, this, &AEvaPawn::Overdrive);
    Input->BindKey(EKeys::X, IE_Pressed, this, &AEvaPawn::AntiField);
    Input->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AEvaPawn::Jump);
    Input->BindKey(EKeys::LeftControl, IE_Pressed, this, &AEvaPawn::Dodge);
    Input->BindKey(EKeys::V, IE_Pressed, this, &AEvaPawn::TogglePerspective);
    Input->BindKey(EKeys::B, IE_Pressed, this, &AEvaPawn::SwapShoulder);
    Input->BindKey(EKeys::Y, IE_Pressed, this, &AEvaPawn::CallAsuka);
    Input->BindKey(EKeys::F1, IE_Pressed, this, &AEvaPawn::ToggleHelp);
    Input->BindKey(EKeys::E, IE_Pressed, this, &AEvaPawn::Connect);
    Input->BindKey(EKeys::Tab, IE_Pressed, this, &AEvaPawn::ToggleLock);
    Input->BindKey(EKeys::Enter, IE_Pressed, this, &AEvaPawn::Confirm);
    Input->BindKey(EKeys::Escape, IE_Pressed, this, &AEvaPawn::PauseMission);
    Input->BindKey(EKeys::R, IE_Pressed, this, &AEvaPawn::Restart);
    Input->BindKey(EKeys::F, IE_Pressed, this, &AEvaPawn::DrawKnife);
    Input->BindKey(EKeys::One, IE_Pressed, this, &AEvaPawn::DrawKnife);
    Input->BindKey(EKeys::Two, IE_Pressed, this, &AEvaPawn::EquipCannon);
    Input->BindKey(EKeys::C, IE_Pressed, this, &AEvaPawn::ReleaseCable);
    Input->BindKey(EKeys::T, IE_Pressed, this, &AEvaPawn::ImpactCinema);
    Input->BindKey(EKeys::O, IE_Pressed, this, &AEvaPawn::OpenWorld);
    Input->BindKey(EKeys::M, IE_Pressed, this, &AEvaPawn::ToggleMap);
    Input->BindKey(EKeys::N, IE_Pressed, this, &AEvaPawn::NextWaypoint);
    Input->BindKey(EKeys::Home, IE_Pressed, this, &AEvaPawn::ReturnToTitle);
}

void AEvaPawn::Tick(float Dt)
{
    Super::Tick(Dt);
    auto* G = Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    auto* PC = Cast<APlayerController>(Controller);
    if (!G || !PC) return;
    OcclusionTimer-=Dt;
    if(OcclusionTimer<=0) { UpdateCameraOcclusion(); OcclusionTimer=.05f; }
    if(!G->bPaused)
    {
        AnimateEquipment(Dt); UpdatePerspective(); Motion.Advance(Dt);
        LandingKick=FMath::Lerp(LandingKick,0.f,FEvaMotion::Blend(12,Dt));
        ComboTime=FMath::Max(0.f,ComboTime-Dt);
        RecoilPitch=FMath::Lerp(RecoilPitch,0.f,FEvaMotion::Blend(14,Dt));
        HitMarkerTime=FMath::Max(0.f,HitMarkerTime-Dt);
        if(ReloadTime>0)
        {
            ReloadTime=FMath::Max(0.f,ReloadTime-Dt);
            if(ReloadTime<=0) { Loadout.Reload(); G->SetNotice("MAGAZINE SEATED // READY"); }
        }
    }
    if(G->bPaused || G->bWorldMap || !G->IsActive()) bFireHeld=false;
    if(!G->bPaused && (!G->IsActive() || Loadout.Equipped!=EEvaWeapon::Cannon || bGuard)) { bCharging=false; CannonCharge=0; }
    if(!G->bPaused && bCharging) CannonCharge=FMath::Min(1.f,CannonCharge+Dt/2.f);
    if(!bHuman)
    {
        const float Zoom=bAiming || bCharging ? (bFirstPerson ? 66.f:62.f) : bSprinting ? 88.f : 80.f;
        Camera->SetFieldOfView(FMath::Lerp(Camera->FieldOfView,Zoom,FEvaMotion::Blend(15,Dt)));
        Boom->TargetArmLength=FMath::Lerp(Boom->TargetArmLength,bFirstPerson ? 0.f : bAiming ? 1800.f : 2800.f,FEvaMotion::Blend(18,Dt));
        Boom->SocketOffset=FMath::Lerp(Boom->SocketOffset,bFirstPerson ? FVector::ZeroVector:FVector(0,ShoulderSide*(bAiming ? 450.f:200.f),100),FEvaMotion::Blend(18,Dt));
    }
    if (!G->CanWalk()) { if(G->bWorldMap && G->IsActive()) MoveEva(FVector::ZeroVector,0,Dt); else MoveVelocity=FVector::ZeroVector; bSprinting=false; DashBuffer=0; Boom->SetWorldRotation(FRotator(CameraPitch, CameraYaw, 0)); return; }
    float MX=0, MY=0;
    PC->GetInputMouseDelta(MX,MY);
    CameraYaw += MX * (bAiming ? .72f : 1.2f);
    CameraPitch = FMath::Clamp(CameraPitch + MY * (bAiming ? .65f : 1.05f), -60.f, bHuman ? 55.f : 45.f);
    if (bLock && !bHuman && G->HasCombatTarget())
    {
        FRotator Target=(G->EnemyAimPoint()-Camera->GetComponentLocation()).Rotation();
        CameraYaw=FMath::FixedTurn(CameraYaw,Target.Yaw,Dt*100.f);
        CameraPitch=FMath::FInterpTo(CameraPitch,Target.Pitch,Dt,7.f);
    }
    Boom->SetWorldRotation(FRotator(CameraPitch+RecoilPitch-LandingKick, CameraYaw, 0));
    FVector Fwd = FRotator(0,CameraYaw,0).Vector();
    FVector Right = FRotationMatrix(FRotator(0,CameraYaw,0)).GetUnitAxis(EAxis::Y);
    FVector Move=G->bDynamicTest && !TestMoveInput.IsNearlyZero() ? TestMoveInput:ReadMovement();
    bGuard = !bHuman && PC->IsInputKeyDown(EKeys::Q) && G->Rules.Battery > 0.f;
    DodgeCooldown = FMath::Max(0.f,DodgeCooldown-Dt);
    if(DashBuffer>0) { DashBuffer=FMath::Max(0.f,DashBuffer-Dt); if(DodgeCooldown<=0 && Motion.DashCharges>0) Dodge(); }
    AttackTime = FMath::Max(0.f,AttackTime-Dt);
    if (!Move.IsNearlyZero() && DodgeTime <= 0.f) DodgeDirection = Move;
    bSprinting=!bHuman && !bGuard && !bAiming && !bCharging && !bFireHeld && PC->IsInputKeyDown(EKeys::LeftShift) && G->Rules.Battery>5 && !Move.IsNearlyZero();
    if(bSprinting) G->Rules.Battery=FMath::Max(0.f,G->Rules.Battery-Dt*1.5f);
    const float Speed=bHuman ? 430.f : bGuard ? 850.f : bAiming ? 1600.f : bSprinting ? 3800.f : 2200.f;
    MoveEva(Move,Speed,Dt);
    FVector Facing = !bHuman && (bAiming || bFireHeld || bCharging) ? Fwd : bLock && !bHuman && G->HasCombatTarget() ? G->EnemyPosition - GetActorLocation() : Move;
    Facing.Z = 0;
    if (!Facing.IsNearlyZero()) SetActorRotation(FMath::Lerp(GetActorRotation().Quaternion(),Facing.Rotation().Quaternion(),FEvaMotion::Blend(20,Dt)).Rotator());
    if(bFireHeld && !bCharging && !bSprinting && ReloadTime<=0)
    {
        if(Loadout.Equipped==EEvaWeapon::Cannon && Loadout.Shells==0 && Loadout.ReserveShells>0) ReloadWeapon();
        else G->Attack(Loadout.Equipped==EEvaWeapon::Cannon,0,true);
    }
    const float Stride=FMath::Clamp(MoveVelocity.Size()/2200.f,0.f,1.6f);
    WalkTime+=Dt*(Stride>.05f ? 8*Stride:1.8f);
    if(EvaRig)
    {
        FVector Local=GetActorTransform().InverseTransformVectorNoScale(MoveVelocity);
        FRotator Lean(bGrounded ? -Local.X/650 : -7,0,-Local.Y/700);
        EvaRig->SetRelativeRotation(FMath::Lerp(EvaRig->GetRelativeRotation().Quaternion(),Lean.Quaternion(),FEvaMotion::Blend(10,Dt)));
        EvaRig->SetRelativeLocation(FVector(0,0,bGrounded ? FMath::Abs(FMath::Sin(WalkTime))*12*Stride : 0));
    }
    for (int32 I=0; I<Limbs.Num(); ++I)
    {
        const float Swing = FMath::Sin(WalkTime + (I<2 ? 0.f : PI)) * (2.f+FMath::Min(Stride,1.5f)*24.f);
        const bool GunPose=I%2==0 && Loadout.Equipped==EEvaWeapon::Cannon;
        const float Pose=GunPose ? (ReloadTime>0 ? 28.f : 65.f+CameraPitch*.4f) : I%2==0 && AttackTime>0 ? 65.f*FMath::Sin(AttackTime*7.f) : Swing;
        Limbs[I]->SetRelativeRotation(FMath::RInterpTo(Limbs[I]->GetRelativeRotation(),FRotator(!bGrounded && I%2==1 ? 30.f:Pose,0,I%2==1 ? -FVector::DotProduct(MoveVelocity,Right)/450.f:0),Dt,18));
    }
    for(int I=0;I<Knees.Num();++I)
    {
        float Flex=!bGrounded ? -65.f : -FMath::Max(0.f,FMath::Sin(WalkTime+I*PI))*35*FMath::Min(Stride,1.5f)-LandingKick*6;
        Knees[I]->SetRelativeRotation(FMath::RInterpTo(Knees[I]->GetRelativeRotation(),FRotator(Flex,0,0),Dt,18));
    }
}

void AEvaPawn::Melee() { bFireHeld=true; if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) if(!G->bWorldMap) G->Attack(false,0,true); }
void AEvaPawn::StopFire() { bFireHeld=false; }
void AEvaPawn::AimPressed() { if(!bHuman) bAiming=true; }
void AEvaPawn::AimReleased() { bAiming=false; }
void AEvaPawn::ReloadWeapon()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(!G || !G->IsActive() || G->bWorldMap || Loadout.Equipped!=EEvaWeapon::Cannon || ReloadTime>0 || Loadout.Shells>=FEvaLoadout::Capacity) return;
    if(Loadout.ReserveShells<=0) { G->SetNotice("NO SPARE SHELLS // VISIT A SERVICE STATION"); return; }
    ReloadTime=1.3f; bCharging=false; CannonCharge=0;
    G->SetNotice("RELOADING // COVER YOUR POSITION");
}
void AEvaPawn::Lance()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(!G || !G->IsActive()) return;
    if(Loadout.Equipped!=EEvaWeapon::Cannon) { G->Attack(false); return; }
    if(ReloadTime>0 || G->bWorldMap || G->LanceCooldown>0 || PickupTime>0 || DrawTime>0 || bGuard || Loadout.Shells<=0) return;
    bCharging=true; CannonCharge=0;
}
void AEvaPawn::ReleaseCharge()
{
    if(bCharging) if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) G->Attack(true,CannonCharge,true);
    bCharging=false; CannonCharge=0;
}
void AEvaPawn::Overdrive() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) G->UseOverdrive(); }
void AEvaPawn::AntiField() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) G->UseAntiField(); }
void AEvaPawn::Dodge()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(G && G->Chapter==EEvaChapter::Title) { G->StartBattle(); return; }
    if(!G || !G->IsActive() || G->bWorldMap || bHuman) return;
    if(DodgeCooldown>0 || Motion.DashCharges<=0) { DashBuffer=.14f; return; }
    if(Motion.Dash(G->Rules))
    {
        FVector Input=ReadMovement();
        if(G->bDynamicTest && !TestMoveInput.IsNearlyZero()) Input=TestMoveInput;
        DodgeDirection=Input.IsNearlyZero() ? FRotator(0,CameraYaw,0).Vector():Input;
        DodgeTime=.32f; DodgeCooldown=.36f; DashBuffer=0; bCharging=false; CannonCharge=0;
    }
}
void AEvaPawn::Connect()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(G && !G->bPaused) G->Interact();
}
void AEvaPawn::ToggleLock() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) bLock=G->HasCombatTarget() && !bLock; }
void AEvaPawn::ImpactCinema() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) if(G->Chapter==EEvaChapter::Title) G->StartImpact(); }
void AEvaPawn::Confirm() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) { if(G->bHelp) ToggleHelp(); else if(G->bPaused) G->bPaused=false; else G->AdvanceStory(); } }
void AEvaPawn::PauseMission() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) if(G->bHelp) ToggleHelp(); else if(G->bStarted && !G->bEnded) { G->bPaused=!G->bPaused; bFireHeld=false; } }
void AEvaPawn::Restart()
{
    if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()))
    {
        if(G->Chapter==EEvaChapter::Impact && (G->bPaused || G->ImpactTime>=60))
        { G->ImpactTime=0; G->ImpactStage=-1; G->ImpactShots=0; G->bPaused=false; G->TickImpact(0); }
        else if(G->bEnded || G->bPaused) { if(G->bOpenWorld) G->StartOpenWorld(); else G->StartBattle(); }
        else ReloadWeapon();
    }
}

AEvaGameMode::AEvaGameMode()
{
    SetActorHiddenInGame(false);
    PrimaryActorTick.bCanEverTick=true;
    DefaultPawnClass=AEvaPawn::StaticClass();
    HUDClass=AEvaHUD::StaticClass();
    Scene=CreateDefaultSubobject<USceneComponent>(TEXT("WorldRoot"));
    RootComponent=Scene;
}

UStaticMeshComponent* AEvaGameMode::Shape(const FString& Kind,FVector P,FVector S,FLinearColor C,float Glow,USceneComponent* Parent,bool Collision)
{
    if(!Surface) Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Surface.M_Surface"));
    if(!VisualWorld)
    {
        VisualWorld=GetWorld()->SpawnActor<AActor>();
        auto* VisualRoot=NewObject<USceneComponent>(VisualWorld);
        VisualWorld->SetRootComponent(VisualRoot);
        VisualRoot->RegisterComponent();
    }
    auto* Mesh=NewObject<UStaticMeshComponent>(Parent ? Parent->GetOwner() : VisualWorld);
    Mesh->SetStaticMesh(KitMesh(Kind));
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetupAttachment(Parent ? Parent : VisualWorld->GetRootComponent());
    Mesh->SetRelativeLocation(P);
    Mesh->SetRelativeScale3D(S);
    Mesh->SetCollisionEnabled(Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->RegisterComponent();
    ApplySurface(Mesh,Kind,C,Glow);
    if(Glow>0 && S.GetMin()<.5f) { Mesh->SetCastShadow(false); Mesh->SetReceivesDecals(false); }
    Meshes.Add(Mesh);
    return Mesh;
}

void AEvaGameMode::ApplySurface(UStaticMeshComponent* Mesh,const FString& Kind,FLinearColor C,float Glow)
{
    if(!Surface) return;
    const bool Armor=Kind.StartsWith("Armor");
    const FString Key=C.ToString()+FString::Printf(TEXT("/%d/%.3f"),Armor,Glow);
    UMaterialInstanceDynamic* Mat=MaterialCache.FindRef(Key);
    if(!Mat)
    {
        Mat=UMaterialInstanceDynamic::Create(Surface,this);
        Mat->SetVectorParameterValue(TEXT("Color"),C);
        Mat->SetScalarParameterValue(TEXT("Glow"),Glow>0 ? Glow : Armor ? .06f:0.f);
        Mat->SetScalarParameterValue(TEXT("Roughness"),Armor ? .68f:.82f);
        Mat->SetScalarParameterValue(TEXT("Metallic"),Armor ? .12f:.04f);
        MaterialCache.Add(Key,Mat);
    }
    Mesh->SetMaterial(0,Mat);
}

UStaticMeshComponent* AEvaGameMode::EffectShape(const FString& Kind,FVector Pos,FVector Scale,FLinearColor Color,float Glow,float Duration,FVector Growth)
{
    UStaticMeshComponent* M=nullptr;
    if(EffectPool.Num()>0 && MeshCache.Contains(Kind))
    {
        M=EffectPool.Pop(); M->SetStaticMesh(MeshCache[Kind]); ApplySurface(M,Kind,Color,Glow);
        M->SetWorldLocation(Pos); M->SetWorldRotation(FRotator::ZeroRotator); M->SetWorldScale3D(Scale); M->SetVisibility(true);
    }
    else M=Shape(Kind,Pos,Scale,Color,Glow);
    M->SetCastShadow(false); M->SetReceivesDecals(false);
    Effects.Add({M,Duration,Duration,Growth}); return M;
}

void AEvaGameMode::BeginPlay()
{
    Super::BeginPlay();
    Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Surface.M_Surface"));
    CitySurface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_City.M_City"));
    DustSurface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Dust.M_Dust"));
    // One shared material drives every aviation obstruction light, so a single parameter blinks them all.
    AviationMaterial=UMaterialInstanceDynamic::Create(CitySurface ? CitySurface:Surface,this);
    AviationMaterial->SetVectorParameterValue(TEXT("Color"),FLinearColor(1,.05f,.03f));
    AviationMaterial->SetScalarParameterValue(TEXT("SurfaceType"),2);
    AviationMaterial->SetScalarParameterValue(TEXT("Glow"),4);
    BuildCity();
    ParticleMesh(0);
    BuildAngel();
    BuildShamshel();
    BuildRamiel();
    SelectAngel(EEvaAngel::Sachiel);
    BuildChapterScenes();
    BuildDepot();
    auto* P=Pilot();
    if(P) { P->SetActorLocation(FVector(0,-5900,780)); P->SetActorRotation(FRotator(0,90,0)); }
    Cable=Shape("Cylinder",FVector::ZeroVector,FVector(1),EvaPalette::Lime,2.f);
    auto* CableMaterial=UMaterialInstanceDynamic::Create(Surface,this);
    CableMaterial->SetVectorParameterValue("Color",EvaPalette::Lime); CableMaterial->SetScalarParameterValue("Glow",2); Cable->SetMaterial(0,CableMaterial);
    PlayerShield=BuildField(EvaPalette::Lime,VisualWorld->GetRootComponent());
    PlayerShield->SetVisibility(false,true);
    ThreatRing=Shape("Cylinder",FVector::ZeroVector,FVector(26,26,.12f),EvaPalette::Orange,3.f);
    ThreatRing->SetVisibility(false);
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor=false;
    }
    SetChapter(EEvaChapter::Title);
    bChapterAuto=FParse::Param(FCommandLine::Get(),TEXT("EvaChapterTest"));
    bChapterSmoke=FParse::Param(FCommandLine::Get(),TEXT("EvaChapterShots"));
    if(bChapterAuto || bChapterSmoke) StartMission();
    else if(FParse::Param(FCommandLine::Get(),TEXT("EvaWorld")) || FParse::Param(FCommandLine::Get(),TEXT("EvaWorldTest")) || FParse::Param(FCommandLine::Get(),TEXT("EvaShamshel")) || FParse::Param(FCommandLine::Get(),TEXT("EvaShooterTest")) || FParse::Param(FCommandLine::Get(),TEXT("EvaRamiel")) || FParse::Param(FCommandLine::Get(),TEXT("EvaDynamicTest")) || FParse::Param(FCommandLine::Get(),TEXT("EvaCompanionTest")) || FParse::Param(FCommandLine::Get(),TEXT("EvaDistrictTest")))
    {
        static bool bWorldAutoLaunched=false;
        if(!bWorldAutoLaunched) { bWorldAutoLaunched=true; StartOpenWorld(); }
    }
    else if(FParse::Param(FCommandLine::Get(),TEXT("EvaImpact")) || FParse::Param(FCommandLine::Get(),TEXT("EvaImpactTest")))
    {
        static bool bImpactAutoLaunched=false;
        if(!bImpactAutoLaunched) { bImpactAutoLaunched=true; StartImpact(); }
    }
    else if(FParse::Param(FCommandLine::Get(),TEXT("EvaSmoke")) || FParse::Param(FCommandLine::Get(),TEXT("EvaAutoBattle")) || FParse::Param(FCommandLine::Get(),TEXT("EvaSystemsTest"))) StartBattle();
}

AEvaPawn* AEvaGameMode::Pilot() const { return Cast<AEvaPawn>(UGameplayStatics::GetPlayerPawn(this,0)); }
void AEvaGameMode::StartMission() { bStarted=true; bEnded=false; bVictory=false; bPhoneUsed=false; bStreetAttack=false; SetChapter(EEvaChapter::Street); }
void AEvaGameMode::SetNotice(const FString& Message) { Notice=Message; NoticeTime=4.f; }

void AEvaGameMode::BuildCity()
{
    using namespace EvaPalette;
    Shape("Cube",FVector(0,0,-90),FVector(320,320,1.8f),FLinearColor(.025f,.037f,.045f),0,nullptr,true);
    for(int X=-4;X<=4;++X)
    {
        Shape("Cube",FVector(X*2800,0,3),FVector(7,290,.06f),FLinearColor(.012f,.018f,.025f));
        Shape("Cube",FVector(0,X*2800,4),FVector(290,7,.06f),FLinearColor(.012f,.018f,.025f));
        for(int Y=-14;Y<=14;++Y)
        {
            Shape("Cube",FVector(X*2800,Y*900,9),FVector(.16f,3.f,.08f),FLinearColor(.48f,.4f,.22f),.2f);
            Shape("Cube",FVector(Y*900,X*2800,9),FVector(3.f,.16f,.08f),FLinearColor(.48f,.4f,.22f),.2f);
        }
    }
    FRandomStream Rand(1701);
    for(int X=-4;X<=4;++X) for(int Y=-4;Y<=4;++Y)
    {
        float PX=X*2800+1400, PY=Y*2800+1400;
        if(FMath::Abs(PX)<1900 && PY>-8200 && PY<8000) continue;
        // Draw in the original order so the story city keeps its layout, cover, and test positions.
        const float H=Rand.FRandRange(700,3100);
        FEvaArchSpec Block;
        Block.District=-1; Block.Height=H;
        Block.Width=Rand.FRandRange(8,14)*100; Block.Depth=Rand.FRandRange(8,14)*100;
        Block.Facade=FLinearColor(.045f,.07f,.085f)*Rand.FRandRange(.65f,1.3f)*3.4f;
        for(int Floor=1;Floor<int(H/230);++Floor) Rand.FRand();
        Block.Style=(X+4)*9+(Y+4);
        Block.Accent=FLinearColor(.06f,.08f,.09f);
        BuildArchitecture(FVector(PX,PY,0),Block);
    }
    // Street kit for the story city: pavements, kerbside lights and markings, clear of the telephone and car.
    auto* Streets=NewSceneRoot(FVector::ZeroVector);
    auto* Pave=CityInstances(Streets,FLinearColor(.2f,.21f,.21f));
    for(int X=-4;X<=4;++X) for(int Side:{-1,1})
    {
        Pave->AddInstance(FTransform(FRotator::ZeroRotator,FVector(X*2800+Side*470,0,14),FVector(2.2f,290,.2f)));
        Pave->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,X*2800+Side*470,15),FVector(290,2.2f,.2f)));
    }
    for(float Y=-9100;Y<=9800;Y+=1400) for(int Side:{-1,1})
    {
        const FVector Light(Side*430,Y,0);
        if(FVector::Dist2D(Light,PhonePosition)<900 || FVector::Dist2D(Light,CarPosition)<900) continue;
        AddStreetLight(-1,Light,Side>0 ? 180:0);
    }
    // Monumental perimeter and evacuation beacons establish the arena boundary.
    for(int I=-7;I<=7;++I)
    {
        Shape("Cube",FVector(I*2100,14500,1300),FVector(18,12,26),FLinearColor(.065f,.09f,.11f));
        Shape("Cube",FVector(I*2100,13880,1900),FVector(.3f,.2f,5),Orange,4.f);
    }
    Shape("Cylinder",Anchor,FVector(10,10,1.5f),FLinearColor(.04f,.06f,.07f));
    Shape("Cube",Anchor+FVector(0,0,550),FVector(2.2f,2.2f,11),FLinearColor(.07f,.11f,.1f));
    Shape("Cube",Anchor+FVector(0,0,1180),FVector(2.6f,2.6f,1),Lime,6.f);
    for(int I=0;I<4;++I) Shape("Cube",Anchor+FVector((I-1.5f)*210,0,40),FVector(1,8,.1f),Lime,2.f);
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>();
    WorldSun=Sun;
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(1);
    Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetAtmosphereSunLight(true);
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    Sun->SetActorRotation(FRotator(-24,-35,0));
    Sun->GetLightComponent()->SetIntensity(6.f);
    Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f,.76f,.6f));
    auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>();
    WorldFill=Fill;
    Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Fill->SetActorRotation(FRotator(-35,-140,0));
    Fill->GetLightComponent()->SetIntensity(4.5f);
    Fill->GetLightComponent()->SetLightColor(FLinearColor(.66f,.74f,1.f));
    Fill->GetLightComponent()->SetCastShadows(false);
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(.9f);
    Sky->GetLightComponent()->SetLightColor(FLinearColor(.25f,.4f,.65f));
    auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();
    Fog->GetComponent()->SetFogDensity(.0035f);
    Fog->GetComponent()->SetFogHeightFalloff(.12f);
    Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.075f,.13f,.17f));
    Fog->GetComponent()->SetVolumetricFog(true);
    auto* PP=GetWorld()->SpawnActor<APostProcessVolume>();
    PP->bUnbound=true;
    PP->Settings.bOverride_AutoExposureMethod=true;
    PP->Settings.AutoExposureMethod=AEM_Manual;
    PP->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    PP->Settings.AutoExposureApplyPhysicalCameraExposure=false;
    PP->Settings.bOverride_AutoExposureBias=true;
    PP->Settings.AutoExposureBias=0.f;
    PP->Settings.bOverride_BloomIntensity=true;
    PP->Settings.BloomIntensity=.28f;
    PP->Settings.bOverride_VignetteIntensity=true;
    PP->Settings.VignetteIntensity=.28f;
    // A distant luminous disc gives the otherwise procedural sky an authored focal point.
    Shape("Sphere",FVector(9500,19000,11000),FVector(45,45,45),FLinearColor(1.f,.34f,.12f),2.f);
}

void AEvaGameMode::BuildAngel()
{
    AngelRoot=NewSceneRoot(EnemyPosition);
    const FLinearColor Skin(.025f,.045f,.035f), Bone(.76f,.8f,.64f);
    Shape("Sphere",FVector(0,0,180),FVector(7,4.6f,10),Skin,0,AngelRoot);
    Shape("Sphere",FVector(0,0,-380),FVector(5,4,4),Skin,0,AngelRoot);
    Shape("AngelMask",FVector(0,-110,850),FVector(4.2f,3,5),Bone,0,AngelRoot);
    for(int Side : {-1,1})
    {
        Shape("Sphere",FVector(Side*102,-172,940),FVector(.8f,.18f,1.1f),FLinearColor(.005f,.006f,.003f),0,AngelRoot);
        Shape("Sphere",FVector(Side*505,0,450),FVector(4.6f,4.8f,3.5f),Skin,0,AngelRoot);
        auto* Arm=Shape("Cylinder",FVector(Side*650,0,-175),FVector(1.4f,1.4f,13),Skin,0,AngelRoot);
        AngelLimbs.Add(Arm);
        Shape("Sphere",FVector(Side*660,-30,-900),FVector(2.8f,2.2f,3),Skin,0,AngelRoot);
        auto* Leg=Shape("Cylinder",FVector(Side*235,0,-980),FVector(2.3f,2.3f,12),Skin,0,AngelRoot);
        AngelLimbs.Add(Leg);
        Shape("Sphere",FVector(Side*235,-170,-1510),FVector(2.6f,5,.95f),Skin,0,AngelRoot);
        for(int I=0;I<3;++I)
        {
            auto* Rib=Shape("Cube",FVector(Side*225,-220,360-I*125),FVector(3.7f,.45f,.48f),Bone,0,AngelRoot);
            Rib->SetRelativeRotation(FRotator(0,0,Side*12));
        }
    }
    auto* Nose=Shape("Cone",FVector(0,-255,780),FVector(1.1f,1.1f,2.2f),Bone,0,AngelRoot);
    Nose->SetRelativeRotation(FRotator(-90,0,0));
    AngelCore=Shape("Sphere",FVector(0,-290,90),FVector(2.9f),FLinearColor(.65f,.015f,.004f),2.2f,AngelRoot);
    EnemyShield=BuildField(FLinearColor(1,.24f,.04f),AngelRoot);
    EnemyShield->SetRelativeLocation(FVector(0,-470,90));
    EnemyShield->SetRelativeRotation(FRotator(0,-90,0));
    SachielRoot=AngelRoot; SachielCore=AngelCore; SachielField=EnemyShield;
}

USceneComponent* AEvaGameMode::BuildField(FLinearColor Color,USceneComponent* Parent)
{
    auto* Root=NewObject<USceneComponent>(Parent->GetOwner());
    Root->SetupAttachment(Parent); Root->RegisterComponent();
    for(int Ring=0;Ring<2;++Ring) for(int I=0;I<6;++I)
    {
        const float R=Ring==0 ? 1.f : .8f;
        float A=I*PI/3.f, B=(I+1)*PI/3.f;
        FVector From(0,FMath::Cos(A)*850*R,FMath::Sin(A)*1000*R);
        FVector To(0,FMath::Cos(B)*850*R,FMath::Sin(B)*1000*R);
        auto* Edge=Shape("Cylinder",(From+To)*.5f,FVector(.14f,.14f,(To-From).Size()/100),Color,2.f,Root);
        Edge->SetRelativeRotation(FRotationMatrix::MakeFromZ(To-From).Rotator());
    }
    return Root;
}

void AEvaGameMode::Pulse(FVector P,FLinearColor Color,float Size)
{
    EffectShape("Sphere",P,FVector(Size*.35f),Color,2.f,.22f,FVector(Size*1.5f));
}

void AEvaGameMode::Attack(bool bRanged,float Charge,bool PlayerAim)
{
    auto* P=Pilot();
    if(!P || !IsActive()) return;
    if(P->DrawTime>0 || P->PickupTime>0 || P->ReloadTime>0 || bWorldMap) return;
    bRanged=P->Loadout.Equipped==EEvaWeapon::Cannon;
    if(P->Loadout.Equipped==EEvaWeapon::Unarmed) { SetNotice("F / DRAW THE PROGRESSIVE KNIFE FROM YOUR SHOULDER"); return; }
    float& Cooldown=bRanged ? LanceCooldown : MeleeCooldown;
    if(Cooldown>0 || P->bGuard) return;
    const float Distance=FVector::Dist2D(P->GetActorLocation(),EnemyPosition);
    if((!bRanged || !PlayerAim) && (!HasCombatTarget() || Distance>(bRanged ? 18000.f : 2300.f))) { SetNotice("OUT OF REACH // CLOSE THE DISTANCE"); return; }
    FVector End=EnemyAimPoint();
    FVector Start=P->CannonRoot->GetComponentTransform().TransformPosition(FVector(1060,0,35));
    bool Hit=HasCombatTarget(), CoreHit=true;
    UPrimitiveComponent* HitStructure=nullptr;
    if(bRanged)
    {
        FHitResult Cover; FCollisionQueryParams Query; Query.AddIgnoredActor(P);
        if(PlayerAim)
        {
            const FVector Eye=P->Camera->GetComponentLocation(), Aim=P->Camera->GetForwardVector();
            float BodyDistance=18000, CoreDistance=18000;
            const bool Body=HasCombatTarget() && FEvaShooter::RaySphere(Eye,Aim,EnemyPosition,bRamiel ? 1400.f : bShamshel ? 1050.f : 760.f,18000,BodyDistance);
            CoreHit=HasCombatTarget() && FEvaShooter::RaySphere(Eye,Aim,EnemyAimPoint(),bRamiel ? 340.f : bShamshel ? 285.f : 220.f,18000,CoreDistance);
            Hit=Body || CoreHit;
            // A visible core receives the precision bonus even where the broad body volume overlaps it.
            End=Eye+Aim*(CoreHit ? CoreDistance : Body ? BodyDistance : 18000.f);
            if(GetWorld()->LineTraceSingleByChannel(Cover,Eye,End,ECC_Visibility,Query)) { End=Cover.ImpactPoint; Hit=false; HitStructure=Cover.GetComponent(); }
            if(GetWorld()->LineTraceSingleByChannel(Cover,Start,End,ECC_Visibility,Query)) { End=Cover.ImpactPoint; Hit=false; HitStructure=Cover.GetComponent(); }
        }
        else if(GetWorld()->LineTraceSingleByChannel(Cover,P->GetActorLocation()+FVector(0,0,450),EnemyPosition,ECC_Visibility,Query))
        { SetNotice("FIRING LINE BLOCKED // MOVE CLEAR OF COVER"); return; }
        if(!P->Loadout.FireCannon(Rules,Charge)) { SetNotice(P->Loadout.Shells==0 ? "CANNON EMPTY // R RELOAD / E SERVICE FOR RESERVES" : "INSUFFICIENT POWER"); return; }
    }
    else if(!Rules.Spend(1.2f)) { SetNotice("INSUFFICIENT POWER"); return; }
    // Cannon rounds that strike a structure damage it; a charged round can fell it outright.
    if(bRanged && HitStructure) if(const int32* Struck=BuildingByComponent.Find(HitStructure)) DamageBuilding(Buildings[*Struck],End,Charge>=.75f ? 2:1);
    P->bCharging=false;
    Cooldown=bRanged ? (Charge>.1f ? 1.1f : .55f) : .55f;
    EvaSound(this,bRanged ? TEXT("Lance") : TEXT("Impact"));
    P->AttackTime=.45f;
    Charge=FMath::Clamp(Charge,0.f,1.f);
    if(!bRanged) { P->KnifeCombo=P->ComboTime>0 ? (P->KnifeCombo%3)+1 : 1; P->ComboTime=1.5f; }
    float KnifeDamage=P->KnifeCombo==3 ? 145.f : P->KnifeCombo==2 ? 105.f : 85.f;
    bool bCore=Hit && Rules.HitEnemy((bRanged ? 145.f*(1+Charge*1.2f)*(CoreHit ? 1.f : .65f) : KnifeDamage)*Systems.DamageScale(),bRanged ? 38.f+Charge*62.f : P->KnifeCombo==3 ? 46.f : 27.f);
    if(!bRanged && P->KnifeCombo==3) SetNotice("PROGRESSIVE KNIFE // FINISHING STRIKE");
    Pulse(End,EvaPalette::Orange,Hit ? (bCore ? 4.f : 2.f) : .8f);
    if(Hit) P->HitMarkerTime=.18f;
    if(Hit && !bCore && Rules.EnemyField<=0)
    {
        VulnerableTime=11.f;
        EvaSound(this,TEXT("Breach"));
        SetNotice("A.T. FIELD COLLAPSED // CORE EXPOSED");
    }
    if(bRanged)
    {
        P->RecoilPitch=FMath::Min(4.f,P->RecoilPitch+.8f+Charge*1.2f);
        FVector Dir=End-Start;
        auto* Beam=EffectShape("Cylinder",(Start+End)*.5f,FVector(.2f+Charge*.5f,.2f+Charge*.5f,Dir.Size()/100.f),FLinearColor(1.f,.65f,.2f),4.f,.12f);
        Beam->SetWorldRotation(FRotationMatrix::MakeFromZ(Dir).Rotator());

    }
    if(Hit && bOpenWorld && Rules.EnemyHealth<=0)
    {
        ResolveWorldVictory();
    }
    else if(Hit && !bOpenWorld && Rules.EnemyHealth<=350) SetChapter(EEvaChapter::Awakening);
}

void AEvaGameMode::Tick(float Dt)
{
    Super::Tick(Dt);
    auto* P=Pilot();
    if(!P) return;
    TickDistrict(Dt);
    if(!bPaused) { TickDestruction(Dt); TickChapter(Dt); TickDepot(Dt); if(Chapter==EEvaChapter::Impact) TickImpact(Dt); }
    if(bChapterAuto || bChapterSmoke) TickChapterAutomation(Dt);
    if(FParse::Param(FCommandLine::Get(),TEXT("EvaMenuShot")))
    {
        if(GetWorld()->GetTimeSeconds()>3.f && !bSmokeCaptured)
        {
            bSmokeCaptured=true;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/EvaMenu.png"),true,false);
        }
        if(GetWorld()->GetTimeSeconds()>6.f) UKismetSystemLibrary::QuitGame(this,nullptr,EQuitPreference::Quit,false);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("EvaAutoBattle")))
    {
        if(bEnded)
        {
            UE_LOG(LogTemp,Display,TEXT("EVA_BATTLE_RESULT: victory=%d health=%.1f reserve=%.1f enemy=%.1f"),bVictory,Rules.Integrity,Rules.Battery,Rules.EnemyHealth);
            FPlatformMisc::RequestExitWithStatus(false,bVictory ? 0 : 1);
            return;
        }
        if(IsActive()) { P->SetActorLocation(FVector(EnemyPosition.X,EnemyPosition.Y-1700,780)); if(P->Loadout.Equipped==EEvaWeapon::Unarmed) P->DrawKnife(); Attack(false); }
        else if(Chapter==EEvaChapter::Hospital && ChapterTime>1) AdvanceStory();
    }
    if(!bPaused)
    {
        for(int32 I=Effects.Num()-1;I>=0;--I)
        {
            auto& E=Effects[I]; E.Remaining-=Dt;
            if(E.Remaining<=0) { E.Mesh->SetVisibility(false); EffectPool.Add(E.Mesh); Effects.RemoveAtSwap(I); }
            else E.Mesh->SetWorldScale3D(E.Mesh->GetComponentScale()+E.Growth*Dt);
        }
    }
    FVector Pos=P->GetActorLocation();
    FVector End=Pos-FVector(0,0,100), Delta=End-CableAnchor();
    Cable->SetVisibility(IsActive() && Rules.bConnected);
    Cable->SetWorldLocation((CableAnchor()+End)*.5f);
    Cable->SetWorldRotation(FRotationMatrix::MakeFromZ(Delta).Rotator());
    Cable->SetWorldScale3D(FVector(.12f,.12f,Delta.Size()/100.f));
    PlayerShield->SetVisibility(IsActive() && P->bGuard,true);
    PlayerShield->SetWorldLocation(Pos+P->GetActorForwardVector()*380);
    PlayerShield->SetWorldRotation(P->GetActorRotation());
    PlayerShield->SetWorldScale3D(FVector(1));
    if(bDynamicTest) TickDynamicTest(Dt);
    if(bCompanionTest) TickCompanionTest(Dt);
    if(bDistrictTest) TickDistrictTest(Dt);
    if(!IsActive()) return;
    MissionTime+=Dt;
    if(FParse::Param(FCommandLine::Get(),TEXT("EvaSystemsTest")))
    {
        static int Step=0;
        static bool OK=true;
        static float StepTime=0;
        StepTime+=Dt;
        if(Step==4 && StepTime>.8f && StepTime-Dt<=.8f)
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/AdvancedSystems.png"),true,false);
        if(Step==0)
        {
            Rules.Battery=120; Rules.Sync=80;
            P->SetActorLocation(DepotApproach); Interact(); Step=1; StepTime=0;
        }
        else if(Step==1 && DepotOpen>=1) { Interact(); Step=2; StepTime=0; }
        else if(Step==2 && P->PickupTime<=0)
        {
            const float Power=Rules.Battery; const int Shells=P->Loadout.Shells;
            for(const auto& B:Buildings) if(B.Center.Z>1200)
            {
                P->SetActorLocation(FVector(B.Center.X,B.Center.Y-2400,780));
                EnemyPosition=FVector(B.Center.X,B.Center.Y+2400,1600);
                Attack(true); OK=OK && P->Loadout.Shells==Shells && Rules.Battery==Power;
                break;
            }
            EnemyPosition=FVector(0,4300,1600);
            P->SetActorLocation(FVector(0,-2300,780)); P->Lance(); OK=OK && P->bCharging; Step=3; StepTime=0;
        }
        else if(Step==3 && P->CannonCharge>=1)
        {
            P->Camera->SetWorldRotation((EnemyAimPoint()-P->Camera->GetComponentLocation()).Rotation());
            P->ReleaseCharge(); OK=OK && P->Loadout.Shells==7 && Rules.EnemyField==0;
            P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
            P->SetActorLocation(EnemyPosition+FVector(0,-2800,-820));
            Rules.EnemyField=100; UseOverdrive(); UseAntiField();
            OK=OK && Systems.Overdrive>0 && Systems.PulseCooldown>0 && Rules.EnemyField==0;
            P->CameraYaw=90; P->CameraPitch=-16;
            Step=4; StepTime=0;
        }
        else if(Step==4 && StepTime>1.5f)
        {
            Attack(true); OK=OK && Rules.EnemyHealth<800 && Rules.EnemyHealth>700;
            Rules.EnemyHealth=5000; P->Loadout.DrawKnife(); P->DrawTime=0; P->ComboTime=0;
            P->SetActorLocation(EnemyPosition+FVector(0,-1600,-820));
            for(int I=0;I<3;++I) { MeleeCooldown=0; Attack(false); }
            OK=OK && P->KnifeCombo==3 && FMath::IsNearlyEqual(Rules.EnemyHealth,4464.f,.1f);
            P->SetActorLocation(Chargers[0]+FVector(0,500,680)); Interact();
            P->SetActorLocation(Chargers[0]+FVector(6400,0,680)); Step=5; StepTime=0;
        }
        else if(Step==5 && StepTime>2.2f)
        {
            OK=OK && !Rules.bConnected;
            StartBattle(); OK=OK && Systems.Overdrive==0 && Systems.PulseCooldown==0 && !P->bCharging;
            UE_LOG(LogTemp,Display,TEXT("EVA_SYSTEMS_RESULT success=%d charged=1 overdrive=1 pulse=1 cable=1 cover=1 combo=1 reset=1"),OK);
            FPlatformMisc::RequestExitWithStatus(false,OK ? 0:1);
        }
        if(MissionTime>30) { UE_LOG(LogTemp,Error,TEXT("EVA_SYSTEMS_TIMEOUT step=%d"),Step); FPlatformMisc::RequestExitWithStatus(false,1); }
    }
    MeleeCooldown=FMath::Max(0.f,MeleeCooldown-Dt);
    LanceCooldown=FMath::Max(0.f,LanceCooldown-Dt);
    HitFlash=FMath::Max(0.f,HitFlash-Dt);
    NoticeTime=FMath::Max(0.f,NoticeTime-Dt);
    Rules.AdvancePower(bOpenWorld && !bWorldEncounter && !Rules.bConnected ? Dt*.25f : Dt,P->bGuard);
    TickSystems(Dt);
    if(bOpenWorld) TickOpenWorld(Dt);
    if(bShooterTest) TickShooterTest(Dt);

    if(Rules.Integrity<=0 || Rules.Battery<=0) { bEnded=true; bVictory=false; ThreatRing->SetVisibility(false); return; }
    if(!HasCombatTarget()) return;
    if(bRamiel) { TickRamiel(Dt); return; }
    if(bShamshel) { TickShamshel(Dt); return; }
    EnemyPosition.Z=1600+FMath::Sin(MissionTime*1.2f)*35;
    if(Telegraph<=0 && FVector::Dist2D(Pos,EnemyPosition)>1850)
    {
        FVector D=(Pos-EnemyPosition).GetSafeNormal2D();
        EnemyPosition+=D*Dt*(Rules.EnemyHealth<500 ? 530.f : 350.f);
    }
    AngelRoot->SetWorldLocation(EnemyPosition);
    // Core always faces the pilot, so damage feedback stays readable while circling.
    AngelRoot->SetWorldRotation(FRotator(0,(Pos-EnemyPosition).Rotation().Yaw+90,0));
    for(int32 I=0;I<AngelLimbs.Num();++I) AngelLimbs[I]->SetRelativeRotation(FRotator(FMath::Sin(MissionTime*3+I*PI)*9,0,0));
    EnemyShield->SetVisibility(Rules.EnemyField>0,true);
    if(Rules.EnemyField<=0)
    {
        VulnerableTime-=Dt;
        if(VulnerableTime<=0) { Rules.EnemyField=100; SetNotice("ENEMY FIELD REGENERATED"); }
    }
    EnemyClock-=Dt;
    if(EnemyClock<=0 && Telegraph<=0)
    {
        ++AttackCount;
        ThreatPosition=Pos; ThreatPosition.Z=20;
        Telegraph=Rules.EnemyHealth<500 ? 1.1f : 1.75f;
        ThreatRing->SetVisibility(true);
        EvaSound(this,TEXT("Alarm"),.35f);
        ThreatRing->SetWorldLocation(ThreatPosition);
        SetNotice(AttackCount%3==0 ? "COMMAND // HIGH-ENERGY STRIKE. EVADE NOW." : "PATTERN DETECTED // LEAVE THE RED ZONE");
    }
    else if(Telegraph>0)
    {
        Telegraph-=Dt;
        const float Radius=AttackCount%3==0 ? 1750.f : 1300.f;
        ThreatRing->SetWorldScale3D(FVector(Radius/50,Radius/50,.08f));
        if(Telegraph<=0)
        {
            ThreatRing->SetVisibility(false);
            Pulse(ThreatPosition+FVector(0,0,500),EvaPalette::Orange,15.f);
            DestroyNearby(ThreatPosition,Radius);
            if(FVector::Dist2D(Pos,ThreatPosition)<Radius)
            {
                FVector TowardEnemy=(EnemyPosition-Pos).GetSafeNormal2D();
                const bool bFacing=FVector::DotProduct(P->GetActorForwardVector(),TowardEnemy)>.25f;
                Rules.ReceiveHit(AttackCount%3==0 ? 32.f : 20.f,P->bGuard && bFacing,P->DodgeTime>0 || (!P->bGrounded && Pos.Z>1450));
                if(P->DodgeTime<=0 && (P->bGrounded || Pos.Z<=1450)) HitFlash=.3f;
            }
            EnemyClock=Rules.EnemyHealth<500 ? 2.f : 3.2f;
        }
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("EvaSmoke")))
    {
        if(MissionTime>3.f && !bSmokeCaptured)
        {
            bSmokeCaptured=true;
            UE_LOG(LogTemp,Display,TEXT("EVA_RENDER: pawn=%s camera=%s meshes=%d firstVisible=%d hidden=%d"),*P->GetActorLocation().ToString(),*P->Camera->GetComponentLocation().ToString(),Meshes.Num(),Meshes[0]->IsVisible(),IsHidden());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/EvaSmoke.png"),true,false);
        }
        if(MissionTime>10.f) { UE_LOG(LogTemp,Display,TEXT("EVA_SMOKE_OK: pawn, city, HUD and encounter running")); UKismetSystemLibrary::QuitGame(this,nullptr,EQuitPreference::Quit,false); }
    }
}
