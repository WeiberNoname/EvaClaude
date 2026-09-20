#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

static void MakeCannon(AEvaGameMode* G,USceneComponent* Root)
{
    const FLinearColor Steel(.13f,.19f,.23f), Dark(.025f,.035f,.05f), Accent(.64f,.82f,.1f);
    G->Shape("Cube",FVector(230,0,0),FVector(6,2.3f,2.8f),Steel,0,Root);
    G->Shape("Cube",FVector(-90,0,-60),FVector(2.8f,1.7f,1.3f),Dark,0,Root);
    G->Shape("Cube",FVector(200,0,-220),FVector(2,1.4f,2.4f),Dark,0,Root);
    auto* Barrel=G->Shape("Cylinder",FVector(720,0,35),FVector(1.15f,1.15f,6.5f),Steel,0,Root);
    Barrel->SetRelativeRotation(FRotator(90,0,0));
    auto* Muzzle=G->Shape("Cylinder",FVector(1050,0,35),FVector(1.65f,1.65f,1.f),Dark,0,Root);
    Muzzle->SetRelativeRotation(FRotator(90,0,0));
    G->Shape("Cube",FVector(200,-119,70),FVector(4.5f,.05f,.3f),Accent,1.4f,Root);
    G->Shape("Cube",FVector(200,119,70),FVector(4.5f,.05f,.3f),Accent,1.4f,Root);
    G->Shape("Cube",FVector(170,0,175),FVector(2.5f,.55f,.45f),Dark,0,Root);
    for(int32 I=0;I<4;++I) G->Shape("Cube",FVector(450+I*100,0,115),FVector(.35f,1.5f,.3f),Dark,0,Root);
}

void AEvaPawn::BuildEquipment()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(!G) return;
    KnifeRoot=G->NewSceneRoot(FVector::ZeroVector);
    KnifeRoot->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
    G->Shape("Cube",FVector(0,0,0),FVector(.55f,.55f,1.5f),FLinearColor(.02f,.03f,.04f),0,KnifeRoot);
    G->Shape("Cube",FVector(0,0,80),FVector(1.3f,.6f,.2f),FLinearColor(.15f,.2f,.25f),0,KnifeRoot);
    G->Shape("Cube",FVector(0,0,340),FVector(.75f,.22f,5.f),FLinearColor(.62f,.72f,.8f),.35f,KnifeRoot);
    auto* Tip=G->Shape("Cone",FVector(0,0,660),FVector(.75f,.22f,1.5f),FLinearColor(.75f,.87f,.92f),.6f,KnifeRoot);
    G->Shape("Cube",FVector(40,0,340),FVector(.06f,.26f,5.2f),FLinearColor(.58f,1.f,.12f),2.f,KnifeRoot);
    KnifeRoot->SetVisibility(false,true);
    CannonRoot=G->NewSceneRoot(FVector::ZeroVector);
    CannonRoot->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
    MakeCannon(G,CannonRoot);
    CannonRoot->SetRelativeScale3D(FVector(.65f));
    KnifeRoot->SetRelativeScale3D(FVector(.75f));
    CannonRoot->SetVisibility(false,true);
    BuildCockpit();
}

void AEvaPawn::SetHumanMode(bool bEnabled)
{
    bHuman=bEnabled; bFirstPerson=false; bGrounded=true; VerticalSpeed=0; LandingKick=0; DashBuffer=0; Motion=FEvaMotion();
    if(CockpitRoot) CockpitRoot->SetVisibility(false,true);
    if(EvaRig) EvaRig->SetVisibility(!bEnabled,true);
    for(auto* M:BodyMeshes) M->SetVisibility(!bEnabled);
    Capsule->SetCapsuleSize(bEnabled ? 25.f : 145.f,bEnabled ? 90.f : 760.f);
    Boom->TargetArmLength=bEnabled ? 0.f : 2950.f;
    Boom->TargetOffset=FVector(0,0,bEnabled ? 65.f : 680.f);
    Boom->SocketOffset=FVector::ZeroVector;
    Camera->SetRelativeRotation(FRotator::ZeroRotator);
    Camera->SetFieldOfView(75);
    bFireHeld=false; bAiming=false; bCharging=false; CannonCharge=0; ReloadTime=0; MoveVelocity=FVector::ZeroVector;
    Boom->bDoCollisionTest=!bEnabled;
    bLock=!bEnabled;
    if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) if(G->bOpenWorld) bLock=false;
    CameraPitch=bEnabled ? 0.f : -16.f;
    CameraYaw=90;
    bGuard=false;
    if(KnifeRoot) KnifeRoot->SetVisibility(false,true);
    if(CannonRoot) CannonRoot->SetVisibility(!bEnabled && Loadout.bHasCannon,true);
}

