#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"

void AEvaPawn::ReturnToTitle() { UGameplayStatics::OpenLevel(this,FName("LastSignal")); }
void AEvaPawn::OpenWorld() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) if(G->Chapter==EEvaChapter::Title) G->StartOpenWorld(); }
void AEvaPawn::ToggleMap() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) if(G->bOpenWorld) G->bWorldMap=!G->bWorldMap; }
void AEvaPawn::NextWaypoint() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) if(G->bOpenWorld) G->SelectedDistrict=(G->SelectedDistrict+1)%4; }

void AEvaGameMode::BuildOpenWorld()
{
    if(bWorldBuilt) return;
    bWorldBuilt=true;
    DistrictNames={TEXT("CENTRAL"),TEXT("HARBOR"),TEXT("UPLAND"),TEXT("INDUSTRIAL")};
    for(FVector Offset:{FVector(-22000,-22000,0),FVector(22000,-22000,0),FVector(-22000,22000,0),FVector(22000,22000,0)}) Districts.Add(WorldCenter+Offset);
    CityBox(WorldCenter+FVector(0,0,-100),FVector(900,900,2),FLinearColor(.05f,.08f,.05f),6,nullptr,true);
    // Arterial roads between the districts share two instanced batches.
    auto* Roads=CityInstances(NewSceneRoot(WorldCenter),FLinearColor(.045f,.052f,.062f),3);
    auto* Lines=CityInstances(Roads->GetAttachParent(),FLinearColor(.65f,.56f,.36f),2);
    for(int I=-2;I<=2;++I)
    {
        const float V=I*22000;
        Roads->AddInstance(FTransform(FRotator::ZeroRotator,FVector(V,0,8),FVector(27,880,.12f)));
        Roads->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,V,8),FVector(880,27,.12f)));
        for(int J=-22;J<=22;++J)
        {
            Lines->AddInstance(FTransform(FRotator::ZeroRotator,FVector(V,J*1900,18),FVector(.18f,7,.06f)));
            Lines->AddInstance(FTransform(FRotator::ZeroRotator,FVector(J*1900,V,18),FVector(7,.18f,.06f)));
        }
    }
    for(int D=0;D<4;++D)
    {
        FVector Base=Districts[D];
        Shape("Cylinder",Base+FVector(0,0,30),FVector(36,36,.5f),FLinearColor(.06f,.09f,.11f));
        Shape("ArmorPylon",Base+FVector(0,0,1200),FVector(6,6,24),FLinearColor(.11f,.16f,.22f));
        for(int I=0;I<4;++I) Shape("Cube",Base+FVector(0,0,480+I*440),FVector(7.5f,7.5f,.35f),FLinearColor(.3f,1,.03f),2);
        ChargerLamps.Add(Shape("Sphere",Base+FVector(0,0,2460),FVector(2),FLinearColor(.3f,1,.03f),2));
        FVector Relay=Base+FVector(0,5500,0);
        Shape("Cylinder",Relay+FVector(0,0,25),FVector(15,15,.4f),FLinearColor(.65f,.21f,.015f),1);
        Shape("ArmorPylon",Relay+FVector(0,0,700),FVector(2.8f,2.8f,14),FLinearColor(.9f,.16f,.025f),1);
        auto* Sign=NewObject<UTextRenderComponent>(VisualWorld);
        Sign->SetupAttachment(VisualWorld->GetRootComponent()); Sign->RegisterComponent();
        Sign->SetWorldLocation(Base+FVector(0,-365,1700)); Sign->SetWorldRotation(FRotator(0,-90,0));
        Sign->SetText(FText::FromString(DistrictNames[D])); Sign->SetWorldSize(125);
        Sign->SetHorizontalAlignment(EHTA_Center); Sign->SetTextRenderColor(FColor(170,255,80));
    }
    Shape("TerrainRidge",WorldCenter+FVector(-53000,0,-300),FVector(230,1100,220),FLinearColor(.065f,.105f,.075f));
    // Every district is assembled from the same destructible kit, each with its own character.
    BuildCentralDistrict();
    BuildHarborDistrict();
    BuildUplandDistrict();
    BuildIndustrialDistrict();
    BuildWorldInfrastructure();
}

