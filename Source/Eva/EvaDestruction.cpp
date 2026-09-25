#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    const FTransform Parked(FRotator::ZeroRotator,FVector(0,0,-60000),FVector(.001f));
    void SetTint(UInstancedStaticMeshComponent* Batch,int32 Index,FLinearColor Tint)
    {
        const float Data[3]={Tint.R,Tint.G,Tint.B};
        Batch->SetCustomData(Index,MakeArrayView(Data,3));
    }
    bool IsRound(const FEvaBuilding& B) { return B.Kind==EEvaArch::Tank || B.Kind==EEvaArch::Stack || B.Kind==EEvaArch::Cooling || B.Kind==EEvaArch::Sphere; }
    float PileHeight(const FEvaBuilding& B) { return FMath::Clamp(B.Height*.12f,180.f,950.f); }
    // Rubble rises out of the ground while the structure sinks into its own dust.
    float RubbleDrop(const FEvaBuilding& B) { return PileHeight(B)+900; }
    float DustScale(const FEvaBuilding& B) { return FMath::Clamp(FMath::Max(B.Extent.X,B.Extent.Y)/1100,.6f,1.5f); }
}

UInstancedStaticMeshComponent* AEvaGameMode::ParticleMesh(int32 Batch)
{
    if(ParticleBatches.Num()==0)
    {
        // Fixed pools: 0 debris chunks, 1 dust/smoke/steam, 2 fire and embers. Spawns beyond capacity are skipped.
        ParticleBatches.SetNum(3);
        const int32 Capacity[3]={260,280,120};
        for(int32 I=0;I<3;++I)
        {
            auto* Mesh=CityInstances(nullptr,FLinearColor::White,I==2 ? 10:0,I==2 ? 4.5f:0,I==0 ? TEXT("DebrisChunk"):TEXT("Sphere"));
            Mesh->SetNumCustomDataFloats(I==1 ? 2:3);
            if(I==1 && DustSurface) Mesh->SetMaterial(0,DustSurface);
            Mesh->SetCullDistances(0,0);
            TArray<FTransform> Hidden; Hidden.Init(Parked,Capacity[I]);
            Mesh->AddInstances(Hidden,false);
            auto& Pool=ParticleBatches[I]; Pool.Mesh=Mesh; Pool.bFades=I==1;
            for(int32 Slot=Capacity[I]-1;Slot>=0;--Slot) Pool.Free.Add(Slot);
        }
        // Shared rubble batches: 0 chunks, 1 floor slabs, 2 rebar, 3 standing stumps, 4 embers.
        const TCHAR* Kinds[5]={TEXT("DebrisChunk"),TEXT("SlabShard"),TEXT("Cube"),TEXT("StumpJagged"),TEXT("Sphere")};
        for(int32 I=0;I<5;++I)
        {
            auto* Mesh=CityInstances(nullptr,FLinearColor::White,I==3 ? 7:I==4 ? 10:I==2 ? 2:0,I==4 ? 5.f:0,Kinds[I]);
            Mesh->SetNumCustomDataFloats(3); Mesh->SetCastShadow(I<2 || I==3); Mesh->SetCullDistances(45000,60000);
            RubbleBatches.Add(Mesh);
        }
    }
    return ParticleBatches[Batch].Mesh;
}

void AEvaGameMode::SpawnParticle(int32 Batch,FVector Position,FVector Velocity,FVector Scale,float Life,float Gravity,float Growth,float Bright,FLinearColor Tint,float Alpha)
{
    ParticleMesh(Batch);
    auto& Pool=ParticleBatches[Batch];
    if(Pool.Free.Num()==0) return;
    FEvaParticle P;
    P.Slot=Pool.Free.Pop(); P.Position=Position; P.Velocity=Velocity; P.Scale=Scale; P.Life=Life;
    P.Gravity=Gravity; P.Growth=Growth; P.Bright=Bright; P.Alpha=Alpha; P.Drag=Batch==1 ? .55f:Batch==2 ? 1.2f:.05f;
    P.Rotation=FRotator(FMath::FRandRange(-180.f,180.f),FMath::FRandRange(-180.f,180.f),FMath::FRandRange(-180.f,180.f));
    if(Batch==0) P.Spin=FRotator(FMath::FRandRange(-260.f,260.f),FMath::FRandRange(-260.f,260.f),FMath::FRandRange(-260.f,260.f));
    if(Pool.bFades) { Pool.Mesh->SetCustomDataValue(P.Slot,0,0); Pool.Mesh->SetCustomDataValue(P.Slot,1,Bright); }
    else SetTint(Pool.Mesh,P.Slot,Tint);
    Pool.Live.Add(P);
}

