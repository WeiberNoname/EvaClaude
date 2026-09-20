#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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
    Shape("Cube",WorldCenter+FVector(0,0,-100),FVector(900,900,2),FLinearColor(.035f,.07f,.065f),0,nullptr,true);
    for(int I=-2;I<=2;++I)
    {
        float V=I*22000;
        Shape("Cube",WorldCenter+FVector(V,0,8),FVector(27,880,.12f),FLinearColor(.018f,.023f,.035f));
        Shape("Cube",WorldCenter+FVector(0,V,8),FVector(880,27,.12f),FLinearColor(.018f,.023f,.035f));
        for(int J=-22;J<=22;++J)
        {
            Shape("Cube",WorldCenter+FVector(V,J*1900,18),FVector(.18f,7,.06f),FLinearColor(.65f,.56f,.36f));
            Shape("Cube",WorldCenter+FVector(J*1900,V,18),FVector(7,.18f,.06f),FLinearColor(.65f,.56f,.36f));
        }
    }
    FRandomStream Rand(2015);
    for(int D=0;D<4;++D)
    {
        FVector Base=Districts[D];
        for(int X=-2;X<=2;++X) for(int Y=-2;Y<=2;++Y)
        {
            if(X==0 || Y==0) continue;
            float H=D==0 ? Rand.FRandRange(3200,7000) : D==2 ? Rand.FRandRange(900,2200) : Rand.FRandRange(1600,3800);
            FEvaBuilding B; B.Center=Base+FVector(X*4800,Y*4800,H*.5f);
            FLinearColor Color=D==0 ? FLinearColor(.065f,.10f,.15f) : D==1 ? FLinearColor(.14f,.17f,.18f) : D==2 ? FLinearColor(.14f,.17f,.11f) : FLinearColor(.17f,.11f,.09f);
            B.Mesh=Shape("Cube",B.Center,FVector(18,19,H/100),Color,0,nullptr,true);
            B.Windows.Add(Shape("Cube",B.Center+FVector(0,0,H*.5f+40),FVector(18.6f,19.6f,.8f),FLinearColor(.015f,.025f,.035f)));
            for(int F=1;F<H/450;++F)
            {
                FLinearColor Light(.10f,.4f,.44f);
                B.Windows.Add(Shape("Cube",FVector(B.Center.X,B.Center.Y-955,F*450),FVector(15,.04f,.14f),Light,1));
                B.Windows.Add(Shape("Cube",FVector(B.Center.X-905,B.Center.Y,F*450),FVector(.04f,16,.14f),Light,1));
            }
            // Vertical facade strips and rooftop equipment make the skyline readable at Eva scale.
            B.Windows.Add(Shape("Cube",B.Center+FVector(910,0,0),FVector(.12f,1,H/100),FLinearColor(.035f,.05f,.065f)));
            B.Windows.Add(Shape("Cube",B.Center+FVector(0,0,H*.5f+150),FVector(6,8,2.3f),FLinearColor(.05f,.055f,.065f)));
            Buildings.Add(B);
        }
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
    // Harbor cranes, container yards, industrial tanks, and an upland ridgeline.
    Shape("Cube",WorldCenter+FVector(10000,-49000,-160),FVector(1000,120,1),FLinearColor(.015f,.11f,.17f),.1f);
    for(int I=0;I<7;++I)
    {
        FVector Port=WorldCenter+FVector(7000+I*4200,-36500,0);
        Shape("Cube",Port+FVector(0,0,2200),FVector(1.8f,2.5f,44),FLinearColor(.55f,.16f,.035f));
        Shape("Cube",Port+FVector(0,-1600,4400),FVector(2.5f,50,2.5f),FLinearColor(.55f,.16f,.035f));
        Shape("Cylinder",Port+FVector(0,-3500,3000),FVector(.12f,.12f,28),FLinearColor(.18f,.2f,.2f));
        Shape("Cube",Port+FVector(1400,800,450),FVector(16,8,9),FLinearColor(.18f,.28f,.29f));
        Shape("Cylinder",Districts[3]+FVector(13000,I*1900-6000,1000),FVector(17,17,20),FLinearColor(.21f,.24f,.26f));
    }
    for(int I=0;I<14;++I)
        Shape("Sphere",WorldCenter+FVector(-47000,I*7200-42000,-600),FVector(100,100,80+I%4*20),FLinearColor(.055f,.12f,.085f));
}

void AEvaGameMode::StartOpenWorld()
{
    StartBattle(); bOpenWorld=true; bWorldEncounter=false; bWorldMap=false;
    bWorldTest=FParse::Param(FCommandLine::Get(),TEXT("EvaWorldTest"));
    bShooterTest=FParse::Param(FCommandLine::Get(),TEXT("EvaShooterTest"));
    bCompanionTest=FParse::Param(FCommandLine::Get(),TEXT("EvaCompanionTest"));
    bDynamicTest=FParse::Param(FCommandLine::Get(),TEXT("EvaDynamicTest"));
    BuildOpenWorld(); Chargers.Empty();
    for(FVector D:Districts) Chargers.Add(D+FVector(0,0,100));
    Rules.Battery=120; Rules.bConnected=true; ActiveCharger=0;
    Pilot()->SetActorLocation(Districts[0]+FVector(0,-1800,780)); Pilot()->bLock=false;
    Pilot()->Loadout.AcquireCannon(); bCannonTaken=true;
    AngelRoot->SetVisibility(false,true); EnemyShield->SetVisibility(false,true);
    SurveyMask=0; CompletedContracts=0;
    FString Slot=bWorldTest ? TEXT("EvaFreeRoam_Test") : bShooterTest ? TEXT("EvaShooter_Test") : bDynamicTest ? TEXT("EvaDynamic_Test") : bCompanionTest ? TEXT("EvaCompanion_Test") : TEXT("EvaFreeRoam");
    if(!bWorldTest && !bShooterTest && !bDynamicTest && !bCompanionTest && UGameplayStatics::DoesSaveGameExist(Slot,0)) if(auto* Save=Cast<UEvaWorldSave>(UGameplayStatics::LoadGameFromSlot(Slot,0)))
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
    if(!UGameplayStatics::SaveGameToSlot(Save,bWorldTest ? TEXT("EvaFreeRoam_Test") : bShooterTest ? TEXT("EvaShooter_Test") : bDynamicTest ? TEXT("EvaDynamic_Test") : bCompanionTest ? TEXT("EvaCompanion_Test") : TEXT("EvaFreeRoam"),0)) SetNotice("PROGRESS SAVE FAILED // CURRENT SESSION CONTINUES");
}
FString AEvaGameMode::WorldPrompt() const
{
    auto* P=Pilot(); if(!P) return "";
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
