#include "EvaGame.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

FVector AEvaPawn::ReadMovement() const
{
    auto* PC=Cast<APlayerController>(Controller); if(!PC) return FVector::ZeroVector;
    FVector Forward=FRotator(0,CameraYaw,0).Vector(), Right=FRotationMatrix(FRotator(0,CameraYaw,0)).GetUnitAxis(EAxis::Y);
    return (Forward*(float(PC->IsInputKeyDown(EKeys::W))-float(PC->IsInputKeyDown(EKeys::S)))
        +Right*(float(PC->IsInputKeyDown(EKeys::D))-float(PC->IsInputKeyDown(EKeys::A)))).GetClampedToMaxSize(1);
}

void AEvaPawn::MoveEva(FVector Direction,float Speed,float Dt)
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()); if(!G) return;
    const int Steps=FMath::Clamp(FMath::CeilToInt(Dt/(1.f/120.f)),1,32);
    const float Step=Dt/Steps;
    for(int I=0;I<Steps;++I)
    {
        FVector Delta=FEvaMotion::Integrate(MoveVelocity,Direction*Speed,Direction.IsNearlyZero() ? 23.f:17.f,Step);
        if(DodgeTime>0)
        {
            const float DashStep=FMath::Min(Step,DodgeTime);
            const float SpeedScale=.72f+.28f*FMath::Sin(FMath::Clamp(DodgeTime/.32f,0.f,1.f)*PI*.5f);
            Delta=DodgeDirection*5500*SpeedScale*DashStep+MoveVelocity*(Step-DashStep);
            DodgeTime=FMath::Max(0.f,DodgeTime-Step);
            if(DodgeTime<=0) MoveVelocity=DodgeDirection*FMath::Max(Speed,2200.f);
        }
        if(!bGrounded && !bHuman) { Delta.Z=VerticalSpeed*Step-4000*Step*Step; VerticalSpeed-=8000*Step; }
        FVector Next=GetActorLocation()+Delta;
        Next.X=G->bOpenWorld ? FMath::Clamp(Next.X,G->WorldCenter.X-40500,G->WorldCenter.X+40500):FMath::Clamp(Next.X,-11200.f,11200.f);
        Next.Y=G->bOpenWorld ? FMath::Clamp(Next.Y,-40500.f,40500.f):FMath::Clamp(Next.Y,-9500.f,10800.f);
        FHitResult Hit; SetActorLocation(Next,true,&Hit);
        if(Hit.bBlockingHit)
        {
            if(Hit.Normal.Z>.6f && VerticalSpeed<0)
            {
                if(VerticalSpeed<-1600)
                {
                    LandingKick=2.2f;
                    G->Pulse(GetActorLocation()-FVector(0,0,740),FLinearColor(.24f,.22f,.17f),7);
                }
                bGrounded=true; VerticalSpeed=0;
            }
            else if(Hit.Normal.Z<-.5f) VerticalSpeed=FMath::Min(0.f,VerticalSpeed);
            FVector Slide=FVector::VectorPlaneProject(Delta*(1-Hit.Time),Hit.Normal);
            SetActorLocation(GetActorLocation()+Slide,true);
        }
    }
    if(bGrounded && !bHuman)
    {
        FHitResult Floor; FCollisionQueryParams Query; Query.AddIgnoredActor(this);
        if(!GetWorld()->LineTraceSingleByChannel(Floor,GetActorLocation(),GetActorLocation()-FVector(0,0,820),ECC_Visibility,Query)) bGrounded=false;
    }
}

void AEvaPawn::Jump()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()); if(!G) return;
    if(G->Chapter==EEvaChapter::Title) { G->StartBattle(); return; }
    if(!bHuman && G->IsActive() && !G->bWorldMap && bGrounded && G->Rules.Spend(5))
    { bGrounded=false; VerticalSpeed=4000; G->Pulse(GetActorLocation()-FVector(0,0,740),FLinearColor(.18f,.22f,.2f),5); }
}