void AEvaGameMode::DebrisBurst(FVector Position,FVector Direction,int32 Count,float Speed,FLinearColor Tint)
{
    for(int32 I=0;I<Count;++I)
    {
        const FVector Velocity=(Direction+FMath::VRand()*.7f).GetSafeNormal()*Speed*FMath::FRandRange(.4f,1.f)+FVector(0,0,Speed*.35f);
        SpawnParticle(0,Position+FMath::VRand()*120,Velocity,FVector(FMath::FRandRange(.5f,2.2f)),FMath::FRandRange(1.8f,3.2f),2600,0,1,Tint*FMath::FRandRange(.55f,1.1f));
    }
}

void AEvaGameMode::SmokePuff(FVector Position,float Size,float Darkness)
{
    SpawnParticle(1,Position+FVector(FMath::FRandRange(-120.f,120.f),FMath::FRandRange(-120.f,120.f),0),
        FVector(FMath::FRandRange(60.f,180.f),FMath::FRandRange(20.f,120.f),FMath::FRandRange(220.f,420.f)),
        FVector(Size*FMath::FRandRange(2.6f,3.6f)),FMath::FRandRange(5.f,7.5f),0,.42f,FMath::Lerp(1.f,.22f,Darkness),FLinearColor::White,1.6f+Darkness);
}

void AEvaGameMode::DistrictDust(FVector Position,float Size)
{
    for(int32 I=0;I<8;++I)
    {
        const float Angle=I*PI/4+FMath::FRandRange(-.2f,.2f);
        const FVector Dir(FMath::Cos(Angle),FMath::Sin(Angle),0);
        SpawnParticle(1,Position+Dir*500*Size+FVector(0,0,220),Dir*FMath::FRandRange(500.f,1100.f)*Size+FVector(0,0,120),
            FVector(FMath::FRandRange(6.f,9.f)*Size),FMath::FRandRange(3.5f,5.f),0,.45f,1,FLinearColor::White,2.2f);
    }
}

void AEvaGameMode::DestroyNearby(FVector P,float Radius)
{
    // Blasts reach any structure whose footprint lies inside the strike, not only those centred in it.
    for(auto& B:Buildings)
    {
        if(B.bDestroyed || !B.DistrictRoot) continue;
        const FVector2D Local(P.X-B.Base.X,P.Y-B.Base.Y);
        const FVector2D Closest(FMath::Clamp(Local.X,-B.Extent.X,B.Extent.X),FMath::Clamp(Local.Y,-B.Extent.Y,B.Extent.Y));
        if((Local-Closest).Size()<Radius) DamageBuilding(B,B.Base+FVector(Closest.X,Closest.Y,FMath::Min(420.f,B.Height*.4f)),1);
    }
    BreakPropsNear(P,Radius);
}

