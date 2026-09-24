#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const FLinearColor Concrete(.29f,.32f,.31f), Steel(.08f,.12f,.14f), Amber(.92f,.36f,.045f), Cyan(.13f,.55f,.58f);
    void Add(UInstancedStaticMeshComponent* Mesh,FVector Position,FVector Scale,FRotator Rotation=FRotator::ZeroRotator)
    { Mesh->AddInstance(FTransform(Rotation,Position,Scale)); }
}

UMaterialInstanceDynamic* AEvaGameMode::DistrictMaterial(FLinearColor Color,int32 Type,float Glow)
{
    const FString Key=Color.ToString()+FString::Printf(TEXT("/%d/%.2f"),Type,Glow);
    if(auto* Existing=CityMaterials.FindRef(Key)) return Existing;
    auto* Material=UMaterialInstanceDynamic::Create(CitySurface ? CitySurface:Surface,this);
    Material->SetVectorParameterValue(TEXT("Color"),Color);
    Material->SetScalarParameterValue(TEXT("SurfaceType"),Type);
    Material->SetScalarParameterValue(TEXT("Roughness"),Type==1 ? .3f:Type==2 ? .56f:.89f);
    Material->SetScalarParameterValue(TEXT("Metallic"),Type==1 ? .35f:Type==2 ? .5f:0);
    Material->SetScalarParameterValue(TEXT("Glow"),Glow);
    CityMaterials.Add(Key,Material); return Material;
}

UStaticMeshComponent* AEvaGameMode::CityBox(FVector Position,FVector Scale,FLinearColor Color,int32 Type,USceneComponent* Parent,bool Collision,float Glow)
{
    auto* Mesh=Shape("Cube",Position,Scale,Color,0,Parent,Collision);
    Mesh->SetMaterial(0,DistrictMaterial(Color,Type,Glow)); return Mesh;
}

UInstancedStaticMeshComponent* AEvaGameMode::CityInstances(USceneComponent* Parent,FLinearColor Color,int32 Type,float Glow)
{
    auto* Mesh=NewObject<UInstancedStaticMeshComponent>(VisualWorld);
    Mesh->SetupAttachment(Parent ? Parent:VisualWorld->GetRootComponent());
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(MeshCache.FindRef(TEXT("Cube")));
    Mesh->SetMaterial(0,DistrictMaterial(Color,Type,Glow));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCastShadow(false); Mesh->SetCullDistances(30000,48000);
    Mesh->RegisterComponent(); Meshes.Add(Mesh); return Mesh;
}

void AEvaGameMode::CitySign(USceneComponent* Parent,FVector Position,const FString& Text,float Size,FColor Color,FRotator Rotation)
{
    auto* Sign=NewObject<UTextRenderComponent>(VisualWorld);
    Sign->SetupAttachment(Parent ? Parent:VisualWorld->GetRootComponent());
    Sign->SetRelativeLocation(Position); Sign->SetRelativeRotation(Rotation);
    Sign->SetHorizontalAlignment(EHTA_Center); Sign->SetVerticalAlignment(EVRTA_TextCenter);
    Sign->SetWorldSize(Size); Sign->SetText(FText::FromString(Text)); Sign->SetTextRenderColor(Color);
    Sign->SetCastShadow(false); Sign->RegisterComponent();
}

