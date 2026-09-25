#include "EvaGame.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"

void AEvaGameMode::TickFieldTest(float Dt)
{
    if(bFieldDone) return;
    FieldTestTime+=Dt;
    auto* P=Pilot(); if(!P) return;
    const FVector Base=Districts[0];
    auto Check=[&](bool OK,const TCHAR* Name) { bFieldOK &= OK; UE_LOG(LogTemp,Display,TEXT("EVA_FIELD_CHECK %s=%d"),Name,OK); };
    auto Next=[&]() { ++FieldTestStep; FieldTestTime=0; };
    auto Capture=[&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false); };
    // Film the Angel's field three-quarters on, from beside Unit-01.
    auto Film=[&]() { const FVector Field=EnemyShield->GetComponentLocation(); StoryView(Field+FVector(2300,-3400,-100),Field-FVector(0,0,150),0); };
    auto Shot=[&](float Charge)
    {
        P->Camera->SetWorldRotation((EnemyAimPoint()-P->Camera->GetComponentLocation()).Rotation());
        LanceCooldown=0; P->ReloadTime=0; Attack(true,Charge,true);
        P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
    };
    if(FieldTestStep==0 && FieldTestTime>2.5f)
    {
        SelectedDistrict=0; BeginWorldEncounter(); EnemyClock=60;
        EnemyPosition=Base+FVector(0,10400,1600); AngelRoot->SetWorldLocation(EnemyPosition);
        P->SetActorLocation(Base+FVector(0,8500,780)); P->SetActorRotation(FRotator(0,90,0)); P->CameraYaw=90; P->CameraPitch=0; P->bLock=false;
        if(Wingman) { Wingman->Order=EEvaOrder::Hold; Wingman->SetActorLocation(Base+FVector(-2800,6800,780)); Wingman->HoldPosition=Wingman->GetActorLocation(); }
        bool Octagons=FieldRings.Num()>=4 && KitMesh(TEXT("OctagonRing"))!=KitMesh(TEXT("Cube"));
        for(const auto& Field:FieldRings) Octagons=Octagons && Field.Rings->GetInstanceCount()==6 && Field.Rings->GetStaticMesh()==KitMesh(TEXT("OctagonRing"));
        Check(Octagons && FieldSurface && RippleBatch && RippleBatch->GetInstanceCount()==RippleFree.Num()+Ripples.Num(),TEXT("octagonal_fields_built"));
        UGameplayStatics::GetPlayerController(this,0)->GetHUD()->bShowHUD=false;
        Next();
    }
    else if(FieldTestStep==1 && FieldTestTime>1.2f)
    {
        Film(); Next();
    }
    else if(FieldTestStep==2 && FieldTestTime>1)
    {
        Check(EnemyShield->IsVisible() && Rules.EnemyField==100,TEXT("field_visible_at_rest"));
        Capture(TEXT("ATFieldIdle.png")); Next();
    }
    else if(FieldTestStep==3 && FieldTestTime>.3f)
    {
        FieldTestRipples=Ripples.Num();
        Shot(0);
        const FTransform T=EnemyShield->GetComponentTransform();
        const float Offset=Ripples.Num() ? FMath::Abs(FVector::DotProduct(Ripples.Last().Center-T.GetLocation(),T.GetUnitAxis(EAxis::X))):1e6f;
        Check(Rules.EnemyField>0 && Rules.EnemyField<100 && Ripples.Num()>=FieldTestRipples+5,TEXT("blocked_round_ripples_on_field"));
        Check(Offset<2,TEXT("ripple_centred_on_field_plane"));
        Next();
    }
    else if(FieldTestStep==4 && FieldTestTime>.14f)
    {
        Capture(TEXT("ATFieldRipple.png")); Next();
    }
    else if(FieldTestStep==5 && FieldTestTime>.6f)
    {
        Rules.EnemyField=20; const int32 Shatters=FieldBursts[0];
        Shot(1);
        Check(Rules.EnemyField==0 && FieldBursts[0]==Shatters+1,TEXT("breach_shatters_field"));
        Next();
    }
    else if(FieldTestStep==6 && FieldTestTime>.12f)
    {
        Capture(TEXT("ATFieldShatter.png")); Next();
    }
    else if(FieldTestStep==7 && FieldTestTime>.5f)
    {
        Check(!EnemyShield->IsVisible() && FieldBursts[1]==0,TEXT("broken_field_hidden_without_double_effect"));
        FieldTestRipples=FieldBursts[2]; VulnerableTime=.05f; Next();
    }
    else if(FieldTestStep==8 && FieldTestTime>.3f)
    {
        Check(Rules.EnemyField==100 && FieldBursts[2]>FieldTestRipples && EnemyShield->IsVisible(),TEXT("field_regenerates_with_restore_rings"));
        Capture(TEXT("ATFieldRestore.png")); Next();
    }
    else if(FieldTestStep==9 && FieldTestTime>.4f)
    {
        // Hold the guard facing Sachiel and let a real strike land on Unit-01's own field.
        P->bTestGuard=true; P->SetActorRotation(FRotator(0,(EnemyPosition-P->GetActorLocation()).Rotation().Yaw,0));
        FieldTestIntegrity=Rules.Integrity; FieldTestRipples=GuardRipples; AttackCount=0; EnemyClock=0;
        const FVector Shield=P->GetActorLocation()+P->GetActorForwardVector()*380;
        StoryView(Shield+FVector(3400,-1600,700),Shield+FVector(0,0,200),0); Next();
    }
    else if(FieldTestStep==10 && AttackCount>0 && Telegraph<=0)
    {
        Check(GuardRipples>FieldTestRipples && Rules.Integrity>=FieldTestIntegrity-5 && PlayerShield->IsVisible(),TEXT("guard_blocks_with_player_field"));
        EnemyClock=60; Next();
    }
    else if(FieldTestStep==11 && FieldTestTime>.12f)
    {
        Capture(TEXT("ATFieldGuard.png")); Next();
    }
    else if(FieldTestStep==12 && FieldTestTime>.5f)
    {
        P->bTestGuard=false; Rules.EnemyField=100; Rules.Sync=100; Rules.Battery=120; Systems.PulseCooldown=0;
        const int32 Shatters=FieldBursts[0];
        UseAntiField();
        Check(Rules.EnemyField==0 && FieldBursts[0]==Shatters+1,TEXT("anti_field_pulse_neutralizes_with_octagons"));
        Film(); Next();
    }
    else if(FieldTestStep==13 && FieldTestTime>.28f)
    {
        Capture(TEXT("ATFieldNeutralize.png")); Next();
    }
    else if(FieldTestStep==14 && FieldTestTime>.6f)
    {
        for(int32 I=0;I<60;++I) FieldImpact(EnemyShield,P->GetActorLocation(),EnemyAimPoint(),1);
        Check(Ripples.Num()<=RippleBatch->GetInstanceCount() && Ripples.Num()+RippleFree.Num()==RippleBatch->GetInstanceCount(),TEXT("ripple_pool_bounded"));
        bFieldDone=true;
        UE_LOG(LogTemp,Display,TEXT("EVA_FIELD_RESULT success=%d shatters=%d dissolves=%d restores=%d guards=%d"),bFieldOK,FieldBursts[0],FieldBursts[1],FieldBursts[2],GuardRipples);
        FPlatformMisc::RequestExitWithStatus(false,bFieldOK ? 0:1);
    }
    if(GetWorld()->GetTimeSeconds()>70) { UE_LOG(LogTemp,Error,TEXT("EVA_FIELD_TIMEOUT step=%d"),FieldTestStep); FPlatformMisc::RequestExitWithStatus(false,1); }
}
