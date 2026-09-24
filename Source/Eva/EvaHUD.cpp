#include "EvaGame.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

namespace HUDColor
{
    const FLinearColor Lime(.55f,1,.06f), White(.85f,.93f,.91f), Muted(.48f,.63f,.63f), Amber(1,.45f,.09f), Cyan(.15f,.85f,1);
    const FLinearColor Panel(.008f,.018f,.026f,.94f);
}
void AEvaHUD::Box(float X,float Y,float W,float H,FLinearColor C) { DrawRect(C,X*SX,Y*SY,W*SX,H*SY); }
void AEvaHUD::Label(const FString& T,float X,float Y,FLinearColor C,float Size)
{ DrawText(T,C,X*SX,Y*SY,GEngine->GetLargeFont(),Size*2.f*FMath::Min(SX,SY),false); }
void AEvaHUD::Meter(float X,float Y,float W,float Fraction,FLinearColor C)
{ Box(X,Y,W,5,FLinearColor(.12f,.16f,.16f,.9f)); Box(X,Y,W*FMath::Clamp(Fraction,0.f,1.f),5,C); }

void AEvaHUD::WorldMarker(FVector Position,const FString& Text,FLinearColor Color)
{
    FVector2D Screen;
    if(auto* PC=GetOwningPlayerController())
        if(PC->ProjectWorldLocationToScreen(Position,Screen) && Screen.X>0 && Screen.X<Canvas->SizeX && Screen.Y>170*SY && Screen.Y<650*SY)
        {
            float X=FMath::Clamp(Screen.X/SX,40.f,1300.f), Y=Screen.Y/SY;
            Box(X-4,Y-4,8,8,Color);
            Label(Text,X+14,Y-8,Color,.48f);
        }
}