void AEvaGameMode::BuildCentralTower(FVector Base,float Height,int32 Style)
{
    FEvaBuilding B; B.Center=Base+FVector(0,0,Height*.5f); B.DistrictRoot=NewSceneRoot(Base);
    B.FacadeColor=Style%3==0 ? Concrete:Style%3==1 ? FLinearColor(.16f,.22f,.25f):FLinearColor(.32f,.28f,.24f);
    const float Width=Style%2 ? 21.f:24.f, Depth=22.f;
    B.Mesh=CityBox(FVector(0,0,Height*.5f),FVector(Width,Depth,Height/100),B.FacadeColor,0,B.DistrictRoot,true);
    auto* Frames=CityInstances(B.DistrictRoot,Steel,2); B.Windows.Add(Frames);
    auto* Glass=CityInstances(B.DistrictRoot,FLinearColor(.035f,.085f,.11f),1); B.Windows.Add(Glass);
    auto* Lit=CityInstances(B.DistrictRoot,FLinearColor(.58f,.39f,.18f),1,.6f); B.Windows.Add(Lit);
    auto* Panels=CityInstances(B.DistrictRoot,B.FacadeColor*.72f,0); B.Windows.Add(Panels);
    const float W=Width*50+5,D=Depth*50+5;
    for(int Floor=1;Floor<Height/320-1;++Floor)
    {
        float Z=Floor*320;
        Add(Frames,FVector(0,0,Z-105),FVector(Width+.25f,Depth+.25f,.32f));
        for(int Column=-3;Column<=3;++Column) for(int Side:{-1,1})
        {
            auto* Pane=(Floor+Column+Style)%5==0 ? Lit:Glass;
            Add(Pane,FVector(Column*285,Side*D,Z),FVector(2.15f,.08f,1.75f));
            Add(Pane,FVector(Side*W,Column*285,Z),FVector(.08f,2.15f,1.75f));
        }
    }
    for(int Side:{-1,1}) for(int Column:{-1,0,1})
    {
        Add(Frames,FVector(Column*W*.8f,Side*(D+12),Height*.5f),FVector(.38f,.38f,Height/100));
        Add(Frames,FVector(Side*(W+12),Column*D*.8f,Height*.5f),FVector(.38f,.38f,Height/100));
    }
    Add(Panels,FVector(0,0,190),FVector(Width+2,Depth+2,3.8f));
    Add(Frames,FVector(0,-D-25,380),FVector(10,3,.24f));
    Add(Glass,FVector(0,-D-45,180),FVector(6,.15f,3.1f));
    Add(Panels,FVector(0,0,Height+50),FVector(Width+1,Depth+1,1));
    Add(Panels,FVector(260,170,Height+300),FVector(10,9,5));
    for(int I=0;I<4;++I) Add(Frames,FVector(-600+I*300,-400,Height+180),FVector(2.2f,4,2.6f));
    Add(Frames,FVector(500,500,Height+750),FVector(.3f,.3f,15));
    if(Style%4==0)
    {
        auto* Crown=CityInstances(B.DistrictRoot,Cyan,2,.9f); B.Windows.Add(Crown);
        Add(Crown,FVector(0,0,Height-500),FVector(Width+.45f,Depth+.45f,.18f));
    }
    auto* Rubble=CityInstances(nullptr,FLinearColor(.13f,.135f,.13f),0); B.Rubble=Rubble;
    FRandomStream Rand(Style+309);
    for(int I=0;I<22;++I)
        Add(Rubble,Base+FVector(Rand.FRandRange(-850,850),Rand.FRandRange(-850,850),Rand.FRandRange(30,180)),
            FVector(Rand.FRandRange(1.5f,4.5f),Rand.FRandRange(1.2f,4.f),Rand.FRandRange(.4f,2.1f)),FRotator(Rand.FRandRange(-30,30),Rand.FRandRange(-180,180),Rand.FRandRange(-20,20)));
    Rubble->SetVisibility(false); Buildings.Add(B);
}

