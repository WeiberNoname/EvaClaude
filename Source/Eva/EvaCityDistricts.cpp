#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

namespace
{
    FTransform Pose(FVector P,FVector Size,FRotator R=FRotator::ZeroRotator) { return FTransform(R,P,Size/100); }

    // Static, non-colliding dressing batched per root with per-instance colour.
    struct FEvaDressing
    {
        AEvaGameMode* Game;
        USceneComponent* Root;
        TMap<FString,UInstancedStaticMeshComponent*> Batches;
        FEvaDressing(AEvaGameMode* InGame,USceneComponent* InRoot):Game(InGame),Root(InRoot) {}
        UInstancedStaticMeshComponent* Batch(int32 Type,float Glow,const TCHAR* Mesh,bool bLandmark)
        {
            const FString Key=FString::Printf(TEXT("%d/%.2f/%s/%d"),Type,Glow,Mesh,int32(bLandmark));
            if(auto* Found=Batches.FindRef(Key)) return Found;
            auto* Instances=Game->CityInstances(Root,FLinearColor::White,Type,Glow,Mesh);
            Instances->SetNumCustomDataFloats(3);
            // Landmarks stay visible across the map and ground themselves with shadows.
            if(bLandmark) { Instances->SetCullDistances(0,0); Instances->SetCastShadow(true); }
            Batches.Add(Key,Instances); return Instances;
        }
        void Box(FLinearColor C,int32 Type,FVector P,FVector Size,FRotator R=FRotator::ZeroRotator,const TCHAR* Mesh=TEXT("Cube"),float Glow=0,bool bLandmark=false)
        {
            auto* Instances=Batch(Type,Glow,Mesh,bLandmark);
            const int32 Index=Instances->AddInstance(Pose(P,Size,R));
            const float Data[3]={C.R,C.G,C.B}; Instances->SetCustomData(Index,MakeArrayView(Data,3));
        }
        void Beam(FLinearColor C,int32 Type,FVector A,FVector B,float Thickness,float Glow=0,bool bLandmark=false)
        {
            const FVector Delta=B-A;
            Box(C,Type,(A+B)*.5f,FVector(Delta.Size(),Thickness,Thickness),Delta.Rotation(),TEXT("Cube"),Glow,bLandmark);
        }
    };

    FEvaArchSpec Spec(EEvaArch Kind,int32 District,float W,float D,float H,int32 Style,FLinearColor Facade,FLinearColor Accent,const TCHAR* Label=TEXT(""))
    {
        FEvaArchSpec S; S.Kind=Kind; S.District=District; S.Width=W; S.Depth=D; S.Height=H; S.Style=Style; S.Facade=Facade; S.Accent=Accent; S.Label=Label;
        return S;
    }
    FVector Lot(FVector Base,int32 X,int32 Y) { return Base+FVector(X*4800,Y*4800,0); }
    const FLinearColor Steel(.08f,.12f,.14f), Concrete(.29f,.3f,.28f), Paint(.72f,.7f,.6f);
}

void AEvaGameMode::BuildStreetGrid(int32 District,FVector Base,int32 Blocks,bool bTrees,bool bPoles)
{
    auto* Root=NewSceneRoot(Base);
    FEvaDressing Street(this,Root);
    FRandomStream Rand(District*977+11);
    const FLinearColor Asphalt(.055f,.065f,.075f), Pave(.24f,.25f,.24f), Line(.68f,.64f,.46f), Zebra(.74f,.74f,.68f);
    const float Half=(Blocks+.5f)*4800+1050;
    const FLinearColor Cars[]={FLinearColor(.75f,.75f,.72f),FLinearColor(.05f,.06f,.07f),FLinearColor(.5f,.05f,.04f),FLinearColor(.08f,.16f,.4f),FLinearColor(.45f,.46f,.47f),FLinearColor(.7f,.55f,.1f)};
    const FLinearColor Vending[]={FLinearColor(.8f,.05f,.04f),FLinearColor(.1f,.3f,.8f),FLinearColor(.85f,.85f,.82f)};
    for(int32 Road=-Blocks-1;Road<=Blocks;++Road)
    {
        const float V=Road*4800+2400;
        Street.Box(Asphalt,3,FVector(V,0,14),FVector(2100,Half*2,10));
        Street.Box(Asphalt,3,FVector(0,V,15),FVector(Half*2,2100,10));
        for(int32 Block=-Blocks;Block<=Blocks;++Block)
        {
            const float U=Block*4800;
            for(int Side:{-1,1})
            {
                // Kerbed pavements run between junctions only, leaving crossings clear.
                Street.Box(Pave,0,FVector(V+Side*1180,U,24),FVector(260,2700,30));
                Street.Box(Pave,0,FVector(U,V+Side*1180,25),FVector(2700,260,30));
                for(float Off:{-1000.f,1000.f})
                {
                    AddStreetLight(District,Base+FVector(V+Side*980,U+Off,0),Side>0 ? 180:0);
                    AddStreetLight(District,Base+FVector(U+Off,V+Side*980,0),Side>0 ? 270:90);
                }
                const bool bTreeSide=bTrees && (!bPoles || Side<0);
                if(bTreeSide) { AddTree(District,Base+FVector(V+Side*1180,U,0),.9f); AddTree(District,Base+FVector(U,V+Side*1180,0),.9f); }
                if(bPoles && Side>0)
                {
                    AddUtilityLine(District,Base+FVector(V+Side*1200,U-1300,0),Base+FVector(V+Side*1200,U+1300,0),3);
                    AddUtilityLine(District,Base+FVector(U-1300,V+Side*1200,0),Base+FVector(U+1300,V+Side*1200,0),3);
                }
                if(Rand.FRand()<.3f) AddCar(District,Base+FVector(V+Side*780,U+Rand.FRandRange(-450.f,450.f),0),90,Cars[Rand.RandHelper(UE_ARRAY_COUNT(Cars))]);
                if(Rand.FRand()<.3f) AddCar(District,Base+FVector(U+Rand.FRandRange(-450.f,450.f),V+Side*780,0),0,Cars[Rand.RandHelper(UE_ARRAY_COUNT(Cars))]);
                if(Rand.FRand()<.18f) AddVending(District,Base+FVector(V+Side*1150,U+Rand.FRandRange(-600.f,600.f),0),Side>0 ? 180:0,Vending[Rand.RandHelper(3)]);
            }
            for(float Dash=U-1200;Dash<=U+1200;Dash+=800)
            {
                Street.Box(Line,2,FVector(V,Dash,25),FVector(15,300,5));
                Street.Box(Line,2,FVector(Dash,V,26),FVector(300,15,5));
            }
        }
    }
    // Zebra crossings on every approach to every junction.
    for(int32 RX=-Blocks-1;RX<=Blocks;++RX) for(int32 RY=-Blocks-1;RY<=Blocks;++RY)
    {
        const FVector2D J(RX*4800+2400,RY*4800+2400);
        for(int Side:{-1,1}) for(int Stripe=-3;Stripe<=3;++Stripe)
        {
            Street.Box(Zebra,2,FVector(J.X+Stripe*280,J.Y+Side*1250,27),FVector(130,300,5));
            Street.Box(Zebra,2,FVector(J.X+Side*1250,J.Y+Stripe*280,28),FVector(300,130,5));
        }
    }
}

