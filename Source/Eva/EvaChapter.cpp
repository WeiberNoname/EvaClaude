#include "EvaGame.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "Sound/SoundBase.h"

static void ChapterSound(UObject* Context,const TCHAR* Name,float Volume=.55f)
{
    FString Asset=FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name);
    if(auto* S=LoadObject<USoundBase>(nullptr,*Asset)) UGameplayStatics::PlaySound2D(Context,S,Volume);
}

USceneComponent* AEvaGameMode::NewSceneRoot(FVector Position)
{
    if(!VisualWorld)
    {
        VisualWorld=GetWorld()->SpawnActor<AActor>();
        auto* Base=NewObject<USceneComponent>(VisualWorld);
        VisualWorld->SetRootComponent(Base); Base->RegisterComponent();
    }
    auto* Root=NewObject<USceneComponent>(VisualWorld);
    Root->SetupAttachment(VisualWorld->GetRootComponent());
    Root->RegisterComponent(); Root->SetWorldLocation(Position);
    return Root;
}

void AEvaGameMode::BuildChapterScenes()
{
    StoryCamera=GetWorld()->SpawnActor<ACameraActor>();
    StoryCamera->GetCameraComponent()->FieldOfView=62.f;
    const FLinearColor White(.7f,.8f,.78f), Dark(.035f,.05f,.06f), Red(.55f,.025f,.015f);
    // A human-scale telephone and car anchor the opening in the same city as the battle.
    auto* Phone=NewSceneRoot(PhonePosition);
    Shape("Cube",FVector(0,0,110),FVector(1.1f,.2f,2.2f),Dark,0,Phone,true);
    Shape("Cube",FVector(0,-20,135),FVector(.55f,.3f,.65f),FLinearColor(.1f,.4f,.25f),0,Phone);
    Shape("Cube",FVector(0,-38,145),FVector(.25f,.02f,.17f),FLinearColor(.4f,1,.5f),1,Phone);
    Shape("Cube",FVector(0,0,230),FVector(1.5f,1.3f,.2f),White,0,Phone);
    for(int S:{-1,1}) Shape("Cube",FVector(S*67,0,110),FVector(.08f,1.2f,2.2f),White,0,Phone);
    CarRoot=NewSceneRoot(CarPosition);
    Shape("Cube",FVector(0,0,65),FVector(1.9f,4.2f,.8f),FLinearColor(.09f,.14f,.48f),0,CarRoot);
    Shape("Cube",FVector(0,-20,125),FVector(1.65f,2.1f,.7f),FLinearColor(.06f,.12f,.23f),0,CarRoot);
    Shape("Cube",FVector(0,92,130),FVector(1.6f,.03f,.55f),FLinearColor(.13f,.3f,.36f),.4f,CarRoot);
    for(int X:{-1,1}) for(int Y:{-1,1})
    {
        auto* Wheel=Shape("Cylinder",FVector(X*100,Y*135,45),FVector(.65f,.65f,.3f),Dark,0,CarRoot);
        Wheel->SetRelativeRotation(FRotator(90,0,0));
    }
    for(int S:{-1,1}) Shape("Cube",FVector(S*60,214,80),FVector(.5f,.04f,.2f),FLinearColor(1,.8f,.4f),3,CarRoot);

    auto Human=[&](FVector Position,FLinearColor Coat,FLinearColor Hair)
    {
        auto* H=NewSceneRoot(Position);
        Shape("Sphere",FVector(0,0,168),FVector(.35f,.34f,.4f),FLinearColor(.68f,.44f,.32f),0,H);
        Shape("Sphere",FVector(0,4,179),FVector(.4f,.38f,.27f),Hair,0,H);
        Shape("Cube",FVector(0,0,115),FVector(.48f,.3f,.65f),Coat,0,H);
        for(int S:{-1,1})
        {
            Shape("Cylinder",FVector(S*14,0,45),FVector(.17f,.17f,.8f),Dark,0,H);
            Shape("Cylinder",FVector(S*33,0,108),FVector(.15f,.15f,.6f),Coat,0,H);
        }
        return H;
    };
    HangarRoot=NewSceneRoot(HangarPosition);
    Shape("Cube",FVector(0,0,-70),FVector(75,90,1.4f),Dark,0,HangarRoot);
    Shape("Cube",FVector(0,2200,1700),FVector(70,2,34),Dark,0,HangarRoot);
    for(int S:{-1,1})
    {
        Shape("Cube",FVector(S*1100,350,1250),FVector(2,3,25),FLinearColor(.15f,.19f,.22f),0,HangarRoot);
        Shape("Cube",FVector(S*900,-20,2100),FVector(15,2,.5f),Red,1.5f,HangarRoot);
        Shape("Cube",FVector(S*2900,0,2600),FVector(.6f,65,.3f),White,2,HangarRoot);
    }
    Shape("Cube",FVector(0,1300,1500),FVector(35,10,.8f),FLinearColor(.13f,.16f,.19f),0,HangarRoot);
    Shape("Cube",FVector(0,800,1660),FVector(35,.1f,.1f),Red,2,HangarRoot);
    Human(HangarPosition+FVector(0,1300,1540),FLinearColor(.05f,.06f,.08f),Dark);
    Human(HangarPosition+FVector(-1100,-950,0),Red,FLinearColor(.1f,.04f,.17f));
    Shape("Cube",FVector(-850,-550,90),FVector(1.1f,2.5f,.2f),White,0,HangarRoot);
    Shape("Sphere",FVector(-850,-620,122),FVector(.4f),FLinearColor(.2f,.45f,.65f),0,HangarRoot);
    Shape("Cube",FVector(-850,-520,116),FVector(.7f,1.4f,.25f),FLinearColor(.86f,.88f,.88f),0,HangarRoot);
    for(int S:{-1,1}) Shape("Cube",FVector(-850+S*48,-550,40),FVector(.07f,2.4f,.8f),White,0,HangarRoot);

    // A compact enclosed entry plug, with physical controls and instrument panels.
    auto* Cockpit=NewSceneRoot(HangarPosition+FVector(18000,0,0));
    Shape("Cube",FVector(0,100,15),FVector(6,10,.3f),Dark,0,Cockpit);
    Shape("Cube",FVector(0,470,290),FVector(6,.3f,5.8f),Dark,0,Cockpit);
    Shape("Cube",FVector(0,451,330),FVector(5.5f,.04f,3.8f),FLinearColor(.014f,.035f,.048f),.8f,Cockpit);
    Shape("Cube",FVector(0,100,590),FVector(6,10,.3f),Dark,0,Cockpit);
    for(int S:{-1,1})
    {
        Shape("Cube",FVector(S*300,100,290),FVector(.3f,10,5.8f),Dark,0,Cockpit);
        Shape("Cube",FVector(S*180,250,130),FVector(1.5f,2,1.f),FLinearColor(.08f,.11f,.13f),0,Cockpit);
        Shape("Cylinder",FVector(S*135,180,190),FVector(.15f,.15f,.8f),FLinearColor(.17f,.19f,.19f),0,Cockpit);
        Shape("Sphere",FVector(S*135,180,230),FVector(.35f),FLinearColor(.1f,.15f,.2f),0,Cockpit);
        Shape("Cube",FVector(S*205,225,192),FVector(1.2f,.85f,.05f),FLinearColor(.17f,.64f,.42f),1.2f,Cockpit);
        Shape("Cube",FVector(S*270,446,335),FVector(.06f,.06f,4.f),FLinearColor(.1f,.6f,.65f),2,Cockpit);
    }
    for(int I=0;I<7;++I)
        Shape("Cube",FVector(-160+I*52,444,280),FVector(.28f,.05f,.2f+I*.08f),FLinearColor(.2f,.85f,.5f),1.5f,Cockpit);

    HospitalRoot=NewSceneRoot(HospitalPosition);
    Shape("Cube",FVector(0,0,-25),FVector(15,22,.5f),FLinearColor(.23f,.29f,.3f),0,HospitalRoot);
    Shape("Cube",FVector(0,950,280),FVector(15,.2f,5.6f),FLinearColor(.35f,.42f,.42f),0,HospitalRoot);
    Shape("Cube",FVector(-700,0,280),FVector(.2f,22,5.6f),White,0,HospitalRoot);
    Shape("Cube",FVector(0,0,580),FVector(15,22,.2f),White,0,HospitalRoot);
    Shape("Cube",FVector(0,180,565),FVector(1.7f,6,.04f),White,2,HospitalRoot);
    Shape("Cube",FVector(0,0,75),FVector(1.8f,3,.6f),White,0,HospitalRoot);
    Shape("Cube",FVector(0,35,115),FVector(1.7f,2.3f,.2f),FLinearColor(.26f,.4f,.45f),0,HospitalRoot);
    Shape("Cube",FVector(-220,150,140),FVector(1,.5f,.85f),Dark,0,HospitalRoot);
    Shape("Cube",FVector(-220,122,148),FVector(.75f,.03f,.38f),FLinearColor(.12f,.7f,.38f),1,HospitalRoot);
    Human(HospitalPosition+FVector(230,420,0),Red,FLinearColor(.1f,.04f,.17f));
}

