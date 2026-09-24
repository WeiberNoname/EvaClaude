#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

namespace
{
    const FLinearColor White(1,1,1), Steel(.08f,.12f,.14f), Glass(.035f,.085f,.11f), Soot(.012f,.013f,.015f);
    const FLinearColor Warm(.62f,.42f,.2f), Cool(.34f,.48f,.55f), Amber(.92f,.36f,.045f), Cyan(.13f,.55f,.58f), Hazard(.95f,.58f,.05f);
    const TCHAR* Brands[]={TEXT("HAKONE BANK"),TEXT("MAGI SYSTEMS"),TEXT("TOKYO-3 LINE"),TEXT("GEOFRONT RAIL"),TEXT("LCL SPA"),TEXT("ASHINOKO"),TEXT("KOTOBUKI"),TEXT("PENPEN COLA"),TEXT("NERV"),TEXT("UNIT MOTORS")};
    const TCHAR* Blades[]={TEXT("C\nA\nF\nE"),TEXT("H\nO\nT\nE\nL"),TEXT("R\nA\nM\nE\nN"),TEXT("B\nO\nO\nK\nS"),TEXT("C\nL\nI\nN\nI\nC"),TEXT("K\nA\nR\nA\nO\nK\nE"),TEXT("P\nH\nO\nT\nO"),TEXT("S\nU\nS\nH\nI")};

    FLinearColor WindowLight(FRandomStream& Rand) { return (Rand.FRand()<.68f ? Warm:Cool)*Rand.FRandRange(.7f,1.2f); }
    FLinearColor SignColor(FRandomStream& Rand)
    {
        const FLinearColor Colors[]={FLinearColor(.9f,.08f,.05f),FLinearColor(.08f,.35f,.95f),FLinearColor(.95f,.55f,.05f),FLinearColor(.1f,.8f,.35f),FLinearColor(.85f,.12f,.6f),FLinearColor(.9f,.9f,.85f)};
        return Colors[Rand.RandHelper(6)];
    }
    FTransform Pose(FVector Position,FVector SizeCm,FRotator Rotation=FRotator::ZeroRotator) { return FTransform(Rotation,Position,SizeCm/100); }

    // Assembles one structure into per-section instanced batches. Anything above the split rides the crown.
    struct FEvaKit
    {
        AEvaGameMode* Game;
        FEvaBuilding& B;
        FRandomStream& Rand;
        TMap<FString,UInstancedStaticMeshComponent*> Batches;
        FEvaKit(AEvaGameMode* InGame,FEvaBuilding& InBuilding,FRandomStream& InRand):Game(InGame),B(InBuilding),Rand(InRand) {}
        int32 Section(float Z) const { return B.Crown && Z>B.Split ? 1:0; }
        USceneComponent* Root(int32 Sec) const { return Sec ? B.Crown:B.DistrictRoot; }
        UInstancedStaticMeshComponent* Batch(int32 Sec,FLinearColor Color,int32 Type,float Glow,const TCHAR* Mesh,bool bTinted,bool bLight)
        {
            const FString Key=FString::Printf(TEXT("%d/%s/%d/%.2f/%s/%d/%d"),Sec,*Color.ToString(),Type,Glow,Mesh,int32(bTinted),int32(bLight));
            if(auto* Found=Batches.FindRef(Key)) return Found;
            auto* Instances=Game->CityInstances(Root(Sec),Color,Type,Glow,Mesh);
            if(bTinted) Instances->SetNumCustomDataFloats(3);
            Instances->SetCullDistances(62000,90000);
            (bLight ? B.Lights:B.Windows).Add(Instances); Batches.Add(Key,Instances); return Instances;
        }
        void Put(UInstancedStaticMeshComponent* Instances,int32 Sec,FVector P,FVector Size,FRotator R,const FLinearColor* Tint)
        {
            if(Sec) P.Z-=B.Split;
            const int32 Index=Instances->AddInstance(Pose(P,Size,R));
            if(Tint) { const float Data[3]={Tint->R,Tint->G,Tint->B}; Instances->SetCustomData(Index,MakeArrayView(Data,3)); }
        }
        // Sizes are centimetres; engine shapes and the generated kit meshes are 100-unit primitives.
        // Details carry their colour per instance, so a section needs one batch per surface type and mesh.
        void Box(FLinearColor C,int32 Type,FVector P,FVector Size,FRotator R=FRotator::ZeroRotator,const TCHAR* Mesh=TEXT("Cube"),float Glow=0)
        { const int32 Sec=Section(P.Z); Put(Batch(Sec,White,Type,Glow,Mesh,true,false),Sec,P,Size,R,&C); }
        void Tint(FLinearColor C,int32 Type,FVector P,FVector Size,FRotator R=FRotator::ZeroRotator,const TCHAR* Mesh=TEXT("Cube"))
        { Box(C,Type,P,Size,R,Mesh); }
        // Lit windows and signs switch off when the structure loses power.
        void Light(FLinearColor C,FVector P,FVector Size,float Glow=.75f,FRotator R=FRotator::ZeroRotator)
        { const int32 Sec=Section(P.Z); Put(Batch(Sec,White,1,Glow,TEXT("Cube"),true,true),Sec,P,Size,R,&C); }
        void Beacon(FVector P,float Size=90)
        {
            const int32 Sec=Section(P.Z);
            const FString Key=FString::Printf(TEXT("Beacon/%d"),Sec);
            auto* Instances=Batches.FindRef(Key);
            if(!Instances)
            {
                Instances=Game->CityInstances(Root(Sec),FLinearColor(1,.05f,.03f),2,4,TEXT("Sphere"));
                if(Game->AviationMaterial) Instances->SetMaterial(0,Game->AviationMaterial);
                B.Lights.Add(Instances); Batches.Add(Key,Instances);
            }
            Put(Instances,Sec,P,FVector(Size),FRotator::ZeroRotator,nullptr);
        }
        // Vertical members are split at the crown so each section carries its own share.
        void Column(FLinearColor C,int32 Type,FVector2D XY,FVector2D Size,float Z0,float Z1,const TCHAR* Mesh=TEXT("Cube"))
        {
            auto Piece=[&](float A,float Z) { if(Z-A>1) Box(C,Type,FVector(XY.X,XY.Y,(A+Z)*.5f),FVector(Size.X,Size.Y,Z-A),FRotator::ZeroRotator,Mesh); };
            if(B.Crown && Z0<B.Split && Z1>B.Split) { Piece(Z0,B.Split); Piece(B.Split,Z1); } else Piece(Z0,Z1);
        }
        void Sign(FVector P,const FString& Text,float Size,FColor Color,float Yaw)
        {
            if(Text.IsEmpty()) return;
            const int32 Sec=Section(P.Z); if(Sec) P.Z-=B.Split;
            Game->CitySign(Root(Sec),P,Text,Size,Color,FRotator(0,Yaw,0));
        }
        UStaticMeshComponent* Body(int32 Sec,FVector P,FVector Size,FLinearColor C,int32 Type,const FString& Mesh=TEXT("Cube"))
        { if(Sec) P.Z-=B.Split; return Game->CityBox(P,Size/100,C,Type,Root(Sec),true,0,Mesh); }
    };