void AEvaPawn::DrawKnife()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(!G || !G->IsActive() || DrawTime>0 || PickupTime>0) return;
    bCharging=false; CannonCharge=0; ReloadTime=0; bFireHeld=false;
    if(Loadout.Equipped==EEvaWeapon::Knife)
    {
        Loadout.Equipped=EEvaWeapon::Unarmed;
        KnifeRoot->SetVisibility(false,true);
        G->SetNotice("PROGRESSIVE KNIFE STOWED");
        return;
    }
    Loadout.DrawKnife();
    DrawTime=.45f;
    KnifeRoot->SetVisibility(true,true);
    G->SetNotice("SHOULDER LOCK RELEASED // DRAWING PROGRESSIVE KNIFE");
}

void AEvaPawn::EquipCannon()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(!G || !G->IsActive() || DrawTime>0 || PickupTime>0) return;
    if(!Loadout.EquipCannon()) { G->SetNotice("REQUEST THE CANNON AT ARMORY 07 // E TO INTERACT"); return; }
    bCharging=false; CannonCharge=0; bFireHeld=false;
    KnifeRoot->SetVisibility(false,true);
    G->SetNotice("HEAVY CANNON READY");
}

void AEvaPawn::ReleaseCable()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(G && G->IsActive() && G->Rules.bConnected) { G->Rules.bConnected=false; G->SetNotice("UMBILICAL RELEASED // INTERNAL RESERVE"); }
}

void AEvaPawn::SkipToBattle()
{
    if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) G->StartBattle();
}

void AEvaPawn::AnimateEquipment(float Dt)
{
    if(!KnifeRoot || !CannonRoot) return;
    if(bHuman) { KnifeRoot->SetVisibility(false,true); CannonRoot->SetVisibility(false,true); return; }
    if(DrawTime>0)
    {
        DrawTime=FMath::Max(0.f,DrawTime-Dt);
        float T=1.f-DrawTime/.45f;
        FVector Position=T<.45f ? FMath::Lerp(FVector(0,265,470),FVector(30,300,850),T/.45f)
            : FMath::Lerp(FVector(30,300,850),FVector(230,340,-50),(T-.45f)/.55f);
        KnifeRoot->SetRelativeLocation(Position);
        KnifeRoot->SetRelativeRotation(FRotator(T> .45f ? (T-.45f)*70.f : 0,0,0));
        if(ShoulderHatch) ShoulderHatch->SetRelativeRotation(FRotator(0,0,FMath::Sin(T*PI)*-42));
    }
    else
    {
        KnifeRoot->SetRelativeLocation(FVector(230,340,-50));
        KnifeRoot->SetRelativeRotation(FRotator(AttackTime>0 ? -80*FMath::Sin(AttackTime*7) : 22,0,0));
        KnifeRoot->SetVisibility(Loadout.Equipped==EEvaWeapon::Knife && !bFirstPerson,true);
        if(ShoulderHatch) ShoulderHatch->SetRelativeRotation(FRotator::ZeroRotator);
    }
    CannonRoot->SetVisibility(Loadout.bHasCannon && !bFirstPerson,true);
    if(PickupTime>0)
    {
        PickupTime=FMath::Max(0.f,PickupTime-Dt);
        const float T=1.f-PickupTime/1.1f;
        const FVector Hand=GetActorTransform().TransformPosition(FVector(420,280,270));
        CannonRoot->SetWorldLocation(FMath::Lerp(PickupStart,Hand,FMath::SmoothStep(0.f,1.f,T)));
        CannonRoot->SetWorldRotation(GetActorRotation());
    }
    else if(Loadout.Equipped==EEvaWeapon::Cannon)
    {
        CannonRoot->SetRelativeLocation(FVector(420-AttackTime*100,280,270));
        if(ReloadTime>0) CannonRoot->SetRelativeRotation(FRotator(-25,0,35*FMath::Sin(ReloadTime/1.3f*PI)));
        else CannonRoot->SetWorldRotation(FRotator((bAiming || bCharging || bFireHeld) ? CameraPitch : 0,GetActorRotation().Yaw,0));
    }
    else
    {
        CannonRoot->SetRelativeLocation(FVector(-230,-90,280));
        CannonRoot->SetRelativeRotation(FRotator(-78,0,0));
    }
}

