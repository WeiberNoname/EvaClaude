#include "EvaRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaPowerTest,"Eva.Rules.PowerAndCosts",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEvaPowerTest::RunTest(const FString& Parameters)
{
    FEvaRules R;
    R.bConnected=false;
    R.AdvancePower(10,false);
    TestEqual(TEXT("Ten disconnected seconds consume ten reserve seconds"),R.Battery,110.f);
    R.AdvancePower(10,true);
    TestEqual(TEXT("Guard has an additional power cost"),R.Battery,86.f);
    TestFalse(TEXT("Insufficient power rejects ability"),R.Spend(100.f));
    TestEqual(TEXT("Rejected ability does not consume power"),R.Battery,86.f);
    R.bConnected=true; R.AdvancePower(10,false);
    TestEqual(TEXT("Recharge is capped"),R.Battery,120.f);
    R.bConnected=false; R.AdvancePower(1000,false);
    TestEqual(TEXT("Battery cannot go negative"),R.Battery,0.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaCombatTest,"Eva.Rules.FieldAndDamage",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEvaCombatTest::RunTest(const FString& Parameters)
{
    FEvaRules R;
    for(int I=0;I<5;++I) R.HitEnemy(75,23);
    TestEqual(TEXT("Breach hit does not leak into core"),R.EnemyHealth,1000.f);
    TestEqual(TEXT("Five melee hits breach field"),R.EnemyField,0.f);
    R.HitEnemy(75,23);
    TestEqual(TEXT("Exposed core takes damage"),R.EnemyHealth,925.f);
    R.ReceiveHit(20,false,true);
    TestEqual(TEXT("Dodge avoids damage"),R.Integrity,100.f);
    R.ReceiveHit(20,true,false);
    TestTrue(TEXT("Field mitigates damage"),FMath::IsNearlyEqual(R.Integrity,97.6f));
    R.Battery=0;
    R.ReceiveHit(20,true,false);
    TestTrue(TEXT("Empty field cannot block"),FMath::IsNearlyEqual(R.Integrity,77.6f));
    R.ReceiveHit(500,false,false);
    TestEqual(TEXT("Integrity clamps at zero"),R.Integrity,0.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaEquipmentTest,"Eva.Rules.Equipment",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEvaEquipmentTest::RunTest(const FString& Parameters)
{
    FEvaRules R;
    FEvaLoadout L;
    TestFalse(TEXT("Cannon cannot be equipped before physical pickup"),L.EquipCannon());
    TestFalse(TEXT("Unowned cannon cannot fire"),L.FireCannon(R));
    L.AcquireCannon();
    TestEqual(TEXT("Rack supplies eight shells"),L.Shells,8);
    L.DrawKnife();
    TestFalse(TEXT("Holstered cannon cannot fire"),L.FireCannon(R));
    TestEqual(TEXT("Rejected fire preserves ammunition"),L.Shells,8);
    L.EquipCannon(); R.Battery=2;
    TestFalse(TEXT("Insufficient power rejects cannon fire"),L.FireCannon(R));
    TestEqual(TEXT("Rejected power cost preserves ammunition"),L.Shells,8);
    R.Battery=120;
    for(int I=0;I<8;++I) TestTrue(TEXT("Loaded shell can fire"),L.FireCannon(R));
    TestFalse(TEXT("Empty cannon cannot fire"),L.FireCannon(R));
    TestEqual(TEXT("No negative ammunition"),L.Shells,0);
    TestEqual(TEXT("Eight shots cost 24 reserve seconds"),R.Battery,96.f);
    L.DrawKnife();
    TestTrue(TEXT("Knife remains available after ammunition is exhausted"),L.Equipped==EEvaWeapon::Knife);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaSystemsTest,"Eva.Rules.AdvancedSystems",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEvaSystemsTest::RunTest(const FString& Parameters)
{
    FEvaRules R; FEvaSystems S;
    TestFalse(TEXT("Low sync rejects overdrive"),S.ActivateOverdrive(R));
    TestEqual(TEXT("Rejected overdrive preserves power"),R.Battery,120.f);
    R.Sync=80;
    TestTrue(TEXT("Threshold sync activates overdrive"),S.ActivateOverdrive(R));
    TestEqual(TEXT("Overdrive costs sync"),R.Sync,60.f);
    TestEqual(TEXT("Overdrive increases damage"),S.DamageScale(),1.6f);
    TestFalse(TEXT("Overdrive cannot stack"),S.ActivateOverdrive(R));
    S.Advance(10,R,0);
    TestEqual(TEXT("Extra drain only applies during eight active seconds"),R.Battery,92.f);
    TestEqual(TEXT("Damage returns to normal"),S.DamageScale(),1.f);
    TestFalse(TEXT("Pulse rejects out of range"),S.AntiField(R,3201));
    TestTrue(TEXT("Pulse accepts range boundary"),S.AntiField(R,3200));
    TestEqual(TEXT("Pulse neutralizes barrier"),R.EnemyField,0.f);
    TestEqual(TEXT("Pulse preserves core health"),R.EnemyHealth,1000.f);
    R.EnemyField=100;
    TestFalse(TEXT("Pulse cooldown blocks repeat"),S.AntiField(R,1000));
    R.bConnected=true;
    S.Advance(1.9f,R,6300); TestTrue(TEXT("Cable allows retreat during grace period"),R.bConnected);
    S.Advance(.1f,R,6000); TestEqual(TEXT("Retreat resets strain"),S.CableStrain,0.f);
    S.Advance(2.f,R,6300); TestFalse(TEXT("Sustained excess tension releases cable"),R.bConnected);
    FEvaLoadout L; L.AcquireCannon(); R.Battery=7;
    TestFalse(TEXT("Full charge requires eight power"),L.FireCannon(R,1));
    TestEqual(TEXT("Rejected charge preserves ammunition"),L.Shells,8);
    R.Battery=8; TestTrue(TEXT("Charge fires at exact power boundary"),L.FireCannon(R,1));
    TestEqual(TEXT("Charge uses one shell"),L.Shells,7);
    TestEqual(TEXT("Charge consumes eight power"),R.Battery,0.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaShooterTest,"Eva.Rules.ShooterAndWhips",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEvaShooterTest::RunTest(const FString& Parameters)
{
    float Distance=0;
    TestTrue(TEXT("Forward ray hits sphere"),FEvaShooter::RaySphere(FVector::ZeroVector,FVector(1,0,0),FVector(100,0,0),10,200,Distance));
    TestEqual(TEXT("Ray returns front surface distance"),Distance,90.f);
    TestFalse(TEXT("Ray misses off-axis target"),FEvaShooter::RaySphere(FVector::ZeroVector,FVector(1,0,0),FVector(100,20,0),10,200,Distance));
    TestFalse(TEXT("Target behind camera cannot be hit"),FEvaShooter::RaySphere(FVector::ZeroVector,FVector(-1,0,0),FVector(100,0,0),10,200,Distance));
    TestFalse(TEXT("Range limits hit"),FEvaShooter::RaySphere(FVector::ZeroVector,FVector(1,0,0),FVector(100,0,0),10,89,Distance));
    TestFalse(TEXT("Zero direction is rejected"),FEvaShooter::RaySphere(FVector::ZeroVector,FVector::ZeroVector,FVector(100,0,0),10,200,Distance));
    TestTrue(TEXT("Whip lane includes edge regardless of altitude"),FEvaShooter::InWhipLane(FVector(500,100,800),FVector::ZeroVector,FVector(1000,0,0),100));
    TestFalse(TEXT("Sidestep escapes whip lane"),FEvaShooter::InWhipLane(FVector(500,101,800),FVector::ZeroVector,FVector(1000,0,0),100));
    TestFalse(TEXT("Whip cannot extend infinitely beyond its endpoint"),FEvaShooter::InWhipLane(FVector(1200,0,0),FVector::ZeroVector,FVector(1000,0,0),100));
    FEvaLoadout L; TestFalse(TEXT("Unowned cannon cannot reload"),L.Reload()); L.AcquireCannon();
    TestFalse(TEXT("Full magazine rejects reload"),L.Reload()); TestEqual(TEXT("Rejected reload preserves reserves"),L.ReserveShells,24);
    L.Shells=2; TestTrue(TEXT("Partial magazine reloads"),L.Reload()); TestEqual(TEXT("Magazine fills to capacity"),L.Shells,8); TestEqual(TEXT("Only missing rounds transfer"),L.ReserveShells,18);
    L.Shells=0; L.ReserveShells=3; L.Reload(); TestEqual(TEXT("Low reserve partially fills magazine"),L.Shells,3); TestEqual(TEXT("Reserve never negative"),L.ReserveShells,0);
    TestFalse(TEXT("No reserve rejects reload"),L.Reload());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaMotionTest,"Eva.Rules.FrameIndependentMotion",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEvaMotionTest::RunTest(const FString& Parameters)
{
    auto Simulate=[](int FPS)
    {
        FVector V=FVector::ZeroVector, P=FVector::ZeroVector;
        for(int I=0;I<FPS;++I) P+=FEvaMotion::Integrate(V,FVector(2200,0,0),17,1.f/FPS);
        for(int I=0;I<FPS;++I) P+=FEvaMotion::Integrate(V,FVector::ZeroVector,23,1.f/FPS);
        return P;
    };
    TestTrue(TEXT("30 and 144 fps travel agree within one centimetre"),Simulate(30).Equals(Simulate(144),1));
    TestTrue(TEXT("60 and 120 fps travel agree within one centimetre"),Simulate(60).Equals(Simulate(120),1));
    FEvaRules R; FEvaMotion M;
    TestTrue(TEXT("First dash available"),M.Dash(R)); TestTrue(TEXT("Second dash available"),M.Dash(R));
    TestFalse(TEXT("Empty dash charges block a third dash"),M.Dash(R));
    TestEqual(TEXT("Rejected dash does not drain power"),R.Battery,116.f);
    M.Advance(1.39f); TestEqual(TEXT("Dash cannot recharge early"),M.DashCharges,0);
    M.Advance(.02f); TestEqual(TEXT("One charge returns at 1.4 seconds"),M.DashCharges,1);
    M.Advance(10); TestEqual(TEXT("Charges cap at two after long frame"),M.DashCharges,2);
    R.Battery=1; TestFalse(TEXT("No power prevents a dash"),M.Dash(R)); TestEqual(TEXT("Failed power check preserves charge"),M.DashCharges,2);
    return true;
}
#endif
