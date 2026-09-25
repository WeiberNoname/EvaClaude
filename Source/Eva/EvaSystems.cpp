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
    // Unit-01 projects its own field into the Angel's: octagons travel across the gap, then the barrier shatters.
    const FVector Chest=P->GetActorLocation()+FVector(0,0,300), Toward=EnemyAimPoint()-Chest;
    for(int I=0;I<5;++I) SpawnRipple(Chest,Toward.Rotation().Quaternion(),FieldColor(PlayerShield),250,1100+I*260,.55f,I*.07f,2.4f,Toward/.62f);
    if(EnemyShield) FieldBurst(EnemyShield,0,EnemyShield->GetComponentLocation());
    WatchedField=0;
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