    // Punched (0), ribbon (1), finned (2) or curtain-wall (3) glazing for the floors between Z0 and Z1.
    void Glazing(FEvaKit& K,int32 Pattern,float Z0,float Z1,float W,float D,float Floor,FLinearColor Frame,FLinearColor Facade)
    {
        const float HW=W*.5f, HD=D*.5f;
        const int32 BX=FMath::Max(2,int32((W-240)/285)), BY=FMath::Max(2,int32((D-240)/285));
        const float SX=(W-240)/BX, SY=(D-240)/BY;
        auto Pane=[&](FVector P,FVector Size,bool bRibbon)
        {
            const float Lit=Pattern==3 ? .36f:.27f;
            if(K.Rand.FRand()<Lit) K.Light(WindowLight(K.Rand),P,Size);
            else if(!bRibbon && Pattern!=3) K.Box(Glass,1,P,Size);
        };
        for(int32 Level=FMath::CeilToInt(Z0/Floor);;++Level)
        {
            const float Z=Level*Floor+Floor*.55f;
            if(Z>Z1-Floor*.3f) break;
            if(Z<Z0+Floor*.2f) continue;
            if(Pattern!=1) K.Box(Frame,2,FVector(0,0,Z-Floor*.38f),FVector(W+26,D+26,30));
            for(int Side:{-1,1})
            {
                if(Pattern==1)
                {
                    K.Box(Glass,1,FVector(0,Side*(HD+3),Z),FVector(W-90,8,Floor*.5f));
                    K.Box(Glass,1,FVector(Side*(HW+3),0,Z),FVector(8,D-90,Floor*.5f));
                }
                for(int I=0;I<BX;++I) Pane(FVector(-HW+120+SX*(I+.5f),Side*(HD+(Pattern==1 ? 7:4)),Z),FVector(SX*.76f,8,Floor*.54f),Pattern==1);
                for(int I=0;I<BY;++I) Pane(FVector(Side*(HW+(Pattern==1 ? 7:4)),-HD+120+SY*(I+.5f),Z),FVector(8,SY*.76f,Floor*.54f),Pattern==1);
            }
        }
        if(Pattern==2 || Pattern==3)
        {
            // Fins read as deep vertical shadow lines; curtain walls get slim mullions instead.
            const FVector2D Fin=Pattern==2 ? FVector2D(40,70):FVector2D(16,16);
            const FLinearColor C=Pattern==2 ? Facade*.82f:Frame;
            for(int Side:{-1,1})
            {
                for(int I=0;I<=BX;++I) K.Column(C,Pattern==2 ? 0:2,FVector2D(-HW+120+SX*I,Side*(HD+Fin.Y*.5f)),FVector2D(Fin.X,Fin.Y),Z0,Z1);
                for(int I=0;I<=BY;++I) K.Column(C,Pattern==2 ? 0:2,FVector2D(Side*(HW+Fin.Y*.5f),-HD+120+SY*I),FVector2D(Fin.Y,Fin.X),Z0,Z1);
            }
        }
    }

