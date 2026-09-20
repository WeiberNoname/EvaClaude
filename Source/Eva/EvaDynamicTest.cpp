#include "EvaGame.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"

void AEvaGameMode::TickDynamicTest(float Dt)
{
    if(bDynamicDone) return;
    auto* P=Pilot(); if(!P) return;
    const double Stamp=FPlatformTime::Seconds();
    if(LastFrameStamp>0 && MissionTime>2 && !bPaused) FrameSamples.Add((Stamp-LastFrameStamp)*1000);
    LastFrameStamp=Stamp;
    DynamicTime+=Dt; DynamicPeak=FMath::Max(DynamicPeak,float(P->GetActorLocation().Z));
    auto Check=[&](bool OK,const TCHAR* Name) { bDynamicOK &= OK; UE_LOG(LogTemp,Display,TEXT("EVA_DYNAMIC_CHECK %s=%d"),Name,OK); };
    auto Next=[&]() { ++DynamicStep; DynamicTime=0; };
    auto Shot=[&](float Charge)
    {
        P->Camera->SetWorldRotation((EnemyAimPoint()-P->Camera->GetComponentLocation()).Rotation());
        Attack(true,Charge,true); P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
    };
    if(DynamicStep==0 && DynamicTime>2)
    {
        Check(bRamiel && EnemyMaxHealth==2200,TEXT("ramiel_selected"));
        DynamicStart=P->GetActorLocation(); P->TestMoveInput=FVector(0,1,0); Next();
    }
    else if(DynamicStep==1 && DynamicTime>.6f)
    {
        Check(FVector::Dist2D(DynamicStart,P->GetActorLocation())>1000,TEXT("responsive_run"));
        DynamicPeak=0; P->Jump(); Check(!P->bGrounded && P->VerticalSpeed==4000,TEXT("powered_jump"));
        DynamicStart=P->GetActorLocation(); P->TestMoveInput=FVector(1,0,0); P->Dodge(); Next();
    }
    else if(DynamicStep==2 && DynamicTime>.7f)
    {
        Check(DynamicPeak>DynamicStart.Z+800,TEXT("jump_apex"));
        Check(P->GetActorLocation().X>DynamicStart.X+1300 && P->Motion.DashCharges==1,TEXT("air_dash_momentum"));
        P->TestMoveInput=FVector::ZeroVector; Next();
    }
    else if(DynamicStep==3 && DynamicTime>1.2f)
    {
        Check(P->bGrounded && FMath::Abs(P->GetActorLocation().Z-780)<35,TEXT("collision_landing"));
        Check(P->Motion.DashCharges==2,TEXT("dash_recharge"));
        P->TogglePerspective(); P->bLock=true; P->bAiming=true; Next();
    }
    else if(DynamicStep==4 && DynamicTime>1)
    {
        Check(P->bFirstPerson && P->CockpitRoot->IsVisible() && !P->EvaRig->IsVisible() && P->Boom->TargetArmLength<1,TEXT("entry_plug_view"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/EntryPlug.png"),true,false);
        Next();
    }
    else if(DynamicStep==5 && !bHelp && DynamicTime>.25f)
    { DynamicMissionStamp=MissionTime; P->ToggleHelp(); DynamicTime=0; }
    else if(DynamicStep==5 && bHelp && DynamicTime>.5f)
    {
        Check(bHelp && bPaused && MissionTime==DynamicMissionStamp,TEXT("help_pauses_simulation"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/ControlsMenu.png"),true,false);
        Next();
    }
    else if(DynamicStep==6 && DynamicTime>.25f)
    {
        P->ToggleHelp(); P->TogglePerspective(); P->bLock=false; P->bAiming=false;
        Check(!bHelp && !bPaused && !P->bFirstPerson && P->EvaRig->IsVisible(),TEXT("return_to_external_view"));
        P->SetActorLocation(Districts[3]+FVector(0,7400,780)); P->MoveVelocity=FVector::ZeroVector;
        P->CameraYaw=90; P->CameraPitch=2;
        ClearAngelHazards(); Telegraph=0; EnemyClock=0; AttackCount=0; Rules.Integrity=100;
        Next();
    }
    else if(DynamicStep==7 && Telegraph>0 && Telegraph<.6f)
    {
        Check(AttackCount==1 && BeamWarnings[0]->IsVisible() && !BeamWarnings[1]->IsVisible(),TEXT("single_beam_lock"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Ramiel.png"),true,false);
        P->TestMoveInput=FVector(1,0,0); P->Dodge(); Next();
    }
    else if(DynamicStep==8 && Telegraph<=0 && DynamicTime>.8f)
    {
        Check(Rules.Integrity==100 && Rules.EnemyField==0,TEXT("dash_evades_locked_beam"));
        P->TestMoveInput=FVector::ZeroVector; P->MoveVelocity=FVector::ZeroVector;
        Rules.EnemyHealth=900; EnemyClock=0; Next();
    }
    else if(DynamicStep==9 && Telegraph>0 && Telegraph<1.4f)
    {
        Check(AttackCount==2 && BeamWarnings[2]->IsVisible(),TEXT("enraged_three_beam_pattern"));
        DynamicCover=Shape("Cube",FMath::Lerp(EnemyPosition,P->GetActorLocation()+FVector(0,0,180),.55),FVector(12,12,45),FLinearColor(.1f,.1f,.1f),0,nullptr,true);
        Next();
    }
    else if(DynamicStep==10 && Telegraph<=0 && DynamicTime>2)
    {
        Check(Rules.Integrity==100 && Rules.EnemyField==0 && RamielCore->IsVisible(),TEXT("cover_and_core_window"));
        Meshes.Remove(DynamicCover); DynamicCover->DestroyComponent(); DynamicCover=nullptr;
        LanceCooldown=0; Shot(1);
        Check(FMath::IsNearlyEqual(Rules.EnemyHealth,581.f,.1f),TEXT("charged_core_counterattack"));
        P->Loadout.Shells=1; P->Loadout.ReserveShells=4; Rules.Battery=120; EnemyClock=20; LanceCooldown=0; P->Melee(); Next();
    }
    else if(DynamicStep==11 && DynamicTime>.25f)
    {
        Check(P->ReloadTime>0 && P->bFireHeld,TEXT("empty_magazine_auto_reload")); Next();
    }
    else if(DynamicStep==12 && DynamicTime>1.6f)
    {
        Check(P->Loadout.ReserveShells==0 && P->Loadout.Shells<4 && P->ReloadTime==0,TEXT("held_fire_resumes_after_reload"));
        P->StopFire(); Rules.EnemyHealth=100; Rules.EnemyField=0; LanceCooldown=0; P->Loadout.Shells=1; Shot(0);
        Check(!bWorldEncounter && CompletedContracts==1 && !BeamShots[0]->IsVisible(),TEXT("ramiel_defeat_cleanup"));
        P->TogglePerspective(); StartBattle();
        Check(!P->bFirstPerson && P->bGrounded && P->Motion.DashCharges==2 && P->VerticalSpeed==0,TEXT("deployment_resets_mobility"));
        bDynamicDone=true;
        FrameSamples.Sort(); double Total=0; for(double F:FrameSamples) Total+=F;
        UE_LOG(LogTemp,Display,TEXT("EVA_FRAME_SAMPLE frames=%d mean_ms=%.3f p95_ms=%.3f p99_ms=%.3f"),FrameSamples.Num(),FrameSamples.Num() ? Total/FrameSamples.Num():0,FrameSamples.Num() ? FrameSamples[FMath::Min(FrameSamples.Num()-1,int(FrameSamples.Num()*.95))]:0,FrameSamples.Num() ? FrameSamples[FMath::Min(FrameSamples.Num()-1,int(FrameSamples.Num()*.99))]:0);
        UE_LOG(LogTemp,Display,TEXT("EVA_DYNAMIC_RESULT success=%d"),bDynamicOK);
        FPlatformMisc::RequestExitWithStatus(false,bDynamicOK ? 0:1);
    }
    if(MissionTime>45) { UE_LOG(LogTemp,Error,TEXT("EVA_DYNAMIC_TIMEOUT step=%d"),DynamicStep); FPlatformMisc::RequestExitWithStatus(false,1); }
}
