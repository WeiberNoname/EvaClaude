#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"

void AEvaGameMode::TickDistrictTest(float Dt)
{
    if(bDistrictDone) return;
    DistrictTestTime+=Dt;
    auto* P=Pilot(); if(!P || !Wingman) return;
    const FVector Base=Districts[0];
    auto Check=[&](bool OK,const TCHAR* Name) { bDistrictOK &= OK; UE_LOG(LogTemp,Display,TEXT("EVA_DISTRICT_CHECK %s=%d"),Name,OK); };
    auto Next=[&]() { ++DistrictTestStep; DistrictTestTime=0; };
    auto Capture=[&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false); };
    if(DistrictTestStep==0 && DistrictTestTime>3)
    {
        int Towers=0,Instances=0;
        for(int I=0;I<Buildings.Num();++I) if(Buildings[I].DistrictRoot) { ++Towers; if(DistrictTestBuilding==INDEX_NONE) DistrictTestBuilding=I; }
        for(auto* Mesh:Meshes) if(auto* Batch=Cast<UInstancedStaticMeshComponent>(Mesh)) Instances+=Batch->GetInstanceCount();
        Check(Towers==16 && Instances>5000 && CitySurface && DustSurface,TEXT("detailed_district_assets_loaded"));
        Check(CityAmbience && CityHum && CityAmbience->IsPlaying() && CityHum->IsPlaying(),TEXT("city_ambience_running"));
        StoryCamera->GetCameraComponent()->SetFieldOfView(65);
        UGameplayStatics::GetPlayerController(this,0)->GetHUD()->bShowHUD=false;
        StoryView(Base+FVector(15500,-19500,11500),Base+FVector(0,2000,2000),0); Next();
    }
    else if(DistrictTestStep==1 && DistrictTestTime>1)
    {
        Capture(TEXT("DistrictSkyline.png")); Next();
    }
    else if(DistrictTestStep==2 && DistrictTestTime>.3f)
    {
        P->SetActorLocation(Base+FVector(0,-5300,780)); P->CameraYaw=90; P->CameraPitch=-8;
        UGameplayStatics::GetPlayerController(this,0)->GetHUD()->bShowHUD=true;
        UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(P); Next();
    }
    else if(DistrictTestStep==3 && DistrictTestTime>1)
    {
        Capture(TEXT("DistrictStreet.png")); Next();
    }
    else if(DistrictTestStep==4 && DistrictTestTime>.3f)
    {
        P->SetActorLocation(CityArmoryPosition+FVector(0,-2800,780));
        Check(WorldPrompt().Contains(TEXT("OPEN ARMORY")),TEXT("armory_discoverable")); Interact();
        UGameplayStatics::GetPlayerController(this,0)->GetHUD()->bShowHUD=false;
        StoryView(CityArmoryPosition+FVector(3800,-9500,3400),CityArmoryPosition+FVector(0,0,2500),0); Next();
    }
    else if(DistrictTestStep==5 && DistrictTestTime>3)
    {
        Check(CityArmoryOpen>=1 && CityDoorLeft->GetRelativeLocation().X<-2000 && CityGunRoot->GetRelativeLocation().Z>1400,TEXT("armory_doors_and_elevator"));
        Capture(TEXT("DistrictArmory.png")); Next();
    }
    else if(DistrictTestStep==6 && DistrictTestTime>.35f)
    {
        P->Loadout.Shells=0; P->Loadout.ReserveShells=0; Interact();
        Check(P->Loadout.Shells==8 && P->Loadout.ReserveShells==24 && P->PickupTime>0 && !CityGunRoot->IsVisible(),TEXT("physical_weapon_pickup")); Next();
    }
    else if(DistrictTestStep==7 && DistrictTestTime>1.3f)
    {
        Rules.bConnected=false; Rules.Battery=30; Rules.Integrity=40; Wingman->Integrity=40; P->Loadout.ReserveShells=0;
        for(FVector Point:SupplyPositions) { P->SetActorLocation(Point+FVector(0,-900,780)); Interact(); }
        Check(SupplyMask==7 && Rules.Battery==105 && P->Loadout.ReserveShells==24 && Rules.Integrity==70 && Wingman->Integrity==70,TEXT("supply_route_rewards_both_units"));
        Rules.Battery=50; P->Loadout.ReserveShells=0; Interact();
        Check(Rules.Battery==50 && P->Loadout.ReserveShells==0,TEXT("cache_cannot_be_farmed"));
        auto& B=Buildings[DistrictTestBuilding]; DestroyNearby(B.Center,100);
        Check(B.DamageStage==1 && !B.bDestroyed && B.Mesh->GetCollisionEnabled()!=ECollisionEnabled::NoCollision,TEXT("blast_damages_but_preserves_cover"));
        P->SetActorLocation(FVector(B.Center.X,B.Center.Y-3500,780)); P->CameraYaw=90; P->CameraPitch=0;
        P->Loadout.AcquireCannon(); P->PickupTime=0; P->bLock=false;
        UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(P); Next();
    }
    else if(DistrictTestStep==8 && DistrictTestTime>.5f)
    {
        auto& B=Buildings[DistrictTestBuilding];
        P->Camera->SetWorldRotation((B.Center-P->Camera->GetComponentLocation()).Rotation()); LanceCooldown=0; Attack(true,0,true);
        P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
        Check(B.bDestroyed && B.DamageStage==2 && P->Loadout.Shells==7 && B.Mesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("cannon_hit_starts_collapse"));
        bPaused=true; DistrictCollapseStamp=B.CollapseTime; Next();
    }
    else if(DistrictTestStep==9 && DistrictTestTime>.4f)
    {
        Check(Buildings[DistrictTestBuilding].CollapseTime==DistrictCollapseStamp,TEXT("pause_freezes_destruction"));
        bPaused=false; auto& B=Buildings[DistrictTestBuilding];
        StoryView(FVector(B.Center.X+3500,B.Center.Y-4300,2500),B.Center,0); Next();
    }
    else if(DistrictTestStep==10 && DistrictTestTime>.9f)
    {
        Capture(TEXT("DistrictCollapse.png")); Next();
    }
    else if(DistrictTestStep==11 && DistrictTestTime>1.6f)
    {
        auto& B=Buildings[DistrictTestBuilding];
        Check(B.Rubble->IsVisible() && !B.Mesh->IsVisible() && B.Rubble->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("collapse_settles_to_nonblocking_rubble"));
        int Lost=BuildingsLost; DestroyNearby(B.Center,100); Check(BuildingsLost==Lost,TEXT("collapse_counted_once"));
        FHitResult Hit; FCollisionQueryParams Query; Query.AddIgnoredActor(P); Query.AddIgnoredActor(Wingman);
        const auto Capsule=FCollisionShape::MakeCapsule(145,760);
        bool Boulevard=GetWorld()->SweepSingleByChannel(Hit,Base+FVector(0,-9500,780),Base+FVector(0,13500,780),FQuat::Identity,ECC_Visibility,Capsule,Query);
        bool Tunnel=GetWorld()->SweepSingleByChannel(Hit,Base+FVector(14500,10500,780),Base+FVector(14500,17000,780),FQuat::Identity,ECC_Visibility,Capsule,Query);
        Check(!Boulevard && !Tunnel,TEXT("mech_sized_routes_remain_clear"));
        SelectedDistrict=0; BeginWorldEncounter(); Rules.Battery=120; Rules.Integrity=100; Rules.EnemyHealth=100000;
        P->SetActorLocation(Base+FVector(0,8500,780)); Wingman->Deploy(Base+FVector(1600,7800,780)); Wingman->Order=EEvaOrder::Assault;
        EnemyClock=1; Rules.EnemyField=0; VulnerableTime=30;
        UGameplayStatics::GetPlayerController(this,0)->GetHUD()->bShowHUD=true;
        StoryView(Base+FVector(6500,-3500,6000),Base+FVector(0,8500,800),0); Next();
    }
    else if(DistrictTestStep==12)
    {
        const double Now=FPlatformTime::Seconds();
        if(DistrictFrameStamp>0 && DistrictTestTime>1) DistrictFrames.Add((Now-DistrictFrameStamp)*1000);
        DistrictFrameStamp=Now;
        Rules.Battery=120; Rules.Integrity=100;
        if(DistrictTestTime<.1f)
        {
            for(auto& B:Buildings) if(B.DistrictRoot && !B.bDestroyed && B.Center.Y>Base.Y+9000) { DestroyNearby(B.Center,100); DestroyNearby(B.Center,100); }
        }
        if(DistrictTestTime>5 && DistrictTestTime-Dt<=5) { Capture(TEXT("DistrictCombat.png")); DistrictMeshCount=Meshes.Num(); }
        if(DistrictTestTime>12)
        {
            Check(Wingman->ShotsFired>0 && AttackCount>0,TEXT("live_angel_and_wingman_combat"));
            Check(Meshes.Num()<=DistrictMeshCount+8 && Effects.Num()<96,TEXT("effects_reuse_bounded_components"));
            DistrictFrames.Sort(); double Total=0; for(double Sample:DistrictFrames) Total+=Sample;
            const int N=DistrictFrames.Num();
            UE_LOG(LogTemp,Display,TEXT("EVA_DISTRICT_FRAME frames=%d mean_ms=%.3f p95_ms=%.3f p99_ms=%.3f meshes=%d"),N,N ? Total/N:0,N ? DistrictFrames[FMath::Min(N-1,int(N*.95))]:0,N ? DistrictFrames[FMath::Min(N-1,int(N*.99))]:0,Meshes.Num());
            StartOpenWorld();
            Check(!Buildings[DistrictTestBuilding].bDestroyed && Buildings[DistrictTestBuilding].Mesh->IsVisible() && !Buildings[DistrictTestBuilding].Rubble->IsVisible() && SupplyMask==0 && CityArmoryStock==0 && CityArmoryOpen==0,TEXT("redeploy_restores_district"));
            Next();
        }
    }
    else if(DistrictTestStep==13 && DistrictTestTime>.3f)
    {
        Check(CityDoorLeft->GetRelativeLocation().X==-640,TEXT("redeploy_closes_armory"));
        bDistrictDone=true; UE_LOG(LogTemp,Display,TEXT("EVA_DISTRICT_RESULT success=%d"),bDistrictOK);
        FPlatformMisc::RequestExitWithStatus(false,bDistrictOK ? 0:1);
    }
    if(GetWorld()->GetTimeSeconds()>90) { UE_LOG(LogTemp,Error,TEXT("EVA_DISTRICT_TIMEOUT step=%d"),DistrictTestStep); FPlatformMisc::RequestExitWithStatus(false,1); }
}