    void Office(FEvaKit& K,const FEvaArchSpec& S)
    {
        auto& B=K.B; auto& Rand=K.Rand;
        const float W=S.Width, D=S.Depth, H=S.Height, Floor=S.District==2 ? 300.f:320.f;
        const int32 Pattern=S.Style%4;
        const bool bSetback=H>3600 && (S.Style/4)%2==1;
        const float CW=bSetback ? W*.8f:W, CD=bSetback ? D*.82f:D;
        const FLinearColor Facade=Pattern==3 ? FLinearColor(.045f,.09f,.12f):S.Facade, Frame=S.Accent;
        B.Mesh=K.Body(0,FVector(0,0,B.Split*.5f),FVector(W,D,B.Split),Facade,Pattern==3 ? 1:0);
        B.CrownMesh=K.Body(1,FVector(0,0,(B.Split+H)*.5f),FVector(CW,CD,H-B.Split),Facade,Pattern==3 ? 1:0);
        const bool bPodium=S.Style%3==0 && H>2000;
        Glazing(K,Pattern,bPodium ? 700.f:380.f,B.Split,W,D,Floor,Frame,Facade);
        Glazing(K,Pattern,B.Split,H,CW,CD,Floor,Frame,Facade);
        // Corner piers anchor the silhouette at Eva scale.
        for(int SX:{-1,1}) for(int SY:{-1,1}) K.Column(Frame,2,FVector2D(SX*(W*.5f+8),SY*(D*.5f+8)),FVector2D(52,52),0,B.Split);
        if(bSetback)
        {
            K.Box(Frame,2,FVector(0,0,B.Split-15),FVector(W+34,D+34,30));
            for(int SX:{-1,1}) K.Tint(FLinearColor(.08f,.2f,.07f),6,FVector(SX*(W+CW)*.25f,0,B.Split-60),FVector((W-CW)*.4f,D*.7f,120),FRotator::ZeroRotator,TEXT("Cube"));
        }
        // Street level: a lit shopfront podium with awnings, or a stone plinth and lobby glazing.
        if(bPodium)
        {
            K.Box(Facade*.7f,0,FVector(0,0,350),FVector(W+220,D+220,700));
            K.Box(Frame,2,FVector(0,0,715),FVector(W+250,D+250,34));
            for(int Side:{-1,1})
            {
                K.Light(WindowLight(Rand)*1.25f,FVector(0,Side*(D*.5f+113),250),FVector(W*.86f,8,300),.95f);
                K.Light(WindowLight(Rand)*1.25f,FVector(Side*(W*.5f+113),0,250),FVector(8,D*.86f,300),.95f);
                K.Tint(SignColor(Rand)*.55f,2,FVector(0,Side*(D*.5f+250),455),FVector(W*.55f,260,20),FRotator(0,0,Side*-8.f));
            }
        }
        else
        {
            K.Box(Facade*.72f,0,FVector(0,0,190),FVector(W+24,D+24,380));
            K.Light(WindowLight(Rand),FVector(0,-(D*.5f+15),170),FVector(W*.42f,8,250),.8f);
        }
        K.Box(Frame,2,FVector(0,-D*.5f-(bPodium ? 330:230),bPodium ? 560:390),FVector(W*.36f,380,24));
        // Roof: parapet, plant room, air handlers, then one of several crowns.
        const float Top=H;
        K.Box(Frame,2,FVector(0,-CD*.5f,Top+55),FVector(CW+30,30,110)); K.Box(Frame,2,FVector(0,CD*.5f,Top+55),FVector(CW+30,30,110));
        K.Box(Frame,2,FVector(-CW*.5f,0,Top+55),FVector(30,CD,110)); K.Box(Frame,2,FVector(CW*.5f,0,Top+55),FVector(30,CD,110));
        K.Box(Facade*.78f,0,FVector(CW*.16f,-CD*.1f,Top+230),FVector(CW*.38f,CD*.34f,460));
        for(int I=0;I<4;++I) K.Box(Steel*1.7f,2,FVector(-CW*.36f+I*250,-CD*.3f,Top+100),FVector(190,290,200));
        const int32 Crown=(S.Style/3)%4;
        if(Crown==0 && H>4200)
        {
            // Helipad with an H marking and amber edge lights.
            const FVector Pad(-CW*.14f,CD*.14f,Top+40);
            K.Box(FLinearColor(.09f,.1f,.11f),0,Pad,FVector(CW*.56f,CD*.56f,40));
            const FLinearColor Paint(.82f,.8f,.66f);
            K.Box(Paint,2,Pad+FVector(-120,0,24),FVector(42,300,6)); K.Box(Paint,2,Pad+FVector(120,0,24),FVector(42,300,6)); K.Box(Paint,2,Pad+FVector(0,0,24),FVector(200,42,6));
            for(int I=0;I<8;++I) K.Light(Amber,Pad+FVector(FMath::Cos(I*PI/4)*CW*.25f,FMath::Sin(I*PI/4)*CD*.25f,30),FVector(40,40,20),2.2f);
        }
        else if(Crown==1)
        {
            // Rooftop billboard facing the street.
            const float BW=FMath::Min(CW*.82f,1900.f);
            for(int SX:{-1,1}) K.Box(Steel,2,FVector(SX*BW*.38f,-CD*.25f,Top+320),FVector(40,40,640));
            K.Light(SignColor(Rand)*.75f,FVector(0,-CD*.25f,Top+660),FVector(BW,40,440),1.15f);
            K.Sign(FVector(0,-CD*.25f-32,Top+660),Brands[Rand.RandHelper(UE_ARRAY_COUNT(Brands))],FMath::Min(BW/7.5f,210.f),FColor(255,252,240),-90);
        }
        else if(Crown==2)
        {
            // Elevated water tank.
            const FVector Tank(-CW*.27f,CD*.2f,Top);
            for(int SX:{-1,1}) for(int SY:{-1,1}) K.Box(Steel,2,Tank+FVector(SX*120,SY*120,150),FVector(24,24,300));
            K.Box(FLinearColor(.46f,.48f,.46f),2,Tank+FVector(0,0,450),FVector(380,380,320),FRotator::ZeroRotator,TEXT("Cylinder"));
            K.Box(FLinearColor(.4f,.42f,.4f),2,Tank+FVector(0,0,640),FVector(400,400,70),FRotator::ZeroRotator,TEXT("Cone"));
        }
        if(Crown==3 || H>5600)
        {
            const float Mast=FMath::Clamp(H*.2f,700.f,1500.f);
            const FVector Foot(CW*.3f,CD*.28f,Top);
            K.Box(Steel*1.6f,2,Foot+FVector(0,0,Mast*.5f),FVector(34,34,Mast));
            for(float F:{.35f,.62f}) K.Box(Steel*1.6f,2,Foot+FVector(0,0,Mast*F),FVector(260,22,22));
            K.Beacon(Foot+FVector(0,0,Mast+40),85);
        }
        if(H>4600) for(int SX:{-1,1}) for(int SY:{-1,1}) K.Beacon(FVector(SX*CW*.48f,SY*CD*.48f,Top+130),65);
        if(Pattern==0) K.Light(Cyan,FVector(0,0,Top-470),FVector(CW+46,CD+46,18),.95f);
        // Blade signs hang off the corner in the older, denser blocks.
        if(S.District<=0 && Rand.FRand()<.5f)
        {
            const int32 Pick=Rand.RandHelper(UE_ARRAY_COUNT(Blades));
            const float Length=FMath::Min(1350.f,B.Split*.7f), Z=FMath::Max(bPodium ? 800.f:450.f,B.Split*.18f)+Length*.5f;
            K.Light(SignColor(Rand)*.7f,FVector(W*.5f-140,-D*.5f-130,Z),FVector(34,230,Length),1.05f);
            K.Box(Steel,2,FVector(W*.5f-140,-D*.5f-20,Z),FVector(24,40,Length*.9f));
            K.Sign(FVector(W*.5f-120,-D*.5f-130,Z),Blades[Pick],Length/7.2f,FColor(255,250,235),0);
        }
    }

