#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "Sound/SoundBase.h"

namespace { const FVector ImpactOrigin(120000,0,0); }

void AEvaGameMode::StartImpact()
{
    if(ImpactRoot) return;
    bStarted=true; bPaused=false; bEnded=false; bVictory=false;
    Chapter=EEvaChapter::Impact; ImpactTime=0; ImpactStage=-1; ImpactShots=0;
    bImpactTest=FParse::Param(FCommandLine::Get(),TEXT("EvaImpactTest"));
    Rules.bConnected=false;
    AngelRoot->SetVisibility(false,true); EnemyShield->SetVisibility(false,true);
    Pilot()->SetHumanMode(true); Pilot()->SetActorLocation(ImpactOrigin+FVector(0,0,-3000));
    ImpactRoot=NewSceneRoot(ImpactOrigin);
    const FLinearColor Dark(.035f,.045f,.06f), Bone(.76f,.79f,.71f), Orange(1,.18f,.018f);
    Shape("Cylinder",FVector(0,0,-140),FVector(310,310,2),Dark,0,ImpactRoot);
    // Dedicated stage: no changes to the first encounter's city or checkpoint.
    FRandomStream Random(2002);
    for(int I=0;I<52;++I)
    {
        float A=I*2*PI/52, R=Random.FRandRange(6500,14500), H=Random.FRandRange(700,3000);
        FVector Pos(FMath::Cos(A)*R,FMath::Sin(A)*R,H*.5f);
        ImpactTowers.Add(Shape("Cube",Pos,FVector(Random.FRandRange(4,10),Random.FRandRange(4,10),H/100),Dark,0,ImpactRoot));
    }
    auto Mech=[&](FVector Pos,FLinearColor Armor,bool Bird)
    {
        auto* Root=NewSceneRoot(ImpactOrigin+Pos);
        Root->AttachToComponent(ImpactRoot,FAttachmentTransformRules::KeepWorldTransform);
        Shape("Sphere",FVector(0,0,850),FVector(4,2.5f,6),Armor,0,Root);
        Shape("Sphere",FVector(0,-25,1260),Bird ? FVector(2.3f,4,1.7f) : FVector(2,1.8f,2.5f),Armor,0,Root);
        Shape("Cube",FVector(0,-142,1260),FVector(1.6f,.12f,.3f),Bird ? FLinearColor(.3f,.015f,.02f) : FLinearColor(.6f,1,.03f),1.8f,Root);
        Shape("Cube",FVector(0,-125,945),FVector(2.6f,.25f,.65f),Bird ? Dark : FLinearColor(1,.4f,.04f),.2f,Root);
        for(int S:{-1,1})
        {
            Shape("Sphere",FVector(S*165,0,470),FVector(1.6f,1.6f,2),Dark,0,Root);
            Shape("Cylinder",FVector(S*165,0,230),FVector(1.5f,1.5f,4.5f),Armor,0,Root);
            Shape("Cube",FVector(S*165,-90,40),FVector(1.7f,3,.8f),Armor,0,Root);
            auto* Arm=Shape("Cylinder",FVector(S*340,0,760),FVector(1.3f,1.3f,6.5f),Armor,0,Root);
            Arm->SetRelativeRotation(FRotator(0,0,S*18));
            if(!Bird && Pos.Y>3000) BerserkArms.Add(Arm);
            Shape("Cube",FVector(S*265,0,1100),FVector(1.2f,1.3f,3.8f),Armor,0,Root);
            if(Bird)
            {
                auto* Wing=Shape("Cube",FVector(S*750,140,1060),FVector(12,1,.5f),Bone,0,Root);
                ImpactWings.Add(Wing);
                for(int F=0;F<8;++F)
                {
                    auto* Feather=Shape("Sphere",FVector(S*(-450+F*145)/12.f,80,-F*44),FVector(1.65f,.5f,5.5f+F*.5f),Bone,0,Wing);
                    // Cancel inherited wing scale, leaving elongated individual feathers.
                    Feather->SetAbsolute(false,false,true);
                    Feather->SetWorldScale3D(FVector(1.65f,.5f,5.5f+F*.5f));
                    Feather->SetRelativeRotation(FRotator(0,0,S*-35));
                }
            }
        }
        if(!Bird) Shape("Cone",FVector(0,0,1480),FVector(.5f,.5f,2.3f),Armor,0,Root);
        return Root;
    };
    FallenEva=Mech(FVector(0,0,0),FLinearColor(.7f,.025f,.015f),false);
    RisingEva=Mech(FVector(0,3800,-1900),FLinearColor(.22f,.035f,.5f),false);
    for(int I=0;I<9;++I) ImpactBirds.Add(Mech(FVector(0,0,5000),Bone,true));
    for(int I=0;I<44;++I)
    {
        FVector Pos(Random.FRandRange(-5600,5600),Random.FRandRange(-5500,7000),0);
        auto* Person=NewSceneRoot(ImpactOrigin+Pos);
        Person->AttachToComponent(ImpactRoot,FAttachmentTransformRules::KeepWorldTransform);
        Shape("Sphere",FVector(0,0,155),FVector(.55f),Bone,.25f,Person);
        Shape("Cylinder",FVector(0,0,75),FVector(.45f,.45f,1.15f),Bone,.25f,Person);
        ImpactPeople.Add(Person);
        auto* Soul=Shape("Sphere",Pos+FVector(0,0,100),FVector(.4f),Orange,3,ImpactRoot);
        Soul->SetVisibility(false); ImpactSouls.Add(Soul);
    }
    ImpactOcean=Shape("Cylinder",FVector(0,0,-220),FVector(1200,1200,1),Orange,.65f,ImpactRoot);
    // Thin expanding rings read as ripples on the LCL surface.
    for(int R=0;R<7;++R)
        for(int I=0;I<48;++I)
        {
            float A=I*2*PI/48;
            auto* Segment=Shape("Cube",FVector::ZeroVector,FVector(1),FLinearColor(1,.5f,.07f),1,ImpactRoot);
            Segment->SetRelativeRotation(FRotator(0,FMath::RadiansToDegrees(A)+90,0));
            Segment->SetVisibility(false); ImpactRings.Add(Segment);
        }
    // A distant human silhouette rises with Instrumentality.
    ImpactFigure=NewSceneRoot(ImpactOrigin+FVector(0,23000,-18000));
    ImpactFigure->AttachToComponent(ImpactRoot,FAttachmentTransformRules::KeepWorldTransform);
    auto* Horizon=ImpactFigure;
    Shape("Sphere",FVector(0,0,13100),FVector(32,20,42),Bone,.8f,Horizon);
    Shape("Sphere",FVector(0,150,14600),FVector(33,20,17),FLinearColor(.55f,.7f,.74f),.5f,Horizon);
    Shape("Cylinder",FVector(0,0,10900),FVector(12,12,15),Bone,.6f,Horizon);
    Shape("Sphere",FVector(0,0,9400),FVector(52,17,27),Bone,.6f,Horizon);
    Shape("Cylinder",FVector(0,0,7000),FVector(24,14,38),Bone,.6f,Horizon);
    Shape("Sphere",FVector(0,0,4750),FVector(35,18,22),Bone,.6f,Horizon);
    for(int S:{-1,1})
    {
        Shape("Sphere",FVector(S*1000,0,2200),FVector(14,14,43),Bone,.6f,Horizon);
        Shape("Sphere",FVector(S*2150,0,9550),FVector(15,16,16),Bone,.6f,Horizon);
        auto* Upper=Shape("Sphere",FVector(S*3750,0,9300),FVector(30,10,10),Bone,.6f,Horizon);
        Upper->SetRelativeRotation(FRotator(0,0,S*10));
        auto* Lower=Shape("Sphere",FVector(S*6200,0,8750),FVector(26,8,8),Bone,.6f,Horizon);
        Lower->SetRelativeRotation(FRotator(0,0,S*15));
        Shape("Sphere",FVector(S*7900,0,8400),FVector(11,5,8),Bone,.6f,Horizon);
        Shape("Sphere",FVector(S*650,-930,13300),FVector(3,.5f,1.1f),FLinearColor(.8f,.015f,.025f),1,Horizon);
    }
    Shape("Sphere",FVector(-16000,35000,19500),FVector(65),FLinearColor(.6f,.015f,.008f),1,ImpactRoot);
    TickImpact(0);
}

