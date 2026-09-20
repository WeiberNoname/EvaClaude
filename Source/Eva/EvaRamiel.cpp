#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void AEvaGameMode::ClearAngelHazards()
{
    for(auto* Lane:WhipTelegraphs) Lane->SetVisibility(false);
    for(auto* Lane:BeamWarnings) Lane->SetVisibility(false);
    for(auto* Beam:BeamShots) Beam->SetVisibility(false);
    if(ThreatRing) ThreatRing->SetVisibility(false);
    BeamTime=0;
}

void AEvaGameMode::BuildRamiel()
{
    RamielRoot=NewSceneRoot(FVector::ZeroVector);
    RamielCrystal=Shape("RamielCrystal",FVector::ZeroVector,FVector(27,27,27),FLinearColor(.035f,.2f,.75f),.18f,RamielRoot);
    auto* CrystalMat=UMaterialInstanceDynamic::Create(Surface,this);
    CrystalMat->SetVectorParameterValue("Color",FLinearColor(.025f,.18f,.85f));
    CrystalMat->SetScalarParameterValue("Glow",.45f);
    CrystalMat->SetScalarParameterValue("Roughness",.24f); CrystalMat->SetScalarParameterValue("Metallic",.35f);
    RamielCrystal->SetMaterial(0,CrystalMat);
    for(int I=0;I<4;++I)
    {
        float A=I*PI*.5f;
        for(float Z:{-1890.f,1890.f})
        {
            FVector Top(0,0,Z), Corner(FMath::Cos(A)*1350,FMath::Sin(A)*1350,0);
            auto* Edge=Shape("Cylinder",(Top+Corner)*.5f,FVector(.09f,.09f,(Top-Corner).Size()/100),FLinearColor(.08f,.55f,1),1.1f,RamielRoot);
            Edge->SetRelativeRotation(FRotationMatrix::MakeFromZ(Top-Corner).Rotator());
        }
        auto* Petal=Shape("RamielCrystal",FVector(FMath::Cos(A)*600,FMath::Sin(A)*600,0),FVector(4,4,12),FLinearColor(.09f,.4f,.95f),.28f,RamielRoot);
        RamielPetals.Add(Petal);
    }
    RamielCore=Shape("Sphere",FVector(0,-1300,0),FVector(6),FLinearColor(.95f,.02f,.12f),1.7f,RamielRoot);
    RamielField=BuildField(FLinearColor(.1f,.6f,1),RamielRoot);
    RamielField->SetRelativeScale3D(FVector(1.4f));
    for(int I=0;I<3;++I)
    {
        BeamWarnings.Add(Shape("Cube",FVector::ZeroVector,FVector(1),FLinearColor(1,.1f,.045f),1.3f));
        BeamShots.Add(Shape("Cylinder",FVector::ZeroVector,FVector(1),FLinearColor(1,.52f,.26f),5));
        BeamShots.Last()->SetCastShadow(false);
    }
    RamielRoot->SetVisibility(false,true); ClearAngelHazards();
}