void AEvaGameMode::StoryView(FVector Location,FVector Target,float Blend)
{
    StoryCamera->SetActorLocation(Location);
    StoryCamera->SetActorRotation((Target-Location).Rotation());
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0)) PC->SetViewTargetWithBlend(StoryCamera,Blend);
}

void AEvaGameMode::SetChapter(EEvaChapter Next)
{
    Chapter=Next; ChapterTime=0; DialogueIndex=0;
    auto* P=Pilot(); if(!P) return;
    UE_LOG(LogTemp,Display,TEXT("EVA_CHAPTER_STAGE %d"),int32(Chapter));
    ThreatRing->SetVisibility(false);
    P->bGuard=false;
    switch(Next)
    {
    case EEvaChapter::Title:
        P->SetHumanMode(false); P->SetActorLocation(FVector(0,-5900,780)); P->SetActorRotation(FRotator(0,90,0));
        StoryView(FVector(-2600,-9200,2100),FVector(0,-3400,1050),0); break;
    case EEvaChapter::Street:
        P->SetHumanMode(true); P->SetActorLocation(FVector(0,-8700,100));
        P->CameraPitch=4; P->CameraYaw=90; P->Loadout=FEvaLoadout();
        EnemyPosition=FVector(0,4300,1600); AngelRoot->SetWorldLocation(EnemyPosition); AngelRoot->SetWorldRotation(FRotator::ZeroRotator); AngelRoot->SetVisibility(true,true);
        CarRoot->SetWorldLocation(CarPosition);
        UGameplayStatics::GetPlayerController(this,0)->SetViewTargetWithBlend(P,.4f); break;
    case EEvaChapter::Rescue:
        ChapterSound(this,TEXT("Rumble"));
        StoryView(CarPosition+FVector(300,-600,200),CarPosition+FVector(0,150,110)); break;
    case EEvaChapter::Hangar:
        P->SetHumanMode(false); P->SetActorLocation(HangarPosition+FVector(0,0,780)); P->SetActorRotation(FRotator(0,-90,0));
        StoryView(HangarPosition+FVector(-700,-2900,1050),HangarPosition+FVector(0,0,1000)); break;
    case EEvaChapter::Decision:
        ChapterSound(this,TEXT("Impact"),.3f);
        StoryView(HangarPosition+FVector(-300,-1300,260),HangarPosition+FVector(-850,-550,115)); break;
    case EEvaChapter::Entry:
        ChapterSound(this,TEXT("Breath"),.9f);
        StoryView(HangarPosition+FVector(18000,0,190),HangarPosition+FVector(18000,450,290)); break;
    case EEvaChapter::Launch:
        ChapterSound(this,TEXT("Launch"));
        P->SetActorLocation(FVector(0,-5900,-2600)); P->SetActorRotation(FRotator(0,90,0));
        StoryView(FVector(-2000,-7300,1400),FVector(0,-5900,400)); break;
    case EEvaChapter::Awakening:
        ChapterSound(this,TEXT("Heartbeat"),.7f);
        bBerserk=true; Rules.bConnected=false; P->Loadout.Equipped=EEvaWeapon::Unarmed;
        StoryView(P->GetActorLocation()+FVector(-1900,-2100,1100),P->GetActorLocation()+FVector(0,0,500)); break;
    case EEvaChapter::Hospital:
        P->SetHumanMode(true); P->SetActorLocation(HospitalPosition+FVector(0,0,100));
        StoryView(HospitalPosition+FVector(0,-50,180),HospitalPosition+FVector(130,420,170),0); break;
    case EEvaChapter::Complete:
        bEnded=true; bVictory=true; break;
    default: break;
    }
}