void AEvaGameMode::DamageBuilding(FEvaBuilding& B,FVector Impact,int32 Stages)
{
    if(!B.DistrictRoot || B.bDestroyed) return;
    const int32 Index=int32(&B-Buildings.GetData());
    FRandomStream Rand(Index*97+B.DamageStage*13+int32(Impact.Z));
    const bool bRound=IsRound(B);
    FVector Local=Impact-B.Base;
    Local.Z=FMath::Clamp(Local.Z,260.f,FMath::Max(300.f,B.Height-200));
    // The struck face decides where scars appear; the structure falls away from the blow that fells it.
    FVector Normal=bRound ? FVector(Local.X,Local.Y,0).GetSafeNormal()
        : B.Extent.X-FMath::Abs(Local.X)<B.Extent.Y-FMath::Abs(Local.Y) ? FVector(FMath::Sign(Local.X),0,0):FVector(0,FMath::Sign(Local.Y),0);
    if(Normal.IsNearlyZero()) { const float A=Index*1.7f; Normal=bRound ? FVector(FMath::Cos(A),FMath::Sin(A),0) : FMath::Abs(FMath::Cos(A))>.7f ? FVector(FMath::Sign(FMath::Cos(A)),0,0):FVector(0,FMath::Sign(FMath::Sin(A)),0); }
    const FVector Tangent(-Normal.Y,Normal.X,0);
    const float FaceDepth=bRound ? B.Extent.X:FMath::Abs(Normal.X)*B.Extent.X+FMath::Abs(Normal.Y)*B.Extent.Y;
    const float Reach=bRound ? B.Extent.X*.7f:FMath::Abs(Tangent.X)*B.Extent.X+FMath::Abs(Tangent.Y)*B.Extent.Y;
    const FVector Face=Normal*FaceDepth+Tangent*FMath::Clamp(FVector::DotProduct(Local,Tangent),-Reach*.7f,Reach*.7f)+FVector(0,0,Local.Z);
    B.FallDirection=-Normal;
    // Blast holes with fires burning inside some of them, grouped around the point of impact.
    const float Spread=FMath::Min(Reach*.6f,650.f);
    const FRotator Facing=Normal.Rotation();
    for(int32 I=0;I<9;++I)
    {
        FVector P=Face+Tangent*Rand.FRandRange(-Spread,Spread)+FVector(0,0,Rand.FRandRange(-420.f,420.f));
        P.Z=FMath::Clamp(P.Z,120.f,B.Height-80);
        const int32 Sec=B.Crown && P.Z>B.Split ? 1:0;
        if(!B.Scars[Sec]) continue;
        const FVector Rel=P-FVector(0,0,Sec ? B.Split:0);
        const FVector Size(60,Rand.FRandRange(160.f,520.f),Rand.FRandRange(140.f,360.f));
        SetTint(B.Scars[Sec],B.Scars[Sec]->AddInstance(FTransform(Facing,Rel+Normal*8,Size/100)),FLinearColor(.008f,.007f,.006f));
        if(Rand.FRand()<.55f) SetTint(B.Scars[Sec],B.Scars[Sec]->AddInstance(FTransform(Facing,Rel+Normal*24,FVector(20,Size.Y*.5f,Size.Z*.45f)/100)),FLinearColor(1,Rand.FRandRange(.26f,.5f),.05f));
    }
    const int32 Before=B.DamageStage;
    B.DamageStage=FMath::Min(2,B.DamageStage+FMath::Max(1,Stages));
    B.SmokePoint=B.Base+Face+Normal*160;
    if(Before==0)
    {
        // Power fails: lit windows and signs go dark and the facade scorches.
        for(auto* Part:B.Lights) Part->SetVisibility(false);
        B.Mesh->SetMaterial(0,DistrictMaterial(B.FacadeColor,7));
        if(B.CrownMesh) B.CrownMesh->SetMaterial(0,DistrictMaterial(B.FacadeColor,7));
    }
    DebrisBurst(B.SmokePoint,Normal,14,1500,B.FacadeColor);
    for(int32 I=0;I<3;++I) SmokePuff(B.SmokePoint,1.2f,.6f);
    DistrictSound(TEXT("Impact"),B.SmokePoint,.7f);
    if(B.DamageStage<2)
    {
        if(CrumbleClock<=0) { DistrictSound(TEXT("Crumble"),B.SmokePoint,.55f); CrumbleClock=.4f; }
        return;
    }
    // Second failure: the frame gives way.
    B.bDestroyed=true; B.CollapseTime=.001f; B.SmokeClock=0; ++BuildingsLost;
    B.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(B.CrownMesh) B.CrownMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CollapseRubble(B);
    DistrictDust(FVector(B.Base.X,B.Base.Y,0),DustScale(B));
    DistrictSound(TEXT("Collapse"),B.Center,.9f);
    DistrictSound(TEXT("Rumble"),B.Base,.45f);
    if(B.Kind==EEvaArch::Tank || B.Kind==EEvaArch::Sphere)
    {
        // Ruptured fuel storage flashes into a fireball before the shell folds.
        for(int32 I=0;I<22;++I)
            SpawnParticle(2,B.Center+FMath::VRand()*B.Extent.X*.6f,FMath::VRand()*900+FVector(0,0,700),FVector(FMath::FRandRange(3.f,6.f)),FMath::FRandRange(.9f,1.7f),-250,.8f,1,FLinearColor(1,FMath::FRandRange(.3f,.6f),.08f));
        for(int32 I=0;I<6;++I) SmokePuff(B.Center+FVector(0,0,B.Height*.5f),2.2f,.95f);
    }
    if(auto* P=Pilot())
    {
        const float Distance=FVector::Dist2D(P->GetActorLocation(),B.Base);
        if(Distance<9000) P->LandingKick=FMath::Max(P->LandingKick,1.6f*(1-Distance/9000));
    }
}