void AEvaHUD::DrawChapter(AEvaGameMode* G)
{
    using namespace HUDColor;
    if(G->Chapter==EEvaChapter::Impact)
    {
        const float T=G->ImpactTime;
        Box(0,0,1600,90,FLinearColor(0,0,0,.95f));
        Box(0,756,1600,144,FLinearColor(0,0,0,.95f));
        Label("T H I R D   I M P A C T",60,34,Amber,.9f);
        Label("CINEMATIC / END OF THE WORLD",1060,39,Muted,.55f);
        const TCHAR* Heading=T<14 ? TEXT("ASUKA / LAST STAND") : T<28 ? TEXT("SHINJI / CONTROL LOST") : T<47 ? TEXT("INSTRUMENTALITY") : TEXT("THE LCL SEA");
        const TCHAR* Caption=T<7 ? TEXT("Unit-02 stands alone. Nine white wings close around her.") : T<14 ? TEXT("Her power fades. The flock descends; Unit-02 disappears beneath them.") : T<28 ? TEXT("Shinji sees the silence below. Unit-01 answers with a roar.") : T<38 ? TEXT("The boundaries between people begin to vanish.") : T<47 ? TEXT("A thousand separate lights return to the same orange sea.") : TEXT("No footsteps. No voices. Only the ocean remains.");
        Label(Heading,75,775,Amber,.65f); Label(Caption,75,817,White,.77f);
        Label("ENTER / TITLE     ESC / PAUSE",75,870,Muted,.5f);
        if(T>=60) Label("SEQUENCE COMPLETE",1190,870,Amber,.5f);
        Meter(75,743,1450,T/60.f,Amber);
        return;
    }
    if(G->Chapter==EEvaChapter::Title)
    {
        Box(0,0,760,900,Panel); Box(52,64,44,5,Lime);
        for(int I=0;I<10;++I) Box(52+I*65,68,38,12,FLinearColor(.8f,.035f,.012f));
        Label("N E R V   /   C H A P T E R   0 1",52,105,Muted,.65f);
        Label("E V A",45,150,White,3.5f);
        Label("A R M O R E D   H O R I Z O N",52,265,Lime,.8f);
        Label("O / ENTER FREE ROAM",52,350,White,1.1f);
        Label("Four connected districts. Optional encounters. Your route.",52,410,Muted,.57f);
        Label("M / MAP    N / WAYPOINT    SHIFT / SPRINT",52,445,Muted,.57f);
        Box(52,501,650,1,Muted);
        Label("WASD / MOVE    MOUSE / LOOK    E / INTERACT",52,537,White,.6f);
        Label("ARMORY CANNON  /  SHOULDER KNIFE  /  UMBILICAL POWER",52,580,Muted,.55f);
        Box(52,671,650,66,Lime);
        Label("ENTER / BEGIN CHAPTER",80,693,FLinearColor(.015f,.035f,.025f),.88f);
        Label("SPACE / BATTLE PRACTICE    Z / OVERDRIVE    X / ANTI-A.T.",52,775,White,.5f);
        Label("T / THIRD IMPACT CINEMATIC",52,815,Amber,.63f);
        Label("UNOFFICIAL FAN ADAPTATION  /  TOKYO-3  /  v0.9",52,865,Muted,.45f);
        return;
    }
    if(G->Chapter==EEvaChapter::Complete)
    {
        Box(0,0,1600,900,FLinearColor(.004f,.014f,.025f,.92f));
        Label("CHAPTER 01 / COMPLETE",310,205,Lime,.75f);
        Label("AN UNFAMILIAR CEILING",310,273,White,1.75f);
        Label("The Angel is gone. Shinji still does not know what saved him.",310,365,Muted,.7f);
        Label(FString::Printf(TEXT("BATTLE  %.0f sec     CITY LOSSES  %d"),G->MissionTime,G->BuildingsLost),310,443,White,.7f);
        Box(310,540,980,62,Lime); Label("ENTER / RETURN TO TITLE",338,560,FLinearColor(.01f,.03f,.025f),.8f);
        Label("R / REPLAY THE BATTLE",310,646,White,.65f);
        return;
    }
    if(G->Chapter==EEvaChapter::Entry)
    {
        float Level=FMath::Clamp(G->ChapterTime/3.f,0.f,1.f);
        Box(0,900*(1-Level),1600,900*Level,FLinearColor(.64f,.28f,.015f,.36f));
        Box(60,100,10,580,Cyan); Box(1530,100,10,580,Cyan);
        Label("ENTRY PLUG / LCL PRESSURIZATION",530,120,Cyan,.7f);
        Meter(530,175,540,Level,Cyan);
    }
    if(G->Chapter==EEvaChapter::Awakening)
    {
        if(G->ChapterTime<3) Box(0,0,1600,900,FLinearColor(0,0,0,.96f));
        else if(G->ChapterTime>7) Box(0,0,1600,900,FLinearColor(1,.8f,.45f,FMath::Max(0.f,1-(G->ChapterTime-7)/1.8f)));
        Label("CONTROL SIGNAL / LOST",570,155,Amber,1.f);
    }
    if(G->Chapter!=EEvaChapter::Street) { Box(0,0,1600,72,FLinearColor(0,0,0,.9f)); Box(0,818,1600,82,FLinearColor(0,0,0,.95f)); }
    Box(35,30,1030,53,Panel); Label(G->Objective(),55,48,Lime,.7f);
    if(G->Chapter==EEvaChapter::Street)
    {
        WorldMarker(G->bPhoneUsed ? G->CarPosition+FVector(0,0,230) : G->PhonePosition+FVector(0,0,270),G->bPhoneUsed ? "MISATO'S CAR" : "TELEPHONE",Cyan);
        Label("+",792,445,White,.6f);
    }
    Box(110,681,1380,130,Panel);
    Box(110,681,4,130,Cyan);
    Label(G->StorySpeaker(),139,698,Cyan,.6f);
    Label(G->StoryLine(),139,738,White,.72f);
    Label(G->StoryLineTwo(),139,781,Muted,.5f);
    FString Prompt=G->InteractionPrompt();
    if(!Prompt.IsEmpty()) { Box(440,595,760,54,Panel); Label(Prompt,463,613,Lime,.7f); }
}

