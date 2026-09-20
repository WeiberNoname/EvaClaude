#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

static void SystemSound(UObject* Context,const TCHAR* Name)
{
    FString Path=FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name);
    if(auto* Sound=LoadObject<USoundBase>(nullptr,*Path)) UGameplayStatics::PlaySound2D(Context,Sound,.5f);
}
void AEvaGameMode::UseOverdrive()
{
    if(!IsActive() || !HasCombatTarget()) return;
    if(!Systems.ActivateOverdrive(Rules))
    { SetNotice("OVERDRIVE REQUIRES 80 SYNC / 8 POWER / READY COOLDOWN"); return; }
    SetNotice("SYNCHRONIZATION OVERDRIVE // DAMAGE +60% / RESERVE DRAIN INCREASED");
    SystemSound(this,TEXT("Heartbeat"));
}
void AEvaGameMode::UseAntiField()
{
    if(!IsActive() || !HasCombatTarget()) return;
    auto* P=Pilot(); if(!P) return;
    if(!Systems.AntiField(Rules,FVector::Dist2D(P->GetActorLocation(),EnemyPosition)))
    { SetNotice("ANTI-A.T. PULSE // WITHIN 32m / 25 SYNC / 12 POWER / ACTIVE ENEMY FIELD"); return; }
    VulnerableTime=8;
    SetNotice("ANTI-A.T. FIELD // BARRIER NEUTRALIZED FOR 8 SECONDS");
    SystemSound(this,TEXT("Breach"));
    for(int I=0;I<24;++I)
    {
        float A=I*2*PI/24;
        FVector Pos=P->GetActorLocation()+FVector(FMath::Cos(A)*1400,FMath::Sin(A)*1400,150);
        auto* Ray=Shape("Cylinder",Pos,FVector(.13f,.13f,9),FLinearColor(1,.35f,.025f),3);
        Ray->SetWorldRotation(FRotator(0,0,90));
        Effects.Add({Ray,.65f,.65f,FVector(.1f,.1f,12)});
    }
}
void AEvaGameMode::TickSystems(float Dt)
{
    auto* P=Pilot(); if(!P) return;
    bool WasConnected=Rules.bConnected;
    float Distance=FVector::Dist2D(P->GetActorLocation(),CableAnchor());
    Systems.Advance(Dt,Rules,Distance);
    bool Warning=Rules.bConnected && Distance>5270;
    if(Warning && !bCableWarning)
    { SetNotice("UMBILICAL TENSION // RETURN TOWARD THE CONNECTED STATION"); SystemSound(this,TEXT("Alarm")); }
    if(WasConnected && !Rules.bConnected)
    { SetNotice("UMBILICAL RELEASED // RUNNING ON INTERNAL RESERVE"); SystemSound(this,TEXT("Alarm")); }
    bCableWarning=Warning;
    if(auto* Material=Cast<UMaterialInstanceDynamic>(Cable->GetMaterial(0)))
        Material->SetVectorParameterValue(TEXT("Color"),Warning ? FLinearColor(1,.06f,.01f) : FLinearColor(.52f,1,.045f));
}