void AEvaGameMode::CollapseRubble(FEvaBuilding& B)
{
    ParticleMesh(0);
    const int32 Index=int32(&B-Buildings.GetData());
    FRandomStream Rand(Index*131+7);
    const FVector2D E=B.Extent;
    const bool bRound=IsRound(B);
    const float Big=FMath::Max(E.X,E.Y), Pile=PileHeight(B), Drop=RubbleDrop(B), Scale=FMath::Clamp(B.Height/4000,.5f,1.3f);
    const FLinearColor Face=B.FacadeColor, Concrete(.26f,.26f,.24f), Rust(.22f,.085f,.04f);
    B.Rubble.Reset();
    auto Piece=[&](int32 Batch,FVector P,FVector Size,FRotator R,FLinearColor Tint)
    {
        FEvaRubblePiece Out; Out.Batch=Batch; Out.Pose=FTransform(R,P,Size);
        FTransform Start=Out.Pose;
        if(Batch!=3) Start.AddToTranslation(FVector(0,0,-Drop));
        if(Batch==4) Start.SetScale3D(FVector(.001f));
        Out.Instance=RubbleBatches[Batch]->AddInstance(Start);
        SetTint(RubbleBatches[Batch],Out.Instance,Tint);
        B.Rubble.Add(Out);
    };
    // The torn lower floors stay standing inside the footprint and are revealed as the frame sinks.
    if(!bRound)
    {
        const float Stump=B.Kind==EEvaArch::Hall ? B.Height*.42f : B.Kind==EEvaArch::Slab ? B.Height*.2f : FMath::Clamp(B.Height*.13f,280.f,1000.f);
        Piece(3,B.Base+FVector(0,0,Stump*.5f),FVector(E.X*.0184f,E.Y*.0184f,Stump/100),FRotator(0,Rand.FRandRange(-2.f,2.f),0),Face*1.4f);
    }
    // A mound of broken concrete, heavier on the side where the crown came down.
    const int32 Chunks=FMath::Clamp(int32(B.Height/75),24,80);
    for(int32 I=0;I<Chunks;++I)
    {
        const float U=Rand.FRandRange(-1.f,1.f), V=Rand.FRandRange(-1.f,1.f), Radial=FMath::Min(1.f,FVector2D(U,V).Size());
        FVector P=B.Base+FVector(U*E.X*1.2f,V*E.Y*1.2f,0)+B.FallDirection*Rand.FRandRange(0.f,Big*.45f);
        const float Size=Rand.FRandRange(1.8f,6.f)*Scale;
        P.Z=Pile*(1-Radial*Radial)*Rand.FRandRange(.45f,1.f)+Size*22;
        Piece(0,P,FVector(Size,Size*Rand.FRandRange(.7f,1.2f),Size*Rand.FRandRange(.6f,1.f)),FRotator(Rand.FRandRange(-40.f,40.f),Rand.FRandRange(-180.f,180.f),Rand.FRandRange(-40.f,40.f)),(Rand.FRand()<.6f ? Face:Concrete)*Rand.FRandRange(.5f,1.05f));
    }
    // Floor plates tilt against the pile with reinforcement tearing out of their edges.
    for(int32 I=0;I<Chunks/3;++I)
    {
        FVector P=B.Base+FVector(Rand.FRandRange(-1.f,1.f)*E.X,Rand.FRandRange(-1.f,1.f)*E.Y,0)+B.FallDirection*Rand.FRandRange(.1f,1.2f)*Big*.55f;
        P.Z=Pile*Rand.FRandRange(.25f,.85f);
        const float Size=Rand.FRandRange(6.f,12.f)*Scale;
        const FRotator R(Rand.FRandRange(-38.f,38.f),Rand.FRandRange(-180.f,180.f),Rand.FRandRange(-30.f,30.f));
        Piece(1,P,FVector(Size,Size*Rand.FRandRange(.6f,1.f),Rand.FRandRange(1.3f,2.2f)),R,(bRound ? Face:Concrete)*Rand.FRandRange(.8f,1.2f));
        for(int32 Bar=0;Bar<2;++Bar)
            Piece(2,P+R.RotateVector(FVector(Rand.FRandRange(-1.f,1.f)*Size*40,Rand.FRandRange(-1.f,1.f)*Size*40,25)),FVector(.12f,.12f,Rand.FRandRange(3.f,7.f)),
                FRotator(Rand.FRandRange(-65.f,65.f),Rand.FRandRange(-180.f,180.f),Rand.FRandRange(-65.f,65.f)),Rust*Rand.FRandRange(.8f,1.3f));
    }
    // A tall crown that topples leaves a trail of wreckage along its fall line.
    const float Throw=B.Crown ? (B.Height-B.Split)*FMath::Sin(FMath::DegreesToRadians(B.MaxTilt)):0;
    if(Throw>Big*.8f)
    {
        const FVector Side(-B.FallDirection.Y,B.FallDirection.X,0);
        const int32 Trail=FMath::Clamp(int32(Throw/260),4,26);
        for(int32 I=0;I<Trail;++I)
        {
            const float T=(I+.5f)/Trail, Size=Rand.FRandRange(1.4f,3.6f)*Scale;
            FVector P=B.Base+B.FallDirection*(Big*.6f+Throw*T)+Side*Rand.FRandRange(-1.f,1.f)*Big*.45f;
            P.Z=Size*22+Rand.FRandRange(0.f,Pile*.4f)*(1-T);
            Piece(0,P,FVector(Size),FRotator(Rand.FRandRange(-40.f,40.f),Rand.FRandRange(-180.f,180.f),0),Face*Rand.FRandRange(.5f,1.f));
        }
        for(int32 I=0;I<4;++I) BreakPropsNear(B.Base+B.FallDirection*(Big*.6f+Throw*(I+.5f)/4),Big*.7f);
    }
    BreakPropsNear(B.Base,Big*1.35f);
    for(int32 I=0;I<(bRound ? 12:8);++I)
        Piece(4,B.Base+FVector(Rand.FRandRange(-.8f,.8f)*E.X,Rand.FRandRange(-.8f,.8f)*E.Y,Pile*Rand.FRandRange(.15f,.7f)),FVector(Rand.FRandRange(.9f,2.4f)),FRotator::ZeroRotator,FLinearColor(1,Rand.FRandRange(.22f,.42f),.05f));
}

