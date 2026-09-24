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
    auto Hud=[&](bool bShow) { UGameplayStatics::GetPlayerController(this,0)->GetHUD()->bShowHUD=bShow; };
    if(DistrictTestStep==0 && DistrictTestTime>3)
    {
        int PerDistrict[4]={0,0,0,0}, Story=0, Instances=0;
        for(int I=0;I<Buildings.Num();++I)
        {
            const auto& B=Buildings[I];
            if(B.District>=0 && B.District<4) ++PerDistrict[B.District]; else ++Story;
            // The tallest tower on the northern blocks, shot from the clear road to its south.
            if(B.District==0 && B.Base.Y>Base.Y && (DistrictTestBuilding==INDEX_NONE || B.Height>Buildings[DistrictTestBuilding].Height)) DistrictTestBuilding=I;
        }
        for(auto* Mesh:Meshes) if(auto* Batch=Cast<UInstancedStaticMeshComponent>(Mesh)) Instances+=Batch->GetInstanceCount();
        UE_LOG(LogTemp,Display,TEXT("EVA_DISTRICT_INVENTORY central=%d harbor=%d upland=%d industrial=%d story=%d props=%d instances=%d components=%d"),PerDistrict[0],PerDistrict[1],PerDistrict[2],PerDistrict[3],Story,Props.Num(),Instances,Meshes.Num());
        Check(PerDistrict[0]==16 && Instances>20000 && CitySurface && DustSurface,TEXT("detailed_district_assets_loaded"));
        Check(PerDistrict[1]>=14 && PerDistrict[2]>=7 && PerDistrict[3]>=20 && Story>=60,TEXT("every_district_uses_destructible_kit"));
        Check(Props.Num()>800 && RubbleBatches.Num()==5 && ParticleBatches.Num()==3 && KitMesh(TEXT("DebrisChunk"))!=KitMesh(TEXT("Cube")) && KitMesh(TEXT("CoolingTower"))!=KitMesh(TEXT("Cube")),TEXT("street_props_and_destruction_pools_ready"));
        bool Loops=DistrictLoops.Num()==3; for(auto* Loop:DistrictLoops) Loops=Loops && Loop && Loop->IsPlaying();
        Check(CityAmbience && CityHum && CityAmbience->IsPlaying() && CityHum->IsPlaying() && Loops,TEXT("city_ambience_running"));
        StoryCamera->GetCameraComponent()->SetFieldOfView(65); Hud(false);
        StoryView(Base+FVector(15500,-19500,11500),Base+FVector(0,2000,2000),0); Next();
    }
    else if(DistrictTestStep==1 && DistrictTestTime>1)
    {
        Capture(TEXT("DistrictSkyline.png")); Next();
    }
    else if(DistrictTestStep==2 && DistrictTestTime>.3f)
    {
        P->SetActorLocation(Base+FVector(0,-5300,780)); P->CameraYaw=90; P->CameraPitch=-8;
        Hud(true); UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(P); Next();
    }
    else if(DistrictTestStep==3 && DistrictTestTime>1)
    {
        Capture(TEXT("DistrictStreet.png")); Next();
    }
    else if(DistrictTestStep==4 && DistrictTestTime>.3f)
    {
        P->SetActorLocation(CityArmoryPosition+FVector(0,-2800,780));
        Check(WorldPrompt().Contains(TEXT("OPEN ARMORY")),TEXT("armory_discoverable")); Interact(); Hud(false);
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
        bool Dark=B.Lights.Num()>0; for(auto* Light:B.Lights) Dark=Dark && !Light->IsVisible();
        Check(Dark && B.Scars[0]->GetInstanceCount()+B.Scars[1]->GetInstanceCount()>=9 && ParticleBatches[1].Live.Num()>0,TEXT("damage_scars_fires_smoke_and_power_loss"));
        P->SetActorLocation(FVector(B.Center.X,B.Center.Y-3500,780)); P->CameraYaw=90; P->CameraPitch=0;
        P->Loadout.AcquireCannon(); P->PickupTime=0; P->bLock=false;
        Hud(true); UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(P); Next();
    }
    else if(DistrictTestStep==8 && DistrictTestTime>.5f)
    {
        auto& B=Buildings[DistrictTestBuilding];
        P->Camera->SetWorldRotation((B.Center-P->Camera->GetComponentLocation()).Rotation()); LanceCooldown=0; Attack(true,0,true);
        P->Camera->SetRelativeRotation(FRotator::ZeroRotator);
        Check(B.bDestroyed && B.DamageStage==2 && P->Loadout.Shells==7 && B.Mesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision && B.CrownMesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("cannon_hit_starts_collapse"));
        bPaused=true; DistrictCollapseStamp=B.CollapseTime; Next();
    }
    else if(DistrictTestStep==9 && DistrictTestTime>.4f)
    {
        Check(Buildings[DistrictTestBuilding].CollapseTime==DistrictCollapseStamp,TEXT("pause_freezes_destruction"));
        bPaused=false; auto& B=Buildings[DistrictTestBuilding]; Hud(false);
        StoryView(FVector(B.Center.X-7600,B.Center.Y-2600,3300),B.Center-FVector(0,0,600),0); Next();
    }
    else if(DistrictTestStep==10 && DistrictTestTime>1.6f)
    {
        auto& B=Buildings[DistrictTestBuilding];
        Check(B.Crown && FMath::Abs(B.Crown->GetRelativeRotation().Pitch)+FMath::Abs(B.Crown->GetRelativeRotation().Roll)>3 && B.DistrictRoot->GetComponentLocation().Z<B.Base.Z-150 && B.Rubble.Num()>20,TEXT("crown_topples_while_frame_sinks"));
        Capture(TEXT("DistrictCollapse.png")); Next();
    }
    else if(DistrictTestStep==11 && DistrictTestTime>2.2f)
    {
        auto& B=Buildings[DistrictTestBuilding];
        Check(B.CollapseTime>=B.CollapseDuration() && !B.Mesh->IsVisible() && B.Rubble.Num()>20 && RubbleBatches[0]->GetCollisionEnabled()==ECollisionEnabled::NoCollision && B.Smolder>0,TEXT("collapse_settles_to_nonblocking_rubble"));
        int Lost=BuildingsLost; DestroyNearby(B.Center,100); Check(BuildingsLost==Lost,TEXT("collapse_counted_once"));
        FHitResult Hit; FCollisionQueryParams Query; Query.AddIgnoredActor(P); Query.AddIgnoredActor(Wingman);
        const auto Capsule=FCollisionShape::MakeCapsule(145,760);
        bool Boulevard=GetWorld()->SweepSingleByChannel(Hit,Base+FVector(0,-9500,780),Base+FVector(0,13500,780),FQuat::Identity,ECC_Visibility,Capsule,Query);
        bool Tunnel=GetWorld()->SweepSingleByChannel(Hit,Base+FVector(14500,10500,780),Base+FVector(14500,17000,780),FQuat::Identity,ECC_Visibility,Capsule,Query);
        Check(!Boulevard && !Tunnel,TEXT("mech_sized_routes_remain_clear"));
        Capture(TEXT("DistrictRubble.png")); Next();
    }
    else if(DistrictTestStep==12 && DistrictTestTime>.4f)
    {
        // A harbour fuel tank ruptures and an industrial chimney folds away from its hit.
        int Tank=INDEX_NONE;
        for(int I=0;I<Buildings.Num();++I)
        {
            if(Tank==INDEX_NONE && Buildings[I].District==1 && Buildings[I].Kind==EEvaArch::Tank) Tank=I;
            if(DistrictTestAux==INDEX_NONE && Buildings[I].District==3 && Buildings[I].Kind==EEvaArch::Stack) DistrictTestAux=I;
        }
        auto& Stack=Buildings[DistrictTestAux];
        DamageBuilding(Buildings[Tank],Buildings[Tank].Center-FVector(0,2500,0),2);
        DamageBuilding(Stack,Stack.Base+FVector(-900,0,2200),2);
        Check(Buildings[Tank].bDestroyed && Stack.bDestroyed && Stack.FallDirection.X>.9f && ParticleBatches[2].Live.Num()>0,TEXT("fuel_tank_ruptures_and_chimney_falls"));
        StoryView(Stack.Base+FVector(2500,-11500,4200),Stack.Base+FVector(2600,0,2200),0); Next();
    }
    else if(DistrictTestStep==13 && DistrictTestTime>1.9f)
    {
        const auto& Stack=Buildings[DistrictTestAux];
        Check(Stack.Crown && Stack.Crown->GetRelativeRotation().Pitch<-20,TEXT("chimney_crown_topples"));
        Capture(TEXT("DistrictIndustrial.png")); Next();
    }
    else if(DistrictTestStep==14 && DistrictTestTime>.3f)
    {
        // Walk straight into a house in the upland neighbourhoods.
        for(int I=0;I<Props.Num() && DistrictTestProp==INDEX_NONE;++I)
            if(Props[I].Radius>=400 && Props[I].Radius<560 && FVector::Dist2D(Props[I].Position,Districts[2])<13000) DistrictTestProp=I;
        P->SetActorLocation(Props[DistrictTestProp].Position+FVector(0,0,780)); P->MoveVelocity=FVector::ZeroVector; Next();
    }
    else if(DistrictTestStep==15 && DistrictTestTime>.3f)
    {
        Check(DistrictTestProp!=INDEX_NONE && Props[DistrictTestProp].bBroken && PropsBroken>0,TEXT("eva_footsteps_crush_street_props"));
        StoryView(Districts[2]+FVector(9000,-17000,7800),Districts[2]+FVector(-1500,2500,600),0); Next();
    }
    else if(DistrictTestStep==16 && DistrictTestTime>1.2f)
    {
        // Screenshots resolve on the next rendered frame, so the camera moves only in the following step.
        Capture(TEXT("DistrictUpland.png")); Next();
    }
    else if(DistrictTestStep==17 && DistrictTestTime>.3f)
    {
        StoryView(Districts[1]+FVector(-9000,-6500,6200),Districts[1]+FVector(2500,-21000,900),0); Next();
    }
    else if(DistrictTestStep==18 && DistrictTestTime>1.2f)
    {
        Capture(TEXT("DistrictHarbor.png")); Next();
    }
    else if(DistrictTestStep==19 && DistrictTestTime>.3f)
    {
        SelectedDistrict=0; BeginWorldEncounter(); Rules.Battery=120; Rules.Integrity=100; Rules.EnemyHealth=100000;
        P->SetActorLocation(Base+FVector(0,8500,780)); Wingman->Deploy(Base+FVector(1600,7800,780)); Wingman->Order=EEvaOrder::Assault;
        EnemyClock=1; Rules.EnemyField=0; VulnerableTime=30;
        for(auto& B:Buildings) if(B.District==0 && !B.bDestroyed && B.Center.Y>Base.Y+9000) { DestroyNearby(B.Center,100); DestroyNearby(B.Center,100); }
        Hud(true); StoryView(Base+FVector(6500,-3500,6000),Base+FVector(0,8500,800),0); Next();
    }
    else if(DistrictTestStep==20)
    {
        const double Now=FPlatformTime::Seconds();
        if(DistrictFrameStamp>0 && DistrictTestTime>1) DistrictFrames.Add((Now-DistrictFrameStamp)*1000);
        DistrictFrameStamp=Now;
        Rules.Battery=120; Rules.Integrity=100;
        if(DistrictTestTime>5 && DistrictTestTime-Dt<=5) { Capture(TEXT("DistrictCombat.png")); DistrictMeshCount=Meshes.Num(); }
        if(DistrictTestTime>12)
        {
            Check(Wingman->ShotsFired>0 && AttackCount>0,TEXT("live_angel_and_wingman_combat"));
            Check(Meshes.Num()<=DistrictMeshCount+8 && Effects.Num()<96,TEXT("effects_reuse_bounded_components"));
            DistrictFrames.Sort(); double Total=0; for(double Sample:DistrictFrames) Total+=Sample;
            const int N=DistrictFrames.Num();
            UE_LOG(LogTemp,Display,TEXT("EVA_DISTRICT_FRAME frames=%d mean_ms=%.3f p95_ms=%.3f p99_ms=%.3f meshes=%d lost=%d props=%d"),N,N ? Total/N:0,N ? DistrictFrames[FMath::Min(N-1,int(N*.95))]:0,N ? DistrictFrames[FMath::Min(N-1,int(N*.99))]:0,Meshes.Num(),BuildingsLost,PropsBroken);
            StartOpenWorld();
            const auto& B=Buildings[DistrictTestBuilding];
            Check(!B.bDestroyed && B.DamageStage==0 && B.Mesh->IsVisible() && B.Rubble.Num()==0 && B.Scars[0]->GetInstanceCount()==0 && SupplyMask==0 && CityArmoryStock==0 && CityArmoryOpen==0,TEXT("redeploy_restores_district"));
            bool Standing=PropsBroken==0 && BuildingsLost==0; for(const auto& Prop:Props) Standing=Standing && !Prop.bBroken;
            Check(Standing && RubbleBatches[0]->GetInstanceCount()==0 && ParticleBatches[0].Live.Num()==0 && !Buildings[DistrictTestAux].bDestroyed,TEXT("redeploy_rebuilds_props_and_clears_rubble"));
            Next();
        }
    }
    else if(DistrictTestStep==21 && DistrictTestTime>.3f)
    {
        Check(CityDoorLeft->GetRelativeLocation().X==-640,TEXT("redeploy_closes_armory"));
        bDistrictDone=true; UE_LOG(LogTemp,Display,TEXT("EVA_DISTRICT_RESULT success=%d"),bDistrictOK);
        FPlatformMisc::RequestExitWithStatus(false,bDistrictOK ? 0:1);
    }
    if(GetWorld()->GetTimeSeconds()>100) { UE_LOG(LogTemp,Error,TEXT("EVA_DISTRICT_TIMEOUT step=%d"),DistrictTestStep); FPlatformMisc::RequestExitWithStatus(false,1); }
}