    void Slab(FEvaKit& K,const FEvaArchSpec& S)
    {
        auto& B=K.B; auto& Rand=K.Rand;
        const float W=S.Width, D=S.Depth, H=S.Height, Floor=300, HW=W*.5f, HD=D*.5f;
        const FLinearColor Face=S.Facade, Trim=S.Facade*1.1f, Rail=S.Accent;
        B.Mesh=K.Body(0,FVector(0,0,B.Split*.5f),FVector(W,D,B.Split),Face,0);
        B.CrownMesh=K.Body(1,FVector(0,0,(B.Split+H)*.5f),FVector(W,D,H-B.Split),Face,0);
        const int32 Units=FMath::Max(3,int32(W/620));
        const float UW=W/Units;
        for(float Z=Floor;Z<H-60;Z+=Floor)
        {
            // Balcony side: slab edge, solid parapet, unit dividers, sliding doors and life on the balconies.
            K.Box(Trim,0,FVector(0,-HD-80,Z-8),FVector(W+40,160,22));
            K.Box(Rail,0,FVector(0,-HD-152,Z+52),FVector(W+40,14,105));
            // Corridor side: an open gallery with front doors.
            K.Box(Trim,0,FVector(0,HD+70,Z-8),FVector(W+20,140,22));
            K.Box(Trim*.94f,0,FVector(0,HD+134,Z+46),FVector(W+20,12,92));
            for(int U=0;U<Units;++U)
            {
                const float X=-HW+UW*(U+.5f);
                if(U) K.Box(Face*.9f,0,FVector(-HW+UW*U,-HD-80,Z+125),FVector(18,150,230));
                if(Rand.FRand()<.32f) K.Light(WindowLight(Rand),FVector(X,-HD-4,Z+118),FVector(UW*.62f,8,200));
                else K.Box(Glass,1,FVector(X,-HD-4,Z+118),FVector(UW*.62f,8,200));
                K.Box(Steel*1.3f,2,FVector(X+UW*.24f,HD+4,Z+102),FVector(90,8,188));
                if(Rand.FRand()<.35f) K.Box(FLinearColor(.55f,.56f,.54f),2,FVector(X-UW*.3f,-HD-55,Z+40),FVector(72,40,56));
                if(Rand.FRand()<.24f) K.Tint(SignColor(Rand)*.55f,0,FVector(X,-HD-162,Z+108),FVector(UW*.42f,6,44));
            }
        }
        K.Column(Face*.86f,0,FVector2D(0,HD+230),FVector2D(420,300),0,H+140);
        for(float Z=Floor*.5f;Z<H;Z+=Floor) K.Light(Cool*.8f,FVector(0,HD+384,Z),FVector(60,8,150),.6f);
        const float Number=FMath::Min(D*.55f,H*.22f);
        K.Sign(FVector(HW+10,0,H*.66f),S.Label,Number,FColor(60,78,84),0);
        K.Sign(FVector(-HW-10,0,H*.66f),S.Label,Number,FColor(60,78,84),180);
        K.Box(Trim,0,FVector(0,0,H+45),FVector(W+20,D+20,90));
        K.Box(Face*.82f,0,FVector(0,HD*.1f,H+220),FVector(480,420,440));
        const FVector Tank(HW*.55f,0,H+80);
        for(int SX:{-1,1}) for(int SY:{-1,1}) K.Box(Steel,2,Tank+FVector(SX*110,SY*110,110),FVector(22,22,220));
        K.Box(FLinearColor(.52f,.54f,.5f),2,Tank+FVector(0,0,340),FVector(340,340,260),FRotator::ZeroRotator,TEXT("Cylinder"));
        for(int I=0;I<3;++I) K.Box(Steel*1.4f,2,FVector(-HW*.6f+I*HW*.4f,-HD*.3f,H+260),FVector(12,12,520));
    }

    void Hall(FEvaKit& K,const FEvaArchSpec& S)
    {
        auto& B=K.B; auto& Rand=K.Rand;
        const float W=S.Width, D=S.Depth, H=S.Height, HW=W*.5f, HD=D*.5f;
        const FLinearColor Wall=S.Facade, Roof=S.Accent;
        B.Mesh=K.Body(0,FVector(0,0,B.Split*.5f),FVector(W,D,B.Split),Wall,5);
        B.CrownMesh=K.Body(1,FVector(0,0,(B.Split+H)*.5f),FVector(W,D,H-B.Split),Wall,5);
        K.Box(Wall*.5f,0,FVector(0,0,90),FVector(W+30,D+30,180));
        if(S.Style%2)
        {
            // Sawtooth factory roof: steep glazed faces admit north light.
            const int32 Teeth=FMath::Max(3,int32(W/850));
            const float TW=W/Teeth, TH=FMath::Min(430.f,H*.32f);
            for(int I=0;I<Teeth;++I)
            {
                const float X=-HW+TW*(I+.5f);
                K.Box(Roof,5,FVector(X,0,H+TH*.5f),FVector(TW,D+30,TH),FRotator::ZeroRotator,TEXT("RoofSaw"));
                if(Rand.FRand()<.55f) K.Light(Cool*.5f,FVector(X+TW*.5f-6,0,H+TH*.45f),FVector(8,D-80,TH*.7f),.55f);
                else K.Box(Glass,1,FVector(X+TW*.5f-6,0,H+TH*.45f),FVector(8,D-80,TH*.7f));
            }
        }
        else
        {
            const float RH=FMath::Min(D*.2f,520.f);
            K.Box(Roof,5,FVector(0,0,H+RH*.5f),FVector(D+70,W+70,RH),FRotator(0,90,0),TEXT("RoofGable"));
            for(int I=0;I<int32(W/900);++I) K.Box(Steel*1.5f,2,FVector(-HW+450+I*900,0,H+RH+40),FVector(260,200,110));
        }
        const int32 Doors=FMath::Clamp(int32(W/900),2,4);
        const float DoorH=FMath::Min(H*.62f,850.f), DoorW=FMath::Min(W/Doors*.62f,540.f);
        for(int I=0;I<Doors;++I)
        {
            const float X=-HW+W/Doors*(I+.5f);
            K.Box(Wall*.55f,5,FVector(X,-HD-8,DoorH*.5f),FVector(DoorW,16,DoorH));
            K.Box(Roof*.8f,2,FVector(X,-HD-14,DoorH+30),FVector(DoorW+60,24,40));
            K.Tint(Hazard,9,FVector(X,-HD-22,45),FVector(DoorW+80,12,90));
            K.Light(Amber,FVector(X,-HD-30,DoorH+90),FVector(60,20,30),2.f);
        }
        K.Box(Steel,2,FVector(0,-HD-260,DoorH+120),FVector(W*.8f,500,26));
        for(int Side:{-1,1})
        {
            K.Box(Glass,1,FVector(0,Side*(HD+4),H-170),FVector(W-260,8,150));
            for(int I=0;I<4;++I) if(Rand.FRand()<.5f) K.Light(WindowLight(Rand)*.8f,FVector(-HW+W*(I+.5f)/4,Side*(HD+7),H-170),FVector(W*.18f,6,120));
        }
        for(int SX:{-1,1}) for(int SY:{-1,1}) K.Column(Steel*1.4f,2,FVector2D(SX*(HW+12),SY*(HD+12)),FVector2D(26,26),0,H);
        if(S.District==3)
            for(int I=0;I<2;++I) K.Box(FLinearColor(.72f,.1f,.05f),8,FVector(-HW*.45f+I*HW*.9f,HD*.3f,H+420),FVector(170,170,840),FRotator::ZeroRotator,TEXT("Cylinder"));
        K.Sign(FVector(0,-HD-14,H*.82f),S.Label,FMath::Clamp(H*.16f,120.f,300.f),FColor(240,230,205),-90);
    }