void AEvaGameMode::AdvanceStory()
{
    if(bPaused) { bPaused=false; return; }
    if(Chapter==EEvaChapter::Impact) { UGameplayStatics::OpenLevel(this,FName("LastSignal")); return; }
    if(Chapter==EEvaChapter::Title) { StartMission(); return; }
    if(Chapter==EEvaChapter::Complete) { UGameplayStatics::OpenLevel(this,FName("LastSignal")); return; }
    if(ChapterTime<.6f) return;
    switch(Chapter)
    {
    case EEvaChapter::Hangar:
        if(++DialogueIndex>=3) SetChapter(EEvaChapter::Decision);
        else if(DialogueIndex==1) StoryView(HangarPosition+FVector(1000,-900,1900),HangarPosition+FVector(0,1250,1650),.4f);
        else StoryView(HangarPosition+FVector(-700,-2900,1050),HangarPosition+FVector(0,0,1000),.4f);
        break;
    case EEvaChapter::Decision:
        if(++DialogueIndex>=2) SetChapter(EEvaChapter::Entry);
        else StoryView(HangarPosition+FVector(-1250,-1450,400),HangarPosition+FVector(-200,0,900),.5f);
        break;
    case EEvaChapter::Hospital:
        if(++DialogueIndex>=3) SetChapter(EEvaChapter::Complete); break;
    default: break;
    }
}

