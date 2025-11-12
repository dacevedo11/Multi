// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterBulletCounterUI.h"

#include "ChatMessageWidget.h"
#include "ShooterBPLibrary.h"
#include "ShooterCharacter.h"
#include "ShooterGameState.h"
#include "ShooterPlayerController.h"
#include "ShooterPlayerState.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CheckBox.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerState.h"

void UShooterBulletCounterUI::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Get the GameState
	AShooterGameState* GameState = UShooterBPLibrary::GetShooterGameState(this);
	
	// Update team score
	if (GameState)
	{
		if (RedScore)
		{
			RedScore->SetText(FText::AsNumber(GameState->RedTeamScore));
		}
        
		if (BlueScore)
		{
			BlueScore->SetText(FText::AsNumber(GameState->BlueTeamScore));
		}
	}
    
	// Update player score
	if (AShooterPlayerController* PlayerController = UShooterBPLibrary::GetShooterController(this))
	{
		if (APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>())
		{
			if (MyScore)
			{
				MyScore->SetText(FText::AsNumber(PlayerState->GetScore()));
			}
		}
	}

	// If we aren't in progress, show the timer
	if (GameState && GameState->GetMatchState() == MatchState::WaitingToStart)
	{
		// If we can't see the timer, make it visible
		if (Timer->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			Timer->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		
		FString TimerText = FString::Printf(TEXT("%f"), GameState->WaitingToStartTime);
		Timer->SetText(FText::FromString(TimerText));

		// If WaitingToStart, ReadyCheckBox is Visible
		ReadyCheckBox->SetVisibility(ESlateVisibility::Visible);
	}
	else if (Timer->GetVisibility() != ESlateVisibility::Hidden)
	{
		// Hide this because match is in progress
		Timer->SetVisibility(ESlateVisibility::Hidden);
		ReadyCheckBox->SetVisibility(ESlateVisibility::Hidden);
	}

}

void UShooterBulletCounterUI::ShowAlert(const FString& AlertMessage, FLinearColor Color, float Duration)
{
	if (AlertText)
	{
		AlertText->SetText(FText::FromString(AlertMessage));
		AlertText->SetColorAndOpacity(Color);
		AlertText->SetVisibility(ESlateVisibility::Visible);

		GetWorld()->GetTimerManager().SetTimer(AlertTimerHandle, this, &UShooterBulletCounterUI::HideAlert, Duration, false);
	}
}