void AEvaHUD::DrawHUD()
{
    Super::DrawHUD(); if(!Canvas) return;
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()); if(!G) return;
    if(G->bCompanionTest && G->CompanionStep<=2) return;
    using namespace HUDColor;
    SX=Canvas->SizeX/1600.f; SY=Canvas->SizeY/900.f;
    Box(0,0,1600,5,Lime);
    if(G->Chapter!=EEvaChapter::Battle) DrawChapter(G);
    else
    {
        auto* P=G->Pilot();
        if(P && P->bFirstPerson)
        {
            DrawCockpit(G);
            if(G->bHelp) DrawControls();
            else if(G->bPaused) { Box(570,390,500,90,Panel); Label("PAUSED / ENTER TO RESUME",595,425,White,.7f); }
            if(G->bEnded) { Box(540,390,620,100,Panel); Label("SIGNAL LOST / R TO REDEPLOY",565,428,Amber,.7f); }
            return;
        }
        Box(30,30,330,125,Panel); Label("EVA-01",50,47,Lime,1.f);
        Label("INTEGRITY",50,90,Muted,.5f); Label(FString::Printf(TEXT("%03.0f"),G->Rules.Integrity),274,86,White,.7f);
        Meter(50,123,288,G->Rules.Integrity/100,Lime);
        Box(395,25,790,70,Panel); Label(G->Objective(),418,48,White,.63f);
        if(G->HasCombatTarget()) {
        Label(G->bRamiel ? "RAMIEL / FIFTH ANGEL" : G->bShamshel ? "SHAMSHEL / FOURTH ANGEL" : "SACHIEL / THIRD ANGEL",606,137,Amber,.65f);
        Meter(520,171,560,G->Rules.EnemyHealth/G->EnemyMaxHealth,Amber);
        Meter(520,183,560,G->Rules.EnemyField/100,FLinearColor(.74f,.48f,1));
        Label(G->Rules.EnemyField>0 ? "A.T. FIELD ACTIVE" : "CORE EXPOSED",692,199,G->Rules.EnemyField>0 ? Muted : Lime,.5f);
        }
        Box(1230,30,340,125,Panel);
        Label(G->Rules.bConnected ? "EXTERNAL POWER / RECHARGING" : "INTERNAL BATTERY",1250,47,G->Rules.bConnected ? Lime : Amber,.58f);
        Label(FString::Printf(TEXT("%02d:%02d"),int(G->Rules.Battery)/60,int(G->Rules.Battery)%60),1250,81,White,1.2f);
        Meter(1250,134,295,G->Rules.Battery/120,G->Rules.bConnected ? Lime : Amber);
        Box(30,195,330,196,Panel);
        Box(30,195,6,196,FLinearColor(.8f,.02f,.01f));
        Label("SYNCHRONIZATION",50,213,Muted,.5f);
        Label(FString::Printf(TEXT("%.0f %%"),G->Rules.Sync),245,209,White,.75f);
        Meter(50,247,288,G->Rules.Sync/100,G->Systems.Overdrive>0 ? Amber : Lime);
        Label(G->Systems.Overdrive>0 ? FString::Printf(TEXT("OVERDRIVE / %.1fs"),G->Systems.Overdrive) : G->Systems.OverdriveCooldown>0 ? FString::Printf(TEXT("Z / COOLDOWN %.0fs"),G->Systems.OverdriveCooldown) : "Z / OVERDRIVE [80 SYNC]",50,270,G->Systems.Overdrive>0 ? Amber : White,.52f);
        Label(G->Systems.PulseCooldown>0 ? FString::Printf(TEXT("X / PULSE COOLDOWN %.0fs"),G->Systems.PulseCooldown) : "X / ANTI-A.T. [25 SYNC / 32m]",50,307,White,.48f);
        if(P) Label(FString::Printf(TEXT("CTRL DASH  %d/2   SPACE JUMP"),P->Motion.DashCharges),50,354,Cyan,.46f);
        if(G->Rules.bConnected && P)
        {
            float Length=FVector::Dist2D(P->GetActorLocation(),G->CableAnchor())/100;
            Box(1230,171,340,92,Panel);
            Label(FString::Printf(TEXT("UMBILICAL  %.0f / 62m"),Length),1250,188,G->bCableWarning ? Amber : Muted,.55f);
            Meter(1250,224,295,Length/62,G->bCableWarning ? Amber : Lime);
            if(G->Systems.CableStrain>0) Label(FString::Printf(TEXT("RELEASE IN %.1fs"),2-G->Systems.CableStrain),1250,238,Amber,.5f);
        }
        bool Critical=!G->Rules.bConnected && G->Rules.Battery<20;
        if(G->Telegraph>0 || Critical || G->Systems.Overdrive>0)
        {
            FLinearColor Red(.95f,.03f,.008f);
            Box(1220,290,350,105,FLinearColor(.1f,.005f,.008f,.88f));
            for(int I=0;I<9;++I) Box(1220+I*39,290,23,4,Red);
            Label(Critical ? "INTERNAL POWER LOW" : G->Telegraph>0 ? G->bRamiel ? (G->Telegraph>.65f ? "BEAM TRACKING" : "BEAM LOCKED / EVADE") : G->bShamshel ? (G->AttackCount%2 ? "WHIP SWEEP" : "TWIN THRUST") : "IMPACT INCOMING" : "OVERDRIVE ACTIVE",1234,312,White,.54f);
            Label(G->Telegraph>0 ? "CTRL DASH / SPACE JUMP" : "E / SERVICE STATION",1234,359,Amber,.43f);
        }
        if(P && P->bCharging)
        {
            Box(573,495,454,76,Panel);
            Label(P->CannonCharge>=1 ? "CAPACITOR READY / RELEASE MMB" : "CHARGING / HOLD MMB",595,513,Amber,.58f);
            Meter(595,551,410,P->CannonCharge,Amber);
        }
        const float Spread=P ? 7+P->MoveVelocity.Size()/300+P->RecoilPitch*5 : 7;
        Box(788-Spread,450,10,1,Lime); Box(802+Spread,450,10,1,Lime); Box(800,438-Spread,1,10,Lime); Box(800,452+Spread,1,10,Lime);
        Box(799,449,3,3,White);
        if(P && P->HitMarkerTime>0)
        {
            DrawLine(790*SX,440*SY,796*SX,446*SY,Amber,2); DrawLine(804*SX,454*SY,810*SX,460*SY,Amber,2);
            DrawLine(790*SX,460*SY,796*SX,454*SY,Amber,2); DrawLine(804*SX,446*SY,810*SX,440*SY,Amber,2);
        }
        if(P)
        {
            if(G->HasCombatTarget()) Label(FString::Printf(TEXT("%.0f m"),FVector::Dist2D(P->GetActorLocation(),G->EnemyPosition)/100),830,440,White,.5f);
            Label(P->bLock && G->HasCombatTarget() ? "AIM ASSIST / TAB TO RELEASE" : "MANUAL AIM / TAB TO ASSIST",45,654,Lime,.5f);
            Box(30,687,310,113,Panel);
            Label(P->Loadout.Equipped==EEvaWeapon::Cannon ? "HEAVY CANNON" : P->Loadout.Equipped==EEvaWeapon::Knife ? "PROGRESSIVE KNIFE" : "WEAPON / STOWED",49,706,White,.66f);
            Label(P->Loadout.Equipped==EEvaWeapon::Cannon ? FString::Printf(TEXT("%02d / %02d RESERVE   R RELOAD"),P->Loadout.Shells,P->Loadout.ReserveShells) : "F / DRAW OR STOW    2 / CANNON",49,751,P->Loadout.Shells==0 && P->Loadout.Equipped==EEvaWeapon::Cannon ? Amber : Muted,.5f);
            if(P->ReloadTime>0) { Label("RELOADING",685,505,Amber,.6f); Meter(685,544,230,1-P->ReloadTime/1.3f,Amber); }
            for(int I=0;I<G->Chargers.Num();++I)
                WorldMarker(G->Chargers[I]+FVector(0,0,1450),FString::Printf(TEXT("POWER %02d / %.0f m"),I+1,FVector::Dist2D(P->GetActorLocation(),G->Chargers[I])/100),Lime);
            if(!G->bOpenWorld) WorldMarker(G->DepotPosition+FVector(0,0,3050),FString::Printf(TEXT("ARMORY 07 / %.0f m"),FVector::Dist2D(P->GetActorLocation(),G->DepotApproach)/100),Amber);
        }
        FString Prompt=G->InteractionPrompt();
        if(!Prompt.IsEmpty()) { Box(374,628,910,49,Panel); Label(Prompt,395,646,Lime,.61f); }
        if(G->NoticeTime>0) { Box(370,718,1190,55,Panel); Box(370,718,4,55,Lime); Label(G->Notice,390,739,White,.55f); }
        Box(30,833,1540,42,Panel);
        Label("WASD MOVE   SHIFT SPRINT   SPACE JUMP   CTRL DASH   LMB FIRE   RMB AIM   R RELOAD   V COCKPIT   B SHOULDER   Y ASUKA   F1 CONTROLS",48,847,White,.48f);
        if(G->HitFlash>0) Box(0,0,1600,900,FLinearColor(1,.13f,.02f,G->HitFlash*.2f));
        if(G->bOpenWorld) DrawWorld(G);
    }
    if(G->bHelp) { DrawControls(); return; }
    if(G->bPaused || (G->bEnded && !G->bVictory))
    {
        Box(0,0,1600,900,FLinearColor(.004f,.012f,.019f,.95f));
        Label(G->bPaused ? "SIGNAL SUSPENDED" : "SIGNAL LOST",340,260,White,1.8f);
        Label(G->bPaused ? "ENTER / RESUME" : G->Rules.Battery<=0 ? "Internal reserve depleted. Reconnect at a green power station." : "Integrity critical. Use Ctrl to dash, Space to jump, or Q to guard.",342,370,Muted,.68f);
        Box(342,505,920,63,Lime); Label(G->Chapter==EEvaChapter::Impact ? "R / REPLAY CINEMATIC" : G->bOpenWorld ? "R / REDEPLOY AT CENTRAL SERVICE" : "R / RESTART FROM DEPLOYMENT",370,526,FLinearColor(.01f,.03f,.02f),.8f);
    }
}