void AEvaGameMode::StartBattle()
{
    CloseComms();
    if(Wingman) { Wingman->bDeployed=false; Wingman->SetActorHiddenInGame(true); }
    SelectAngel(EEvaAngel::Sachiel);
    bOpenWorld=false; bWorldMap=false; bHelp=false;
    bStarted=true; bPaused=false; bEnded=false; bVictory=false; bBerserk=false;
    Rules=FEvaRules(); Rules.Battery=65; Rules.bConnected=false;
    Systems=FEvaSystems(); bCableWarning=false;
    MissionTime=0; EnemyClock=12; Telegraph=0; VulnerableTime=0; ActiveCharger=0;
    MeleeCooldown=0; LanceCooldown=0; AttackCount=0;
    BuildingsLost=0;
    for(auto& B:Buildings)
    {
        B.bDestroyed=false;
        FVector Scale=B.Mesh->GetComponentScale(); Scale.Z=B.Center.Z/50.f;
        B.Mesh->SetWorldScale3D(Scale); B.Mesh->SetWorldLocation(B.Center);
        B.Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        for(auto* Window:B.Windows) Window->SetVisibility(true);
    }
    for(auto& Effect:Effects) { Meshes.Remove(Effect.Mesh); Effect.Mesh->DestroyComponent(); }
    Effects.Empty();
    bDepotOpening=false; DepotOpen=0; bCannonTaken=false;
    auto* P=Pilot(); if(!P) return;
    P->Loadout=FEvaLoadout(); P->DrawTime=0; P->PickupTime=0; P->bCharging=false; P->CannonCharge=0; P->ComboTime=0; P->KnifeCombo=0;
    P->ReloadTime=0; P->bFireHeld=false; P->bAiming=false; P->RecoilPitch=0; P->HitMarkerTime=0; P->MoveVelocity=FVector::ZeroVector;
    P->SetHumanMode(false); P->SetActorLocation(FVector(0,-5900,780)); P->SetActorRotation(FRotator(0,90,0));
    P->DodgeTime=0; P->DodgeCooldown=0;
    EnemyPosition=FVector(0,4300,1600);
    AngelRoot->SetWorldLocation(EnemyPosition); AngelRoot->SetVisibility(true,true);
    SetChapter(EEvaChapter::Battle);
    UGameplayStatics::GetPlayerController(this,0)->SetViewTargetWithBlend(P,.5f);
    SetNotice("MISATO // CONNECT TO THE GREEN POWER STATION. ARMORY 07 HAS YOUR CANNON.");
}