void AEvaGameMode::BuildDepot()
{
    auto* Root=NewSceneRoot(DepotPosition);
    FLinearColor Wall(.055f,.075f,.10f), Trim(.85f,.4f,.07f);
    Shape("Cube",FVector(-1200,0,1400),FVector(2,20,28),Wall,0,Root,true);
    Shape("Cube",FVector(1200,0,1400),FVector(2,20,28),Wall,0,Root,true);
    Shape("Cube",FVector(0,1000,1400),FVector(26,2,28),Wall,0,Root,true);
    Shape("Cube",FVector(0,0,2850),FVector(26,22,1),Wall,0,Root,true);
    Shape("Cube",FVector(0,-1040,2650),FVector(25,.2f,.5f),Trim,3,Root);
    DepotLeft=Shape("Cube",FVector(-550,-1050,1320),FVector(11,1,26),FLinearColor(.1f,.14f,.19f),0,Root);
    DepotRight=Shape("Cube",FVector(550,-1050,1320),FVector(11,1,26),FLinearColor(.1f,.14f,.19f),0,Root);
    // Door panels stay query-free so an interrupted opening can never trap the player.
    for(int32 I=0;I<5;++I)
        Shape("Cube",FVector(-800+I*400,-1600,8),FVector(2,7,.1f),Trim,1.3f,Root);
    DepotLift=Shape("Cube",FVector(0,0,200),FVector(15,13,1.3f),FLinearColor(.13f,.17f,.2f),0,Root);
    DepotGunRoot=NewSceneRoot(DepotPosition+FVector(0,0,450));
    DepotGunRoot->SetWorldRotation(FRotator(0,-90,0));
    MakeCannon(this,DepotGunRoot);
    // Second power station extends the usable battlefield without infinite reserve.
    Chargers={Anchor,FVector(-1600,1800,100)};
    for(int32 I=0;I<Chargers.Num();++I)
    {
        FVector A=Chargers[I];
        if(I>0)
        {
            Shape("Cylinder",A,FVector(11,11,1.4f),FLinearColor(.04f,.07f,.08f));
            Shape("Cube",A+FVector(0,0,550),FVector(2.2f,2.2f,11),FLinearColor(.08f,.14f,.13f));
        }
        ChargerLamps.Add(Shape("Sphere",A+FVector(0,0,1280),FVector(1.6f),FLinearColor(.3f,1,.12f),2.5f));
        for(int J=0;J<3;++J) Shape("Cube",A+FVector(0,0,320+J*240),FVector(3.5f,3.5f,.3f),FLinearColor(.2f,.8f,.6f),1.5f);
    }
}

void AEvaGameMode::TickDepot(float Dt)
{
    if(bDepotOpening) DepotOpen=FMath::Min(1.f,DepotOpen+Dt/2.6f);
    float T=FMath::SmoothStep(0.f,1.f,DepotOpen);
    DepotLeft->SetRelativeLocation(FVector(-550-T*1050,-1050,1320));
    DepotRight->SetRelativeLocation(FVector(550+T*1050,-1050,1320));
    DepotLift->SetRelativeLocation(FVector(0,0,200+T*950));
    DepotGunRoot->SetWorldLocation(DepotPosition+FVector(0,0,450+T*950));
    DepotGunRoot->SetVisibility(!bCannonTaken,true);
    for(int I=0;I<ChargerLamps.Num();++I)
        ChargerLamps[I]->SetWorldScale3D(FVector(Rules.bConnected && ActiveCharger==I ? 1.7f+FMath::Sin(GetWorld()->TimeSeconds*5)*.2f : 1.1f));
}