    void Tank(FEvaKit& K,const FEvaArchSpec& S)
    {
        auto& B=K.B;
        const float R=S.Width*.5f, H=S.Height;
        B.Mesh=K.Body(0,FVector(0,0,B.Split*.5f),FVector(R*2,R*2,B.Split),S.Facade,2,TEXT("Cylinder"));
        B.CrownMesh=K.Body(1,FVector(0,0,(B.Split+H)*.5f),FVector(R*2,R*2,H-B.Split),S.Facade,2,TEXT("Cylinder"));
        K.Box(S.Facade*1.04f,2,FVector(0,0,H),FVector(R*2,R*2,R*.36f),FRotator::ZeroRotator,TEXT("Sphere"));
        for(float Z:{H*.33f,H*.66f}) K.Box(S.Accent,2,FVector(0,0,Z),FVector(R*2+14,R*2+14,40),FRotator::ZeroRotator,TEXT("Cylinder"));
        K.Tint(Hazard,9,FVector(0,0,60),FVector(R*2+10,R*2+10,120),FRotator::ZeroRotator,TEXT("Cylinder"));
        K.Box(Steel*1.5f,2,FVector(0,0,H+50),FVector(R*2+36,R*2+36,10),FRotator::ZeroRotator,TEXT("Cylinder"));
        // A spiral stair wraps the shell up to the roof.
        const int32 Steps=14;
        const float Run=.42f*(R+45), Rise=H/Steps;
        for(int I=0;I<Steps;++I)
        {
            const float A=I*.42f;
            K.Box(Steel*1.4f,2,FVector(FMath::Cos(A)*(R+45),FMath::Sin(A)*(R+45),(I+.5f)*Rise),FVector(Run*1.05f,70,14),
                FRotator(FMath::RadiansToDegrees(FMath::Atan2(Rise,Run)),FMath::RadiansToDegrees(A)+90,0));
        }
        K.Sign(FVector(0,-R-14,H*.55f),S.Label,FMath::Min(R*.34f,260.f),FColor(40,52,58),-90);
    }

    void Stack(FEvaKit& K,const FEvaArchSpec& S)
    {
        auto& B=K.B;
        const float R=S.Width*.5f, H=S.Height;
        K.Box(FLinearColor(.24f,.25f,.24f),0,FVector(0,0,200),FVector(R*2+420,R*2+420,400));
        B.Mesh=K.Body(0,FVector(0,0,B.Split*.5f),FVector(R*2,R*2,B.Split),S.Facade,8,TEXT("Cylinder"));
        B.CrownMesh=K.Body(1,FVector(0,0,(B.Split+H)*.5f),FVector(R*1.8f,R*1.8f,H-B.Split),S.Facade,8,TEXT("Cylinder"));
        for(float F:{.62f,.92f})
        {
            K.Box(Steel,2,FVector(0,0,H*F),FVector(R*2+190,R*2+190,30),FRotator::ZeroRotator,TEXT("Cylinder"));
            K.Box(Steel*1.4f,2,FVector(0,0,H*F+80),FVector(R*2+200,R*2+200,8),FRotator::ZeroRotator,TEXT("Cylinder"));
            for(int I=0;I<3;++I) K.Beacon(FVector(FMath::Cos(I*2*PI/3)*(R+70),FMath::Sin(I*2*PI/3)*(R+70),H*F+130),70);
        }
        K.Column(Steel*1.4f,2,FVector2D(R+12,0),FVector2D(22,50),400,H);
        K.Box(Soot,0,FVector(0,0,H+12),FVector(R*1.62f,R*1.62f,24),FRotator::ZeroRotator,TEXT("Cylinder"));
    }

    void Cooling(FEvaKit& K,const FEvaArchSpec& S)
    {
        auto& B=K.B;
        const float R=S.Width*.5f, H=S.Height;
        // A hidden core carries collision inside the open hyperboloid shell.
        B.Mesh=K.Body(0,FVector(0,0,H*.4f),FVector(R*1.12f,R*1.12f,H*.8f),FLinearColor(.07f,.075f,.075f),0,TEXT("Cylinder"));
        K.Box(S.Facade,0,FVector(0,0,H*.5f),FVector(R*2,R*2,H),FRotator::ZeroRotator,TEXT("CoolingTower"));
        for(int I=0;I<18;++I)
        {
            const float A=I*2*PI/18;
            K.Box(S.Facade*.75f,0,FVector(FMath::Cos(A)*R*.97f,FMath::Sin(A)*R*.97f,130),FVector(45,45,280),FRotator(0,FMath::RadiansToDegrees(A),16));
        }
        K.Box(S.Facade*.55f,0,FVector(0,0,H*.97f),FVector(R*1.42f,R*1.42f,60),FRotator::ZeroRotator,TEXT("Cylinder"));
    }

