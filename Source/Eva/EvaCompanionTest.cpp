#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"

void AEvaGameMode::TickCompanionTest(float Dt)
{
    if(bCompanionDone) return; CompanionTime+=Dt;
    auto* P=Pilot(); auto* W=Wingman; if(!P || !W) return;
    auto Check=[&](bool OK,const TCHAR* Name) { bCompanionOK &= OK; UE_LOG(LogTemp,Display,TEXT("EVA_COMPANION_CHECK %s=%d"),Name,OK); };
    auto Next=[&]() { ++CompanionStep; CompanionTime=0; };
    auto Capture=[&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false); };
    if(CompanionStep==0 && CompanionTime>2)
    {
        Check(W->bDeployed && !W->IsHidden() && W->Parts.Num()>100 && P->BodyMeshes.Num()>100 && W->Limbs.Num()==4,TEXT("distinct_units_deployed"));
        P->SetActorLocation(Districts[0]+FVector(0,3500,780)); W->SetActorLocation(P->GetActorLocation()+FVector(1800,0,0));
        ReplyAsuka("hold here"); W->SetActorRotation(FRotator(0,90,0)); P->Loadout.Equipped=EEvaWeapon::Unarmed;
        StoryView(P->GetActorLocation()+FVector(3100,4300,900),P->GetActorLocation()+FVector(750,0,70),0); Next();
    }
    else if(CompanionStep==1 && CompanionTime>1)
    {
        Capture(TEXT("EvaDuo.png")); Next();
    }
    else if(CompanionStep==2 && CompanionTime>.3f)
    {
        UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(P); P->CameraYaw=25; P->CameraPitch=0; P->TogglePerspective(); Next();
    }
    else if(CompanionStep==3 && CompanionTime>1)
    {
        Check(P->bFirstPerson && P->CockpitRoot->IsVisible() && !P->EvaRig->IsVisible(),TEXT("panoramic_entry_plug")); Capture(TEXT("EntryPlug08.png")); Next();
    }
    else if(CompanionStep==4 && CompanionTime>.3f)
    {
        W->Integrity=82; ToggleComms(); CompanionMissionStamp=MissionTime;
        SubmitComms("Are you okay?"); Check(CommsHistory.Last().Contains("82"),TEXT("contextual_chat_status"));
        SubmitComms("cover me"); Check(W->Order==EEvaOrder::Assault,TEXT("chat_changes_squad_order"));
        SubmitComms("What is the plan?"); Next();
    }
    else if(CompanionStep==5 && CompanionTime>.5f)
    {
        Check(bCommsOpen && bPaused && MissionTime==CompanionMissionStamp && UGameplayStatics::GetPlayerController(this,0)->bShowMouseCursor,TEXT("call_pauses_and_focuses_ui"));
        Capture(TEXT("AsukaCall.png")); Next();
    }
    else if(CompanionStep==6 && CompanionTime>.3f)
    {
        CloseComms(); Check(!bCommsOpen && !bPaused && !UGameplayStatics::GetPlayerController(this,0)->bShowMouseCursor,TEXT("call_restores_game_input"));
        bPaused=true; ToggleComms(); CloseComms(); Check(bPaused,TEXT("existing_pause_preserved")); bPaused=false;
        ReplyAsuka("follow me"); P->TogglePerspective(); P->SetActorLocation(P->GetActorLocation()+FVector(0,4500,0)); CompanionStart=W->GetActorLocation(); Next();
    }
    else if(CompanionStep==7 && CompanionTime>2)
    {
        Check(FVector::Dist2D(CompanionStart,W->GetActorLocation())>1000,TEXT("ai_follows_player"));
        ReplyAsuka("hold here"); CompanionStart=W->GetActorLocation(); Next();
    }
    else if(CompanionStep==8 && CompanionTime>1.5f)
    {
        Check(FVector::Dist2D(CompanionStart,W->GetActorLocation())<350,TEXT("hold_order_stops_movement"));
        SelectedDistrict=0; BeginWorldEncounter(); EnemyClock=30; Rules.EnemyField=0; VulnerableTime=30; Rules.EnemyHealth=5000;
        P->SetActorLocation(Districts[0]+FVector(0,8800,780)); W->SetActorLocation(Districts[0]+FVector(1200,8700,780));
        W->Velocity=FVector::ZeroVector; ReplyAsuka("cover me"); Next();
    }
    else if(CompanionStep==9 && CompanionTime>3)
    {
        Check(W->ShotsFired>0 && Rules.EnemyHealth<5000,TEXT("ai_cover_fire_damages_angel"));
        ReplyAsuka("don't attack"); int Shots=W->ShotsFired; W->FireCooldown=0; W->RecoverTime=float(Shots);
        Next();
    }
    else if(CompanionStep==10 && CompanionTime>1)
    {
        Check(W->ShotsFired==int(W->RecoverTime),TEXT("cease_fire_is_respected"));
        ReplyAsuka("attack"); Rules.EnemyField=0; Rules.EnemyHealth=1; W->FireCooldown=0; Next();
    }
    else if(CompanionStep==11 && CompanionTime>2)
    {
        Check(!bWorldEncounter && CompletedContracts==1,TEXT("companion_victory_records_contract"));
        W->Integrity=0; P->SetActorLocation(Districts[0]+FVector(0,-1000,780)); WorldInteract(); Check(W->Integrity==100,TEXT("service_repairs_unit02"));
        ToggleComms(); StartBattle(); Check(!bCommsOpen && !bPaused && !W->bDeployed && W->IsHidden(),TEXT("story_reset_closes_call_and_retires_wingman"));
        bCompanionDone=true; UE_LOG(LogTemp,Display,TEXT("EVA_COMPANION_RESULT success=%d"),bCompanionOK);
        FPlatformMisc::RequestExitWithStatus(false,bCompanionOK ? 0:1);
    }
    if(GetWorld()->GetTimeSeconds()>60) { UE_LOG(LogTemp,Error,TEXT("EVA_COMPANION_TIMEOUT step=%d"),CompanionStep); FPlatformMisc::RequestExitWithStatus(false,1); }
}