void AEvaGameMode::BuildOpenBlock(int32 District,FVector Center,bool bPark)
{
    // The open blocks around each district's service pylon become parks or car parks; nothing here blocks an Eva.
    FEvaDressing Ground(this,NewSceneRoot(Center));
    FRandomStream Rand(int32(Center.X*.01f+Center.Y*.03f));
    const FLinearColor Stone(.36f,.35f,.32f);
    if(bPark)
    {
        Ground.Box(FLinearColor(.06f,.12f,.05f),6,FVector(0,0,30),FVector(2150,2150,16));
        Ground.Box(Stone,0,FVector(0,0,40),FVector(2150,240,12));
        Ground.Box(Stone,0,FVector(0,0,41),FVector(240,2150,12));
        Ground.Box(FLinearColor(.42f,.42f,.4f),0,FVector(0,0,70),FVector(720,720,70),FRotator::ZeroRotator,TEXT("Cylinder"));
        Ground.Box(FLinearColor(.03f,.09f,.12f),4,FVector(0,0,102),FVector(620,620,16),FRotator::ZeroRotator,TEXT("Cylinder"));
        Ground.Box(FLinearColor(.5f,.5f,.48f),0,FVector(0,0,190),FVector(90,90,240),FRotator::ZeroRotator,TEXT("Cylinder"));
        for(int I=0;I<4;++I)
        {
            const float A=I*PI*.5f+PI*.25f;
            Ground.Box(FLinearColor(.3f,.2f,.12f),0,FVector(FMath::Cos(A)*560,FMath::Sin(A)*560,70),FVector(220,60,40),FRotator(0,FMath::RadiansToDegrees(A)+90,0));
        }
        for(int I=0;I<8;++I)
        {
            const float A=I*PI/4+PI/8;
            AddTree(District,Center+FVector(FMath::Cos(A)*Rand.FRandRange(700.f,900.f),FMath::Sin(A)*Rand.FRandRange(700.f,900.f),0),Rand.FRandRange(.85f,1.2f));
        }
        return;
    }
    // Surface car park with painted bays, parked cars and a pay booth.
    const FLinearColor Cars[]={FLinearColor(.75f,.75f,.72f),FLinearColor(.05f,.06f,.07f),FLinearColor(.5f,.05f,.04f),FLinearColor(.08f,.16f,.4f),FLinearColor(.45f,.46f,.47f),FLinearColor(.7f,.55f,.1f)};
    Ground.Box(FLinearColor(.08f,.085f,.09f),3,FVector(0,0,30),FVector(2150,2150,12));
    for(int Row=-1;Row<=1;++Row)
    {
        for(int Bay=-4;Bay<=5;++Bay) Ground.Box(Paint,2,FVector(Bay*210-105,Row*700,38),FVector(12,480,4));
        for(int Bay=-4;Bay<=4;++Bay) if(Rand.FRand()<.6f) AddCar(District,Center+FVector(Bay*210,Row*700,0),90,Cars[Rand.RandHelper(UE_ARRAY_COUNT(Cars))]);
    }
    Ground.Box(FLinearColor(.72f,.72f,.68f),0,FVector(-900,-960,150),FVector(220,160,260));
    Ground.Box(FLinearColor(.9f,.8f,.3f),1,FVector(-900,-1042,230),FVector(180,6,50),FRotator::ZeroRotator,TEXT("Cube"),1.2f);
}