FString AEvaGameMode::Objective() const
{
    if(bOpenWorld) return bWorldEncounter ? "OPTIONAL ENCOUNTER // NEUTRALIZE THE ANGEL" : "FREE ROAM // "+DistrictNames[SelectedDistrict]+" WAYPOINT";
    switch(Chapter)
    {
    case EEvaChapter::Street: return bPhoneUsed ? "REACH MISATO'S CAR" : "FIND A WORKING TELEPHONE";
    case EEvaChapter::Battle:
        if(!Rules.bConnected && Rules.Battery<45) return "LOW POWER // CONNECT TO A GREEN CHARGER";
        if(!bCannonTaken) return "OPEN ARMORY 07 AND RETRIEVE THE CANNON";
        return Rules.EnemyField>0 ? "BREACH SACHIEL'S A.T. FIELD" : "CORE EXPOSED // PRESS THE ATTACK";
    case EEvaChapter::Hangar: return "NERV // THE CAGE";
    case EEvaChapter::Decision: return "THE OTHER PILOT";
    case EEvaChapter::Entry: return "ENTRY PLUG // SYNCHRONIZING";
    case EEvaChapter::Launch: return "UNIT-01 // LAUNCH";
    case EEvaChapter::Awakening: return "SIGNAL LOST";
    case EEvaChapter::Hospital: return "AN UNFAMILIAR CEILING";
    default: return "CHAPTER 01 // THE BOY BENEATH THE GIANT";
    }
}

FString AEvaGameMode::StorySpeaker() const
{
    if(Chapter==EEvaChapter::Hangar) return DialogueIndex==0 ? "MISATO" : DialogueIndex==1 ? "GENDO" : "SHINJI";
    if(Chapter==EEvaChapter::Decision) return DialogueIndex==0 ? "SHINJI" : "MISATO";
    if(Chapter==EEvaChapter::Hospital) return DialogueIndex==1 ? "SHINJI" : "MISATO";
    if(Chapter==EEvaChapter::Street) return bPhoneUsed ? "MISATO // RADIO" : "SHINJI";
    return "MISATO // COMMAND";
}

FString AEvaGameMode::StoryLine() const
{
    switch(Chapter)
    {
    case EEvaChapter::Street: return bPhoneUsed ? "Shinji! Over here. Get into the car!" : "The whole city is empty. Why did he ask me to come here?";
    case EEvaChapter::Rescue: return "Stay down. We are going underground.";
    case EEvaChapter::Hangar:
        return DialogueIndex==0 ? "This is Unit-01. Your father is waiting above us." : DialogueIndex==1 ? "You will pilot it. The Angel must be stopped." : "I came to see you. I don't even know what this thing is.";
    case EEvaChapter::Decision: return DialogueIndex==0 ? "That girl can barely stand. You're sending her instead?" : "The Eva moved to protect you. We still don't know why.";
    case EEvaChapter::Entry: return "The liquid carries oxygen. Let yourself breathe. I'm staying on the line.";
    case EEvaChapter::Launch: return "Surface route clear. Unit-01, launching.";
    case EEvaChapter::Awakening: return ChapterTime<3 ? "Shinji? Answer me. We've lost the pilot signal!" : "Unit-01 is moving. There is no command input.";
    case EEvaChapter::Hospital: return DialogueIndex==0 ? "You're safe. The Angel is gone." : DialogueIndex==1 ? "What happened after the radio stopped?" : "Rest now. We will talk when you're ready.";
    default: return "";
    }
}

FString AEvaGameMode::StoryLineTwo() const
{
    switch(Chapter)
    {
    case EEvaChapter::Street: return bPhoneUsed ? "Follow the cyan marker. The creature is still coming." : "WASD / WALK     MOUSE / LOOK     E / INTERACT";
    case EEvaChapter::Rescue: return "Above the tunnel, the military guns fall silent.";
    case EEvaChapter::Hangar: return "ENTER / CONTINUE";
    case EEvaChapter::Decision: return DialogueIndex==0 ? "A tremor. Falling debris. An enormous hand moves between Shinji and the ceiling." : "E OR ENTER / CHOOSE TO ENTER THE EVA";
    case EEvaChapter::Entry: return "A first breath. Then the city appears around him.";
    case EEvaChapter::Launch: return "Human scale falls away. The streets are beneath your feet.";
    case EEvaChapter::Awakening: return "The breathing inside the cockpit is no longer in time with his own.";
    case EEvaChapter::Hospital: return "ENTER / CONTINUE";
    default: return "";
    }
}