void AEvaGameMode::BuildCentralDistrict()
{
    CitySurface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_City.M_City"));
    DustSurface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Dust.M_Dust"));
    const FVector Base=Districts[0];
    auto* Kit=NewSceneRoot(Base);
    auto* Asphalt=CityInstances(Kit,FLinearColor(.055f,.065f,.075f),3);
    auto* Paving=CityInstances(Kit,FLinearColor(.24f,.25f,.24f));
    auto* Paint=CityInstances(Kit,FLinearColor(.68f,.64f,.46f),2);
    auto* Metal=CityInstances(Kit,Steel,2);
    auto* Lamps=CityInstances(Kit,Amber,2,1.3f);
    for(int Road=-3;Road<=3;++Road)
    {
        float V=Road*4800+2400;
        Add(Asphalt,FVector(V,0,14),FVector(21,342,.1f));
        Add(Asphalt,FVector(0,V,15),FVector(342,21,.1f));
        for(int Side:{-1,1})
        {
            Add(Paving,FVector(V+Side*1170,0,24),FVector(2.5f,342,.28f));
            Add(Paving,FVector(0,V+Side*1170,25),FVector(342,2.5f,.28f));
        }
        for(int Mark=-18;Mark<=18;++Mark)
        {
            Add(Paint,FVector(V,Mark*900,25),FVector(.15f,3,.05f));
            Add(Paint,FVector(Mark*900,V,26),FVector(3,.15f,.05f));
        }
    }
    // Clear deployment boulevard and eastern supply route connect the detailed blocks.
    Add(Asphalt,FVector(0,0,18),FVector(28,360,.1f));
    Add(Asphalt,FVector(14500,0,19),FVector(42,350,.1f));
    for(int Row=-3;Row<=3;++Row) for(int Side:{-1,1})
    {
        float Y=Row*4300;
        Add(Metal,FVector(Side*1800,Y,500),FVector(.2f,.2f,10));
        Add(Metal,FVector(Side*1570,Y,960),FVector(4.8f,.25f,.25f));
        Add(Lamps,FVector(Side*1410,Y,945),FVector(1.5f,.55f,.08f));
        for(int Stripe=-4;Stripe<=4;++Stripe) Add(Paint,FVector(Stripe*230,Y+450,29),FVector(1.3f,5.2f,.04f));
    }
    int Style=0;
    for(int X=-2;X<=2;++X) for(int Y=-2;Y<=2;++Y) if(X && Y)
    {
        const float Height=3900.f+float((Style*719)%3300);
        BuildCentralTower(Base+FVector(X*4800,Y*4800,0),Height,Style++);
        // Parked human-scale vehicles and traffic islands reinforce the difference in scale.
        Add(Metal,FVector(X*4800-1250,Y*4800-300,90),FVector(1.65f,4.3f,1.55f));
        Add(Paving,FVector(X*4800+1320,Y*4800,32),FVector(2.4f,18,.45f));
    }
    // A covered access tunnel leaves a 58 m clear opening for both units.
    FVector Tunnel(14500,13700,0);
    for(int Side:{-1,1}) CityBox(Tunnel+FVector(Side*3200,0,2100),FVector(6,64,42),Concrete,0,Kit,true);
    CityBox(Tunnel+FVector(0,0,4400),FVector(70,64,5),Concrete,0,Kit,true);
    CityBox(Tunnel+FVector(0,-3270,4220),FVector(57,.5f,4.8f),Steel,2,Kit);
    CitySign(Kit,Tunnel+FVector(0,-3310,4230),TEXT("GEOFRONT / ACCESS E-3"),180,FColor(245,175,70));
    for(int I=0;I<5;++I) Add(Lamps,Tunnel+FVector(0,-2600+I*1200,4110),FVector(45,.3f,.1f));
    // Segmented suspended lines sag between pylons; decorative wires never block the Eva.
    for(int I=0;I<4;++I)
    {
        FVector P(18000,-13000+I*8200,0);
        Add(Metal,P+FVector(0,0,3900),FVector(1.2f,1.5f,78));
        Add(Metal,P+FVector(0,0,7500),FVector(18,1.4f,1.2f));
        if(I==3) continue;
        for(int Wire:{-1,0,1}) for(int Segment=0;Segment<10;++Segment)
        {
            auto Point=[&](float T) { return P+FVector(Wire*700,T*8200,7500-650*FMath::Sin(T*PI)); };
            FVector A=Point(Segment/10.f), B=Point((Segment+1)/10.f), Delta=B-A;
            Add(Metal,(A+B)*.5f,FVector(Delta.Size()/100,.055f,.055f),Delta.Rotation());
        }
    }
    // Armory 07: retracting doors, visible weapon elevator, and a replenishing rack.
    CityArmoryPosition=Base+FVector(0,-13700,0); CityArmoryRoot=NewSceneRoot(CityArmoryPosition);
    for(int Side:{-1,1})
    {
        CityBox(FVector(Side*1900,0,2900),FVector(13,36,58),Concrete,0,CityArmoryRoot,true);
        CityBox(FVector(Side*1900,-1830,2900),FVector(8,.3f,48),Steel,2,CityArmoryRoot);
        for(int I=0;I<7;++I) CityBox(FVector(Side*1900,-1860,700+I*680),FVector(7,.2f,.2f),Amber,2,CityArmoryRoot,false,.7f);
    }
    CityBox(FVector(0,0,5700),FVector(50,36,5),Concrete,0,CityArmoryRoot,true);
    CityBox(FVector(0,1300,2000),FVector(25,5,40),Steel,2,CityArmoryRoot,true);
    CitySign(CityArmoryRoot,FVector(0,-1840,5050),TEXT("07 / WEAPON SUPPLY"),205,FColor(255,198,92));
    CityDoorLeft=CityBox(FVector(-640,-1700,1450),FVector(12.8f,1,29),Steel,2,CityArmoryRoot,true);
    CityDoorRight=CityBox(FVector(640,-1700,1450),FVector(12.8f,1,29),Steel,2,CityArmoryRoot,true);
    CityGunRoot=NewSceneRoot(FVector::ZeroVector); CityGunRoot->AttachToComponent(CityArmoryRoot,FAttachmentTransformRules::KeepRelativeTransform);
    CityBox(FVector(0,0,-180),FVector(22,13,1.2f),Steel,2,CityGunRoot);
    BuildCannon(CityGunRoot);
    auto* BayLight=NewObject<URectLightComponent>(VisualWorld);
    BayLight->SetupAttachment(CityArmoryRoot); BayLight->SetMobility(EComponentMobility::Movable);
    BayLight->SetRelativeLocation(FVector(0,-1100,3600)); BayLight->SetRelativeRotation(FRotator(-45,90,0));
    BayLight->SetIntensity(45000); BayLight->SetAttenuationRadius(5000); BayLight->SetSourceWidth(1800); BayLight->SetSourceHeight(100);
    BayLight->SetLightColor(FLinearColor(1,.78f,.48f)); BayLight->SetCastShadows(false); BayLight->RegisterComponent();
    // Three marked emergency caches lead along the eastern road to the access tunnel.
    for(int I=0;I<3;++I)
    {
        FVector Position=Base+FVector(12600,-8500+I*8800,0); SupplyPositions.Add(Position);
        auto* Relay=NewSceneRoot(Position);
        CityBox(FVector(0,0,230),FVector(7,6,4.6f),Concrete,0,Relay);
        CityBox(FVector(0,0,850),FVector(3.7f,3.7f,9),Steel,2,Relay);
        auto* Lamp=CityBox(FVector(0,-191,1000),FVector(3,.1f,2),Cyan,2,Relay,false,2); SupplyLamps.Add(Lamp);
        CitySign(Relay,FVector(0,-205,650),FString::Printf(TEXT("SUPPLY / 0%d"),I+1),85,FColor(155,230,230));
        CitySign(Relay,FVector(0,-330,1550),TEXT("EVACUATION ROUTE"),90,FColor(230,200,125));
        for(int Side:{-1,1}) CityBox(FVector(Side*430,0,190),FVector(.5f,.5f,3.8f),Amber,2,Relay);
    }
    CitySign(Kit,FVector(-2300,-3500,2000),TEXT("TOKYO-3\nCENTRAL DEFENSE DISTRICT"),135,FColor(205,220,220));
    auto Loop=[&](const TCHAR* Name) -> UAudioComponent*
    {
        FString Path=FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name);
        auto* Sound=LoadObject<USoundWave>(nullptr,*Path); if(!Sound) return nullptr;
        Sound->bLooping=true; Sound->VirtualizationMode=EVirtualizationMode::PlayWhenSilent;
        return UGameplayStatics::SpawnSound2D(this,Sound,.01f,1,0,nullptr,false,false);
    };
    CityAmbience=Loop(TEXT("CityWind")); CityHum=Loop(TEXT("CityHum"));
}