void AEvaHUD::DrawWorld(AEvaGameMode* G)
{
    using namespace HUDColor;
    auto* P=G->Pilot(); if(!P || G->Districts.Num()!=4) return;
    FVector Beacon=G->Districts[G->SelectedDistrict]+FVector(0,5500,1800);
    WorldMarker(Beacon,G->DistrictNames[G->SelectedDistrict]+FString::Printf(TEXT(" / %.0fm"),FVector::Dist2D(P->GetActorLocation(),Beacon)/100),Amber);
    if(G->SelectedDistrict==0)
    {
        WorldMarker(G->CityArmoryPosition+FVector(0,0,4400),TEXT("07 / ARMORY"),Amber);
        int Nearest=INDEX_NONE; float Distance=22000;
        for(int I=0;I<G->SupplyPositions.Num();++I) if(!(G->SupplyMask&(1<<I)))
        {
            float D=FVector::Dist2D(P->GetActorLocation(),G->SupplyPositions[I]);
            if(D<Distance) { Distance=D; Nearest=I; }
        }
        if(Nearest!=INDEX_NONE) WorldMarker(G->SupplyPositions[Nearest]+FVector(0,0,1800),FString::Printf(TEXT("SUPPLY 0%d / %.0fm"),Nearest+1,Distance/100),Cyan);
    }
    int Count=0; for(int I=0;I<4;++I) if(G->SurveyMask&(1<<I)) ++Count;
    if(G->Wingman && G->Wingman->bDeployed) { Box(1190,417,380,73,Panel); Label("02 / ASUKA / "+G->Wingman->OrderName(),1205,431,Amber,.44f); Label(FString::Printf(TEXT("INTEGRITY %.0f  /  Y CALL"),G->Wingman->Integrity),1205,461,White,.48f); }
    if(!G->bWorldEncounter) { Box(395,91,790,38,Panel);
    Label(FString::Printf(TEXT("SURVEY %d/4   CONTRACTS %d   CITY LOSSES %d   M MAP / N WAYPOINT / F1 CONTROLS"),Count,G->CompletedContracts,G->BuildingsLost),412,102,Muted,.48f); }
    if(!G->bWorldMap) return;
    Box(270,155,1070,628,FLinearColor(.006f,.014f,.022f,.98f));
    Label("OPERATIONS MAP / FREE ROAM",309,182,Lime,.9f);
    Label("N / CYCLE WAYPOINT    M / CLOSE    HOME / TITLE    LIVE MAP",309,224,Muted,.55f);
    const float X=380,Y=280,W=730,H=390;
    for(int I=0;I<5;++I) { Box(X+W*I/4,Y,1,H,Muted); Box(X,Y+H*I/4,W,1,Muted); }
    // Structures as footprints: intact in slate, burning in amber, collapsed in red.
    for(const auto& B:G->Buildings) if(B.District>=0)
    {
        const FVector Lot=B.Base-G->WorldCenter;
        const float MX=X+(Lot.X/81000+.5f)*W, MY=Y+(.5f-Lot.Y/81000)*H;
        const float SW=FMath::Max(3.f,B.Extent.X*2/81000*W), SH=FMath::Max(3.f,B.Extent.Y*2/81000*H);
        Box(MX-SW*.5f,MY-SH*.5f,SW,SH,B.bDestroyed ? FLinearColor(.75f,.08f,.05f,.9f):B.DamageStage ? FLinearColor(1,.45f,.09f,.8f):FLinearColor(.2f,.28f,.3f,.8f));
    }
    for(int I=0;I<4;++I)
    {
        FVector Local=G->Districts[I]-G->WorldCenter;
        float MX=X+(Local.X/81000+.5f)*W, MY=Y+(.5f-Local.Y/81000)*H;
        Box(MX-6,MY-6,12,12,I==G->SelectedDistrict ? Amber:Lime);
        Label(G->DistrictNames[I],MX+15,MY-9,White,.58f);
        Label(I==3 ? "RAMIEL / BEAM ENCOUNTER" : I==1 ? "SHAMSHEL / WHIP ENCOUNTER" : "SACHIEL / SERVICE + ENCOUNTER",MX-60,MY+25,Muted,.39f);
    }
    FVector Local=P->GetActorLocation()-G->WorldCenter;
    for(int I=0;I<G->SupplyPositions.Num();++I)
    {
        FVector Supply=G->SupplyPositions[I]-G->WorldCenter;
        float MX=X+(Supply.X/81000+.5f)*W, MY=Y+(.5f-Supply.Y/81000)*H;
        Box(MX-3,MY-3,6,6,G->SupplyMask&(1<<I) ? Muted:Cyan);
    }
    float PX=X+(Local.X/81000+.5f)*W,PY=Y+(.5f-Local.Y/81000)*H;
    Box(PX-4,PY-4,8,8,Cyan); Label("EVA-01",PX+10,PY+5,Cyan,.5f);
    Label("NORTH",690,256,Muted,.45f);
    Label(FString::Printf(TEXT("GREEN / SERVICE    AMBER / SELECTED    CYAN / YOU    RED / COLLAPSED %d"),G->BuildingsLost),309,700,White,.55f);
    int Supplies=0; for(int I=0;I<3;++I) if(G->SupplyMask&(1<<I)) ++Supplies;
    Label(FString::Printf(TEXT("CENTRAL / %d OF 3 SUPPLIES   ARMORY SOUTH   EVACUATION ROUTE EAST"),Supplies),309,747,Cyan,.49f);
}

