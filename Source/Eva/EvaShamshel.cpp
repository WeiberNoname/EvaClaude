#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"

void AEvaGameMode::BuildShamshel()
{
    ShamshelRoot=NewSceneRoot(FVector::ZeroVector);
    const FLinearColor Red(.38f,.045f,.038f), Armor(.64f,.15f,.105f), Bone(.76f,.74f,.65f), Dark(.035f,.012f,.02f);
    Shape("ArmorTorso",FVector(0,0,80),FVector(10,6.5f,23),Red,0,ShamshelRoot);
    Shape("Sphere",FVector(0,-90,1080),FVector(13,6.5f,5),Bone,0,ShamshelRoot);
    Shape("ArmorPlate",FVector(0,-180,1310),FVector(12,3.5f,1.5f),Bone,0,ShamshelRoot);
    for(int S:{-1,1})
    {
        Shape("Sphere",FVector(S*380,-364,1120),FVector(1.8f,.3f,1.6f),Dark,0,ShamshelRoot);
        Shape("Sphere",FVector(S*380,-382,1120),FVector(.45f,.16f,.85f),FLinearColor(1,.2f,.04f),2,ShamshelRoot);
        Shape("ArmorPlate",FVector(S*515,0,480),FVector(3,6,7),Armor,0,ShamshelRoot)->SetRelativeRotation(FRotator(0,0,S*22));
        for(int I=0;I<6;++I)
        {
            auto* Rib=Shape("ArmorPlate",FVector(S*(330-I*27),-285,-I*130+250),FVector(2.9f-I*.2f,.75f,.65f),Armor,0,ShamshelRoot);
            Rib->SetRelativeRotation(FRotator(0,0,S*24));
            Shape("Cone",FVector(S*(350-I*30),80,-I*130+200),FVector(.7f,.7f,3.4f),Red,0,ShamshelRoot)->SetRelativeRotation(FRotator(0,0,S*70));
        }
        for(int I=0;I<18;++I) WhipSegments.Add(Shape("Cylinder",FVector(S*600,-200,300),FVector(.5f,.5f,1),FLinearColor(1,.12f,.42f),3,ShamshelRoot));
        WhipTelegraphs.Add(Shape("Cube",FVector::ZeroVector,FVector(1),FLinearColor(1,.05f,.04f),1.3f));
    }
    for(int I=0;I<7;++I)
        Shape("ArmorPlate",FVector(0,80+I*80,-750-I*175),FVector(3.7f-I*.4f,3.5f-I*.35f,2.5f),I%2 ? Red:Armor,0,ShamshelRoot);
    ShamshelCore=Shape("Sphere",FVector(0,-400,540),FVector(4.8f,3,4.8f),FLinearColor(.75f,.008f,.015f),2,ShamshelRoot);
    ShamshelField=BuildField(FLinearColor(1,.3f,.09f),ShamshelRoot);
    ShamshelField->SetRelativeLocation(FVector(0,-600,440));
    ShamshelField->SetRelativeRotation(FRotator(0,-90,0));
    ShamshelField->SetRelativeScale3D(FVector(1.25f));
    ShamshelRoot->SetVisibility(false,true);
    for(auto* Lane:WhipTelegraphs) Lane->SetVisibility(false);
}

void AEvaGameMode::SelectAngel(EEvaAngel Kind)
{
    if(SachielRoot) SachielRoot->SetVisibility(false,true);
    if(ShamshelRoot) ShamshelRoot->SetVisibility(false,true);
    for(auto* Lane:WhipTelegraphs) Lane->SetVisibility(false);
    if(RamielRoot) RamielRoot->SetVisibility(false,true);
    ClearAngelHazards();
    const bool Shamshel=Kind==EEvaAngel::Shamshel;
    bShamshel=Shamshel; bRamiel=Kind==EEvaAngel::Ramiel;
    AngelRoot=bRamiel ? RamielRoot : Shamshel ? ShamshelRoot:SachielRoot;
    AngelCore=bRamiel ? RamielCore : Shamshel ? ShamshelCore:SachielCore;
    EnemyShield=bRamiel ? RamielField : Shamshel ? ShamshelField:SachielField;
    EnemyMaxHealth=bRamiel ? 2200.f : Shamshel ? 1800.f:1000.f;
    WhipStrikeTime=0;
}