void AEvaGameMode::DistrictSound(const TCHAR* Name,FVector Position,float Volume)
{
    if(!CityAttenuation)
    {
        CityAttenuation=NewObject<USoundAttenuation>(this);
        CityAttenuation->Attenuation.bAttenuate=true; CityAttenuation->Attenuation.bSpatialize=true;
        CityAttenuation->Attenuation.AttenuationShapeExtents=FVector(1600);
        CityAttenuation->Attenuation.FalloffDistance=20000;
    }
    FString Path=FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name);
    if(auto* Sound=LoadObject<USoundBase>(nullptr,*Path)) UGameplayStatics::PlaySoundAtLocation(this,Sound,Position,FRotator::ZeroRotator,Volume,1,0,CityAttenuation);
}

void AEvaGameMode::DistrictDust(FVector Position)
{
    for(int I=0;I<6 && Effects.Num()<96;++I)
    {
        float Angle=I*PI/3;
        auto* Cloud=EffectShape(TEXT("Sphere"),Position+FVector(FMath::Cos(Angle)*600,FMath::Sin(Angle)*600,230+I*70),FVector(4,4,2.5f),Concrete,0,2.4f+I*.1f,FVector(3,3,1.2f));
        if(DustSurface) Cloud->SetMaterial(0,DustSurface);
    }
}