FVector AEvaGameMode::CableAnchor() const
{ return Chargers.IsValidIndex(ActiveCharger) ? Chargers[ActiveCharger] : Anchor; }

FString AEvaGameMode::InteractionPrompt() const
{
    auto* P=Pilot(); if(!P) return TEXT("");
    if(Chapter==EEvaChapter::Street)
    {
        if(!bPhoneUsed && FVector::Dist2D(P->GetActorLocation(),PhonePosition)<300) return "E / TRY THE TELEPHONE";
        if(bPhoneUsed && FVector::Dist2D(P->GetActorLocation(),CarPosition)<420) return "E / GET INTO MISATO'S CAR";
        return "";
    }
    if(!IsActive()) return "";
    if(bOpenWorld) return WorldPrompt();
    for(int I=0;I<Chargers.Num();++I)
        if(FVector::Dist2D(P->GetActorLocation(),Chargers[I])<1300)
            return Rules.bConnected && ActiveCharger==I ? "CHARGING // C TO RELEASE CABLE" : "E / CONNECT UMBILICAL + RECHARGE";
    if(FVector::Dist2D(P->GetActorLocation(),DepotApproach)<1600)
    {
        if(!bDepotOpening) return "E / OPEN ARMORY 07";
        if(DepotOpen<1) return "ARMORED DOORS OPENING // CANNON RACK RISING";
        return bCannonTaken ? "E / RELOAD CANNON" : "E / PULL CANNON FROM THE RACK";
    }
    return "";
}

void AEvaGameMode::Interact()
{
    auto* P=Pilot(); if(!P || bPaused || bEnded) return;
    if(Chapter==EEvaChapter::Street)
    {
        if(!bPhoneUsed && FVector::Dist2D(P->GetActorLocation(),PhonePosition)<300)
        { bPhoneUsed=true; ChapterTime=0; SetNotice("NO CONNECTION. A CAR IS APPROACHING FROM THE LEFT."); return; }
        if(bPhoneUsed && FVector::Dist2D(P->GetActorLocation(),CarPosition)<420) { SetChapter(EEvaChapter::Rescue); return; }
    }
    if(Chapter==EEvaChapter::Decision) { AdvanceStory(); return; }
    if(!IsActive()) return;
    if(bOpenWorld) { WorldInteract(); return; }
    for(int I=0;I<Chargers.Num();++I)
    {
        if(FVector::Dist2D(P->GetActorLocation(),Chargers[I])<1300)
        {
            ActiveCharger=I; Rules.bConnected=true;
            SetNotice("UMBILICAL LOCKED // EXTERNAL POWER RESTORED");
            return;
        }
    }
    if(FVector::Dist2D(P->GetActorLocation(),DepotApproach)<1600)
    {
        if(!bDepotOpening) { bDepotOpening=true; SetNotice("ARMORY 07 AUTHORIZED // STAND BY FOR WEAPON DELIVERY"); return; }
        if(DepotOpen<1) { SetNotice("WEAPON ELEVATOR IN MOTION"); return; }
        if(P->PickupTime>0) return;
        if(!bCannonTaken)
        {
            bCannonTaken=true;
            P->Loadout.AcquireCannon();
            P->PickupStart=DepotGunRoot->GetComponentLocation();
            P->PickupTime=1.1f;
            SetNotice("CANNON ACQUIRED // LMB TO FIRE / F TO DRAW KNIFE");
        }
        else if(P->Loadout.bHasCannon)
        { P->Loadout.AcquireCannon(); P->ReloadTime=0; P->EquipCannon(); SetNotice("CANNON RESUPPLIED // 8 LOADED / 24 RESERVE"); }
        return;
    }
    SetNotice("APPROACH AN AMBER ARMORY OR A GREEN POWER STATION");
}