void AEvaGameMode::TickChapter(float Dt)
{
    ChapterTime+=Dt;
    auto* P=Pilot(); if(!P) return;
    if(Chapter==EEvaChapter::Street)
    {
        if(ChapterTime>4 && !bStreetAttack)
        {
            bStreetAttack=true;
            ChapterSound(this,TEXT("Breach"),.3f);
            Pulse(EnemyPosition+FVector(-500,0,100),FLinearColor(1,.45f,.05f),14);
            SetNotice("PATTERN BLUE // EVACUATION IN PROGRESS");
        }
        AngelRoot->SetRelativeRotation(FRotator(0,FMath::Sin(ChapterTime*.3f)*8,0));
    }
    else if(Chapter==EEvaChapter::Rescue)
    {
        CarRoot->SetWorldLocation(CarPosition+FVector(0,ChapterTime*240,0));
        StoryView(CarRoot->GetComponentLocation()+FVector(320,-600,200),CarRoot->GetComponentLocation()+FVector(0,160,100),0);
        if(ChapterTime>5) SetChapter(EEvaChapter::Hangar);
    }
    else if(Chapter==EEvaChapter::Decision && P->Limbs.Num()>0)
        P->Limbs[0]->SetRelativeRotation(FRotator(-65*FMath::Clamp(ChapterTime/1.3f,0.f,1.f),0,0));
    else if(Chapter==EEvaChapter::Entry && ChapterTime>6) SetChapter(EEvaChapter::Launch);
    else if(Chapter==EEvaChapter::Launch)
    {
        P->SetActorLocation(FVector(0,-5900,FMath::Lerp(-2600.f,780.f,FMath::SmoothStep(0.f,1.f,ChapterTime/4.f))));
        if(ChapterTime>4.5f) StartBattle();
    }
    else if(Chapter==EEvaChapter::Awakening)
    {
        if(ChapterTime>=3 && ChapterTime-Dt<3) ChapterSound(this,TEXT("Roar"),.8f);
        if(ChapterTime>3 && ChapterTime<7)
        {
            FVector Target(EnemyPosition.X,EnemyPosition.Y-1150,780);
            P->SetActorLocation(FMath::VInterpTo(P->GetActorLocation(),Target,Dt,2.f));
            P->SetActorRotation(FRotator(0,90,0));
            for(int I=0;I<P->Limbs.Num();++I) P->Limbs[I]->SetRelativeRotation(FRotator(FMath::Sin(ChapterTime*13+I)*65,0,0));
            StoryView(P->GetActorLocation()+FVector(-2100,-2300,1200),EnemyPosition,0);
            if(FMath::Fmod(ChapterTime,.6f)<Dt) Pulse(EnemyPosition,FLinearColor(1,.3f,.05f),6);
        }
        if(ChapterTime>7 && Rules.EnemyHealth>0)
        {
            Rules.EnemyHealth=0; AngelRoot->SetVisibility(false,true);
            Pulse(EnemyPosition,FLinearColor(1,.5f,.15f),30);
        }
        if(ChapterTime>9.5f) SetChapter(EEvaChapter::Hospital);
    }
}