void AEvaGameMode::DamageDistrictBuilding(FEvaBuilding& B)
{
    if(!B.DistrictRoot || B.bDestroyed) return;
    ++B.DamageStage;
    if(B.DamageStage==1)
    {
        B.Mesh->SetMaterial(0,DistrictMaterial(B.FacadeColor*.42f,0));
        for(int I=0;I<B.Windows.Num();++I) if(I==2 || I==4) B.Windows[I]->SetVisibility(false);
        DistrictDust(FVector(B.Center.X,B.Center.Y,0));
        DistrictSound(TEXT("Impact"),B.Center,.65f);
    }
    else
    {
        B.bDestroyed=true; B.CollapseTime=.01f; ++BuildingsLost;
        B.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        DistrictDust(FVector(B.Center.X,B.Center.Y,0));
        DistrictSound(TEXT("Collapse"),B.Center,.85f);
    }
}

FString AEvaGameMode::DistrictPrompt() const
{
    const auto* P=Pilot(); if(!P || !bOpenWorld) return TEXT("");
    if(FVector::Dist2D(P->GetActorLocation(),CityArmoryPosition+FVector(0,-2700,0))<1750)
        return !bCityArmoryOpening ? TEXT("E / OPEN ARMORY 07") : CityArmoryOpen<1 ? TEXT("ARMORED DOORS OPENING / WEAPON LIFT RISING") : CityArmoryStock>0 ? TEXT("ARMORY / REPLENISHING RACK") : TEXT("E / RETRIEVE CANNON + AMMUNITION");
    for(int I=0;I<SupplyPositions.Num();++I)
        if(!(SupplyMask&(1<<I)) && FVector::Dist2D(P->GetActorLocation(),SupplyPositions[I])<1500) return TEXT("E / RECOVER EMERGENCY POWER + SHELLS");
    return TEXT("");
}

bool AEvaGameMode::DistrictInteract()
{
    auto* P=Pilot(); if(!P || DistrictPrompt().IsEmpty()) return false;
    if(FVector::Dist2D(P->GetActorLocation(),CityArmoryPosition+FVector(0,-2700,0))<1750)
    {
        if(!bCityArmoryOpening) { bCityArmoryOpening=true; DistrictSound(TEXT("Hydraulics"),CityArmoryPosition,.6f); SetNotice(TEXT("ARMORY 07 // DOORS RETRACTING / WEAPON LIFT ONLINE")); }
        else if(CityArmoryOpen>=1 && CityArmoryStock<=0)
        {
            P->Loadout.AcquireCannon(); P->ReloadTime=0; P->bFireHeld=false; P->bCharging=false; P->CannonCharge=0;
            P->PickupStart=CityGunRoot->GetComponentLocation(); P->PickupTime=1.1f;
            CityArmoryStock=12; CityGunRoot->SetVisibility(false,true);
            SetNotice(TEXT("CANNON RECEIVED // 8 LOADED / 24 RESERVE")); DistrictSound(TEXT("Launch"),CityArmoryPosition,.4f);
        }
        return true;
    }
    for(int I=0;I<SupplyPositions.Num();++I) if(!(SupplyMask&(1<<I)) && FVector::Dist2D(P->GetActorLocation(),SupplyPositions[I])<1500)
    {
        SupplyMask|=1<<I; Rules.Battery=FMath::Min(120.f,Rules.Battery+25);
        P->Loadout.ReserveShells=FMath::Min(24,P->Loadout.ReserveShells+8);
        SupplyLamps[I]->SetMaterial(0,DistrictMaterial(FLinearColor(.22f,.55f,.12f),2,.3f));
        if(SupplyMask==7) { Rules.Integrity=FMath::Min(100.f,Rules.Integrity+30); if(Wingman) Wingman->Integrity=FMath::Min(100.f,Wingman->Integrity+30); }
        DistrictSound(TEXT("Launch"),SupplyPositions[I],.35f);
        SetNotice(SupplyMask==7 ? TEXT("SUPPLY ROUTE COMPLETE // BOTH UNITS REPAIRED +30") : TEXT("EMERGENCY CACHE // +25 POWER / +8 RESERVE SHELLS")); return true;
    }
    return false;
}