void AEvaGameMode::TickCollapse(FEvaBuilding& B,float Dt)
{
    const float Duration=B.CollapseDuration();
    B.CollapseTime=FMath::Min(Duration,B.CollapseTime+Dt);
    const float T=B.CollapseTime/Duration;
    // Gravity-shaped descent after a brief shudder; chimneys fold rather than sinking out of sight.
    const float Fall=FMath::Clamp((T-.1f)/.9f,0.f,1.f);
    const float Sink=Fall*Fall*(B.Kind==EEvaArch::Stack ? B.Split*.85f:B.Height+300);
    const FVector Side=FVector::CrossProduct(FVector::UpVector,B.FallDirection).GetSafeNormal();
    const float Shudder=T<.2f ? FMath::Sin(B.CollapseTime*75)*30*(1-T/.2f):0;
    B.DistrictRoot->SetWorldLocationAndRotation(B.Base+Side*Shudder-FVector(0,0,Sink),FQuat(Side,FMath::DegreesToRadians(FMath::Min(B.MaxTilt,8.f)*.3f*Fall)));
    if(B.Crown)
    {
        // The crown hinges on the edge away from the blow, then rides the failing base down.
        const float Tip=FMath::Pow(FMath::Clamp((T-.04f)/.82f,0.f,1.f),1.7f);
        const FQuat Hinge(Side,FMath::DegreesToRadians(B.MaxTilt*Tip));
        const FVector2D F(B.FallDirection.X,B.FallDirection.Y);
        float Reach=B.Extent.X;
        if(!IsRound(B)) Reach=FMath::Min(FMath::Abs(F.X)>1e-4f ? B.Extent.X/FMath::Abs(F.X):1e8f,FMath::Abs(F.Y)>1e-4f ? B.Extent.Y/FMath::Abs(F.Y):1e8f);
        const FVector Edge(F.X*Reach,F.Y*Reach,B.Split), Origin(0,0,B.Split);
        B.Crown->SetRelativeLocationAndRotation(Edge+Hinge.RotateVector(Origin-Edge)-FVector(0,0,Sink*.15f),Hinge);
    }
    const float Rise=FMath::SmoothStep(.12f,.95f,T), Drop=RubbleDrop(B);
    for(const auto& Piece:B.Rubble)
    {
        if(Piece.Batch==3) continue;
        FTransform Pose=Piece.Pose;
        if(Piece.Batch==4) Pose.SetScale3D(Pose.GetScale3D()*FMath::Clamp((T-.75f)/.25f,.001f,1.f));
        else Pose.AddToTranslation(FVector(0,0,-(1-Rise)*Drop));
        RubbleBatches[Piece.Batch]->UpdateInstanceTransform(Piece.Instance,Pose,false,false,true);
    }
    // A ground-hugging dust wave rolls out while a column sheds from the crumbling top.
    const float Size=DustScale(B);
    const FVector Top=B.Base+FVector(0,0,FMath::Max(250.f,B.Height-Sink));
    B.SmokeClock-=Dt;
    for(int32 Burst=0;B.SmokeClock<=0 && Burst<4 && T<1;++Burst)
    {
        B.SmokeClock+=.06f;
        const float Angle=FMath::FRandRange(0.f,2*PI);
        const FVector Dir(FMath::Cos(Angle),FMath::Sin(Angle),0);
        SpawnParticle(1,B.Base+FVector(Dir.X*B.Extent.X,Dir.Y*B.Extent.Y,260),Dir*FMath::FRandRange(900.f,2000.f)*Size+FVector(0,0,FMath::FRandRange(0.f,260.f)),FVector(FMath::FRandRange(8.f,13.f)*Size),FMath::FRandRange(3.8f,5.5f),0,.5f,1,FLinearColor::White,2.8f);
        SpawnParticle(1,Top+FVector(FMath::FRandRange(-1.f,1.f)*B.Extent.X,FMath::FRandRange(-1.f,1.f)*B.Extent.Y,0),FVector(FMath::FRandRange(-300.f,300.f),FMath::FRandRange(-300.f,300.f),FMath::FRandRange(150.f,600.f)),FVector(FMath::FRandRange(6.f,10.f)*Size),FMath::FRandRange(2.8f,4.4f),0,.42f,.85f,FLinearColor::White,2.4f);
        if(T<.85f) SpawnParticle(0,Top+FVector(Dir.X*B.Extent.X,Dir.Y*B.Extent.Y,-FMath::FRandRange(0.f,400.f)),Dir*FMath::FRandRange(300.f,900.f)+FVector(0,0,FMath::FRandRange(100.f,500.f)),FVector(FMath::FRandRange(.8f,2.6f)),FMath::FRandRange(2.f,3.2f),2600,0,1,B.FacadeColor*FMath::FRandRange(.5f,1.f));
    }
    if(B.CollapseTime>=Duration)
    {
        B.DistrictRoot->SetVisibility(false,true);
        B.Smolder=24; B.SmokeClock=.5f;
        B.SmokePoint=B.Base+FVector(0,0,PileHeight(B));
        DistrictDust(FVector(B.Base.X,B.Base.Y,0),Size*1.3f);
        if(CrumbleClock<=0) { DistrictSound(TEXT("Crumble"),B.Base,.7f); CrumbleClock=.3f; }
    }
}