    void Globe(FEvaKit& K,const FEvaArchSpec& S)
    {
        auto& B=K.B;
        const float R=S.Width*.5f, Z=S.Height-R;
        B.Mesh=K.Body(0,FVector(0,0,Z),FVector(R*2,R*2,R*2),S.Facade,2,TEXT("Sphere"));
        for(int I=0;I<10;++I)
        {
            const float A=I*2*PI/10;
            K.Box(S.Accent,2,FVector(FMath::Cos(A)*R*.8f,FMath::Sin(A)*R*.8f,Z*.5f),FVector(80,80,Z),FRotator::ZeroRotator,TEXT("Cylinder"));
        }
        K.Box(Steel*1.3f,2,FVector(0,0,Z),FVector(R*2+70,R*2+70,26),FRotator::ZeroRotator,TEXT("Cylinder"));
        K.Box(Steel*1.3f,2,FVector(0,0,Z+R+30),FVector(280,280,60),FRotator::ZeroRotator,TEXT("Cylinder"));
        K.Box(Steel*1.4f,2,FVector(R*.55f,-R*1.02f,Z*.5f),FVector(Z*1.3f,90,20),FRotator(-38,0,0));
        K.Tint(Hazard,9,FVector(0,0,50),FVector(R*1.7f,R*1.7f,100),FRotator::ZeroRotator,TEXT("Cylinder"));
        K.Sign(FVector(0,-R-24,Z),S.Label,R*.3f,FColor(40,60,70),-90);
    }
}

UStaticMesh* AEvaGameMode::KitMesh(const FString& Kind)
{
    if(auto* Found=MeshCache.FindRef(Kind)) return Found;
    const bool bEngine=Kind==TEXT("Cube") || Kind==TEXT("Sphere") || Kind==TEXT("Cylinder") || Kind==TEXT("Cone") || Kind==TEXT("Plane");
    const FString Path=(bEngine ? TEXT("/Engine/BasicShapes/"):TEXT("/Game/Models/"))+Kind+TEXT(".")+Kind;
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
    if(!Mesh && !bEngine)
    {
        UE_LOG(LogTemp,Warning,TEXT("EVA_MISSING_MESH %s; using a cube"),*Kind);
        Mesh=KitMesh(TEXT("Cube"));
    }
    MeshCache.Add(Kind,Mesh); return Mesh;
}

int32 AEvaGameMode::BuildArchitecture(FVector Base,const FEvaArchSpec& S)
{
    FEvaBuilding B;
    B.Kind=S.Kind; B.District=S.District; B.Base=Base; B.Height=S.Height; B.FacadeColor=S.Facade;
    const bool bRound=S.Kind==EEvaArch::Tank || S.Kind==EEvaArch::Stack || S.Kind==EEvaArch::Cooling || S.Kind==EEvaArch::Sphere;
    B.Extent=bRound ? FVector2D(S.Width*.5f):FVector2D(S.Width*.5f,S.Depth*.5f);
    B.Center=Base+FVector(0,0,S.Height*.5f);
    const float Floor=S.Kind==EEvaArch::Slab || S.District==2 ? 300.f:320.f;
    switch(S.Kind)
    {
    case EEvaArch::Office: B.Split=FMath::Max(Floor,FMath::GridSnap(S.Height*.56f,Floor)); B.MaxTilt=S.Height>3000 ? 24:14; break;
    case EEvaArch::Slab: B.Split=FMath::Max(Floor,FMath::GridSnap(S.Height*.55f,Floor)); B.MaxTilt=10; break;
    case EEvaArch::Hall: B.Split=S.Height*.5f; B.MaxTilt=5; break;
    case EEvaArch::Tank: B.Split=S.Height*.6f; B.MaxTilt=7; break;
    case EEvaArch::Stack: B.Split=S.Height*.45f; B.MaxTilt=82; break;
    default: B.Split=S.Height; B.MaxTilt=0; break;
    }
    B.DistrictRoot=NewSceneRoot(Base);
    if(S.Kind!=EEvaArch::Cooling && S.Kind!=EEvaArch::Sphere)
    {
        B.Crown=NewSceneRoot(Base+FVector(0,0,B.Split));
        B.Crown->AttachToComponent(B.DistrictRoot,FAttachmentTransformRules::KeepWorldTransform);
    }
    FRandomStream Rand(S.Style*7919+int32(FMath::Abs(Base.X)*.013f+FMath::Abs(Base.Y)*.029f));
    FEvaKit Kit(this,B,Rand);
    switch(S.Kind)
    {
    case EEvaArch::Office: Office(Kit,S); break;
    case EEvaArch::Slab: Slab(Kit,S); break;
    case EEvaArch::Hall: Hall(Kit,S); break;
    case EEvaArch::Tank: Tank(Kit,S); break;
    case EEvaArch::Stack: Stack(Kit,S); break;
    case EEvaArch::Cooling: Cooling(Kit,S); break;
    case EEvaArch::Sphere: Globe(Kit,S); break;
    }
    // Blast scars and fires are added at runtime to the section that was hit.
    for(int32 Section=0;Section<(B.Crown ? 2:1);++Section)
    {
        B.Scars[Section]=CityInstances(Section ? B.Crown:B.DistrictRoot,FLinearColor::White,10,3.2f);
        B.Scars[Section]->SetNumCustomDataFloats(3);
    }
    B.SmokePoint=B.Center;
    B.Skin[0]=B.Mesh->GetMaterial(0);
    if(B.CrownMesh) B.Skin[1]=B.CrownMesh->GetMaterial(0);
    const int32 Index=Buildings.Add(B);
    BuildingByComponent.Add(B.Mesh,Index);
    if(B.CrownMesh) BuildingByComponent.Add(B.CrownMesh,Index);
    if(S.Kind==EEvaArch::Stack || S.Kind==EEvaArch::Cooling) SteamVents.Add(Index);
    return Index;
}