void AEvaPawn::TogglePerspective()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode());
    if(G && G->IsActive() && !bHuman) { bFirstPerson=!bFirstPerson; UpdatePerspective(); }
}
void AEvaPawn::SwapShoulder() { if(!bHuman && !bFirstPerson) ShoulderSide*=-1; }
void AEvaPawn::ToggleHelp()
{
    if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()))
    {
        if(!G->bHelp) { bHelpWasPaused=G->bPaused; G->bHelp=true; G->bPaused=true; bFireHeld=false; bCharging=false; }
        else { G->bHelp=false; G->bPaused=bHelpWasPaused; }
    }
}
void AEvaPawn::UpdatePerspective()
{
    const bool Cockpit=bFirstPerson && !bHuman;
    if(EvaRig && EvaRig->IsVisible()!=(!Cockpit && !bHuman)) EvaRig->SetVisibility(!Cockpit && !bHuman,true);
    if(CockpitRoot && CockpitRoot->IsVisible()!=Cockpit) CockpitRoot->SetVisibility(Cockpit,true);
    if(Cockpit && KnifeRoot && KnifeRoot->IsVisible()) KnifeRoot->SetVisibility(false,true);
    if(Cockpit && CannonRoot && CannonRoot->IsVisible()) CannonRoot->SetVisibility(false,true);
    Boom->bEnableCameraLag=!Cockpit && !bHuman;
    if(Cockpit) { Boom->TargetArmLength=0; Boom->SocketOffset=FVector::ZeroVector; }
}

void AEvaPawn::BuildCockpit()
{
    auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode()); if(!G) return;
    CockpitRoot=G->NewSceneRoot(FVector::ZeroVector);
    CockpitRoot->AttachToComponent(Camera,FAttachmentTransformRules::KeepRelativeTransform);
    const FLinearColor Shell(.035f,.048f,.06f), Edge(.075f,.11f,.13f), Orange(.95f,.28f,.035f), Cyan(.12f,.8f,.71f), White(.65f,.73f,.77f);
    auto Part=[&](const FString& Kind,FVector P,FVector S,FLinearColor C,float Glow=.2f)
    { auto* M=G->Shape(Kind,P,S,C,Glow,CockpitRoot); M->SetCastShadow(false); M->SetReceivesDecals(false); return M; };
    auto Bar=[&](FVector A,FVector B,float Width,FLinearColor C,float Glow)
    { auto* M=Part("Cube",(A+B)*.5f,FVector(Width,Width,(B-A).Size()/100),C,Glow); M->SetRelativeRotation(FRotationMatrix::MakeFromZ(B-A).Rotator()); };
    // An elliptical pressure ring surrounds an unobstructed panoramic neural display.
    for(int I=0;I<40;++I)
    {
        const float A=2*PI*I/40, B=2*PI*(I+1)/40;
        Bar(FVector(94,FMath::Cos(A)*73,FMath::Sin(A)*46),FVector(94,FMath::Cos(B)*73,FMath::Sin(B)*46),.025f,Edge,.3f);
        if(I%5!=0) Bar(FVector(89,FMath::Cos(A)*71,FMath::Sin(A)*44),FVector(89,FMath::Cos(B)*71,FMath::Sin(B)*44),.008f,Orange,1.1f);
    }
    for(int S:{-1,1})
    {
        Part("ArmorPlate",FVector(81,S*52,-41),FVector(.46f,.59f,.2f),Shell);
        // Compact instrument display, angled toward the pilot.
        auto* Console=Part("Cube",FVector(75,S*39,-24),FVector(.025f,.3f,.135f),FLinearColor(.015f,.11f,.12f),.65f);
        Console->SetRelativeRotation(FRotator(0,-S*12,0));
        for(int I=0;I<5;++I)
        {
            Part("Cube",FVector(73.5f,S*39,-19.5f-I*2.1f),FVector(.008f,.2f-I*.023f,.007f),I==0 ? Orange:Cyan,1.1f);
            Part("Sphere",FVector(73,S*(26+I*4),-31),FVector(.014f),I==4 ? Orange:Cyan,1.1f);
        }
        Part("Cylinder",FVector(64,S*25,-26),FVector(.055f,.055f,.18f),Edge)->SetRelativeRotation(FRotator(-22,0,0));
        Part("Sphere",FVector(61,S*25,-19),FVector(.075f,.06f,.055f),Shell);
        Part("Sphere",FVector(58,S*24,-23),FVector(.105f,.1f,.065f),White);
        Bar(FVector(26,S*17,-39),FVector(57,S*24,-24),.08f,White,.15f);
        Part("Cube",FVector(55,S*24,-23),FVector(.06f,.1f,.024f),Shell);
        // Seat bolsters sit below peripheral vision rather than obstructing the sightline.
        Part("ArmorPlate",FVector(28,S*34,-40),FVector(.38f,.22f,.24f),Shell);
    }
    Part("ArmorPlate",FVector(65,0,-47),FVector(.4f,.95f,.12f),Shell);
    Part("Cube",FVector(61,0,-40),FVector(.015f,.4f,.06f),FLinearColor(.012f,.14f,.15f),.9f);
    for(int I=-3;I<=3;++I) Part("Cube",FVector(60,I*4,-40),FVector(.008f,.014f,.032f),Cyan,1.4f);
    CockpitRoot->SetVisibility(false,true);
}
