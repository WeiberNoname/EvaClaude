#pragma once
#include "CoreMinimal.h"

// Pure gameplay state. Rendering and animation never decide damage or power.
struct FEvaRules
{
    float Integrity = 100.f;
    float Battery = 120.f;
    float EnemyHealth = 1000.f;
    float EnemyField = 100.f;
    float Sync = 72.f;
    bool bConnected = true;

    void AdvancePower(float Delta, bool bGuarding)
    {
        Battery = FMath::Clamp(Battery + Delta * (bConnected ? 16.f : -(bGuarding ? 2.4f : 1.f)), 0.f, 120.f);
    }
    bool Spend(float Cost)
    {
        if (Battery < Cost) return false;
        Battery -= Cost;
        return true;
    }
    bool HitEnemy(float Damage, float Breach)
    {
        if (EnemyField > 0.f)
        {
            EnemyField = FMath::Max(0.f, EnemyField - Breach);
            Sync = FMath::Min(100.f, Sync + 1.5f);
            return false;
        }
        EnemyHealth = FMath::Max(0.f, EnemyHealth - Damage);
        Sync = FMath::Min(100.f, Sync + 2.f);
        return true;
    }
    void ReceiveHit(float Damage, bool bGuarding, bool bEvading)
    {
        if (bEvading) return;
        const bool bBlocked = bGuarding && Spend(5.f);
        Integrity = FMath::Max(0.f, Integrity - Damage * (bBlocked ? .12f : 1.f));
        Sync = FMath::Clamp(Sync + (bBlocked ? 2.f : -4.f), 0.f, 100.f);
    }
};

enum class EEvaWeapon : uint8 { Unarmed, Knife, Cannon };
enum class EEvaAngel : uint8 { Sachiel, Shamshel, Ramiel };

struct FEvaMotion
{
    static float Blend(float Rate,float Dt) { return 1.f-FMath::Exp(-Rate*FMath::Max(0.f,Dt)); }
    // Exact integration of exponential velocity response for a constant input over this step.
    static FVector Integrate(FVector& Velocity,FVector Desired,float Rate,float Dt)
    {
        const FVector Old=Velocity;
        const float Alpha=Blend(Rate,Dt);
        Velocity=FMath::Lerp(Old,Desired,Alpha);
        return Desired*Dt+(Old-Desired)*(Alpha/Rate);
    }
    int32 DashCharges=2;
    float Recharge=0;
    bool Dash(FEvaRules& R)
    {
        if(DashCharges<=0 || !R.Spend(2)) return false;
        --DashCharges; return true;
    }
    void Advance(float Dt)
    {
        if(DashCharges>=2) { Recharge=0; return; }
        Recharge+=Dt;
        while(Recharge>=1.4f && DashCharges<2) { Recharge-=1.4f; ++DashCharges; }
        if(DashCharges==2) Recharge=0;
    }
};

struct FEvaLoadout
{
    EEvaWeapon Equipped = EEvaWeapon::Unarmed;
    bool bHasCannon = false;
    int32 Shells = 0;
    int32 ReserveShells = 0;
    static constexpr int32 Capacity = 8;
    void AcquireCannon() { bHasCannon = true; Shells = Capacity; ReserveShells = 24; Equipped = EEvaWeapon::Cannon; }
    bool Reload()
    {
        if(!bHasCannon || Shells>=Capacity || ReserveShells<=0) return false;
        const int32 Transfer=FMath::Min(Capacity-Shells,ReserveShells);
        Shells+=Transfer; ReserveShells-=Transfer; return true;
    }
    bool EquipCannon() { if (!bHasCannon) return false; Equipped = EEvaWeapon::Cannon; return true; }
    void DrawKnife() { Equipped = EEvaWeapon::Knife; }
    bool FireCannon(FEvaRules& Rules, float Charge = 0.f)
    {
        if (!bHasCannon || Equipped != EEvaWeapon::Cannon || Shells <= 0 || !Rules.Spend(3.f+5.f*FMath::Clamp(Charge,0.f,1.f))) return false;
        --Shells;
        return true;
    }
};

enum class EEvaChapter : uint8
{
    Title, Street, Rescue, Hangar, Decision, Entry, Launch, Battle, Awakening, Hospital, Complete, Impact
};

// Costs and cooldowns shared by player input and automation.
struct FEvaSystems
{
    float Overdrive = 0.f;
    float OverdriveCooldown = 0.f;
    float PulseCooldown = 0.f;
    float CableStrain = 0.f;
    bool ActivateOverdrive(FEvaRules& R)
    {
        if(OverdriveCooldown>0 || R.Sync<80 || R.Battery<8) return false;
        R.Sync-=20; R.Battery-=8; Overdrive=8; OverdriveCooldown=24; return true;
    }
    bool AntiField(FEvaRules& R,float Distance)
    {
        if(PulseCooldown>0 || R.Sync<25 || R.Battery<12 || Distance>3200 || R.EnemyField<=0) return false;
        R.Sync-=25; R.Battery-=12; R.EnemyField=0; PulseCooldown=18; return true;
    }
    void Advance(float Dt,FEvaRules& R,float CableDistance)
    {
        const float Active=FMath::Min(Dt,Overdrive);
        R.Battery=FMath::Max(0.f,R.Battery-Active*2.5f);
        Overdrive=FMath::Max(0.f,Overdrive-Dt);
        OverdriveCooldown=FMath::Max(0.f,OverdriveCooldown-Dt);
        PulseCooldown=FMath::Max(0.f,PulseCooldown-Dt);
        CableStrain=R.bConnected && CableDistance>6200 ? CableStrain+Dt : 0.f;
        if(CableStrain>=2) { R.bConnected=false; CableStrain=0; }
        if(R.Battery<=0) Overdrive=0;
    }
    float DamageScale() const { return Overdrive>0 ? 1.6f : 1.f; }
};

struct FEvaShooter
{
    static bool RaySphere(FVector Start,FVector Direction,FVector Center,float Radius,float Range,float& Distance)
    {
        Direction=Direction.GetSafeNormal(); if(Direction.IsNearlyZero()) return false;
        FVector Delta=Start-Center; float B=FVector::DotProduct(Delta,Direction);
        float C=Delta.SizeSquared()-Radius*Radius, D=B*B-C;
        if(D<0) return false;
        float T=-B-FMath::Sqrt(D); if(T<0) T=-B+FMath::Sqrt(D);
        if(T<0 || T>Range) return false; Distance=T; return true;
    }
    static bool InWhipLane(FVector Point,FVector Start,FVector End,float Width)
    {
        Point.Z=Start.Z=End.Z=0;
        return FVector::DistSquared(Point,FMath::ClosestPointOnSegment(Point,Start,End))<=Width*Width;
    }
};
