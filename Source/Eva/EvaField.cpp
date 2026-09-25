#include "EvaGame.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    const FTransform Parked(FRotator::ZeroRotator,FVector(0,0,-60000),FVector(.001f));
    constexpr int32 RippleCapacity=120;
    constexpr int32 FieldRingCount=6;
    constexpr float FieldRadius=950;
    // The ring mesh is an octagon of radius 50 in the YZ plane; scale Y and Z to set its radius.
    FTransform RingPose(FVector Center,FQuat Rotation,float Radius) { return FTransform(Rotation,Center,FVector(1,Radius/50,Radius/50)); }
}

UMaterialInstanceDynamic* AEvaGameMode::FieldMaterial(FLinearColor Color)
{
    auto* Material=UMaterialInstanceDynamic::Create(FieldSurface ? FieldSurface:Surface,this);
    Material->SetVectorParameterValue(TEXT("Color"),Color);
    Material->SetScalarParameterValue(TEXT("Intensity"),3);
    Material->SetScalarParameterValue(TEXT("Glow"),3);
    return Material;
}

USceneComponent* AEvaGameMode::BuildField(FLinearColor Color,USceneComponent* Parent)
{
    // An A.T. field reads as concentric octagons: a steady rim with rings flowing outward from its centre.
    auto* Root=NewObject<USceneComponent>(Parent->GetOwner());
    Root->SetupAttachment(Parent); Root->RegisterComponent();
    auto* Rings=NewObject<UInstancedStaticMeshComponent>(Parent->GetOwner());
    Rings->SetupAttachment(Root);
    Rings->SetMobility(EComponentMobility::Movable);
    Rings->SetStaticMesh(KitMesh(TEXT("OctagonRing")));
    Rings->SetMaterial(0,FieldMaterial(Color));
    Rings->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rings->SetCastShadow(false);
    Rings->SetNumCustomDataFloats(4);
    Rings->RegisterComponent();
    for(int32 I=0;I<FieldRingCount;++I)
    {
        const int32 Index=Rings->AddInstance(RingPose(FVector::ZeroVector,FQuat::Identity,FieldRadius*(I+1)/FieldRingCount));
        const float Data[4]={1,1,1,.6f};
        Rings->SetCustomData(Index,MakeArrayView(Data,4));
    }
    Meshes.Add(Rings);
    FEvaFieldRings Field; Field.Root=Root; Field.Rings=Rings; Field.Color=Color;
    FieldRings.Add(Field);
    return Root;
}

void AEvaGameMode::BuildFieldEffects()
{
    // Every ripple, shard and flash shares one pooled batch; spawns beyond its capacity are skipped.
    if(RippleBatch) return;
    RippleBatch=CityInstances(nullptr,FLinearColor::White,0,0,TEXT("OctagonRing"));
    RippleBatch->SetMaterial(0,FieldMaterial(FLinearColor::White));
    RippleBatch->SetNumCustomDataFloats(4);
    RippleBatch->SetCullDistances(0,0);
    TArray<FTransform> Hidden; Hidden.Init(Parked,RippleCapacity);
    RippleBatch->AddInstances(Hidden,false);
    for(int32 Slot=RippleCapacity-1;Slot>=0;--Slot) RippleFree.Add(Slot);
}

void AEvaGameMode::SpawnRipple(FVector Center,FQuat Rotation,FLinearColor Color,float From,float To,float Life,float Delay,float Peak,FVector Drift)
{
    BuildFieldEffects();
    if(RippleFree.Num()==0) return;
    FEvaRipple Ripple;
    Ripple.Slot=RippleFree.Pop(); Ripple.Center=Center; Ripple.Rotation=Rotation; Ripple.From=From; Ripple.To=To;
    Ripple.Life=Life; Ripple.Delay=Delay; Ripple.Peak=Peak; Ripple.Drift=Drift;
    const float Data[4]={Color.R,Color.G,Color.B,0};
    RippleBatch->SetCustomData(Ripple.Slot,MakeArrayView(Data,4));
    Ripples.Add(Ripple);
}

FLinearColor AEvaGameMode::FieldColor(const USceneComponent* Field) const
{
    for(const auto& Rings:FieldRings) if(Rings.Root==Field) return Rings.Color;
    return FLinearColor(1,.3f,.06f);
}

FVector AEvaGameMode::FieldImpact(USceneComponent* Field,FVector From,FVector To,float Strength)
{
    if(!Field) return To;
    // Find where the strike meets the field plane, keep it inside the octagon, and ripple from there.
    const FTransform T=Field->GetComponentTransform();
    const FVector Normal=T.GetUnitAxis(EAxis::X);
    FVector Point=To;
    const float Denominator=FVector::DotProduct(To-From,Normal);
    if(FMath::Abs(Denominator)>1e-3f) Point=From+(To-From)*FMath::Clamp(FVector::DotProduct(T.GetLocation()-From,Normal)/Denominator,0.f,1.f);
    FVector Local=T.InverseTransformPosition(Point);
    Local.X=0;
    const float Reach=FVector2D(Local.Y,Local.Z).Size();
    if(Reach>FieldRadius*.82f) Local*=FieldRadius*.82f/Reach;
    Point=T.TransformPosition(Local);
    const float Size=FieldRadius*T.GetScale3D().Y;
    const FLinearColor Color=FieldColor(Field);
    const FQuat Rotation=T.GetRotation();
    for(int32 Wave=0;Wave<4;++Wave)
        SpawnRipple(Point,Rotation,Color,50,Size*(.45f+.2f*Wave)*FMath::Clamp(Strength,.6f,2.f),.5f+.08f*Wave,Wave*.07f,(2.6f-.4f*Wave)*FMath::Clamp(Strength,.7f,1.6f));
    // A hot, tight octagon marks the exact point of contact.
    SpawnRipple(Point,Rotation,FLinearColor(1,.8f,.45f),90,Size*.22f,.24f,0,6.5f);
    return Point;
}