void AEvaGameMode::TickImpact(float Dt)
{
    ImpactTime=FMath::Min(ImpactTime+Dt,64.f);
    const float T=ImpactTime;
    const int Stage=T<14 ? 0 : T<28 ? 1 : T<47 ? 2 : 3;
    if(Stage!=ImpactStage)
    {
        ImpactStage=Stage;
        const TCHAR* Name=Stage==0 ? TEXT("Alarm") : Stage==1 ? TEXT("Roar") : Stage==2 ? TEXT("Breach") : TEXT("Breath");
        FString Path=FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name);
        if(auto* Sound=LoadObject<USoundBase>(nullptr,*Path)) UGameplayStatics::PlaySound2D(this,Sound,.65f);
        UE_LOG(LogTemp,Display,TEXT("EVA_IMPACT_STAGE %d"),Stage);
    }
    float Fall=FMath::SmoothStep(0.f,1.f,FMath::Clamp((T-5)/5,0.f,1.f));
    FallenEva->SetRelativeRotation(FRotator(0,0,Fall*86));
    FallenEva->SetRelativeLocation(FVector(0,0,Fall*120));
    FallenEva->SetVisibility(T<35,true);
    for(int I=0;I<ImpactBirds.Num();++I)
    {
        float A=I*2*PI/9+T*.13f;
        float Dive=FMath::SmoothStep(0.f,1.f,FMath::Clamp((T-7)/4,0.f,1.f));
        float Rise=FMath::SmoothStep(0.f,1.f,FMath::Clamp((T-15)/8,0.f,1.f));
        float Radius=FMath::Lerp(FMath::Lerp(3600.f,1050.f,Dive),4700.f,Rise);
        float Height=FMath::Lerp(FMath::Lerp(3100.f,250.f,Dive),6300.f,Rise)+FMath::Sin(T*2+I)*80;
        ImpactBirds[I]->SetRelativeLocation(FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,Height));
        ImpactBirds[I]->SetRelativeRotation(FRotator(0,FMath::RadiansToDegrees(A)-90,0));
        for(int S=0;S<2;++S) ImpactWings[I*2+S]->SetRelativeRotation(FRotator(0,0,(S==0 ? -1:1)*(15+FMath::Sin(T*2.4f+I)*24)));
    }
    float Ascend=FMath::SmoothStep(0.f,1.f,FMath::Clamp((T-14)/12,0.f,1.f));
    RisingEva->SetRelativeLocation(FVector(0,3800,FMath::Lerp(-1900.f,5600.f,Ascend)));
    RisingEva->SetRelativeRotation(FRotator(FMath::Sin(T*15)*(Stage==1 ? 3:0),0,FMath::Sin(T*19)*(Stage==1 ? 4:0)));
    float Flood=FMath::SmoothStep(0.f,1.f,FMath::Clamp((T-28)/19,0.f,1.f));
    float Level=FMath::Lerp(-220.f,1900.f,Flood);
    ImpactFigure->SetVisibility(T>=28,true);
    ImpactFigure->SetRelativeLocation(FVector(0,23000,FMath::Lerp(-18000.f,0.f,Flood)));
    for(int I=0;I<BerserkArms.Num();++I)
        BerserkArms[I]->SetRelativeRotation(FRotator(Stage==1 ? FMath::Sin(T*11)*18:0,0,(I==0 ? -1:1)*(18+Ascend*62)));
    ImpactOcean->SetRelativeLocation(FVector(0,0,Level));
    ImpactOcean->SetVisibility(T>=28);
    for(int I=0;I<ImpactPeople.Num();++I)
    {
        float Dissolve=FMath::Clamp((T-29-I*.18f)/2,0.f,1.f);
        ImpactPeople[I]->SetRelativeScale3D(FVector(FMath::Max(.001f,1-Dissolve)));
        ImpactPeople[I]->SetVisibility(Dissolve<1,true);
        ImpactSouls[I]->SetVisibility(Dissolve>0 && T<49);
        FVector Pos=ImpactPeople[I]->GetRelativeLocation();
        Pos.Z=FMath::Max(Level+100,100+FMath::Max(0.f,T-29-I*.18f)*280);
        ImpactSouls[I]->SetRelativeLocation(Pos);
        ImpactSouls[I]->SetRelativeScale3D(FVector(.4f+Dissolve*1.2f));
    }
    for(int I=0;I<ImpactTowers.Num();++I)
    {
        FVector Pos=ImpactTowers[I]->GetRelativeLocation();
        // The ocean overtakes the skyline; towers sink as the transformation completes.
        Pos.Z=ImpactTowers[I]->GetRelativeScale3D().Z*50-FMath::Max(0.f,T-38)*240;
        ImpactTowers[I]->SetRelativeLocation(Pos);
    }
    for(int I=0;I<ImpactRings.Num();++I)
    {
        int R=I/48, S=I%48; float A=S*2*PI/48;
        float Radius=1800+FMath::Fmod(T*190+R*1900,14000.f);
        ImpactRings[I]->SetVisibility(T>28);
        ImpactRings[I]->SetRelativeLocation(FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,Level+56+FMath::Sin(T+R)*5));
        ImpactRings[I]->SetRelativeScale3D(FVector(Radius*.00132f,.15f,.035f));
    }
    FVector Eye,Target;
    if(Stage==0)
    {
        float A=T*.055f;
        Eye=FVector(FMath::Sin(A)*7300,-FMath::Cos(A)*7300,3200);
        Target=FVector(0,0,1200);
    }
    else if(Stage==1)
    {
        Eye=FVector(-3800,-2500,2600+Ascend*4000);
        Target=RisingEva->GetRelativeLocation()+FVector(0,0,900);
    }
    else
    {
        float Pull=FMath::Clamp((T-28)/30,0.f,1.f);
        Eye=FVector(FMath::Lerp(-8500.f,-13500.f,Pull),FMath::Lerp(-12500.f,-21000.f,Pull),FMath::Lerp(8000.f,11000.f,Pull));
        Target=FVector(0,8000,Stage==2 ? 3800:5800);
    }
    StoryView(ImpactOrigin+Eye,ImpactOrigin+Target,0);
    if(bImpactTest && ImpactShots<=Stage && T>(Stage==0 ? 12:Stage==1 ? 24:Stage==2 ? 42:55))
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Impact_%02d.png"),Stage),true,false);
        ++ImpactShots;
    }
    if(bImpactTest && T>=60)
    {
        bool OK=ImpactShots==4 && ImpactBirds.Num()==9 && !FallenEva->IsVisible() && ImpactOcean->GetRelativeLocation().Z>1800;
        for(auto* Person:ImpactPeople) OK=OK && !Person->IsVisible();
        const int32 Captured=ImpactShots;
        const float OceanHeight=ImpactOcean->GetRelativeLocation().Z;
        bImpactTest=false; ImpactTime=0; ImpactStage=-1; TickImpact(0);
        bool ReplayOK=ImpactStage==0 && FallenEva->IsVisible() && !ImpactOcean->IsVisible();
        for(auto* Person:ImpactPeople) ReplayOK=ReplayOK && Person->IsVisible() && Person->GetRelativeScale3D().Equals(FVector(1));
        for(auto* Tower:ImpactTowers) ReplayOK=ReplayOK && FMath::IsNearlyEqual(Tower->GetRelativeLocation().Z,Tower->GetRelativeScale3D().Z*50);
        OK=OK && ReplayOK;
        UE_LOG(LogTemp,Display,TEXT("EVA_IMPACT_RESULT success=%d stages=%d birds=%d ocean=%.0f replay=%d"),OK,Captured,ImpactBirds.Num(),OceanHeight,ReplayOK);
        FPlatformMisc::RequestExitWithStatus(false,OK ? 0:1);
    }
}