void AEvaGameMode::BreakProp(int32 Index,FVector From)
{
    auto& Prop=Props[Index];
    if(Prop.bBroken) return;
    Prop.bBroken=true; ++PropsBroken;
    FVector Away=(Prop.Position-From).GetSafeNormal2D();
    if(Away.IsNearlyZero()) Away=FVector(1,0,0);
    const FVector Axis=FVector::CrossProduct(FVector::UpVector,Away);
    FRandomStream Rand(Index*31+PropsBroken);
    const FQuat Tip(Axis,FMath::DegreesToRadians(Rand.FRandRange(74.f,88.f)));
    for(auto& Part:Prop.Parts)
    {
        FTransform Pose=Part.Rest;
        FVector Location=Pose.GetLocation(), Scale=Pose.GetScale3D();
        switch(Part.Mode)
        {
        case EEvaBreak::Flatten:
            // Crushed flat and spread slightly under the foot.
            Location.Z=Prop.Position.Z+(Location.Z-Prop.Position.Z)*.18f;
            Scale*=FVector(1.1f,1.1f,.18f);
            Pose.SetRotation(FQuat(FVector::UpVector,Rand.FRandRange(-.25f,.25f))*Pose.GetRotation());
            break;
        case EEvaBreak::Topple:
            Location=Prop.Position+Tip.RotateVector(Location-Prop.Position);
            Pose.SetRotation(Tip*Pose.GetRotation());
            break;
        case EEvaBreak::Hide:
            Scale=FVector(.001f); Location.Z-=4000;
            break;
        case EEvaBreak::Drop:
            Location=FVector(Location.X,Location.Y,Prop.Position.Z+70)+Away*120;
            Scale.Z*=.55f;
            Pose.SetRotation(FQuat(Axis,FMath::DegreesToRadians(14))*Pose.GetRotation());
            break;
        case EEvaBreak::Scatter:
        {
            const FVector Push=Away*Rand.FRandRange(150.f,700.f)+FVector(Rand.FRandRange(-250.f,250.f),Rand.FRandRange(-250.f,250.f),0);
            Location=FVector(Location.X+Push.X,Location.Y+Push.Y,Prop.Position.Z+Scale.Z*50+Rand.FRandRange(0.f,Scale.Z*50));
            Pose.SetRotation(FQuat(FVector::UpVector,Rand.FRandRange(-.9f,.9f))*FQuat(Away,Rand.FRandRange(-.18f,.18f))*Pose.GetRotation());
            break;
        }
        }
        Pose.SetLocation(Location); Pose.SetScale3D(Scale);
        Part.Batch->UpdateInstanceTransform(Part.Instance,Pose,false,false,true);
    }
    SpawnParticle(1,Prop.Position+FVector(0,0,150),FVector(0,0,120)+Away*300,FVector(FMath::Clamp(Prop.Radius/120,1.5f,4.f)),2.2f,0,.5f,1);
    DebrisBurst(Prop.Position+FVector(0,0,200),Away,Prop.Radius>200 ? 5:2,600,FLinearColor(.3f,.3f,.28f));
    if(CrumbleClock<=0) { DistrictSound(TEXT("Crumble"),Prop.Position,.3f); CrumbleClock=.25f; }
}

