// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)


#include "ShooterGameState.h"
#include "ShooterPlayerController.h"
#include "ShooterBPLibrary.h"
#include "ShooterGameMode.h"
#include "Net/UnrealNetwork.h"

AShooterGameState::AShooterGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AShooterGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Check if Waiting To Start
	if (GetMatchState() == MatchState::WaitingToStart)
	{
		// Check if PlayerArray has more than 0 players AND PlayersReady is equal to number players in Player Array 
		if (PlayerArray.Num() > 0 && PlayersReady == PlayerArray.Num())
		{
			WaitingToStartTime -= DeltaSeconds;
			if (WaitingToStartTime <= 0.0f)
			{
				WaitingToStartTime = 0.0f;
				// On server, actually start the match!
				if (GetLocalRole() == ROLE_Authority)
				{
					if (AShooterGameMode* GameMode = UShooterBPLibrary::GetShooterGameMode(this))
					{
						GameMode->StartMatch();
						MulticastOnAlert(TEXT("STARTING MATCH"), FLinearColor::Green, 3.0f);
					}
				}
			}	
		}
	}
}

void AShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterGameState, RedTeamScore);
	DOREPLIFETIME(AShooterGameState, BlueTeamScore);
	DOREPLIFETIME(AShooterGameState, PlayersReady);
		 
	// Only send this on the "initial bunch" when the client first gets this actor replicated
	DOREPLIFETIME_CONDITION(AShooterGameState, WaitingToStartTime, COND_InitialOnly);
}

void AShooterGameState::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	// If we're on the server, set the waiting to start time
	if (GetLocalRole() == ROLE_Authority)
	{
		WaitingToStartTime = GetDefaultGameMode<AShooterGameMode>()->WaitingToStartDuration;
	}

}

void AShooterGameState::MulticastOnAlert_Implementation(const FString& Text, FLinearColor Color, float Duration)
{
	AShooterPlayerController* PlayerController = UShooterBPLibrary::GetShooterController(this, 0);
	if (PlayerController)
	{
		PlayerController->OnAlert(Text, Color, Duration);
	}
}