void AEvaGameMode::FieldBurst(USceneComponent* Field,int32 Kind,FVector Point)
{
    if(!Field) return;
    ++FieldBursts[FMath::Clamp(Kind,0,2)];
    const FTransform T=Field->GetComponentTransform();
    const FQuat Rotation=T.GetRotation();
    const FVector Center=T.GetLocation(), Normal=T.GetUnitAxis(EAxis::X);
    const float Size=FieldRadius*T.GetScale3D().Y;
    const FLinearColor Color=FieldColor(Field);
    if(Kind==0)
    {
        // Broken: the octagons blow outward from the breach and the field sheds glowing fragments.
        for(int32 Wave=0;Wave<6;++Wave) SpawnRipple(Point,Rotation,Color,Size*.2f,Size*(1.5f+.25f*Wave),.42f+.05f*Wave,Wave*.035f,3.2f-.3f*Wave);
        for(int32 Shard=0;Shard<12;++Shard)
        {
            const float Angle=Shard*2*PI/12+FMath::FRandRange(-.2f,.2f);
            const FVector Offset=T.TransformVectorNoScale(FVector(0,FMath::Cos(Angle),FMath::Sin(Angle)))*Size*FMath::FRandRange(.25f,.85f);
            const float Piece=FMath::FRandRange(90.f,240.f);
            SpawnRipple(Center+Offset,Rotation,Color,Piece,Piece*.6f,FMath::FRandRange(.6f,1.f),.03f,2.6f,Offset.GetSafeNormal()*FMath::FRandRange(600.f,1300.f)+Normal*FMath::FRandRange(-400.f,400.f));
        }
    }
    else if(Kind==1)
    {
        // Lowered by the Angel itself: the field thins and drifts apart.
        for(int32 Wave=0;Wave<3;++Wave) SpawnRipple(Center,Rotation,Color,Size*(.9f-.25f*Wave),Size*(1.3f-.2f*Wave),.6f,Wave*.08f,1.3f);
    }
    else
    {
        // Regenerated: rings fold back inward and settle into the barrier.
        for(int32 Wave=0;Wave<4;++Wave) SpawnRipple(Center,Rotation,Color,Size*1.9f,Size*(.95f-.18f*Wave),.45f,Wave*.07f,2.2f);
    }
}

void AEvaGameMode::GuardRipple(FVector Source)
{
    if(!PlayerShield) return;
    ++GuardRipples;
    FieldImpact(PlayerShield,Source,PlayerShield->GetComponentLocation(),1.2f);
}

void AEvaGameMode::TickFields(float Dt)
{
    FieldClock+=Dt;
    for(auto& Field:FieldRings)
    {
        if(!Field.Root->IsVisible()) continue;
        for(int32 I=0;I<FieldRingCount;++I)
        {
            // The last ring is the steady rim; the others keep flowing out from the centre and fade at the edge.
            const bool bRim=I==FieldRingCount-1;
            const float Phase=FMath::Frac(FieldClock*.42f+I/float(FieldRingCount-1));
            const float Radius=bRim ? FieldRadius:FieldRadius*(.16f+.8f*Phase);
            // Kept dim at rest, as in the show, so contact ripples clearly outshine the standing field.
            const float Glow=bRim ? .45f+.12f*FMath::Sin(FieldClock*6.5f):.38f*FMath::Sin(PI*Phase);
            Field.Rings->UpdateInstanceTransform(I,RingPose(FVector::ZeroVector,FQuat::Identity,Radius),false,false,true);
            Field.Rings->SetCustomDataValue(I,3,Glow);
        }
    }
    for(int32 I=Ripples.Num()-1;I>=0;--I)
    {
        auto& Ripple=Ripples[I];
        Ripple.Age+=Dt;
        const float T=(Ripple.Age-Ripple.Delay)/Ripple.Life;
        if(T>=1)
        {
            RippleBatch->UpdateInstanceTransform(Ripple.Slot,Parked,false,false,true);
            RippleBatch->SetCustomDataValue(Ripple.Slot,3,0);
            RippleFree.Add(Ripple.Slot); Ripples.RemoveAtSwap(I);
            continue;
        }
        if(T<0) continue;
        const float Radius=FMath::Lerp(Ripple.From,Ripple.To,1-FMath::Square(1-T));
        RippleBatch->UpdateInstanceTransform(Ripple.Slot,RingPose(Ripple.Center+Ripple.Drift*(Ripple.Age-Ripple.Delay),Ripple.Rotation,Radius),false,false,true);
        RippleBatch->SetCustomDataValue(Ripple.Slot,3,Ripple.Peak*FMath::Min(1.f,T*10)*FMath::Pow(1-T,1.3f));
    }
    // Fields that an Angel lowers or regenerates on its own dissolve or re-form visibly; breaks are handled at the hit.
    if(EnemyShield && AngelRoot && AngelRoot->IsVisible() && HasCombatTarget() && EnemyShield==WatchedShield)
    {
        if(WatchedField>0 && Rules.EnemyField<=0) FieldBurst(EnemyShield,1,EnemyShield->GetComponentLocation());
        else if(WatchedField<=0 && Rules.EnemyField>0) FieldBurst(EnemyShield,2,EnemyShield->GetComponentLocation());
    }
    WatchedShield=EnemyShield; WatchedField=Rules.EnemyField;
}
