#include "EvaGame.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

class SEvaComms : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SEvaComms) {} SLATE_ARGUMENT(AEvaGameMode*, Game) SLATE_END_ARGS()
    TWeakObjectPtr<AEvaGameMode> Game;
    TSharedPtr<SEditableTextBox> Input;
    TSharedPtr<SScrollBox> Transcript;
    void Construct(const FArguments& Args)
    {
        Game=Args._Game;
        const FLinearColor Red(.95f,.16f,.055f), Pale(.83f,.91f,.9f);
        ChildSlot
        [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.002f,.007f,.012f,.88f)).Padding(35)
          .HAlign(HAlign_Center).VAlign(VAlign_Center)
          [ SNew(SBox).WidthOverride(960).HeightOverride(670)
            [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.018f,.028f,.036f)).Padding(28)
              [ SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
                [ SNew(STextBlock).Text(FText::FromString("NERV / SECURE PILOT LINK    02" )).ColorAndOpacity(Red).Font(FCoreStyle::GetDefaultFontStyle("Bold",22)) ]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,5)
                [ SNew(STextBlock).Text(FText::FromString("ASUKA LANGLEY / UNIT-02")).ColorAndOpacity(Pale).Font(FCoreStyle::GetDefaultFontStyle("Bold",30)) ]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)
                [ SNew(STextBlock).Text(FText::FromString("LOCAL TACTICAL DIALOGUE  /  SIMULATION PAUSED  /  ESC TO END CALL")).ColorAndOpacity(FLinearColor(.4f,.7f,.72f)).Font(FCoreStyle::GetDefaultFontStyle("Regular",11)) ]
                +SVerticalBox::Slot().FillHeight(1).Padding(0,0,0,12)
                [ SAssignNew(Transcript,SScrollBox) ]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                [ SNew(SHorizontalBox)
                  +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[OrderButton("Follow me")]
                  +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[OrderButton("Hold here")]
                  +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[OrderButton("Cover me")]
                  +SHorizontalBox::Slot().FillWidth(1)[OrderButton("Status")]
                ]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                [ SAssignNew(Input,SEditableTextBox).HintText(FText::FromString("Talk to Asuka or give an order... (Enter to send)"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).OnTextCommitted(this,&SEvaComms::Committed) ]
                +SVerticalBox::Slot().AutoHeight()
                [ SNew(SHorizontalBox)
                  +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString("Try: What is the plan? / Are you okay? / Regroup / Thanks" )).AutoWrapText(true).ColorAndOpacity(Pale)]
                  +SHorizontalBox::Slot().AutoWidth().Padding(12,0)[SNew(SButton).Text(FText::FromString("SEND")).OnClicked_Lambda([this]() { Send(Input->GetText().ToString()); return FReply::Handled(); })]
                  +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString("END CALL")).OnClicked_Lambda([this]() { if(Game.IsValid()) Game->CloseComms(); return FReply::Handled(); })]
                ]
              ]
            ]
          ]
        ];
        Refresh();
    }
    TSharedRef<SWidget> OrderButton(FString Text)
    {
        return SNew(SButton).Text(FText::FromString(Text)).ContentPadding(FMargin(10,8)).HAlign(HAlign_Center).OnClicked_Lambda([this,Text]() { Send(Text); return FReply::Handled(); });
    }
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override
    {
        if(Event.GetKey()==EKeys::Escape) { if(Game.IsValid()) Game->CloseComms(); return FReply::Handled(); }
        return SCompoundWidget::OnKeyDown(Geometry,Event);
    }
    void Committed(const FText& Text,ETextCommit::Type Type) { if(Type==ETextCommit::OnEnter) Send(Text.ToString()); }
    void Send(FString Text)
    {
        Text=Text.TrimStartAndEnd().Left(280); if(Text.IsEmpty() || !Game.IsValid()) return;
        Game->CommsHistory.Add("YOU / "+Text);
        Game->CommsHistory.Add("ASUKA / "+Game->ReplyAsuka(Text));
        while(Game->CommsHistory.Num()>12) Game->CommsHistory.RemoveAt(0);
        Input->SetText(FText::GetEmpty()); Refresh();
        FSlateApplication::Get().SetKeyboardFocus(Input);
    }
    void Refresh()
    {
        Transcript->ClearChildren(); if(!Game.IsValid()) return;
        for(const FString& Line:Game->CommsHistory)
            Transcript->AddSlot().Padding(0,7)[SNew(STextBlock).Text(FText::FromString(Line)).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity(Line.StartsWith("ASUKA") ? FLinearColor(1,.66f,.48f):FLinearColor(.6f,.84f,.88f))];
        Transcript->ScrollToEnd();
    }
};