UInstancedStaticMeshComponent* AEvaGameMode::PropBatch(int32 District,FLinearColor Color,int32 Type,float Glow,const TCHAR* MeshKind,bool bShadow)
{
    const FString Key=FString::Printf(TEXT("%d/%s/%d/%.2f/%s/%d"),District,*Color.ToString(),Type,Glow,MeshKind,int32(bShadow));
    if(auto* Found=PropBatches.FindRef(Key)) return Found;
    auto* Batch=CityInstances(nullptr,Color,Type,Glow,MeshKind);
    Batch->SetNumCustomDataFloats(3); Batch->SetCastShadow(bShadow); Batch->SetCullDistances(26000,40000);
    PropBatches.Add(Key,Batch); return Batch;
}

int32 AEvaGameMode::AddProp(FVector Position,float Radius)
{
    FEvaProp Prop; Prop.Position=Position; Prop.Radius=Radius;
    const int32 Index=Props.Add(Prop);
    PropGrid.FindOrAdd(FIntPoint(FMath::FloorToInt(Position.X/2000),FMath::FloorToInt(Position.Y/2000))).Add(Index);
    return Index;
}

void AEvaGameMode::AddPropPart(int32 Prop,UInstancedStaticMeshComponent* Batch,const FTransform& Rest,EEvaBreak Mode,FLinearColor Tint)
{
    FEvaPropPart Part; Part.Batch=Batch; Part.Rest=Rest; Part.Mode=Mode;
    Part.Instance=Batch->AddInstance(Rest);
    const float Data[3]={Tint.R,Tint.G,Tint.B}; Batch->SetCustomData(Part.Instance,MakeArrayView(Data,3));
    Props[Prop].Parts.Add(Part);
}

void AEvaGameMode::AddTree(int32 District,FVector P,float Scale)
{
    FRandomStream Rand(int32(P.X*.1f+P.Y*.37f));
    const int32 Prop=AddProp(P,160*Scale);
    const float Trunk=Rand.FRandRange(380,520)*Scale;
    AddPropPart(Prop,PropBatch(District,FLinearColor(.12f,.08f,.05f),0,0,TEXT("Cylinder")),Pose(P+FVector(0,0,Trunk*.5f),FVector(46*Scale,46*Scale,Trunk)),EEvaBreak::Topple);
    auto* Canopy=PropBatch(District,White,6,0,TEXT("Sphere"),true);
    const FLinearColor Leaf=FLinearColor(.07f,.15f,.055f)*Rand.FRandRange(.75f,1.3f);
    AddPropPart(Prop,Canopy,Pose(P+FVector(0,0,Trunk+120*Scale),FVector(Rand.FRandRange(420,560),Rand.FRandRange(420,560),Rand.FRandRange(360,470))*Scale),EEvaBreak::Topple,Leaf);
    AddPropPart(Prop,Canopy,Pose(P+FVector(Rand.FRandRange(-90,90),Rand.FRandRange(-90,90),Trunk+320)*FVector(Scale,Scale,1),FVector(310,310,280)*Scale),EEvaBreak::Topple,Leaf*1.18f);
}

void AEvaGameMode::AddHouse(int32 District,FVector P,float Width,float Depth,bool bTurned,FLinearColor Wall,FLinearColor Roof)
{
    FRandomStream Rand(int32(P.X*.21f+P.Y*.13f));
    const int32 Prop=AddProp(P,FMath::Max(Width,Depth)*.5f);
    const FRotator Yaw(0,bTurned ? 90:0,0);
    const float H=Rand.FRandRange(520,600), RoofH=Depth*.34f;
    AddPropPart(Prop,PropBatch(District,White,0,0,TEXT("Cube"),true),Pose(P+FVector(0,0,H*.5f),FVector(Width,Depth,H),Yaw),EEvaBreak::Flatten,Wall);
    // The gable ridge runs along the long side; the generated prism's ridge is its local Y axis.
    AddPropPart(Prop,PropBatch(District,White,2,0,TEXT("RoofGable"),true),Pose(P+FVector(0,0,H+RoofH*.5f),FVector(Depth+90,Width+90,RoofH),Yaw+FRotator(0,90,0)),EEvaBreak::Drop,Roof);
    auto* Windows=PropBatch(District,White,1,.65f);
    for(int Side:{-1,1}) for(int I=-1;I<=1;++I)
    {
        const FVector Local(I*Width*.3f,Side*(Depth*.5f+4),H*(I==0 && Side<0 ? .3f:.64f));
        const bool bDoor=I==0 && Side<0;
        const FLinearColor Tint=bDoor ? FLinearColor(.05f,.035f,.025f):Rand.FRand()<.4f ? WindowLight(Rand)*.9f:FLinearColor(.02f,.035f,.045f);
        AddPropPart(Prop,Windows,Pose(P+Yaw.RotateVector(Local),bDoor ? FVector(110,8,220):FVector(Width*.18f,8,130),Yaw),EEvaBreak::Hide,Tint);
    }
    if(Rand.FRand()<.45f) AddPropPart(Prop,PropBatch(District,White,2,0,TEXT("Cube")),Pose(P+Yaw.RotateVector(FVector(Width*.3f,Depth*.5f+40,H*.55f)),FVector(80,50,60),Yaw),EEvaBreak::Hide,FLinearColor(.55f,.56f,.54f));
}