FVector AEvaGameMode::EnemyAimPoint() const
{
    return AngelCore ? AngelCore->GetComponentLocation() : EnemyPosition;
}

void AEvaGameMode::TickShamshel(float Dt)
{
    auto* P=Pilot(); if(!P) return;
    const FVector Pos=P->GetActorLocation();
    const bool Enraged=Rules.EnemyHealth<EnemyMaxHealth*.5f;
    WhipStrikeTime=FMath::Max(0.f,WhipStrikeTime-Dt);
    if(Telegraph<=0 && WhipStrikeTime<=0)
    {
        FVector Toward=(Pos-EnemyPosition).GetSafeNormal2D();
        float Distance=FVector::Dist2D(Pos,EnemyPosition);
        if(Distance>4800) EnemyPosition+=Toward*Dt*(Enraged ? 680.f:450.f);
        else if(Distance<2400) EnemyPosition-=Toward*Dt*230.f;
        AngelRoot->SetWorldRotation(FRotator(0,Toward.Rotation().Yaw+90,0));
    }
    EnemyPosition.Z=2050+FMath::Sin(MissionTime*1.8f)*100;
    AngelRoot->SetWorldLocation(EnemyPosition);
    if(Rules.EnemyField<=0)
    {
        VulnerableTime-=Dt;
        if(VulnerableTime<=0) Rules.EnemyField=100;
    }
    EnemyShield->SetVisibility(Rules.EnemyField>0,true);
    EnemyClock-=Dt;
    if(EnemyClock<=0 && Telegraph<=0)
    {
        ++AttackCount; ThreatPosition=Pos; ThreatPosition.Z=24; WhipOrigin=EnemyPosition; WhipOrigin.Z=24;
        Telegraph=Enraged ? 1.25f:1.85f;
        const bool Sweep=AttackCount%2==1;
        ThreatRing->SetVisibility(Sweep); ThreatRing->SetWorldLocation(ThreatPosition);
        ThreatRing->SetWorldScale3D(FVector(48,48,.06f));
        FVector Direction=(ThreatPosition-WhipOrigin).GetSafeNormal2D();
        FVector Side=FVector::CrossProduct(Direction,FVector::UpVector);
        for(int I=0;I<WhipTelegraphs.Num();++I)
        {
            auto* Lane=WhipTelegraphs[I]; Lane->SetVisibility(!Sweep);
            FVector A=WhipOrigin+Side*(I==0 ? -400:400), B=ThreatPosition+Direction*1400+Side*(I==0 ? -400:400);
            Lane->SetWorldLocation((A+B)*.5f); Lane->SetWorldRotation((B-A).Rotation());
            Lane->SetWorldScale3D(FVector((B-A).Size()/100,11,.08f));
        }
        SetNotice(Sweep ? "SHAMSHEL / WHIP SWEEP // LEAVE THE CIRCLE OR DODGE" : "SHAMSHEL / TWIN THRUST // SIDESTEP THE RED LANES");
    }
    else if(Telegraph>0)
    {
        Telegraph-=Dt;
        if(Telegraph<=0)
        {
            const bool Sweep=AttackCount%2==1;
            FVector Direction=(ThreatPosition-WhipOrigin).GetSafeNormal2D();
            FVector Side=FVector::CrossProduct(Direction,FVector::UpVector);
            bool Hit=Sweep && FVector::Dist2D(Pos,ThreatPosition)<2400;
            if(!Sweep) for(int S:{-1,1}) Hit |= FEvaShooter::InWhipLane(Pos,WhipOrigin+Side*S*400,ThreatPosition+Direction*1400+Side*S*400,550);
            if(Hit)
            {
                const bool Facing=FVector::DotProduct(P->GetActorForwardVector(),(EnemyPosition-Pos).GetSafeNormal2D())>.25f;
                Rules.ReceiveHit(Sweep ? 24.f:32.f,P->bGuard && Facing,P->DodgeTime>0 || (Sweep && !P->bGrounded && Pos.Z>1450));
                if(P->DodgeTime<=0 && !(Sweep && !P->bGrounded && Pos.Z>1450)) HitFlash=.3f;
            }
            Pulse(ThreatPosition+FVector(0,0,250),FLinearColor(1,.1f,.3f),12);
            DestroyNearby(ThreatPosition,Sweep ? 2400:1100);
            ThreatRing->SetVisibility(false); for(auto* Lane:WhipTelegraphs) Lane->SetVisibility(false);
            WhipStrikeTime=.65f; EnemyClock=Enraged ? 3.5f:4.8f;
            Rules.EnemyField=0; VulnerableTime=Enraged ? 2.8f:4.f;
            SetNotice("WHIPS RETRACTING // CORE EXPOSED / FIRE NOW");
        }
    }
    // Curved energy ribbons use the same locked strike point as the damage telegraph.
    const FTransform Transform=AngelRoot->GetComponentTransform();
    for(int Hand=0;Hand<2;++Hand)
    {
        const float S=Hand==0 ? -1.f:1.f;
        FVector Start=Transform.TransformPosition(FVector(S*570,-260,440));
        FVector End=Transform.TransformPosition(FVector(S*1700,-2100,-1000));
        if(Telegraph>0) End=Transform.TransformPosition(FVector(S*2100,500,1100));
        if(WhipStrikeTime>0) End=ThreatPosition+FVector(S*400,0,700);
        FVector Last=Start;
        for(int I=0;I<18;++I)
        {
            float T=(I+1)/18.f;
            FVector Next=FMath::Lerp(Start,End,T);
            Next+=Transform.TransformVectorNoScale(FVector(S*FMath::Sin(T*PI)*420,FMath::Sin(T*PI*2+MissionTime*5)*180,FMath::Sin(T*PI)*500));
            auto* Segment=WhipSegments[Hand*18+I];
            Segment->SetWorldLocation((Last+Next)*.5f);
            Segment->SetWorldRotation(FRotationMatrix::MakeFromZ(Next-Last).Rotator());
            const float Width=(WhipStrikeTime>0 ? .62f:.36f)*(1-T*.55f);
            Segment->SetWorldScale3D(FVector(Width,Width,(Next-Last).Size()/100));
            Last=Next;
        }
    }
}