void AEvaHUD::DrawControls()
{
    using namespace HUDColor;
    Box(0,0,1600,900,FLinearColor(.004f,.009f,.015f,.97f));
    Box(75,64,1450,5,Lime);
    Label("EVA-01 / PILOT TRAINING",90,95,White,1.3f);
    Label("SIMULATION PAUSED / F1 OR ESC TO RETURN     Y / CALL ASUKA IN FREE ROAM",90,162,Cyan,.62f);
    const TCHAR* Keys[]={TEXT("W A S D"),TEXT("SHIFT"),TEXT("SPACE"),TEXT("LEFT CTRL"),TEXT("MOUSE / RMB"),TEXT("LMB"),TEXT("MMB"),TEXT("R"),TEXT("F / 1 / 2"),TEXT("V / B"),TEXT("Q"),TEXT("Z / X"),TEXT("E / C"),TEXT("TAB"),TEXT("M / N"),TEXT("ESC / HOME")};
    const TCHAR* Actions[]={TEXT("Move and strafe"),TEXT("Sprint / keep momentum"),TEXT("Powered jump / 5 reserve"),TEXT("Directional dash / two charges"),TEXT("Look / hold to aim"),TEXT("Hold to fire / knife combo"),TEXT("Hold and release charged shot"),TEXT("Reload / hold fire to resume"),TEXT("Knife / knife / cannon"),TEXT("Cockpit view / swap shoulder"),TEXT("Hold directional A.T. guard"),TEXT("Overdrive / anti-A.T. pulse"),TEXT("Interact or service / release cable"),TEXT("Toggle aim assist"),TEXT("World map / next waypoint"),TEXT("Pause / return to title")};
    for(int I=0;I<16;++I)
    {
        float X=90+(I/8)*735, Y=240+(I%8)*61;
        Label(Keys[I],X,Y,Lime,.67f); Label(Actions[I],X+205,Y+2,White,.55f);
        Box(X,Y+42,665,1,FLinearColor(.1f,.18f,.2f,.6f));
    }
    Label("MOVE / AIM / FIRE: build speed, dash across locked attacks, then hit an exposed core.",90,761,Amber,.6f);
    Label("Green pylons restore power and ammunition. Amber beacons start encounters. R redeploys from pause or defeat.",90,813,Muted,.5f);
}

