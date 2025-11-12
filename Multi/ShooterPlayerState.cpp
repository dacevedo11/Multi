// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)


#include "ShooterPlayerState.h"

#include "ShooterBulletCounterUI.h"
#include "ShooterCharacter.h"
#include "ShooterGameState.h"
#include "ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AShooterPlayerState::AShooterPlayerState()
{
	SetNetUpdateFrequency(4);

	// Initialize with default text
	KillstreakMessages.Add(3, TEXT("is on a killing spree!"));
	KillstreakMessages.Add(5, TEXT("is dominating!"));
	KillstreakMessages.Add(7, TEXT("is UNSTOPPABLE!"));
	KillstreakMessages.Add(9, TEXT("is GODLIKE!"));
}

void AShooterPlayerState::AddToKillStreak()
{
	KillStreak++;

	if (ShouldSendStreakAlert(KillStreak))
	{
		FString* MessagePtr = KillstreakMessages.Find(KillStreak);
		
		if (MessagePtr)
		{
			FString PlayerName = GetPlayerName();
			FString AlertMessage = FString::Printf(TEXT("%s %s"), *PlayerName, **MessagePtr);

			// Get team color
			FLinearColor TeamColor = FLinearColor::White;
			if (AShooterCharacter* Character = Cast<AShooterCharacter>(GetPawn()))
			{
				TeamColor = (Character->Team == EShooterTeam::Red) ? FLinearColor::Red : FLinearColor::Blue;
			}
            
			// Send alert via GameState
			if (AShooterGameState* GameState = GetWorld()->GetGameState<AShooterGameState>())
			{
				GameState->MulticastOnAlert(AlertMessage, TeamColor, 2.5f);
			}

			MulticastPlayKillStreakSound(KillStreak);
			
			MarkStreakAlertSent(KillStreak);
		}
	}
}

void AShooterPlayerState::ResetKillStreak()
{
	KillStreak = 0;
	SentStreakAlerts.Empty();
}

bool AShooterPlayerState::ShouldSendStreakAlert(int32 Streak) const
{
	// Only send alerts for specific thresholds that haven't been sent yet
	if (Streak == 3 || Streak == 5 || Streak == 7 || Streak == 9)
	{
		return !SentStreakAlerts.Contains(Streak);
	}
	return false;
}

void AShooterPlayerState::MarkStreakAlertSent(int32 Streak)
{
	SentStreakAlerts.AddUnique(Streak);
}

void AShooterPlayerState::ServerSendChatMessage_Implementation(EShooterTeam ChatTeam, const FString& SenderName,
	const FString& Message)
{
	// Get the game state
	if (AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		// Loop through all the players
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (AShooterPlayerState* ShooterPS = Cast<AShooterPlayerState>(PlayerState))
			{
				// Determine if this player should receive the message
				bool bShouldReceive = false;
                
				if (ChatTeam == EShooterTeam::None) // All chat
				{
					bShouldReceive = true; // Everyone gets all chat
				}
				else // Team chat
				{
					bShouldReceive = (ShooterPS->Team == ChatTeam); // Only same team gets team chat
				}
                
				if (bShouldReceive)
				{
					ShooterPS->ClientReceiveChatMessage(ChatTeam, SenderName, Message);
				}
			}
		}
		
	}
}

void AShooterPlayerState::MulticastPlayKillStreakSound_Implementation(int32 StreakCount)
{
	if (USoundBase** Sound = KillStreakSounds.Find(StreakCount))
	{
		if (*Sound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), *Sound);
		}
	}
}

void AShooterPlayerState::ClientReceiveChatMessage_Implementation(EShooterTeam ChatTeam, const FString& SenderName,
	const FString& Message)
{
	if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(GetOwner()))
	{
		if (PC->BulletCounterUI)
		{
			PC->BulletCounterUI->AddChatMessage(ChatTeam, SenderName, Message);
		}
	}
}

void AShooterPlayerState::ServerSetReadyState_Implementation(bool bIsReady)
{
	// Get the game state
	if (AShooterGameState* GameState = GetWorld()->GetGameState<AShooterGameState>())
	{
		if (bIsReady)
		{
			// Player is checking ready
			GameState->PlayersReady++;
		}
		else
		{
			// Player is unchecking ready
			GameState->PlayersReady--;
			// Do not go below 0
			GameState->PlayersReady = FMath::Max(0, GameState->PlayersReady);
		}
	}
}

void AShooterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerState, Team);
}