void AEvaGameMode::TickShooterTest(float Dt)
{
    if(bShooterTestDone) return;
    auto* P=Pilot(); if(!P) return;
    ShooterTestTime+=Dt;
    auto Check=[&](bool Result,const TCHAR* Name)
    {
        bShooterChecksOK &= Result;
        UE_LOG(LogTemp,Display,TEXT("EVA_SHOOTER_CHECK %s=%d"),Name,Result);
    };
    if(ShooterTestStep==0 && ShooterTestTime>2)
    {
        Check(bShamshel && bWorldEncounter && EnemyMaxHealth==1800,TEXT("shamshel"));
        P->bAiming=true; P->Camera->SetWorldRotation(FRotator(50,-90,0));
        Attack(true,0,true);
        P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
        Check(P->Loadout.Shells==7 && Rules.EnemyField==100,TEXT("miss_spends_shell_only"));
        ShooterTestStep=1; ShooterTestTime=0;
    }
    else if(ShooterTestStep==1 && ShooterTestTime>.7f)
    {
        P->Camera->SetWorldRotation((EnemyAimPoint()-P->Camera->GetComponentLocation()).Rotation());
        Attack(true,0,true);
        P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
        Check(P->Loadout.Shells==6 && Rules.EnemyField==62,TEXT("crosshair_hit"));
        auto* Cover=Shape("Cube",(P->Camera->GetComponentLocation()+EnemyAimPoint())*.5f,FVector(8,8,14),FLinearColor(.1f,.1f,.1f),0,nullptr,true);
        P->Camera->SetWorldRotation((EnemyAimPoint()-P->Camera->GetComponentLocation()).Rotation());
        LanceCooldown=0; Attack(true,0,true);
        P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
        Check(P->Loadout.Shells==5 && Rules.EnemyField==62,TEXT("cover_stops_player_shot"));
        Meshes.Remove(Cover); Cover->DestroyComponent();
        P->ReloadWeapon(); Check(P->ReloadTime>0,TEXT("reload_started"));
        Attack(true,0,true); Check(P->Loadout.Shells==5,TEXT("reload_blocks_fire"));
        ShooterTestStep=2; ShooterTestTime=0;
    }
    else if(ShooterTestStep==2 && ShooterTestTime>1.8f)
    {
        Check(P->Loadout.Shells==8 && P->Loadout.ReserveShells==21,TEXT("reload_finished"));
        P->CameraYaw=86; P->CameraPitch=12;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Shamshel.png"),true,false);
        // Restart the first windup to verify its full warning and evade window.
        EnemyClock=0; Telegraph=0; AttackCount=0;
        ShooterTestStep=3; ShooterTestTime=0;
    }
    else if(ShooterTestStep==3 && Telegraph>0)
    {
        Check(AttackCount==1 && ThreatRing->IsVisible(),TEXT("sweep_warning"));
        P->DodgeDirection=FVector::ZeroVector; P->DodgeTime=3; ShooterTestStep=4; ShooterTestTime=0;
    }
    else if(ShooterTestStep==4 && Telegraph<=0 && ShooterTestTime>1)
    {
        Check(Rules.Integrity==100 && Rules.EnemyField==0,TEXT("evade_and_counter_window"));
        P->DodgeTime=0; EnemyClock=0;
        ShooterTestStep=5; ShooterTestTime=0;
    }
    else if(ShooterTestStep==5 && Telegraph>0)
    {
        Check(AttackCount==2 && WhipTelegraphs[0]->IsVisible(),TEXT("thrust_warning"));
        P->SetActorLocation(ThreatPosition+FVector(0,0,756));
        ShooterTestStep=6; ShooterTestTime=0;
    }
    else if(ShooterTestStep==6 && Telegraph<=0 && ShooterTestTime>1)
    {
        Check(Rules.Integrity==68,TEXT("thrust_damage"));
        Rules.EnemyHealth=100; Rules.EnemyField=0; LanceCooldown=0;
        P->Camera->SetWorldRotation((EnemyAimPoint()-P->Camera->GetComponentLocation()).Rotation());
        Attack(true,0,true);
        P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
        Check(!bWorldEncounter && CompletedContracts==1 && !AngelRoot->IsVisible(),TEXT("defeat_returns_to_roam"));
        P->Loadout.Shells=8; LanceCooldown=0; P->Melee();
        ShooterTestStep=7; ShooterTestTime=0;
    }
    else if(ShooterTestStep==7 && ShooterTestTime>1.3f)
    {
        Check(P->Loadout.Shells<=6 && P->Loadout.Shells>=4,TEXT("held_fire_cadence"));
        P->StopFire();
        bShooterTestDone=true;
        UE_LOG(LogTemp,Display,TEXT("EVA_SHOOTER_RESULT success=%d"),bShooterChecksOK);
        FPlatformMisc::RequestExitWithStatus(false,bShooterChecksOK ? 0:1);
    }
    if(MissionTime>30)
    {
        UE_LOG(LogTemp,Error,TEXT("EVA_SHOOTER_TIMEOUT step=%d"),ShooterTestStep);
        FPlatformMisc::RequestExitWithStatus(false,1);
    }
}