void AEvaGameMode::BuildHarborDistrict()
{
    const int32 D=1; const FVector Base=Districts[D];
    BuildStreetGrid(D,Base,2,false,false);
    CityBox(Base+FVector(0,-2600,4),FVector(262,316,.08f),FLinearColor(.2f,.21f,.2f));
    const FLinearColor Pale(.32f,.38f,.42f), Cream(.55f,.52f,.44f), Rust(.42f,.14f,.08f), Sage(.28f,.36f,.32f), Roof(.14f,.16f,.17f), Blue(.1f,.18f,.3f), Tank(.62f,.64f,.62f);
    // Quay-side warehouses, fuel storage, a container yard, offices and the maritime tower.
    BuildArchitecture(Lot(Base,-2,-2),Spec(EEvaArch::Hall,D,2600,2400,1500,0,Pale,Blue,TEXT("PIER 1")));
    BuildArchitecture(Lot(Base,-1,-2),Spec(EEvaArch::Hall,D,2600,2300,1400,2,Cream,Roof,TEXT("PIER 2")));
    BuildArchitecture(Lot(Base,1,-2),Spec(EEvaArch::Hall,D,2600,2400,1600,4,Rust,Roof,TEXT("PIER 3")));
    BuildArchitecture(Lot(Base,2,-2),Spec(EEvaArch::Hall,D,2500,2300,1300,1,Sage,Blue,TEXT("COLD STORE")));
    BuildArchitecture(Lot(Base,-2,-1),Spec(EEvaArch::Tank,D,2300,2300,1600,0,Tank,Steel,TEXT("FUEL 1")));
    BuildArchitecture(Lot(Base,1,-1),Spec(EEvaArch::Tank,D,2200,2200,1800,1,Tank,Steel,TEXT("FUEL 2")));
    BuildArchitecture(Lot(Base,2,-1),Spec(EEvaArch::Tank,D,2000,2000,1500,2,Tank*.9f,Steel,TEXT("FUEL 3")));
    BuildArchitecture(Lot(Base,-2,1),Spec(EEvaArch::Office,D,2200,1800,2600,1,FLinearColor(.36f,.38f,.37f),Steel));
    BuildArchitecture(Lot(Base,-1,1),Spec(EEvaArch::Hall,D,2600,2200,1300,3,Pale*.9f,Roof,TEXT("FISH MARKET")));
    BuildArchitecture(Lot(Base,1,1),Spec(EEvaArch::Office,D,2200,2000,3200,6,FLinearColor(.42f,.38f,.32f),Steel));
    BuildArchitecture(Lot(Base,2,1),Spec(EEvaArch::Hall,D,2600,2400,1500,0,Cream*.9f,Blue,TEXT("CUSTOMS")));
    BuildArchitecture(Lot(Base,-2,2),Spec(EEvaArch::Office,D,2300,2200,4800,3,FLinearColor(.3f,.33f,.34f),Steel));
    BuildArchitecture(Lot(Base,-1,2),Spec(EEvaArch::Hall,D,2600,2300,1400,5,Rust*.9f,Roof,TEXT("DEPOT 7")));
    BuildArchitecture(Lot(Base,1,2),Spec(EEvaArch::Tank,D,2100,2100,1700,3,Tank,Steel,TEXT("FUEL 4")));
    BuildArchitecture(Lot(Base,2,2),Spec(EEvaArch::Office,D,2200,2000,3600,4,FLinearColor(.48f,.46f,.42f),Steel));
    for(int X:{-1,1}) for(int Y:{-1,1}) AddContainerStack(D,Lot(Base,-1,-1)+FVector(X*620,Y*560,0),2,3,true);
    for(const FIntPoint Open:{FIntPoint(-2,0),FIntPoint(-1,0),FIntPoint(1,0),FIntPoint(2,0),FIntPoint(0,-1),FIntPoint(0,-2),FIntPoint(0,2)}) BuildOpenBlock(D,Lot(Base,Open.X,Open.Y),Open.X==0 && Open.Y==2);
    // The quay: apron, container yard, gantry cranes on rails, bollards and fenders at the water's edge.
    auto* Quay=NewSceneRoot(Base);
    FEvaDressing Dock(this,Quay);
    Dock.Box(FLinearColor(.25f,.25f,.23f),0,FVector(-20000,-18025,6),FVector(86000,9950,12),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    for(float Y:{-19200.f,-21800.f}) Dock.Box(Steel*1.5f,2,FVector(-20000,Y,16),FVector(86000,60,14),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    for(float X=-60000;X<=22000;X+=900)
    {
        Dock.Box(FLinearColor(.05f,.05f,.055f),2,FVector(X,-22700,70),FVector(60,60,90),FRotator::ZeroRotator,TEXT("Cylinder"));
        Dock.Box(FLinearColor(.02f,.02f,.02f),0,FVector(X+450,-23060,-150),FVector(260,90,280));
    }
    for(int Row=0;Row<2;++Row) for(int I=0;I<16;++I) AddContainerStack(D,Base+FVector(-15000+I*1900,-14600-Row*2600,0),3,3,false);
    const FLinearColor Crane[]={FLinearColor(.62f,.08f,.04f),FLinearColor(.08f,.22f,.5f)};
    for(int I=0;I<4;++I)
    {
        const FVector C=Base+FVector(-11000+I*6200,-20500,0);
        const FLinearColor Livery=Crane[I%2];
        FEvaDressing Gantry(this,NewSceneRoot(C));
        for(int SX:{-1,1}) for(int SY:{-1,1}) Gantry.Box(Livery,2,FVector(SX*900,SY*1300,2100),FVector(170,170,4200),FRotator::ZeroRotator,TEXT("Cube"),0,true);
        for(int SX:{-1,1}) { Gantry.Box(Livery,2,FVector(SX*900,0,3000),FVector(140,2600,140),FRotator::ZeroRotator,TEXT("Cube"),0,true); Gantry.Beam(Livery,2,FVector(SX*900,-1300,300),FVector(SX*900,1300,2900),90,0,true); }
        Gantry.Box(Livery,2,FVector(0,-4200,4350),FVector(420,12500,260),FRotator::ZeroRotator,TEXT("Cube"),0,true);
        Gantry.Box(FLinearColor(.75f,.75f,.72f),2,FVector(0,1500,4700),FVector(1200,900,600),FRotator::ZeroRotator,TEXT("Cube"),0,true);
        Gantry.Box(Livery,2,FVector(0,-600,5900),FVector(160,160,2600),FRotator::ZeroRotator,TEXT("Cube"),0,true);
        for(float Y:{-9000.f,-4500.f}) Gantry.Beam(Steel*1.4f,2,FVector(0,-600,7100),FVector(0,Y,4480),30,0,true);
        Gantry.Box(Steel*1.4f,2,FVector(0,-6800,3950),FVector(360,500,300),FRotator::ZeroRotator,TEXT("Cube"),0,true);
        Gantry.Beam(FLinearColor(.02f,.02f,.02f),2,FVector(0,-6800,3800),FVector(0,-6800,1200),12,0,true);
        Gantry.Box(FLinearColor(1,.05f,.03f),2,FVector(0,-600,7240),FVector(90,90,90),FRotator::ZeroRotator,TEXT("Sphere"),4,true);
    }
    // Two freighters alongside and a lighthouse on the breakwater.
    for(int I=0;I<2;++I)
    {
        const FVector Ship=Base+FVector(I ? 9000:-9500,-27400,0);
        FEvaDressing Hull(this,NewSceneRoot(Ship));
        const float Length=I ? 16000:19000;
        Hull.Box(I ? FLinearColor(.08f,.14f,.3f):FLinearColor(.4f,.08f,.05f),2,FVector(0,0,-20),FVector(Length,3100,1400),FRotator::ZeroRotator,TEXT("ShipHull"),0,true);
        const float Stern=-Length*.5f+1800;
        Hull.Box(FLinearColor(.82f,.82f,.78f),0,FVector(Stern,0,1400),FVector(1800,2700,1600),FRotator::ZeroRotator,TEXT("Cube"),0,true);
        Hull.Box(FLinearColor(.78f,.78f,.74f),0,FVector(Stern,0,2500),FVector(1300,3300,600),FRotator::ZeroRotator,TEXT("Cube"),0,true);
        Hull.Box(FLinearColor(.6f,.42f,.2f),1,FVector(Stern+660,0,2520),FVector(10,3100,220),FRotator::ZeroRotator,TEXT("Cube"),.9f,true);
        Hull.Box(FLinearColor(.1f,.1f,.1f),8,FVector(Stern-500,0,3300),FVector(420,420,1200),FRotator::ZeroRotator,TEXT("Cylinder"),0,true);
        FRandomStream Rand(I*41+5);
        const FLinearColor Box[]={FLinearColor(.55f,.08f,.04f),FLinearColor(.06f,.2f,.45f),FLinearColor(.1f,.32f,.14f),FLinearColor(.75f,.36f,.05f),FLinearColor(.45f,.46f,.44f)};
        for(float X=Stern+1600;X<Length*.5f-2400;X+=1260) for(int Row=-5;Row<=5;++Row) for(int Tier=0;Tier<3;++Tier)
            if(Rand.FRand()<.85f) Hull.Box(Box[Rand.RandHelper(5)]*Rand.FRandRange(.8f,1.1f),5,FVector(X,Row*250,810+Tier*262),FVector(1220,244,259),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    }
    FEvaDressing Light(this,NewSceneRoot(Base+FVector(19000,-30000,0)));
    Light.Box(FLinearColor(.3f,.31f,.29f),0,FVector(-2500,0,-120),FVector(6000,900,500),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    Light.Box(FLinearColor(.7f,.08f,.05f),8,FVector(0,0,1400),FVector(420,420,2800),FRotator::ZeroRotator,TEXT("Cylinder"),0,true);
    Light.Box(FLinearColor(1,.85f,.55f),1,FVector(0,0,2950),FVector(300,300,260),FRotator::ZeroRotator,TEXT("Cylinder"),3,true);
    Light.Box(FLinearColor(.08f,.08f,.08f),2,FVector(0,0,3150),FVector(380,380,160),FRotator::ZeroRotator,TEXT("Cone"),0,true);
    for(int Side:{-1,1}) Dock.Box(Steel*1.4f,2,FVector(Side*2300,-13000,850),FVector(90,90,1700),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    Dock.Box(FLinearColor(.08f,.2f,.34f),1,FVector(0,-13000,1580),FVector(4700,60,300),FRotator::ZeroRotator,TEXT("Cube"),.5f,true);
    CitySign(Quay,FVector(0,-12960,1580),TEXT("PORT OF TOKYO-3  /  HARBOR DISTRICT"),150,FColor(235,225,190),FRotator(0,90,0));
}

void AEvaGameMode::BuildUplandDistrict()
{
    const int32 D=2; const FVector Base=Districts[D];
    BuildStreetGrid(D,Base,2,true,true);
    CityBox(Base+FVector(0,0,4),FVector(262,262,.08f),FLinearColor(.055f,.09f,.045f),6);
    const FLinearColor Cream(.62f,.58f,.5f), Mist(.5f,.55f,.58f), Sand(.58f,.52f,.44f), Rail(.55f,.56f,.53f);
    BuildArchitecture(Lot(Base,-2,2),Spec(EEvaArch::Slab,D,2500,1150,2400,0,Cream,Rail,TEXT("1")));
    BuildArchitecture(Lot(Base,-1,2),Spec(EEvaArch::Slab,D,2500,1150,2700,1,Mist,Rail,TEXT("2")));
    BuildArchitecture(Lot(Base,2,2),Spec(EEvaArch::Slab,D,2500,1150,2100,2,Sand,Rail,TEXT("3")));
    BuildArchitecture(Lot(Base,-1,-2),Spec(EEvaArch::Slab,D,2500,1150,1800,3,Cream*.95f,Rail,TEXT("5")));
    BuildArchitecture(Lot(Base,-2,-1),Spec(EEvaArch::Office,D,1900,1700,3300,1,FLinearColor(.58f,.54f,.47f),FLinearColor(.3f,.28f,.25f)));
    BuildArchitecture(Lot(Base,1,-2),Spec(EEvaArch::Office,D,2400,1600,1000,0,FLinearColor(.5f,.5f,.48f),Steel));
    // Tokyo-3 Municipal No.1 Junior High: a long classroom block beside its sports ground.
    BuildArchitecture(Lot(Base,1,1)+FVector(0,500,0),Spec(EEvaArch::Office,D,2600,1100,1300,13,FLinearColor(.72f,.7f,.64f),FLinearColor(.35f,.38f,.36f)));
    auto* Grounds=NewSceneRoot(Base);
    FEvaDressing Yard(this,Grounds);
    CitySign(Grounds,FVector(4800,4730,1100),TEXT("TOKYO-3 MUNICIPAL NO.1 JUNIOR HIGH"),95,FColor(40,55,60));
    Yard.Box(FLinearColor(.36f,.27f,.18f),0,FVector(9600,4800,30),FVector(2500,2500,20));
    for(int I=0;I<28;++I)
    {
        const float A=I*2*PI/28, B=(I+1)*2*PI/28;
        Yard.Beam(Paint,2,FVector(9600+FMath::Cos(A)*1000,4800+FMath::Sin(A)*800,42),FVector(9600+FMath::Cos(B)*1000,4800+FMath::Sin(B)*800,42),16);
    }
    for(int Side:{-1,1})
    {
        Yard.Box(Paint,2,FVector(9600+Side*850,4800,120),FVector(12,300,12));
        for(int Post:{-1,1}) Yard.Box(Paint,2,FVector(9600+Side*850,4800+Post*150,70),FVector(12,12,140));
    }
    for(int I=0;I<8;++I) AddTree(D,Base+FVector(8350+I*360,6150,0),.8f);
    // Hilltop shrine: stone approach, vermilion gates, lanterns and a cedar grove.
    const FVector Shrine=Lot(Base,1,2);
    FEvaDressing Stone(this,NewSceneRoot(Shrine));
    Stone.Box(FLinearColor(.42f,.42f,.4f),0,FVector(0,-300,30),FVector(360,2300,40));
    Stone.Box(FLinearColor(.4f,.39f,.37f),0,FVector(0,560,70),FVector(1700,1300,120));
    AddHouse(D,Shrine+FVector(0,600,110),1250,950,false,FLinearColor(.34f,.1f,.05f),FLinearColor(.12f,.12f,.13f));
    const FLinearColor Vermilion(.72f,.08f,.03f);
    for(float Y:{-1200.f,-500.f})
    {
        const int32 Gate=AddProp(Shrine+FVector(0,Y,0),380);
        auto* Wood=PropBatch(D,FLinearColor::White,0,0,TEXT("Cylinder"),true);
        auto* Beams=PropBatch(D,FLinearColor::White,0,0,TEXT("Cube"),true);
        for(int Side:{-1,1}) AddPropPart(Gate,Wood,Pose(Shrine+FVector(Side*330,Y,370),FVector(60,60,740)),EEvaBreak::Topple,Vermilion);
        AddPropPart(Gate,Beams,Pose(Shrine+FVector(0,Y,760),FVector(1060,90,60)),EEvaBreak::Topple,FLinearColor(.06f,.05f,.05f));
        AddPropPart(Gate,Beams,Pose(Shrine+FVector(0,Y,690),FVector(900,70,55)),EEvaBreak::Topple,Vermilion);
        AddPropPart(Gate,Beams,Pose(Shrine+FVector(0,Y,590),FVector(820,50,45)),EEvaBreak::Topple,Vermilion);
    }
    for(int Side:{-1,1}) for(float Y:{-900.f,-150.f}) Stone.Box(FLinearColor(.46f,.46f,.43f),0,FVector(Side*320,Y,120),FVector(80,80,220));
    for(int I=0;I<10;++I)
    {
        const float A=I*2*PI/10;
        AddTree(D,Shrine+FVector(FMath::Cos(A)*1150,FMath::Sin(A)*1150+200,0),1.25f);
    }
    // Low-rise neighbourhoods: pitched roofs, block walls and a car or garden tree per plot.
    const FLinearColor Walls[]={FLinearColor(.72f,.7f,.64f),FLinearColor(.66f,.62f,.54f),FLinearColor(.58f,.6f,.58f),FLinearColor(.7f,.66f,.58f),FLinearColor(.55f,.58f,.52f)};
    const FLinearColor Roofs[]={FLinearColor(.13f,.18f,.26f),FLinearColor(.18f,.1f,.07f),FLinearColor(.42f,.14f,.07f),FLinearColor(.1f,.16f,.12f),FLinearColor(.2f,.21f,.22f)};
    const FLinearColor Cars[]={FLinearColor(.75f,.75f,.72f),FLinearColor(.5f,.05f,.04f),FLinearColor(.08f,.16f,.4f)};
    FRandomStream Rand(2718);
    FEvaDressing Plots(this,NewSceneRoot(Base));
    const FIntPoint Neighbourhoods[]={FIntPoint(-2,1),FIntPoint(-1,1),FIntPoint(-1,-1),FIntPoint(1,-1),FIntPoint(-2,-2),FIntPoint(2,-2)};
    for(const FIntPoint Block:Neighbourhoods) for(int X:{-1,1}) for(int Y:{-1,1})
    {
        const FVector Plot=Lot(Base,Block.X,Block.Y)+FVector(X*640,Y*600,0);
        const bool bTurned=Rand.FRand()<.4f;
        AddHouse(D,Plot+FVector(Rand.FRandRange(-60.f,60.f),Rand.FRandRange(-40.f,40.f),0),Rand.FRandRange(820.f,1000.f),Rand.FRandRange(700.f,820.f),bTurned,Walls[Rand.RandHelper(5)],Roofs[Rand.RandHelper(5)]);
        const FVector L=Plot-Base;
        Plots.Box(FLinearColor(.44f,.43f,.4f),0,FVector(L.X,L.Y+Y*570,60),FVector(1200,24,120));
        Plots.Box(FLinearColor(.44f,.43f,.4f),0,FVector(L.X+X*610,L.Y,60),FVector(24,1100,120));
        if(Rand.FRand()<.5f) AddTree(D,Plot+FVector(-X*420,-Y*420,0),.65f);
        else AddCar(D,Plot+FVector(-X*380,-Y*420,0),bTurned ? 0:90,Cars[Rand.RandHelper(3)]);
    }
    for(const FIntPoint Open:{FIntPoint(-2,0),FIntPoint(-1,0),FIntPoint(1,0),FIntPoint(2,0),FIntPoint(0,-1),FIntPoint(0,-2),FIntPoint(0,2)}) BuildOpenBlock(D,Lot(Base,Open.X,Open.Y),true);
    // Neighbourhood park with a pond and play equipment.
    const FVector Park=Lot(Base,2,-1);
    FEvaDressing Play(this,NewSceneRoot(Park));
    Play.Box(FLinearColor(.02f,.07f,.09f),4,FVector(-350,300,20),FVector(1100,800,20),FRotator::ZeroRotator,TEXT("Cylinder"));
    Play.Box(FLinearColor(.8f,.2f,.05f),2,FVector(550,-450,160),FVector(500,90,18),FRotator(-30,0,0));
    Play.Box(FLinearColor(.1f,.4f,.8f),2,FVector(330,-450,120),FVector(90,90,240));
    for(int I=0;I<3;++I) Play.Box(FLinearColor(.85f,.75f,.1f),2,FVector(200+I*160,300,110),FVector(20,380,220));
    for(int I=0;I<7;++I) AddTree(D,Park+FVector(FMath::Cos(I*.9f)*1050,FMath::Sin(I*.9f)*950,0),1.05f);
    CitySign(Grounds,FVector(0,-11800,1400),TEXT("UPLAND RESIDENTIAL / EVACUATION SHELTERS 12-18"),120,FColor(190,225,190));
}

void AEvaGameMode::BuildIndustrialDistrict()
{
    const int32 D=3; const FVector Base=Districts[D];
    BuildStreetGrid(D,Base,2,false,false);
    CityBox(Base+FVector(1500,0,4),FVector(292,262,.08f),FLinearColor(.17f,.17f,.16f));
    const FLinearColor Grey(.4f,.42f,.4f), Roof(.18f,.2f,.22f), Rust(.38f,.14f,.08f), Teal(.2f,.32f,.33f), Tank(.6f,.62f,.6f), Red(.62f,.07f,.04f);
    BuildArchitecture(Lot(Base,-2,2),Spec(EEvaArch::Hall,D,2600,2400,1400,1,Grey,Roof,TEXT("PLANT 1")));
    BuildArchitecture(Lot(Base,-1,2),Spec(EEvaArch::Stack,D,640,640,7600,0,Red,Steel));
    BuildArchitecture(Lot(Base,1,2),Spec(EEvaArch::Cooling,D,2500,2500,4400,0,FLinearColor(.5f,.5f,.47f),Steel));
    BuildArchitecture(Lot(Base,2,2),Spec(EEvaArch::Hall,D,2600,2400,1500,3,Teal,Roof,TEXT("PLANT 2")));
    BuildArchitecture(Lot(Base,-2,1),Spec(EEvaArch::Tank,D,2300,2300,1700,0,Tank,Steel,TEXT("T-01")));
    BuildArchitecture(Lot(Base,-1,1),Spec(EEvaArch::Hall,D,2600,2200,1600,0,Rust,Roof,TEXT("SHIPPING")));
    BuildArchitecture(Lot(Base,1,1),Spec(EEvaArch::Sphere,D,2400,2400,3000,0,FLinearColor(.66f,.66f,.62f),Steel,TEXT("LNG 2")));
    BuildArchitecture(Lot(Base,2,1),Spec(EEvaArch::Stack,D,560,560,6800,1,Red,Steel));
    BuildArchitecture(Lot(Base,-2,-1),Spec(EEvaArch::Hall,D,2600,2400,1400,5,Grey*.9f,Roof,TEXT("PLANT 3")));
    BuildArchitecture(Lot(Base,-1,-1),Spec(EEvaArch::Office,D,2000,1600,2600,1,FLinearColor(.36f,.37f,.35f),Steel));
    BuildArchitecture(Lot(Base,1,-1),Spec(EEvaArch::Hall,D,2600,2400,1500,7,Teal*.9f,Roof,TEXT("PLANT 4")));
    BuildArchitecture(Lot(Base,2,-1),Spec(EEvaArch::Tank,D,2200,2200,1800,1,Tank,Steel,TEXT("T-02")));
    BuildArchitecture(Lot(Base,-2,-2),Spec(EEvaArch::Hall,D,2600,2300,1600,2,Rust*.9f,Roof,TEXT("WAREHOUSE 5")));
    BuildArchitecture(Lot(Base,-1,-2),Spec(EEvaArch::Tank,D,2100,2100,1600,2,Tank*.92f,Steel,TEXT("T-03")));
    BuildArchitecture(Lot(Base,1,-2),Spec(EEvaArch::Hall,D,2600,2300,1500,4,Grey,Roof,TEXT("WAREHOUSE 6")));
    BuildArchitecture(Lot(Base,2,-2),Spec(EEvaArch::Sphere,D,2300,2300,2900,1,FLinearColor(.66f,.66f,.62f),Steel,TEXT("LNG 1")));
    for(const FIntPoint Open:{FIntPoint(-2,0),FIntPoint(-1,0),FIntPoint(1,0),FIntPoint(2,0),FIntPoint(0,-1),FIntPoint(0,-2),FIntPoint(0,2)}) BuildOpenBlock(D,Lot(Base,Open.X,Open.Y),false);
    // Eastern tank farm behind a pipe rack.
    for(int I=0;I<7;++I)
        BuildArchitecture(Base+FVector(14800,-6900+I*2300,0),Spec(EEvaArch::Tank,D,1700,1700,2000,I,Tank*(I%2 ? .92f:1.f),Steel,*FString::Printf(TEXT("F-%d"),I+1)));
    FEvaDressing Pipes(this,NewSceneRoot(Base));
    const FLinearColor PipeColors[]={FLinearColor(.55f,.5f,.2f),FLinearColor(.25f,.3f,.35f),FLinearColor(.5f,.14f,.08f)};
    for(float Y=-9000;Y<=9000;Y+=1800)
    {
        for(int Side:{-1,1}) Pipes.Box(Steel*1.4f,2,FVector(13400+Side*180,Y,1050),FVector(40,40,2100));
        Pipes.Box(Steel*1.4f,2,FVector(13400,Y,2050),FVector(420,40,40));
    }
    for(int I=0;I<3;++I) Pipes.Box(PipeColors[I],2,FVector(13300+I*100,0,2130),FVector(80,80,18000),FRotator(0,0,90),TEXT("Cylinder"));
    // Covered conveyor from the stack's boiler house to Plant 1, well above Eva height.
    Pipes.Box(Grey*.8f,5,FVector(-7200,9600,2600),FVector(2600,380,380));
    for(float X:{-8200.f,-6200.f}) Pipes.Box(Steel*1.4f,2,FVector(X,9600,1200),FVector(60,60,2400));
    Pipes.Box(Grey*.85f,5,FVector(-4800,10500,700),FVector(900,700,1400));
    // Substation: transformers and a lattice gantry.
    for(int I=0;I<4;++I)
    {
        Pipes.Box(FLinearColor(.34f,.38f,.36f),2,FVector(-14500,-4200+I*700,220),FVector(380,300,440));
        Pipes.Box(FLinearColor(.5f,.36f,.2f),0,FVector(-14500,-4200+I*700,520),FVector(60,60,160),FRotator::ZeroRotator,TEXT("Cylinder"));
    }
    CitySign(Pipes.Root,FVector(0,-11800,1400),TEXT("INDUSTRIAL ZONE / HAKONE STEEL WORKS"),130,FColor(240,200,120));
}

void AEvaGameMode::BuildWorldInfrastructure()
{
    // Sea to the south with a proper quay wall, and mountains closing the caldera on the other sides.
    CityBox(WorldCenter+FVector(-40000,-150000,-370),FVector(5000,2100,1),FLinearColor(.02f,.075f,.1f),4);
    CityBox(WorldCenter+FVector(0,-45000,-300),FVector(900,2.4f,6),FLinearColor(.2f,.21f,.2f));
    Shape("TerrainRidge",WorldCenter+FVector(-4000,54500,-300),FVector(230,1250,210),FLinearColor(.06f,.1f,.07f))->SetRelativeRotation(FRotator(0,90,0));
    Shape("TerrainRidge",WorldCenter+FVector(54500,-3000,-300),FVector(230,1150,240),FLinearColor(.065f,.1f,.075f))->SetRelativeRotation(FRotator(0,180,0));
    // Elevated expressway crossing the city between the districts, with stalled evacuation traffic.
    auto* Root=NewSceneRoot(WorldCenter);
    FEvaDressing Way(this,Root);
    const float Deck=2800, Length=93000;
    const FLinearColor Grey(.3f,.31f,.3f);
    Way.Box(Grey,0,FVector(0,0,Deck),FVector(Length,2800,200),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    Way.Box(FLinearColor(.06f,.065f,.075f),3,FVector(0,0,Deck+106),FVector(Length,2640,12),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    for(int Side:{-1,1}) Way.Box(Grey*1.1f,0,FVector(0,Side*1380,Deck+160),FVector(Length,50,120),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    for(float X=-Length*.5f+600;X<Length*.5f;X+=1200) for(int Lane:{-1,1}) Way.Box(Paint,2,FVector(X,Lane*660,Deck+114),FVector(420,16,4));
    Way.Box(Paint,2,FVector(0,0,Deck+114),FVector(Length,20,4));
    for(float X=-Length*.5f+1500;X<Length*.5f;X+=3000)
    {
        Way.Box(Steel*1.4f,2,FVector(X,0,Deck+620),FVector(26,26,1000));
        for(int Side:{-1,1}) Way.Box(FLinearColor(1,.72f,.4f),2,FVector(X,Side*260,Deck+1100),FVector(130,50,14),FRotator::ZeroRotator,TEXT("Cube"),1.6f);
    }
    const float Roads[]={-44000,-22000,0,22000,44000};
    for(float X=-44000+2200;X<44000;X+=4400)
    {
        bool bRoad=false; for(float R:Roads) bRoad|=FMath::Abs(X-R)<1800;
        if(bRoad) continue;
        CityBox(FVector(X,0,(Deck-100)*.5f),FVector(5.5f,5.5f,(Deck-100)/100),Grey,0,Root,true);
        Way.Box(Grey*.92f,0,FVector(X,0,Deck-170),FVector(800,2700,140),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    }
    FRandomStream Rand(1995);
    const FLinearColor Cars[]={FLinearColor(.75f,.75f,.72f),FLinearColor(.05f,.06f,.07f),FLinearColor(.5f,.05f,.04f),FLinearColor(.08f,.16f,.4f),FLinearColor(.45f,.46f,.47f)};
    for(int I=0;I<34;++I)
    {
        const float X=Rand.FRandRange(-40000.f,40000.f), Y=(Rand.FRand()<.5f ? -1:1)*(Rand.FRand()<.5f ? 330:990);
        const bool bTruck=Rand.FRand()<.2f;
        const FLinearColor Paint2=bTruck ? FLinearColor(.72f,.72f,.68f):Cars[Rand.RandHelper(5)];
        Way.Box(Paint2,2,FVector(X,Y,Deck+(bTruck ? 330:180)),bTruck ? FVector(1000,250,380):FVector(430,176,90),FRotator(0,Rand.FRandRange(-6.f,6.f),0));
        if(!bTruck) Way.Box(FLinearColor(.03f,.05f,.06f),1,FVector(X-25,Y,Deck+250),FVector(230,160,70));
    }
    for(float X:{-30000.f,12000.f})
    {
        for(int Side:{-1,1}) Way.Box(Steel*1.3f,2,FVector(X,Side*1500,Deck+450),FVector(40,40,900));
        Way.Box(FLinearColor(.05f,.35f,.15f),1,FVector(X,0,Deck+850),FVector(40,2600,360),FRotator::ZeroRotator,TEXT("Cube"),.8f);
        CitySign(Root,FVector(X-30,0,Deck+850),X<0 ? TEXT("CENTRAL  /  HARBOR   EXIT 3"):TEXT("INDUSTRIAL  /  UPLAND   EXIT 5"),120,FColor(235,240,235),FRotator(0,180,0));
    }
    for(int Side:{-1,1})
    {
        Way.Box(Grey*.9f,0,FVector(Side*47200,0,Deck+500),FVector(1400,3800,1900),FRotator::ZeroRotator,TEXT("Cube"),0,true);
        Way.Box(FLinearColor(.01f,.01f,.012f),0,FVector(Side*46490,0,Deck+420),FVector(20,2700,760),FRotator::ZeroRotator,TEXT("Cube"),0,true);
    }
}