void AEvaGameMode::ResetDistrict()
{
    SupplyMask=0; CityArmoryOpen=0; CityArmoryStock=0; bCityArmoryOpening=false; CityStepClock=0;
    for(auto& B:Buildings) if(B.DistrictRoot)
    {
        B.DamageStage=0; B.CollapseTime=0; B.DistrictRoot->SetWorldScale3D(FVector(1)); B.DistrictRoot->SetWorldRotation(FRotator::ZeroRotator);
        B.Mesh->SetMaterial(0,DistrictMaterial(B.FacadeColor,0)); B.Rubble->SetVisibility(false);
    }
    for(auto* Lamp:SupplyLamps) Lamp->SetMaterial(0,DistrictMaterial(Cyan,2,2));
    if(CityGunRoot) CityGunRoot->SetVisibility(true,true);
}

void AEvaGameMode::SetDistrictLighting(bool Enabled)
{
    if(WorldSun)
    {
        WorldSun->SetActorRotation(Enabled ? FRotator(-16,-48,0):FRotator(-24,-35,0));
        WorldSun->GetLightComponent()->SetIntensity(Enabled ? 5.2f:6.f);
        WorldSun->GetLightComponent()->SetLightColor(Enabled ? FLinearColor(1,.65f,.4f):FLinearColor(1,.76f,.6f));
    }
    if(WorldFill)
    {
        WorldFill->SetActorRotation(Enabled ? FRotator(-30,110,0):FRotator(-35,-140,0));
        WorldFill->GetLightComponent()->SetIntensity(Enabled ? 3.6f:4.5f);
    }
}

void AEvaGameMode::TickDistrict(float Dt)
{
    auto* P=Pilot(); if(!P || !bWorldBuilt) return;
    const bool Active=bOpenWorld && IsActive();
    const float Proximity=Active ? FMath::Clamp(1.f-FMath::Max(0.f,FVector::Dist2D(P->GetActorLocation(),Districts[0])-17000)/13000,0.f,1.f):0;
    if(CityAmbience) { CityAmbience->SetPaused(!Active); CityAmbience->SetVolumeMultiplier(Proximity*.24f); }
    if(CityHum) { CityHum->SetPaused(!Active); CityHum->SetVolumeMultiplier(Proximity*(bWorldEncounter ? .045f:.13f)); }
    if(!Active) return;
    if(bCityArmoryOpening) CityArmoryOpen=FMath::Min(1.f,CityArmoryOpen+Dt/2.6f);
    CityDoorLeft->SetRelativeLocation(FVector(-640-1450*CityArmoryOpen,-1700,1450));
    CityDoorRight->SetRelativeLocation(FVector(640+1450*CityArmoryOpen,-1700,1450));
    CityGunRoot->SetRelativeLocation(FVector(0,0,350+1100*CityArmoryOpen));
    CityArmoryStock=FMath::Max(0.f,CityArmoryStock-Dt); CityGunRoot->SetVisibility(CityArmoryStock<=0,true);
    for(auto& B:Buildings) if(B.DistrictRoot && B.CollapseTime>0 && B.CollapseTime<2)
    {
        B.CollapseTime=FMath::Min(2.f,B.CollapseTime+Dt);
        float T=B.CollapseTime/2, Ease=T*T;
        B.DistrictRoot->SetWorldScale3D(FVector(1,1,1-Ease*.95f));
        B.DistrictRoot->SetWorldRotation(FRotator(FMath::Sin(T*PI)*2,0,FMath::Sin(T*PI*3)*.65f));
        if(B.CollapseTime>=2)
        {
            B.Mesh->SetVisibility(false); for(auto* Part:B.Windows) Part->SetVisibility(false);
            B.Rubble->SetVisibility(true); DistrictDust(FVector(B.Center.X,B.Center.Y,0));
        }
    }
    CityStepClock-=Dt;
    if(Proximity>0 && P->bGrounded && P->MoveVelocity.Size2D()>500 && CityStepClock<=0)
    {
        CityStepClock=P->bSprinting ? .34f:.5f; DistrictSound(TEXT("MechStep"),P->GetActorLocation()-FVector(0,0,700),.27f);
    }
}
