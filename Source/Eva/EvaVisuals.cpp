#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

void AEvaPawn::BuildEvaVisuals()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()); if(!G) return;
    EvaRig=NewObject<USceneComponent>(this); EvaRig->SetupAttachment(GetRootComponent()); EvaRig->RegisterComponent();
    G->BuildUnit(EvaRig,false,BodyMeshes,Limbs,Knees,ShoulderHatch);
}

void AEvaPawn::UpdateCameraOcclusion()
{
    TArray<USceneComponent*> NextOccluders;
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(G && !bHuman && !bFirstPerson && G->Chapter==EEvaChapter::Battle)
    {
        FVector A=Camera->GetComponentLocation(), B=GetActorLocation()+FVector(0,0,400);
        for(auto& Building:G->Buildings)
        {
            if(Building.bDestroyed || !Building.DistrictRoot || FVector::DistSquared2D(Building.Center,A)>FMath::Square(6000.f+Building.Extent.GetMax())) continue;
            FBox Bounds=Building.Bounds().ExpandBy(90);
            if(FMath::LineBoxIntersection(Bounds,A,B,B-A) || Bounds.ComputeSquaredDistanceToPoint(A)<250.f*250.f) NextOccluders.Add(Building.DistrictRoot);
        }
    }
    for(auto* M:CameraOccluders) if(IsValid(M) && !NextOccluders.Contains(M)) M->SetHiddenInGame(false,true);
    for(auto* M:NextOccluders) if(!CameraOccluders.Contains(M)) M->SetHiddenInGame(true,true);
    CameraOccluders=MoveTemp(NextOccluders);
}