void AEvaGameMode::BreakPropsNear(FVector Position,float Radius)
{
    const int32 X0=FMath::FloorToInt((Position.X-Radius-800)/2000), X1=FMath::FloorToInt((Position.X+Radius+800)/2000);
    const int32 Y0=FMath::FloorToInt((Position.Y-Radius-800)/2000), Y1=FMath::FloorToInt((Position.Y+Radius+800)/2000);
    for(int32 X=X0;X<=X1;++X) for(int32 Y=Y0;Y<=Y1;++Y) if(const auto* Cell=PropGrid.Find(FIntPoint(X,Y)))
        for(int32 Index:*Cell) if(!Props[Index].bBroken && FVector::Dist2D(Props[Index].Position,Position)<Radius+Props[Index].Radius) BreakProp(Index,Position);
}

// Instance transform and custom-data edits are streamed incrementally by the engine's instance data manager,
// so pools and rubble never force a full render-state rebuild.
void AEvaGameMode::TickDestruction(float Dt)
{
    CityClock+=Dt; CrumbleClock=FMath::Max(0.f,CrumbleClock-Dt);
    if(AviationMaterial) AviationMaterial->SetScalarParameterValue(TEXT("Glow"),FMath::Fmod(CityClock,1.6f)<.3f ? 7.f:.3f);
    auto* P=Pilot();
    const FVector Viewer=P ? P->GetActorLocation():FVector::ZeroVector;
    for(auto& B:Buildings)
    {
        if(B.bDestroyed && B.CollapseTime<B.CollapseDuration()) { TickCollapse(B,Dt); continue; }
        const bool bBurning=B.DamageStage==1 && !B.bDestroyed;
        if(B.Smolder>0) B.Smolder=FMath::Max(0.f,B.Smolder-Dt);
        if((!bBurning && B.Smolder<=0) || FVector::DistSquared2D(B.Base,Viewer)>FMath::Square(34000.f)) continue;
        B.SmokeClock-=Dt;
        if(B.SmokeClock>0) continue;
        // Damaged structures burn and shed debris; collapsed lots smoulder for a while.
        B.SmokeClock=bBurning ? .3f:.55f;
        SmokePuff(B.SmokePoint,bBurning ? 1.1f:1.4f,bBurning ? .78f:.55f);
        if(bBurning && FMath::FRand()<.3f) SpawnParticle(0,B.SmokePoint,FVector(FMath::FRandRange(-200.f,200.f),FMath::FRandRange(-200.f,200.f),0),FVector(FMath::FRandRange(.6f,1.6f)),3,2600,0,1,B.FacadeColor);
        if(FMath::FRand()<.55f) SpawnParticle(2,B.SmokePoint+FMath::VRand()*140,FVector(FMath::FRandRange(-40.f,40.f),FMath::FRandRange(-40.f,40.f),FMath::FRandRange(80.f,260.f)),FVector(FMath::FRandRange(.4f,.9f)),FMath::FRandRange(.6f,1.1f),-100,.2f,1,FLinearColor(1,FMath::FRandRange(.3f,.5f),.08f));
    }
    // Chimneys and cooling towers keep working while they stand.
    SteamClock-=Dt;
    if(SteamClock<=0)
    {
        SteamClock=.3f;
        for(int32 Index:SteamVents)
        {
            const auto& B=Buildings[Index];
            if(B.bDestroyed || FMath::FRand()<.45f || FVector::DistSquared2D(B.Base,Viewer)>FMath::Square(30000.f)) continue;
            const bool bSteam=B.Kind==EEvaArch::Cooling;
            SpawnParticle(1,B.Base+FVector(FMath::FRandRange(-150.f,150.f),FMath::FRandRange(-150.f,150.f),B.Height+(bSteam ? -250:150)),
                FVector(FMath::FRandRange(80.f,220.f),FMath::FRandRange(30.f,120.f),FMath::FRandRange(260.f,480.f)),FVector(FMath::FRandRange(4.f,6.f)*(bSteam ? 1.5f:.8f)),FMath::FRandRange(5.f,7.f),0,.5f,bSteam ? 2.4f:1.1f);
        }
    }
    // Street details give way under Eva footsteps.
    auto Crush=[&](const FVector& Feet)
    {
        if(Feet.Z>1500) return;
        const FIntPoint Cell(FMath::FloorToInt(Feet.X/2000),FMath::FloorToInt(Feet.Y/2000));
        for(int32 X=-1;X<=1;++X) for(int32 Y=-1;Y<=1;++Y) if(const auto* List=PropGrid.Find(Cell+FIntPoint(X,Y)))
            for(int32 Index:*List) if(!Props[Index].bBroken && FVector::Dist2D(Props[Index].Position,Feet)<Props[Index].Radius+240) BreakProp(Index,Feet);
    };
    if(P && !P->bHuman && Chapter==EEvaChapter::Battle) Crush(Viewer);
    if(Wingman && !Wingman->IsHidden()) Crush(Wingman->GetActorLocation());
    for(auto& Pool:ParticleBatches)
    {
        if(!Pool.Mesh || Pool.Live.Num()==0) continue;
        for(int32 I=Pool.Live.Num()-1;I>=0;--I)
        {
            auto& Q=Pool.Live[I];
            Q.Age+=Dt;
            if(Q.Age>=Q.Life)
            {
                Pool.Mesh->UpdateInstanceTransform(Q.Slot,Parked,false,false,true);
                if(Pool.bFades) Pool.Mesh->SetCustomDataValue(Q.Slot,0,0);
                Pool.Free.Add(Q.Slot); Pool.Live.RemoveAtSwap(I);
                continue;
            }
            Q.Velocity.Z-=Q.Gravity*Dt; Q.Velocity*=FMath::Exp(-Q.Drag*Dt); Q.Position+=Q.Velocity*Dt;
            const float Floor=Q.Scale.Z*34;
            if(Q.Gravity>0 && Q.Position.Z<Floor)
            {
                Q.Position.Z=Floor; Q.Velocity.Z=FMath::Abs(Q.Velocity.Z)*.2f;
                Q.Velocity.X*=.5f; Q.Velocity.Y*=.5f; Q.Spin*=.45f;
            }
            Q.Rotation+=Q.Spin*Dt;
            const float T=Q.Age/Q.Life;
            const float Size=(1+Q.Growth*Q.Age)*(Pool.bFades ? 1.f:FMath::Min(1.f,(1-T)*5));
            Pool.Mesh->UpdateInstanceTransform(Q.Slot,FTransform(Q.Rotation,Q.Position,Q.Scale*Size),false,false,true);
            if(Pool.bFades) Pool.Mesh->SetCustomDataValue(Q.Slot,0,Q.Alpha*FMath::Min(1.f,T*7)*FMath::Pow(1-T,1.4f));
        }
    }
}

