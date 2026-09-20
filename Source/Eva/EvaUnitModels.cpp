#include "EvaGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

// Shared articulated construction with distinct silhouettes, masks and armor for each unit.
void AEvaGameMode::BuildUnit(USceneComponent* Root,bool Red,TArray<UStaticMeshComponent*>& Parts,TArray<UStaticMeshComponent*>& Limbs,TArray<USceneComponent*>& Knees,UStaticMeshComponent*& Hatch)
{
    const FLinearColor Paint=Red ? FLinearColor(.65f,.025f,.018f):FLinearColor(.38f,.13f,.63f);
    const FLinearColor Light=Red ? FLinearColor(.9f,.07f,.025f):FLinearColor(.52f,.24f,.76f);
    const FLinearColor Dark(.019f,.024f,.034f), Steel(.22f,.27f,.32f), Silver(.52f,.61f,.65f);
    const FLinearColor Accent=Red ? FLinearColor(.95f,.31f,.04f):FLinearColor(.42f,.85f,.065f), Gold(.93f,.51f,.06f);
    auto Part=[&](const FString& Kind,FVector P,FVector S,FLinearColor C,float Glow=0.f,USceneComponent* Parent=nullptr)
    { auto* M=Shape(Kind,P,S,C,Glow,Parent ? Parent:Root); Parts.Add(M); return M; };
    // Rib cage / abdominal plates over a narrow flexible waist.
    Part("ArmorChest08",FVector(-13,0,350),FVector(3.25f,4.05f,3.2f),Dark);
    Part("ArmorWaist08",FVector(0,0,115),FVector(2.4f,2.8f,2.3f),Dark);
    Part("ArmorPelvis08",FVector(0,0,-30),FVector(2.8f,3.35f,2.25f),Paint);
    Part("ArmorPelvis08",FVector(100,0,-65),FVector(.65f,1.85f,1.8f),Red ? Steel:Light);
    for(int I=0;I<3;++I)
    {
        auto* Rib=Part("ArmorChest08",FVector(72-I*5,0,95+I*84),FVector(1.1f,2.75f+I*.23f,.86f),I%2 ? Paint:Light);
        Rib->SetRelativeRotation(FRotator(10,0,0));
    }
    Part("ArmorPlate",FVector(-102,0,330),FVector(.7f,2.6f,3.2f),Steel);
    Part("Cylinder",FVector(0,0,530),FVector(.8f,.86f,1.1f),Dark);
    for(int S:{-1,1})
    {
        // Split breastplate, clavicle vents and green/orange collar housings.
        auto* Chest=Part("ArmorChest08",FVector(83,S*106,394),FVector(1.5f,2.12f,1.78f),Red ? Paint:Silver);
        Chest->SetRelativeRotation(FRotator(0,0,S*17));
        Part("ArmorPlate",FVector(130,S*102,442),FVector(.48f,1.65f,.66f),Red ? Accent:Steel)->SetRelativeRotation(FRotator(0,0,S*18));
        for(int I=0;I<3;++I) Part("ArmorPlate",FVector(153,S*109,404-I*24),FVector(.05f,1.1f,.105f),Dark);
        Part("ArmorPlate",FVector(30,S*75,514),FVector(1.1f,.65f,1.16f),Gold)->SetRelativeRotation(FRotator(0,0,S*20));
        auto* Pylon=Part("ArmorPylon08",FVector(-60,S*249,695),FVector(1.4f,1.45f,6.8f),Red ? Paint:Dark);
        if(S==1) Hatch=Pylon;
        Part("ArmorPylon08",FVector(-4,S*250,727),FVector(.18f,.79f,5.65f),Red ? Steel:Paint);
        Part("ArmorPlate",FVector(-32,S*250,978),FVector(.68f,.72f,.68f),Accent);
        Part(Red ? "ArmorShoulder02":"ArmorPlate",FVector(8,S*258,451),FVector(1.7f,1.48f,Red ? 2.85f:1.85f),Red ? Light:Accent);
        Part("ArmorPlate",FVector(78,S*258,459),FVector(.24f,1.1f,1.25f),Red ? Paint:Accent);
        auto* Arm=Part("Sphere",FVector(0,S*299,370),FVector(.89f),Dark); Limbs.Add(Arm);
        Part("ArmorThigh08",FVector(0,0,-104),FVector(1.48f,1.36f,2.23f),Paint,0,Arm);
        Part("ArmorPlate",FVector(45,0,-175),FVector(.32f,.74f,.64f),Accent,0,Arm);
        Part("Sphere",FVector(0,0,-225),FVector(.69f),Steel,0,Arm);
        Part("ArmorForearm08",FVector(3,0,-350),FVector(1.65f,1.5f,2.55f),Red ? Paint:Dark,0,Arm);
        Part("ArmorPlate",FVector(51,0,-331),FVector(.28f,.82f,1.93f),Red ? Steel:Accent,0,Arm);
        Part("ArmorPlate",FVector(-19,0,-354),FVector(.68f,1.28f,2.06f),Paint,0,Arm);
        Part("ArmorPlate",FVector(18,0,-504),FVector(.88f,1.16f,.93f),Paint,0,Arm);
        for(int I=0;I<4;++I)
        {
            Part("Sphere",FVector(43,-34+I*22,-515),FVector(.15f),Gold,0,Arm);
            Part("ArmorPlate",FVector(26,-34+I*22,-559),FVector(.22f,.19f,.66f),Silver,0,Arm);
            Part("ArmorPlate",FVector(39,-34+I*22,-595+FMath::Abs(I-1)*6),FVector(.18f,.17f,.32f),Paint,0,Arm);
        }
        Part("ArmorPlate",FVector(54,S*60,-539),FVector(.3f,.24f,.56f),Steel,0,Arm)->SetRelativeRotation(FRotator(0,0,S*27));
        auto* Leg=Part("Sphere",FVector(0,S*106,-98),FVector(.93f),Dark); Limbs.Add(Leg);
        Part("ArmorThigh08",FVector(0,0,-145),FVector(2.2f,1.95f,3.1f),Paint,0,Leg);
        Part("ArmorPlate",FVector(48,S*20,-88),FVector(.7f,1.38f,1.5f),Light,0,Leg);
        Part("Sphere",FVector(0,0,-310),FVector(.88f),Dark,0,Leg);
        Part("ArmorPlate",FVector(48,0,-315),FVector(.8f,1.2f,1.3f),Red ? Paint:Dark,0,Leg);
        Part("ArmorPlate",FVector(82,0,-275),FVector(.17f,.58f,.54f),Red ? Steel:Gold,0,Leg);
        auto* Knee=NewSceneRoot(FVector::ZeroVector); Knee->AttachToComponent(Leg,FAttachmentTransformRules::KeepRelativeTransform); Knee->SetRelativeLocation(FVector(0,0,-310)); Knees.Add(Knee);
        Part("ArmorCalf08",FVector(-5,0,-141),FVector(2.08f,1.72f,2.88f),Paint,0,Knee);
        Part("ArmorPlate",FVector(46,0,-134),FVector(.38f,.8f,2.35f),Red ? Steel:Dark,0,Knee);
        Part("ArmorPlate",FVector(51,0,-185),FVector(.12f,.5f,1.23f),Accent,0,Knee);
        Part("ArmorBoot08",FVector(43,0,-288),FVector(2.75f,1.63f,1.12f),Paint,0,Knee);
        Part("ArmorPlate",FVector(105,0,-274),FVector(1.05f,1.19f,.72f),Accent,0,Knee);
        Part("ArmorBoot08",FVector(43,0,-325),FVector(2.85f,1.67f,.24f),Red ? Dark:Gold,0,Knee);
        Part("Sphere",FVector(39,S*51,-249),FVector(.22f),Dark,0,Knee);
        if(Red) Part("ArmorPlate",FVector(4,S*76,-135),FVector(1.25f,.3f,2.17f),Steel,0,Leg);
    }
    // Unit-01's horned helmet and pale jaw / Unit-02's white cheeks and four optics.
    Part(Red ? "ArmorHelmet02":"ArmorHelmet01",FVector(15,0,652),FVector(2.2f,1.9f,2.16f),Paint);
    Part("ArmorPlate",FVector(109,0,613),FVector(.67f,1.15f,.96f),Red ? Paint:Silver);
    Part("ArmorPlate",FVector(135,0,650),FVector(.2f,1.4f,.29f),Dark);
    Part("ArmorPlate",FVector(120,0,592),FVector(.36f,.67f,.55f),Paint);
    if(!Red) Part("ArmorHorn",FVector(83,0,817),FVector(.43f,.43f,2.67f),Paint)->SetRelativeRotation(FRotator(15,0,0));
    for(int S:{-1,1})
    {
        Part("ArmorPlate",FVector(111,S*49,678),FVector(.1f,.69f,.2f),Red ? FLinearColor(.35f,1,.42f):Gold,1.1f)->SetRelativeRotation(FRotator(0,0,S*15));
        Part("ArmorPlate",FVector(108,S*54,694),FVector(.29f,.83f,.2f),Light)->SetRelativeRotation(FRotator(0,0,S*19));
        Part("ArmorPlate",FVector(72,S*66,618),FVector(.75f,.39f,.81f),Red ? Silver:Paint);
        Part("ArmorHorn",FVector(-40,S*74,700),FVector(.5f,.35f,1.1f),Red ? Silver:Dark)->SetRelativeRotation(FRotator(-30,0,S*20));
        if(Red)
        {
            Part("Sphere",FVector(112,S*46,646),FVector(.2f,.29f,.16f),FLinearColor(.35f,1,.42f),1.1f);
            Part("ArmorHorn",FVector(0,S*57,769),FVector(.33f,.25f,1.11f),Silver)->SetRelativeRotation(FRotator(0,0,-S*25));
        }
        auto* Mark=NewObject<UTextRenderComponent>(Root->GetOwner()); Mark->SetupAttachment(Root); Mark->RegisterComponent();
        Mark->SetRelativeLocation(FVector(93,S*250,564)); Mark->SetRelativeRotation(FRotator(0,0,0));
        Mark->SetText(FText::FromString(Red ? "02":"01")); Mark->SetTextRenderColor(Red ? FColor(255,183,86):FColor(208,225,191));
        Mark->SetWorldSize(44); Mark->SetHorizontalAlignment(EHTA_Center); Mark->SetCastShadow(false);
    }
    // Long legs and a compact rib cage preserve the reference silhouettes.
    for(auto* M:Parts) if(M->GetAttachParent()==Root)
    {
        const bool Leg=Limbs.Num()==4 && (M==Limbs[1] || M==Limbs[3]);
        const bool Arm=Limbs.Num()==4 && (M==Limbs[0] || M==Limbs[2]);
        FVector Position=M->GetRelativeLocation(), Scale=M->GetRelativeScale3D();
        Position.Z=Leg ? 60.f:(Position.Z+98.f)*.82f+60.f;
        Scale.Z*=Leg ? 1.24f:Arm ? .98f:.82f;
        M->SetRelativeLocation(Position); M->SetRelativeScale3D(Scale);
    }

}