void UShooterBulletCounterUI::HideAlert()
{
	if (AlertText)
	{
		AlertText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UShooterBulletCounterUI::ShowDeathScreen(float RespawnTime)
{
	//  Show death message
	if (DeathMessage)
	{
		DeathMessage->SetText(FText::FromString("You are dead:("));
		DeathMessage->SetVisibility(ESlateVisibility::Visible);
	}

	// Respawn timer
	if (RespawnTimer)
	{
		TimeUntilRespawn = RespawnTime;
		RespawnTimer->SetVisibility(ESlateVisibility::Visible);

		UpdateRespawnCountdown();

		GetWorld()->GetTimerManager().SetTimer(RespawnCountdownHandle, this, &UShooterBulletCounterUI::UpdateRespawnCountdown, 1.0f, true);
	}
}

void UShooterBulletCounterUI::HideDeathScreen()
{
	// Hide death screen
	if (DeathMessage)
	{
		DeathMessage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (RespawnTimer)
	{
		RespawnTimer->SetVisibility(ESlateVisibility::Hidden);
	}

	GetWorld()->GetTimerManager().ClearTimer(RespawnCountdownHandle);
}

void UShooterBulletCounterUI::AddChatMessage(EShooterTeam Team, const FString& SenderName, const FString& Message)
{
	// Create a new chat message widget instance
	UChatMessageWidget* NewMessageWidget = CreateWidget<UChatMessageWidget>(this, ChatMessageWidgetClass);

	// Check NewMessageWidget
	if (!NewMessageWidget)
	{
		return;
	}

	// Add to ChatMessages vertical box
	if (ChatMessages)
	{
		ChatMessages->AddChildToVerticalBox(NewMessageWidget);
	}

	// Set the sender name
	if (NewMessageWidget->Sender)
	{
		NewMessageWidget->Sender->SetText(FText::FromString(SenderName));
	}

	// Set the message content
	if (NewMessageWidget->Message)
	{
		NewMessageWidget->Message->SetText(FText::FromString(Message));
	}

	// Set channel text and color
	if (NewMessageWidget->Channel)
	{
		if (Team == EShooterTeam::None) // All chat
		{
			NewMessageWidget->Channel->SetText(FText::FromString("[All]"));
			NewMessageWidget->Channel->SetColorAndOpacity(FLinearColor::White);
		}
		else // Team chat
		{
			NewMessageWidget->Channel->SetText(FText::FromString("[Team]"));

			// Get local player's team for color
			if (AShooterPlayerController* PC = UShooterBPLibrary::GetShooterController(this))
			{
				if (AShooterCharacter* LocalCharacter = PC->GetPawn<AShooterCharacter>())
				{
					FLinearColor TeamColor = (LocalCharacter->Team == EShooterTeam::Red) ? FLinearColor::Red : FLinearColor::Blue;
					NewMessageWidget->Channel->SetColorAndOpacity(TeamColor);
				}
			}
		}
	}

	// Scroll to end
	if (ChatScrollBox)
	{
		ChatScrollBox->ScrollToEnd();
	}
}

void UShooterBulletCounterUI::StartChatInput(bool bIsTeamChat)
{
	if (ChatTextBox && ChatTeam && ChatEntry)
	{
		// Show ChatEntry
		ChatEntry->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		
		// Set the chat type text
		if (bIsTeamChat)
		{
			ChatTeam->SetText(FText::FromString("[Team]"));
		}
		else
		{
			ChatTeam->SetText(FText::FromString("[All]"));
		}
        
		// Focus the text box
		ChatTextBox->SetKeyboardFocus();
        
		// Change input mode to UI only
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(ChatTextBox->TakeWidget());
				PC->SetInputMode(InputMode);
				PC->SetShowMouseCursor(true);
			}
		}
	}
}

void UShooterBulletCounterUI::UpdateRespawnCountdown()
{
	if (RespawnTimer)
	{
		int32 SecondsRemaining = FMath::CeilToInt(TimeUntilRespawn);
		FString TimerText = FString::Printf(TEXT("Respawning in: %d"), SecondsRemaining);
		RespawnTimer->SetText(FText::FromString(TimerText));
	}
    
	TimeUntilRespawn -= 1.0f;
    
	if (TimeUntilRespawn <= 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(RespawnCountdownHandle);
		HideDeathScreen();
	}
}

void UShooterBulletCounterUI::NativeConstruct()
{
	Super::NativeConstruct();
    
	// Bind to the ChatTextBox OnTextCommitted delegate
	if (ChatTextBox)
	{
		ChatTextBox->OnTextCommitted.AddDynamic(this, &UShooterBulletCounterUI::OnChatTextCommitted);
	}

	// Bind to the ReadyCheckBox state change
	if (ReadyCheckBox)
	{
		ReadyCheckBox->OnCheckStateChanged.AddDynamic(this, &UShooterBulletCounterUI::OnReadyCheckStateChanged);
	}
}

void UShooterBulletCounterUI::NativeDestruct()
{
	// Remove the delegate binding
	if (ChatTextBox)
	{
		ChatTextBox->OnTextCommitted.RemoveDynamic(this, &UShooterBulletCounterUI::OnChatTextCommitted);
	}
	
	if (ReadyCheckBox)
	{
		ReadyCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UShooterBulletCounterUI::OnReadyCheckStateChanged);
	}
    
	Super::NativeDestruct();
}

void UShooterBulletCounterUI::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
        if (CommitMethod == ETextCommit::OnEnter)
        {
            // Get the message text
            FString MessageText = Text.ToString();
            
            if (!MessageText.IsEmpty())
            {
                bool bIsTeamChat = false;
                if (ChatTeam)
                {
                    FString ChatTeamText = ChatTeam->GetText().ToString();
                    bIsTeamChat = ChatTeamText.Contains("[Team]");
                }
                
                // Get the player controller
                if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(GetOwningPlayer()))
                {
                	// Get the player state
                    if (AShooterPlayerState* PS = PC->GetPlayerState<AShooterPlayerState>())
                    {
                        EShooterTeam TeamToSend = bIsTeamChat ? PS->Team : EShooterTeam::None;
                        
                        // Get sender name
                        FString SenderName = PS->GetPlayerName();
                        PS->ServerSendChatMessage(TeamToSend, SenderName, MessageText);
                    }
                }
            }
            
            // Clear the text box
            if (ChatTextBox)
            {
                ChatTextBox->SetText(FText::GetEmpty());
            }

        	// Hide ChatEntry
        	if (ChatEntry)
        	{
        		ChatEntry->SetVisibility(ESlateVisibility::Hidden);
        	}
            
            // Set input mode back to game only
            UWidgetBlueprintLibrary::SetInputMode_GameOnly(GetOwningPlayer());
            
            // Hide mouse cursor
            if (APlayerController* PC = GetOwningPlayer())
            {
                PC->SetShowMouseCursor(false);
            }
        }
}

void UShooterBulletCounterUI::OnReadyCheckStateChanged(bool bIsChecked)
{
	if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(GetOwningPlayer()))
	{
		if (AShooterPlayerState* PS = PC->GetPlayerState<AShooterPlayerState>())
		{
			PS->ServerSetReadyState(bIsChecked);
		}
	}
}