void AEvaGameMode::AddCar(int32 District,FVector P,float Yaw,FLinearColor Paint)
{
    const int32 Prop=AddProp(P,230);
    const FRotator R(0,Yaw,0);
    AddPropPart(Prop,PropBatch(District,White,2,0,TEXT("Cube"),true),Pose(P+FVector(0,0,72),FVector(430,176,82),R),EEvaBreak::Flatten,Paint);
    AddPropPart(Prop,PropBatch(District,White,1,.3f),Pose(P+R.RotateVector(FVector(-25,0,148)),FVector(230,160,72),R),EEvaBreak::Flatten,FLinearColor(.03f,.05f,.06f));
    AddPropPart(Prop,PropBatch(District,White,1,2.f),Pose(P+R.RotateVector(FVector(-216,0,80)),FVector(8,150,18),R),EEvaBreak::Hide,FLinearColor(.8f,.03f,.02f));
}

void AEvaGameMode::AddStreetLight(int32 District,FVector P,float Yaw)
{
    const int32 Prop=AddProp(P,70);
    const FRotator R(0,Yaw,0);
    const FLinearColor Pole(.1f,.12f,.13f);
    AddPropPart(Prop,PropBatch(District,Pole,2,0,TEXT("Cylinder")),Pose(P+FVector(0,0,500),FVector(26,26,1000)),EEvaBreak::Topple);
    AddPropPart(Prop,PropBatch(District,Pole,2),Pose(P+R.RotateVector(FVector(160,0,990)),FVector(320,18,18),R),EEvaBreak::Topple);
    AddPropPart(Prop,PropBatch(District,FLinearColor(1,.72f,.4f),2,1.6f),Pose(P+R.RotateVector(FVector(290,0,974)),FVector(110,50,14),R),EEvaBreak::Topple);
}

void AEvaGameMode::AddUtilityLine(int32 District,FVector From,FVector To,int32 Poles)
{
    const FVector Along=(To-From).GetSafeNormal2D(), Across(-Along.Y,Along.X,0);
    const FRotator Arm=Across.Rotation();
    auto* Wood=PropBatch(District,White,0,0,TEXT("Cylinder"));
    auto* Fittings=PropBatch(District,White,2);
    auto* Wires=PropBatch(District,FLinearColor(.02f,.022f,.025f),2);
    TArray<int32> Line;
    for(int I=0;I<Poles;++I)
    {
        const FVector P=FMath::Lerp(From,To,I/float(Poles-1));
        const int32 Prop=AddProp(P,70); Line.Add(Prop);
        AddPropPart(Prop,Wood,Pose(P+FVector(0,0,560),FVector(30,30,1120)),EEvaBreak::Topple,FLinearColor(.42f,.42f,.4f));
        AddPropPart(Prop,Fittings,Pose(P+FVector(0,0,1030),FVector(210,14,14),Arm),EEvaBreak::Topple,FLinearColor(.1f,.11f,.12f));
        AddPropPart(Prop,Fittings,Pose(P+FVector(0,0,900),FVector(150,12,12),Arm),EEvaBreak::Topple,FLinearColor(.1f,.11f,.12f));
        if(I%2==0) AddPropPart(Prop,PropBatch(District,White,2,0,TEXT("Cylinder")),Pose(P+Across*45+FVector(0,0,820),FVector(55,55,95)),EEvaBreak::Topple,FLinearColor(.35f,.37f,.37f));
    }
    // Sagging spans belong to both poles, so either pole falling drops the wires.
    for(int I=0;I+1<Poles;++I) for(int Wire=-1;Wire<=1;++Wire) for(int Seg=0;Seg<4;++Seg)
    {
        auto Point=[&](float T) { return FMath::Lerp(From,To,(I+T)/float(Poles-1))+Across*Wire*90+FVector(0,0,1028-(Wire ? 0:128)-FMath::Sin(T*PI)*110); };
        const FVector A=Point(Seg/4.f), C=Point((Seg+1)/4.f), Delta=C-A;
        FEvaPropPart Part; Part.Batch=Wires; Part.Mode=EEvaBreak::Hide;
        Part.Rest=FTransform(Delta.Rotation(),(A+C)*.5f,FVector(Delta.Size()/100,.05f,.05f));
        Part.Instance=Wires->AddInstance(Part.Rest);
        const float Data[3]={1,1,1}; Wires->SetCustomData(Part.Instance,MakeArrayView(Data,3));
        Props[Line[I]].Parts.Add(Part); Props[Line[I+1]].Parts.Add(Part);
    }
}

void AEvaGameMode::AddVending(int32 District,FVector P,float Yaw,FLinearColor Face)
{
    const int32 Prop=AddProp(P,90);
    const FRotator R(0,Yaw,0);
    AddPropPart(Prop,PropBatch(District,White,2),Pose(P+FVector(0,0,95),FVector(70,95,190),R),EEvaBreak::Topple,Face);
    AddPropPart(Prop,PropBatch(District,White,1,1.4f),Pose(P+R.RotateVector(FVector(37,0,120)),FVector(6,80,110),R),EEvaBreak::Topple,FLinearColor(.8f,.85f,.9f));
}

void AEvaGameMode::AddContainerStack(int32 District,FVector P,int32 Rows,int32 Tiers,bool bTurned)
{
    FRandomStream Rand(int32(P.X*.07f+P.Y*.19f));
    const FLinearColor Colors[]={FLinearColor(.55f,.08f,.04f),FLinearColor(.06f,.2f,.45f),FLinearColor(.1f,.32f,.14f),FLinearColor(.75f,.36f,.05f),FLinearColor(.45f,.46f,.44f),FLinearColor(.72f,.72f,.68f),FLinearColor(.3f,.06f,.08f)};
    const int32 Prop=AddProp(P,650);
    const FRotator R(0,bTurned ? 90:0,0);
    auto* Boxes=PropBatch(District,White,5,0,TEXT("Cube"),true);
    for(int Row=0;Row<Rows;++Row) for(int Tier=0;Tier<Tiers;++Tier)
    {
        if(Tier>0 && Rand.FRand()<.2f) continue;
        const FVector Local(Rand.FRandRange(-20,20),(Row-(Rows-1)*.5f)*252,Tier*262+130);
        AddPropPart(Prop,Boxes,Pose(P+R.RotateVector(Local),FVector(1220,244,259),R),EEvaBreak::Scatter,Colors[Rand.RandHelper(UE_ARRAY_COUNT(Colors))]*Rand.FRandRange(.8f,1.1f));
    }
}