void AEvaPawn::CallAsuka() { if(auto* G=Cast<AEvaGameMode>(GetWorld()->GetAuthGameMode())) G->ToggleComms(); }
void AEvaGameMode::ToggleComms()
{
    if(bCommsOpen) { CloseComms(); return; }
    if(!bOpenWorld || bEnded || !Wingman || !Wingman->bDeployed || bHelp) { SetNotice("ASUKA LINK AVAILABLE DURING FREE ROAM // O FROM TITLE"); return; }
    if(!GEngine || !GEngine->GameViewport) return;
    if(CommsHistory.Num()==0) CommsHistory.Add("ASUKA / Unit-02, connected. I'll cover your flank. Tell me what you need, Shinji.");
    bCallWasPaused=bPaused; bPaused=true; bCommsOpen=true;
    Pilot()->bFireHeld=false; Pilot()->bCharging=false; Pilot()->CannonCharge=0; Pilot()->bAiming=false; Pilot()->bGuard=false;
    SAssignNew(CommsPanel,SEvaComms).Game(this);
    GEngine->GameViewport->AddViewportWidgetContent(CommsPanel.ToSharedRef(),100);
    if(auto* PC=Cast<APlayerController>(Pilot()->GetController()))
    { PC->FlushPressedKeys(); FInputModeUIOnly Mode; Mode.SetWidgetToFocus(CommsPanel->Input); PC->SetInputMode(Mode); PC->bShowMouseCursor=true; }
    FSlateApplication::Get().SetKeyboardFocus(CommsPanel->Input);
}
void AEvaGameMode::CloseComms()
{
    if(!bCommsOpen) return;
    if(CommsPanel.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(CommsPanel.ToSharedRef());
    CommsPanel.Reset(); bCommsOpen=false; bPaused=bCallWasPaused;
    if(Pilot()) if(auto* PC=Cast<APlayerController>(Pilot()->GetController())) { PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false; }
}
void AEvaGameMode::SubmitComms(const FString& Message) { if(CommsPanel.IsValid()) CommsPanel->Send(Message); }
void AEvaGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    CloseComms();
    if(CityAmbience) { CityAmbience->Stop(); CityAmbience->DestroyComponent(); }
    if(CityHum) { CityHum->Stop(); CityHum->DestroyComponent(); }
    Super::EndPlay(Reason);
}
FString AEvaGameMode::ReplyAsuka(const FString& Message)
{
    if(!Wingman || !Wingman->bDeployed) return "Unit-02 isn't deployed. Open the free-roam operation and call me there.";
    FString T=Message.ToLower().TrimStartAndEnd();
    auto Has=[&](const TCHAR* Word) { return T.Contains(Word); };
    if(T.StartsWith(TEXT("hold")) || T.StartsWith(TEXT("stay")) || T.StartsWith(TEXT("stop")) || Has(TEXT("don't attack")) || Has(TEXT("do not attack")))
    { Wingman->Order=EEvaOrder::Hold; Wingman->HoldPosition=Wingman->GetActorLocation(); return "Holding position and fire. I'll dodge if I have to. Call regroup when you want me moving again."; }
    if(Has(TEXT("regroup")) || Has(TEXT("come back")) || Has(TEXT("retreat")))
    { Wingman->Order=EEvaOrder::Regroup; return "Pulling back to you. I'll stop firing until we're together. Keep a lane open!"; }
    if(Has(TEXT("follow")) || Has(TEXT("come with")))
    { Wingman->Order=EEvaOrder::Follow; return "Right flank, with you. Don't make me chase you across the entire city."; }
    if(Has(TEXT("cover me")) || T.StartsWith(TEXT("attack")) || T.StartsWith(TEXT("engage")) || T.StartsWith(TEXT("fight")) || Has(TEXT("please attack")) || Has(TEXT("can you attack")))
    { Wingman->Order=EEvaOrder::Assault; return bWorldEncounter ? "Unit-02 engaging. I'll pressure the field; you take the exposed core. Move!":"I'll take point when we find an Angel. Pick an amber beacon and I'll be ready."; }
    if(Has(TEXT("status")) || Has(TEXT("okay")) || Has(TEXT("health")) || Has(TEXT("damage")))
        return FString::Printf(TEXT("Unit-02 integrity: %.0f percent. %s. Your reserve is %.0f seconds. %s"),Wingman->Integrity,*Wingman->OrderName(),Rules.Battery,Wingman->Integrity<=0 ? TEXT("I'm disabled. Use a green service pylon to repair both units."):TEXT("We're still in this. Keep your eyes up."));
    if(Has(TEXT("plan")) || Has(TEXT("angel")) || Has(TEXT("ramiel")) || Has(TEXT("help")) || Has(TEXT("shamshel")))
    {
        if(!bWorldEncounter) return "Green pylons repair and resupply both of us. Amber beacons start an encounter. Say follow, hold, cover me, or regroup to change my orders.";
        if(bRamiel) return "Watch the crystal. Wait for the beam to lock, dash sideways, then hit the red core when it opens. Buildings can take the beam for you.";
        if(bShamshel) return "Don't stand in the whip lanes. Jump the sweep or dash clear, then punish the exposed core. I'll keep firing from the flank.";
        return "Breach the A.T. field together. Jump or dash out of the red ground warning, then drive your shots into the core.";
    }
    if(Has(TEXT("thank")) || Has(TEXT("nice")) || Has(TEXT("good job"))) return "Of course. Unit-02 doesn't do half measures. Just stay with me on the next pass.";
    if(Has(TEXT("scared")) || Has(TEXT("afraid")) || Has(TEXT("nervous"))) return "Then breathe. Pick one thing: move, aim, fire. I'll stay on your flank. You don't have to do all of it alone.";
    if(Has(TEXT("sorry"))) return "Save the apology. Check your reserve, reset your aim, and try again. I'm still here.";
    if(Has(TEXT("who")) || Has(TEXT("name"))) return "Asuka Langley, pilot of Unit-02. Your extremely capable wingman. Try to keep up.";
    if(Has(TEXT("hello")) || Has(TEXT("hey")) || T=="hi" || Has(TEXT("asuka"))) return "I hear you, Shinji. Unit-02 is on the line. Need a status report, a plan, or some cover?";
    if(Has(TEXT("bye"))) return "Roger. End the call when you're ready. I'll carry out the current order.";
    return "I'm here. Tell me what you need: a status report, a plan for this Angel, or an order like follow, hold, cover me, or regroup.";
}