void AEvaGameMode::ResetCity()
{
    for(auto& B:Buildings)
    {
        if(!B.DistrictRoot) continue;
        B.bDestroyed=false; B.DamageStage=0; B.CollapseTime=0; B.Smolder=0; B.SmokeClock=0; B.Rubble.Reset();
        B.DistrictRoot->SetWorldLocationAndRotation(B.Base,FQuat::Identity);
        B.DistrictRoot->SetWorldScale3D(FVector(1));
        if(B.Crown) B.Crown->SetRelativeLocationAndRotation(FVector(0,0,B.Split),FQuat::Identity);
        B.DistrictRoot->SetVisibility(true,true);
        B.Mesh->SetMaterial(0,B.Skin[0]); B.Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        if(B.CrownMesh) { B.CrownMesh->SetMaterial(0,B.Skin[1]); B.CrownMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }
        for(auto* Scar:B.Scars) if(Scar) Scar->ClearInstances();
    }
    for(auto* Batch:RubbleBatches) Batch->ClearInstances();
    for(auto& Pool:ParticleBatches)
    {
        if(!Pool.Mesh) continue;
        for(const auto& Q:Pool.Live)
        {
            Pool.Mesh->UpdateInstanceTransform(Q.Slot,Parked,false,false,true);
            if(Pool.bFades) Pool.Mesh->SetCustomDataValue(Q.Slot,0,0);
            Pool.Free.Add(Q.Slot);
        }
        Pool.Live.Reset();
    }
    for(auto& Prop:Props) if(Prop.bBroken)
    {
        Prop.bBroken=false;
        for(auto& Part:Prop.Parts) Part.Batch->UpdateInstanceTransform(Part.Instance,Part.Rest,false,false,true);
    }
    PropsBroken=0; BuildingsLost=0;
}