void AEvaGameMode::StartOpenWorld()
{
    StartBattle(); bOpenWorld=true; bWorldEncounter=false; bWorldMap=false;
    bWorldTest=FParse::Param(FCommandLine::Get(),TEXT("EvaWorldTest"));
    bShooterTest=FParse::Param(FCommandLine::Get(),TEXT("EvaShooterTest"));
    bCompanionTest=FParse::Param(FCommandLine::Get(),TEXT("EvaCompanionTest"));
    bDynamicTest=FParse::Param(FCommandLine::Get(),TEXT("EvaDynamicTest"));
    bDistrictTest=FParse::Param(FCommandLine::Get(),TEXT("EvaDistrictTest"));
    BuildOpenWorld(); Chargers.Empty();
    SetDistrictLighting(true);
    for(FVector D:Districts) Chargers.Add(D+FVector(0,0,100));
    Rules.Battery=120; Rules.bConnected=true; ActiveCharger=0;
    Pilot()->SetActorLocation(Districts[0]+FVector(0,-1800,780)); Pilot()->bLock=false;
    Pilot()->Loadout.AcquireCannon(); bCannonTaken=true;
    AngelRoot->SetVisibility(false,true); EnemyShield->SetVisibility(false,true);
    SurveyMask=0; CompletedContracts=0;
    FString Slot=bWorldTest ? TEXT("EvaFreeRoam_Test") : bShooterTest ? TEXT("EvaShooter_Test") : bDynamicTest ? TEXT("EvaDynamic_Test") : bCompanionTest ? TEXT("EvaCompanion_Test") : bDistrictTest ? TEXT("EvaDistrict_Test") : TEXT("EvaFreeRoam");
    if(!bWorldTest && !bShooterTest && !bDynamicTest && !bCompanionTest && !bDistrictTest && UGameplayStatics::DoesSaveGameExist(Slot,0)) if(auto* Save=Cast<UEvaWorldSave>(UGameplayStatics::LoadGameFromSlot(Slot,0)))
    { SurveyMask=Save->SurveyMask&15; CompletedContracts=FMath::Max(0,Save->Contracts); }
    SetNotice("FREE ROAM // M MAP / N WAYPOINT / SHIFT SPRINT / E SERVICE OR OPTIONAL ENCOUNTER");
    if(bShooterTest || FParse::Param(FCommandLine::Get(),TEXT("EvaShamshel")))
    {
        SelectedDistrict=1; ActiveCharger=1;
        Pilot()->SetActorLocation(Districts[1]+FVector(0,6800,780)); BeginWorldEncounter();
    }
    if(bDynamicTest || FParse::Param(FCommandLine::Get(),TEXT("EvaRamiel")))
    {
        SelectedDistrict=3; ActiveCharger=3; Pilot()->SetActorLocation(Districts[3]+FVector(0,7400,780)); BeginWorldEncounter();
    }
    DeployWingman();
}
void AEvaGameMode::SaveWorldProgress()
{
    auto* Save=Cast<UEvaWorldSave>(UGameplayStatics::CreateSaveGameObject(UEvaWorldSave::StaticClass()));
    Save->SurveyMask=SurveyMask; Save->Contracts=CompletedContracts;
    if(!UGameplayStatics::SaveGameToSlot(Save,bWorldTest ? TEXT("EvaFreeRoam_Test") : bShooterTest ? TEXT("EvaShooter_Test") : bDynamicTest ? TEXT("EvaDynamic_Test") : bCompanionTest ? TEXT("EvaCompanion_Test") : bDistrictTest ? TEXT("EvaDistrict_Test") : TEXT("EvaFreeRoam"),0)) SetNotice("PROGRESS SAVE FAILED // CURRENT SESSION CONTINUES");
}
FString AEvaGameMode::WorldPrompt() const
{
    auto* P=Pilot(); if(!P) return "";
    const FString District=DistrictPrompt(); if(!District.IsEmpty()) return District;
    for(int I=0;I<Districts.Num();++I)
    {
        if(FVector::Dist2D(P->GetActorLocation(),Districts[I])<2400) return "E / SERVICE: REPAIR + AMMO + UMBILICAL";
        if(!bWorldEncounter && FVector::Dist2D(P->GetActorLocation(),Districts[I]+FVector(0,5500,0))<1800) return I==3 ? "E / RAMIEL: BEAM ENCOUNTER" : I==1 ? "E / SHAMSHEL: ENERGY WHIP ENCOUNTER" : "E / SACHIEL: OPTIONAL ANGEL ENCOUNTER";
    }
    return "";
}
bool AEvaGameMode::WorldInteract()
{
    auto* P=Pilot(); if(!P) return false;
    if(DistrictInteract()) return true;
    for(int I=0;I<Districts.Num();++I)
    {
        if(FVector::Dist2D(P->GetActorLocation(),Districts[I])<2400)
        {
            if(Wingman) Wingman->Integrity=100;
            Rules.Integrity=100; Rules.bConnected=true; Rules.Battery=120; ActiveCharger=I;
            P->Loadout.AcquireCannon(); P->bCharging=false; P->CannonCharge=0;
            P->ReloadTime=0; P->bFireHeld=false;
            SetNotice("SERVICE COMPLETE // INTEGRITY + AMMUNITION + EXTERNAL POWER RESTORED"); return true;
        }
        if(!bWorldEncounter && FVector::Dist2D(P->GetActorLocation(),Districts[I]+FVector(0,5500,0))<1800)
        { SelectedDistrict=I; BeginWorldEncounter(); return true; }
    }
    SetNotice("FOLLOW A GREEN SERVICE PYLON OR AN AMBER ENCOUNTER BEACON"); return true;
}
void AEvaGameMode::BeginWorldEncounter()
{
    EncounterDistrict=SelectedDistrict; SelectAngel(EncounterDistrict==3 ? EEvaAngel::Ramiel : EncounterDistrict==1 ? EEvaAngel::Shamshel:EEvaAngel::Sachiel);
    bWorldEncounter=true; Rules.EnemyHealth=EnemyMaxHealth; Rules.EnemyField=100;
    EnemyPosition=Districts[SelectedDistrict]+FVector(0,13000,bRamiel ? 3400:1600);
    EnemyClock=5; Telegraph=0; VulnerableTime=0; AttackCount=0; WhipStrikeTime=0;
    AngelRoot->SetWorldLocation(EnemyPosition); AngelRoot->SetVisibility(true,true);
    EnemyShield->SetVisibility(true,true); Pilot()->bLock=false;
    SetNotice(bRamiel ? "RAMIEL // KEEP MOVING / DASH AFTER THE BEAM LOCKS / FIRE DURING COOLDOWN" : bShamshel ? "SHAMSHEL // EVADE THE WHIPS, THEN FIRE ON THE EXPOSED CORE" : "SACHIEL // RMB AIM / LMB FIRE / TAB AIM ASSIST");
}
void AEvaGameMode::TickOpenWorld(float Dt)
{
    auto* P=Pilot(); if(!P) return;
    for(int I=0;I<Districts.Num();++I)
        if(!(SurveyMask&(1<<I)) && FVector::Dist2D(P->GetActorLocation(),Districts[I])<8500)
        { SurveyMask|=1<<I; SaveWorldProgress(); SetNotice("DISTRICT SURVEYED // "+DistrictNames[I]); }
    if(bWorldEncounter && FVector::Dist2D(P->GetActorLocation(),Districts[EncounterDistrict])>25000)
    {
        bWorldEncounter=false; Pilot()->bLock=false; Telegraph=0; ThreatRing->SetVisibility(false);
        AngelRoot->SetVisibility(false,true); EnemyShield->SetVisibility(false,true);
        ClearAngelHazards();
        SetNotice("CONTACT LOST // FREE EXPLORATION RESUMED");
    }
    if(!bWorldTest || bWorldTestDone) return;
    WorldTestTime+=Dt;
    if(WorldTestStep==0 && WorldTestTime>2)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/OpenWorld.png"),true,false);
        WorldTestStep=1; WorldTestTime=0;
    }
    else if(WorldTestStep>=1 && WorldTestStep<=4 && WorldTestTime>1)
    {
        int I=WorldTestStep-1;
        P->SetActorLocation(Districts[I]+FVector(0,-1000,780)); Rules.Integrity=30; P->Loadout.Shells=0;
        Interact(); bWorldChecksOK=bWorldChecksOK && Rules.Integrity==100 && P->Loadout.Shells==8 && ActiveCharger==I;
        ++WorldTestStep; WorldTestTime=0;
    }
    else if(WorldTestStep==5 && WorldTestTime>1)
    {
        bWorldChecksOK=bWorldChecksOK && SurveyMask==15;
        SelectedDistrict=0; P->SetActorLocation(Districts[0]+FVector(0,5400,780)); Interact();
        bWorldChecksOK=bWorldChecksOK && bWorldEncounter;
        Rules.EnemyField=0; Rules.EnemyHealth=100; LanceCooldown=0;
        Attack(true); bWorldChecksOK=bWorldChecksOK && !bWorldEncounter && CompletedContracts==1;
        auto* Save=Cast<UEvaWorldSave>(UGameplayStatics::LoadGameFromSlot(TEXT("EvaFreeRoam_Test"),0));
        bWorldChecksOK=bWorldChecksOK && Save && Save->SurveyMask==15 && Save->Contracts==1;
        bWorldMap=true; WorldTestStep=6; WorldTestTime=0;
    }
    else if(WorldTestStep==6 && WorldTestTime>1)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/OpenWorldMap.png"),true,false);
        WorldTestStep=7; WorldTestTime=0;
    }
    else if(WorldTestStep==7 && WorldTestTime>1)
    {
        bWorldMap=false;
        P->SetActorLocation(Districts[0]+FVector(0,2500,780));
        StoryView(P->GetActorLocation()+FVector(1800,2200,850),P->GetActorLocation()+FVector(0,0,150),0);
        WorldTestStep=8; WorldTestTime=0;
    }
    else if(WorldTestStep==8 && WorldTestTime>1)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/EvaArmor.png"),true,false);
        WorldTestStep=9; WorldTestTime=0;
    }
    else if(WorldTestStep==9 && WorldTestTime>1)
    {
        bWorldTestDone=true;
        UE_LOG(LogTemp,Display,TEXT("EVA_WORLD_RESULT success=%d districts=%d contracts=%d"),bWorldChecksOK,SurveyMask,CompletedContracts);
        FPlatformMisc::RequestExitWithStatus(false,bWorldChecksOK ? 0:1);
    }
}