void AEvaHUD::DrawCockpit(AEvaGameMode* G)
{
    using namespace HUDColor;
    auto* P=G->Pilot(); if(!P) return;
    Box(0,0,1600,900,FLinearColor(.65f,.24f,.015f,.016f));
    Label("NERV / DIRECT NEURAL INTERFACE",60,32,Muted,.48f);
    Label("ENTRY PLUG 01",60,64,White,.95f);
    Label("PANORAMIC LINK / NORMAL",1190,40,Cyan,.5f);
    Label(G->Rules.bConnected ? "UMBILICAL CONNECTED" : FString::Printf(TEXT("INTERNAL RESERVE %02d:%02d"),int(G->Rules.Battery)/60,int(G->Rules.Battery)%60),1170,74,G->Rules.bConnected ? Lime:Amber,.6f);
    Meter(1170,111,365,G->Rules.Battery/120,G->Rules.bConnected ? Lime:Amber);
    for(int S:{-1,1}) for(int I=0;I<9;++I)
    {
        float X=S<0 ? 54:1546, Y=288+I*32;
        Box(S<0 ? X:X-18,Y,I%2 ? 9:18,1,FLinearColor(.23f,.64f,.62f,.6f));
    }
    DrawLine(740*SX,450*SY,772*SX,450*SY,Cyan,1); DrawLine(828*SX,450*SY,860*SX,450*SY,Cyan,1);
    Box(799,435,1,9,White); Box(799,456,1,9,White); Box(797,448,5,5,White);
    if(P->HitMarkerTime>0) { Label("X",783,432,Amber,.8f); }
    if(G->HasCombatTarget())
    {
        Label(G->bRamiel ? "TARGET / RAMIEL":G->bShamshel ? "TARGET / SHAMSHEL":"TARGET / SACHIEL",625,130,Amber,.58f);
        Meter(580,169,440,G->Rules.EnemyHealth/G->EnemyMaxHealth,Amber);
        Meter(580,181,440,G->Rules.EnemyField/100,Cyan);
        Label(G->Rules.EnemyField<=0 ? "CORE EXPOSED / FIRE":"A.T. FIELD DETECTED",665,199,White,.44f);
        Label(FString::Printf(TEXT("%.0f m"),FVector::Dist(P->GetActorLocation(),G->EnemyPosition)/100),872,443,White,.48f);
    }
    if(G->Telegraph>0)
    {
        Box(581,266,438,43,FLinearColor(.1f,.007f,.005f,.74f));
        Label(G->bRamiel ? G->Telegraph>.65f ? "BEAM TRACKING":"BEAM LOCKED / EVADE":"INCOMING / EVADE OR GUARD",610,281,Amber,.57f);
    }
    Label(FString::Printf(TEXT("INTEGRITY  %.0f"),G->Rules.Integrity),76,579,White,.66f); Meter(76,616,235,G->Rules.Integrity/100,Lime);
    Label(FString::Printf(TEXT("SYNC  %.0f%%"),G->Rules.Sync),76,644,Cyan,.64f); Meter(76,680,235,G->Rules.Sync/100,Cyan);
    Label(P->Loadout.Equipped==EEvaWeapon::Cannon ? "PALLET / CANNON":P->Loadout.Equipped==EEvaWeapon::Knife ? "PROGRESSIVE KNIFE":"WEAPON STOWED",1200,579,White,.57f);
    Label(FString::Printf(TEXT("%02d / %02d    R RELOAD"),P->Loadout.Shells,P->Loadout.ReserveShells),1200,613,Amber,.62f);
    Label(FString::Printf(TEXT("DASH  %d/2   /   SPACE JUMP"),P->Motion.DashCharges),1200,655,Cyan,.48f);
    if(P->ReloadTime>0) { Label("RELOADING",710,520,Amber,.6f); Meter(690,554,220,1-P->ReloadTime/1.3f,Amber); }
    if(P->bCharging) { Label("CAPACITOR / HOLD MMB",663,520,Amber,.6f); Meter(660,554,280,P->CannonCharge,Amber); }
    if(G->Wingman && G->Wingman->bDeployed) Label("02 / ASUKA / "+G->Wingman->OrderName()+" / Y CALL",545,784,Amber,.48f);
    FString Prompt=G->InteractionPrompt();
    if(!Prompt.IsEmpty()) Label(Prompt,430,610,Lime,.53f);
    if(G->NoticeTime>0) { Box(380,817,1150,27,FLinearColor(.005f,.02f,.024f,.6f)); Label(G->Notice,395,825,Muted,.4f); }
    Box(230,850,1180,38,FLinearColor(.005f,.015f,.02f,.62f));
    Label("WASD MOVE   SHIFT SPRINT   SPACE JUMP   CTRL DASH   LMB FIRE   RMB AIM   V EXTERNAL   Y ASUKA   F1 CONTROLS",250,868,White,.47f);
    if(G->HitFlash>0) Box(0,0,1600,900,FLinearColor(1,.1f,.015f,G->HitFlash*.25f));
    if(G->bWorldMap) DrawWorld(G);
}
