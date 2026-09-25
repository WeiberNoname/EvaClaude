#include "EvaGame.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AEvaWingman::AEvaWingman()
{
    PrimaryActorTick.bCanEverTick=true;
    Capsule=CreateDefaultSubobject<UCapsuleComponent>(TEXT("WingmanCollision")); RootComponent=Capsule;
    Capsule->InitCapsuleSize(155,760); Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Capsule->SetCollisionObjectType(ECC_Pawn); Capsule->SetCollisionResponseToAllChannels(ECR_Block);
    Capsule->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore); Capsule->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore); Capsule->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
    Rig=CreateDefaultSubobject<USceneComponent>(TEXT("Unit02Armor")); Rig->SetupAttachment(Capsule);
}
void AEvaWingman::BeginPlay()
{
    Super::BeginPlay(); auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()); if(!G) return;
    G->BuildUnit(Rig,true,Parts,Limbs,Knees,Hatch);
    Rifle=G->NewSceneRoot(FVector::ZeroVector); Rifle->AttachToComponent(Rig,FAttachmentTransformRules::KeepRelativeTransform);
    Rifle->SetRelativeLocation(FVector(470,290,270));
    G->Shape("ArmorPlate",FVector::ZeroVector,FVector(4.8f,1.1f,1.6f),FLinearColor(.08f,.1f,.12f),0,Rifle);
    G->Shape("Cube",FVector(200,0,0),FVector(2.7f,.66f,.65f),FLinearColor(.12f,.16f,.19f),0,Rifle);
    G->Shape("Cube",FVector(70,0,-92),FVector(.8f,.7f,1.4f),FLinearColor(.04f,.045f,.05f),0,Rifle);
}
void AEvaWingman::Deploy(FVector Position)
{
    bDeployed=true; Integrity=100; Order=EEvaOrder::Follow; Velocity=FVector::ZeroVector;
    FireCooldown=1; EvadeTime=0; PreviousTelegraph=0; ShotsFired=0;
    FRotator Rotation(0,90,0); GetWorld()->FindTeleportSpot(this,Position,Rotation);
    SetActorLocationAndRotation(Position,Rotation); HoldPosition=Position; SetActorHiddenInGame(false);
}
FString AEvaWingman::OrderName() const
{
    if(Integrity<=0) return "DISABLED / E SERVICE TO REPAIR";
    switch(Order) { case EEvaOrder::Hold:return "HOLDING"; case EEvaOrder::Assault:return "ENGAGING"; case EEvaOrder::Regroup:return "REGROUPING"; default:return "FOLLOWING"; }
}
void AEvaWingman::Tick(float Dt)
{
    Super::Tick(Dt); auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()); if(!G) return;
    const bool Visible=bDeployed && G->bOpenWorld && G->Chapter==EEvaChapter::Battle;
    if(IsHidden()==Visible) SetActorHiddenInGame(!Visible);
    if(!Visible || !G->IsActive()) return;
    auto* P=G->Pilot(); if(!P) return;
    if(Integrity<=0) { Rig->SetRelativeRotation(FRotator(-22,0,0)); Velocity=FVector::ZeroVector; return; }
    FireCooldown=FMath::Max(0.f,FireCooldown-Dt); EvadeTime=FMath::Max(0.f,EvadeTime-Dt);
    FVector Position=GetActorLocation(), Goal=HoldPosition;
    const bool Combat=G->bWorldEncounter;
    const FVector ToEnemy=(G->EnemyPosition-Position).GetSafeNormal2D();
    if(Order!=EEvaOrder::Hold)
    {
        Goal=P->GetActorLocation()-P->GetActorForwardVector()*1250+P->GetActorRightVector()*1450;
        if(Combat && Order==EEvaOrder::Assault) Goal=G->EnemyPosition-ToEnemy*4600+FVector(-ToEnemy.Y,ToEnemy.X,0)*1800;
        if(Order==EEvaOrder::Regroup && FVector::Dist2D(Position,Goal)<700) Order=EEvaOrder::Follow;
    }
    Goal.Z=780;
    if(G->Telegraph>0 && Combat)
    {
        bool Threat=G->bRamiel || FVector::Dist2D(Position,G->ThreatPosition)<2600;
        if(Threat) { EvadeTime=.35f; Goal=Position+FVector(-ToEnemy.Y,ToEnemy.X,0)*2600; }
    }
    if(PreviousTelegraph>0 && G->Telegraph<=0 && Combat)
    {
        bool Hit=G->bRamiel ? FMath::PointDistToLine(Position+FVector(0,0,180),(G->BeamAim-G->EnemyPosition).GetSafeNormal(),G->EnemyPosition)<440 : FVector::Dist2D(Position,G->ThreatPosition)<(G->bShamshel ? 2100:1750);
        FHitResult Cover; FCollisionQueryParams Query; Query.AddIgnoredActor(this); Query.AddIgnoredActor(P);
        if(Hit && !GetWorld()->LineTraceSingleByChannel(Cover,G->EnemyPosition,Position+FVector(0,0,300),ECC_Visibility,Query)) Integrity=FMath::Max(0.f,Integrity-(EvadeTime>0 ? 3.f:18.f));
    }
    PreviousTelegraph=G->Telegraph;
    if(!Combat && FVector::Dist2D(Position,P->GetActorLocation())>18000)
    {
        FRotator Rotation=GetActorRotation(); if(GetWorld()->FindTeleportSpot(this,Goal,Rotation)) { SetActorLocation(Goal); Velocity=FVector::ZeroVector; } return;
    }
    FVector Direction=(Goal-Position).GetSafeNormal2D();
    if(FVector::Dist2D(Position,Goal)<280) Direction=FVector::ZeroVector;
    if(FVector::Dist2D(Position,P->GetActorLocation())<900) Direction=(Position-P->GetActorLocation()).GetSafeNormal2D();
    // Sweep the same capsule used for movement; try both sides before sliding along cover.
    FCollisionQueryParams Query; Query.AddIgnoredActor(this); Query.AddIgnoredActor(P);
    auto Clear=[&](FVector D) { FHitResult Hit; return !GetWorld()->SweepSingleByChannel(Hit,Position,Position+D*1000,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeCapsule(175,735),Query); };
    if(!Direction.IsNearlyZero() && !Clear(Direction))
    {
        FVector Left=Direction.RotateAngleAxis(70,FVector::UpVector), Right=Direction.RotateAngleAxis(-70,FVector::UpVector);
        Direction=Clear(Left) ? Left:Clear(Right) ? Right:FVector::ZeroVector;
    }
    const float Speed=EvadeTime>0 ? 4600:Order==EEvaOrder::Regroup ? 3600:2600;
    FVector Delta=FEvaMotion::Integrate(Velocity,Direction*Speed,13,Dt); Delta.Z=0;
    FHitResult Hit; AddActorWorldOffset(Delta,true,&Hit);
    if(Hit.bBlockingHit) AddActorWorldOffset(FVector::VectorPlaneProject(Delta*(1-Hit.Time),Hit.Normal),true);
    FVector Face=Combat ? ToEnemy:Direction;
    if(!Face.IsNearlyZero()) SetActorRotation(FMath::Lerp(GetActorRotation().Quaternion(),Face.Rotation().Quaternion(),FEvaMotion::Blend(10,Dt)));
    const float Stride=FMath::Min(Velocity.Size()/2200.f,1.5f); WalkPhase+=Dt*8*Stride;
    Rig->SetRelativeRotation(FRotator(-Stride*3,0,EvadeTime>0 ? 7:0));
    for(int I=0;I<Limbs.Num();++I) Limbs[I]->SetRelativeRotation(FRotator(I%2==0 && Combat ? 65: FMath::Sin(WalkPhase+(I<2 ? 0:PI))*26*Stride,0,0));
    for(int I=0;I<Knees.Num();++I) Knees[I]->SetRelativeRotation(FRotator(-FMath::Max(0.f,FMath::Sin(WalkPhase+I*PI))*35*Stride,0,0));
    Rifle->SetVisibility(Combat,true);
    if(Combat && Order!=EEvaOrder::Regroup && Order!=EEvaOrder::Hold && FireCooldown<=0 && FVector::Dist2D(GetActorLocation(),G->EnemyPosition)<16000)
    {
        FVector Start=Rifle->GetComponentTransform().TransformPosition(FVector(360,0,0)), End=G->EnemyAimPoint();
        FHitResult Cover;
        if(!GetWorld()->LineTraceSingleByChannel(Cover,Start,End,ECC_Visibility,Query))
        {
            ++ShotsFired; FireCooldown=Order==EEvaOrder::Assault ? 1.25f:1.9f;
            const float Before=G->Rules.EnemyField;
            G->Rules.HitEnemy(Order==EEvaOrder::Assault ? 62:42,12);
            if(Before>0 && G->Rules.EnemyField<=0) G->VulnerableTime=8;
            // Unit-02's rounds splash against the Angel's field the same way the player's do.
            if(Before>0)
            {
                End=G->FieldImpact(G->EnemyShield,Start,End,.75f);
                if(G->Rules.EnemyField<=0) { G->FieldBurst(G->EnemyShield,0,End); G->WatchedField=0; }
            }
            auto* Beam=G->EffectShape("Cylinder",(Start+End)*.5f,FVector(.18f,.18f,(End-Start).Size()/100),FLinearColor(1,.18f,.025f),3,.1f);
            Beam->SetWorldRotation(FRotationMatrix::MakeFromZ(End-Start).Rotator()); G->Pulse(End,FLinearColor(1,.19f,.03f),1.2f);
            G->ResolveWorldVictory();
        }
        else FireCooldown=.25f;
    }
}
void AEvaGameMode::DeployWingman()
{
    if(bWorldTest || bShooterTest || bDynamicTest) { if(Wingman) Wingman->bDeployed=false; return; }
    if(!Wingman) Wingman=GetWorld()->SpawnActor<AEvaWingman>();
    if(Wingman && Pilot()) Wingman->Deploy(Pilot()->GetActorLocation()+FVector(1600,-500,0));
}
void AEvaGameMode::ResolveWorldVictory()
{
    if(!bOpenWorld || !bWorldEncounter || Rules.EnemyHealth>0) return;
    bWorldEncounter=false; Pilot()->bLock=false; ++CompletedContracts; SaveWorldProgress();
    AngelRoot->SetVisibility(false,true); EnemyShield->SetVisibility(false,true); Telegraph=0; ClearAngelHazards();
    Rules.Battery=FMath::Min(120.f,Rules.Battery+35); Rules.Integrity=FMath::Min(100.f,Rules.Integrity+25);
    SetNotice("CONTRACT COMPLETE // PROGRESS SAVED / FREE EXPLORATION RESUMED");
}