void AEvaGameMode::TickRamiel(float Dt)
{
    auto* P=Pilot(); if(!P) return;
    const bool Enraged=Rules.EnemyHealth<EnemyMaxHealth*.5f;
    const FVector Pos=P->GetActorLocation()+FVector(0,0,180);
    EnemyPosition.Z=3400+FMath::Sin(MissionTime*.9f)*120;
    AngelRoot->SetWorldLocation(EnemyPosition);
    AngelRoot->SetWorldRotation(FRotator(0,MissionTime*16,0));
    FVector Facing=(Pos-EnemyPosition).GetSafeNormal();
    RamielCore->SetWorldLocation(EnemyPosition+Facing*1400);
    RamielField->SetWorldLocation(EnemyPosition+Facing*1550);
    RamielField->SetWorldRotation(Facing.Rotation());
    if(Rules.EnemyField<=0)
    {
        VulnerableTime-=Dt;
        if(VulnerableTime<=0) Rules.EnemyField=100;
    }
    const bool Open=Rules.EnemyField<=0;
    RamielCore->SetVisibility(Open);
    RamielField->SetVisibility(!Open,true);
    const float OpenScale=Open ? .64f:1.f;
    RamielCrystal->SetRelativeScale3D(FMath::Lerp(RamielCrystal->GetRelativeScale3D(),FVector(27*OpenScale),FEvaMotion::Blend(6,Dt)));
    for(int I=0;I<RamielPetals.Num();++I)
    {
        const float Angle=I*PI*.5f;
        const FVector Target(FMath::Cos(Angle)*(Open ? 1700:600),FMath::Sin(Angle)*(Open ? 1700:600),Open ? 350:0);
        RamielPetals[I]->SetRelativeLocation(FMath::Lerp(RamielPetals[I]->GetRelativeLocation(),Target,FEvaMotion::Blend(7,Dt)));
        RamielPetals[I]->SetRelativeRotation(FRotator(Open ? 35:0,I*90,0));
    }
    if(BeamTime>0)
    {
        BeamTime=FMath::Max(0.f,BeamTime-Dt);
        if(BeamTime<=0) for(auto* Beam:BeamShots) Beam->SetVisibility(false);
    }
    EnemyClock-=Dt;
    if(EnemyClock<=0 && Telegraph<=0 && BeamTime<=0)
    {
        ++AttackCount; Telegraph=Enraged ? 1.55f:2.2f; BeamAim=Pos;
        SetNotice(AttackCount%2 ? "RAMIEL // TRACKING BEAM / DASH WHEN THE LINE LOCKS" : "RAMIEL // THREE BEAMS / FIND THE GAP OR USE COVER");
    }
    else if(Telegraph>0)
    {
        const float Before=Telegraph;
        Telegraph=FMath::Max(0.f,Telegraph-Dt);
        // The final 0.65 seconds are locked: no last-frame tracking or unavoidable hits.
        if(Before>.65f) BeamAim=Pos;
        FVector Direction=(BeamAim-EnemyPosition).GetSafeNormal();
        const int Count=AttackCount%2 ? 1:3;
        for(int I=0;I<3;++I)
        {
            auto* Lane=BeamWarnings[I]; Lane->SetVisibility(I<Count);
            if(I>=Count) continue;
            FVector Ray=Direction.RotateAngleAxis(Count==1 ? 0.f:(I-1)*16.f,FVector::UpVector);
            FVector End=EnemyPosition+Ray*20000;
            // Ground ribbon projects the complete lane; the actual beam remains three-dimensional.
            FVector A=EnemyPosition, B=End; A.Z=B.Z=25;
            Lane->SetWorldLocation((A+B)*.5f); Lane->SetWorldRotation((B-A).Rotation());
            Lane->SetWorldScale3D(FVector((B-A).Size()/100,Before>.65f ? 3.f:8.f,.06f));
        }
        if(Telegraph<=0)
        {
            bool Hit=false;
            for(int I=0;I<Count;++I)
            {
                FVector Ray=Direction.RotateAngleAxis(Count==1 ? 0.f:(I-1)*16.f,FVector::UpVector);
                FVector Start=EnemyPosition+Ray*1400, End=EnemyPosition+Ray*20000;
                FHitResult Cover; FCollisionQueryParams Query; Query.AddIgnoredActor(P);
                const bool Blocked=GetWorld()->LineTraceSingleByChannel(Cover,Start,End,ECC_Visibility,Query);
                if(Blocked) End=Cover.ImpactPoint;
                const FVector Closest=FMath::ClosestPointOnSegment(Pos,Start,End);
                Hit |= FVector::DistSquared(Pos,Closest)<FMath::Square(440.f);
                auto* Beam=BeamShots[I]; Beam->SetVisibility(true); Beam->SetWorldLocation((Start+End)*.5f);
                Beam->SetWorldRotation(FRotationMatrix::MakeFromZ(End-Start).Rotator());
                Beam->SetWorldScale3D(FVector(3,3,(End-Start).Size()/100));
                Pulse(End,FLinearColor(1,.35f,.08f),9);
            }
            if(Hit)
            {
                bool Guard=P->bGuard && FVector::DotProduct(P->GetActorForwardVector(),(EnemyPosition-P->GetActorLocation()).GetSafeNormal2D())>.25f;
                Rules.ReceiveHit(Enraged ? 40:32,Guard,P->DodgeTime>0);
                if(P->DodgeTime<=0) HitFlash=.35f;
            }
            for(auto* Lane:BeamWarnings) Lane->SetVisibility(false);
            BeamTime=.45f; Rules.EnemyField=0; VulnerableTime=Enraged ? 3.4f:4.8f; EnemyClock=VulnerableTime+.7f;
            SetNotice("RAMIEL // CORE OPEN / COUNTERATTACK NOW");
        }
    }
}