void AEvaGameMode::TickChapterAutomation(float Dt)
{
    // Integration harness visits the same interactions and inventory gates as a player.
    if(bPaused) return;
    auto* P=Pilot(); if(!P) return;
    const float Wait=bChapterSmoke ? 2.5f : .7f;
    if(bChapterSmoke && int32(Chapter)!=ShotStage && ChapterTime>1.8f)
    {
        ShotStage=int32(Chapter);
        if(Chapter==EEvaChapter::Street)
        {
            auto* Cam=UGameplayStatics::GetPlayerCameraManager(this,0);
            const FVector Start=Cam->GetCameraLocation();
            const FRotator Rot=Cam->GetCameraRotation()+FRotator(10,0,0);
            const FVector End=Start+Rot.Vector()*15000;
            float Near=MAX_flt; UStaticMeshComponent* Hit=nullptr;
            for(auto* M:Meshes)
            {
                if(M && M->IsVisible() && FMath::LineBoxIntersection(M->Bounds.GetBox(),Start,End,End-Start))
                { float D=FVector::Dist(Start,M->Bounds.GetBox().GetClosestPointTo(Start)); if(D<Near) { Near=D; Hit=M; } }
            }
            UE_LOG(LogTemp,Display,TEXT("EVA_STREET_CAMERA pawn=%s cam=%s rot=%s obstruction=%s location=%s scale=%s"),*P->GetActorLocation().ToString(),*Start.ToString(),*Rot.ToString(),Hit ? *Hit->GetName() : TEXT("none"),Hit ? *Hit->GetComponentLocation().ToString() : TEXT(""),Hit ? *Hit->GetComponentScale().ToString() : TEXT(""));
        }
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Chapter_%02d.png"),ShotStage),true,false);
    }
    if(Chapter==EEvaChapter::Street && ChapterTime>Wait)
    {
        if(!bPhoneUsed) { P->SetActorLocation(PhonePosition+FVector(0,-140,100)); Interact(); }
        else { P->SetActorLocation(CarPosition+FVector(0,-180,100)); Interact(); }
    }
    else if((Chapter==EEvaChapter::Hangar || Chapter==EEvaChapter::Decision || Chapter==EEvaChapter::Hospital) && ChapterTime>Wait*(DialogueIndex+1)) AdvanceStory();
    else if(IsActive())
    {
        switch(AutoStep)
        {
        case 0:
            P->SetActorLocation(Chargers[0]+FVector(0,600,680));
            Rules.Battery=20; Interact(); ++AutoStep; break;
        case 1:
            if(Rules.Battery>60)
            { bTestCharged=true; P->SetActorLocation(DepotApproach); Interact(); ++AutoStep; }
            break;
        case 2:
            if(DepotOpen>=1) { Interact(); bTestCannon=P->Loadout.bHasCannon; ++AutoStep; }
            break;
        case 3:
            if(P->PickupTime<=0)
            {
                if(bChapterSmoke) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Equipment_Cannon.png"),true,false);
                ++AutoStep;
            }
            break;
        case 4:
            P->SetActorLocation(FVector(0,-2300,780));
            Attack(true);
            if(P->Loadout.Shells<=5) { P->DrawKnife(); bTestKnife=P->Loadout.Equipped==EEvaWeapon::Knife; ++AutoStep; }
            break;
        case 5:
            P->SetActorLocation(FVector(EnemyPosition.X,EnemyPosition.Y-1600,780));
            if(P->DrawTime<=0)
            {
                if(bChapterSmoke && !bTestBattleDamage) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Equipment_Knife.png"),true,false);
                Attack(false);
                bTestBattleDamage=Rules.EnemyHealth<1000;
            }
            break;
        }
    }
    if(Chapter==EEvaChapter::Complete || (bEnded && !bVictory) || GetWorld()->TimeSeconds>150)
    {
        const bool Won=bVictory;
        bool OK=Won && bTestCharged && bTestCannon && bTestKnife && bTestBattleDamage;
        bool ResetOK=false;
        if(OK)
        {
            if(Buildings.Num()>0) DestroyNearby(Buildings[0].Center,200);
            StartBattle();
            ResetOK=Rules.Integrity==100 && Rules.Battery==65 && BuildingsLost==0 && !P->Loadout.bHasCannon && DepotOpen==0;
            if(Buildings.Num()>0) ResetOK=ResetOK && !Buildings[0].bDestroyed && Buildings[0].Mesh->GetComponentLocation().Equals(Buildings[0].Center);
            OK=OK && ResetOK;
        }
        UE_LOG(LogTemp,Display,TEXT("EVA_CHAPTER_RESULT success=%d charged=%d cannon=%d knife=%d damaged=%d victory=%d checkpoint=%d"),OK,bTestCharged,bTestCannon,bTestKnife,bTestBattleDamage,Won,ResetOK);
        FPlatformMisc::RequestExitWithStatus(false,OK ? 0 : 1);
    }
}
