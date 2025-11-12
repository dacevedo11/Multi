// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"

#include "EngineUtils.h"
#include "ShooterCharacter.h"
#include "ShooterGameState.h"
#include "ShooterPlayerController.h"
#include "ShooterUI.h"
#include "Engine/PlayerStartPIE.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

bool AShooterGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

AActor* AShooterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Choose a player start
	APlayerStart* FoundPlayerStart = nullptr;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<APlayerStart*> UnOccupiedStartPoints;
	TArray<APlayerStart*> OccupiedStartPoints;
	TArray<APlayerStart*> BackupStartPoints;
	UWorld* World = GetWorld();

	// Figure out the player's team
	EShooterTeam Team = Player->GetPlayerState<AShooterPlayerState>()->Team;

	// Assign start tag based on team
	FName PlayerStartTag = NAME_None;
	switch (Team)
	{
	case EShooterTeam::None:
		PlayerStartTag = "Backup";
		break;
	case EShooterTeam::Red:
		PlayerStartTag = "Red";
		break;
	case EShooterTeam::Blue:
		PlayerStartTag = "Blue";
		break;
	}
	
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (PlayerStart->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			FoundPlayerStart = PlayerStart;
			break;
		}
		else if (PlayerStart->PlayerStartTag == "Backup")
		{
			BackupStartPoints.Add(PlayerStart);
		}
		// Prioritize player starts which match the team
		else if (PlayerStartTag == NAME_None || PlayerStartTag == PlayerStart->PlayerStartTag)
		{
			FVector ActorLocation = PlayerStart->GetActorLocation();
			const FRotator ActorRotation = PlayerStart->GetActorRotation();
			if (!World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
			{
				UnOccupiedStartPoints.Add(PlayerStart);
			}
			else if (World->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
			{
				OccupiedStartPoints.Add(PlayerStart);
			}
		}
	}
	if (FoundPlayerStart == nullptr)
	{
		// Remove starts that have opposing team players within 1000 units
        for (int32 i = UnOccupiedStartPoints.Num() - 1; i >= 0; i--)
        {
            APlayerStart* StartPoint = UnOccupiedStartPoints[i];
            FVector StartLocation = StartPoint->GetActorLocation();
            
            // Check for opposing team players within 1000 units
            for (TActorIterator<AShooterCharacter> CharIt(World); CharIt; ++CharIt)
            {
                AShooterCharacter* Character = *CharIt;
                if (Character && Character->Team != Team && Character->Team != EShooterTeam::None)
                {
                    float Distance = FVector::Dist(StartLocation, Character->GetActorLocation());
                    if (Distance <= 1000.0f)
                    {
                        UnOccupiedStartPoints.RemoveAt(i);
                        break; // No need to check more characters for this start point
                    }
                }
            }
        }
        
        for (int32 i = OccupiedStartPoints.Num() - 1; i >= 0; i--)
        {
            APlayerStart* StartPoint = OccupiedStartPoints[i];
            FVector StartLocation = StartPoint->GetActorLocation();
            
            // Check for opposing team players within 1000 units
            for (TActorIterator<AShooterCharacter> CharIt(World); CharIt; ++CharIt)
            {
                AShooterCharacter* Character = *CharIt;
                if (Character && Character->Team != Team && Character->Team != EShooterTeam::None)
                {
                    float Distance = FVector::Dist(StartLocation, Character->GetActorLocation());
                    if (Distance <= 1000.0f)
                    {
                        OccupiedStartPoints.RemoveAt(i);
                        break; // No need to check more characters for this start point
                    }
                }
            }
        }

		if (UnOccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (OccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
		else if (BackupStartPoints.Num() > 0)
		{
			FoundPlayerStart = BackupStartPoints[FMath::RandRange(0, BackupStartPoints.Num() - 1)];
		}
	}
	return FoundPlayerStart;
}

bool AShooterGameMode::ReadyToEndMatch_Implementation()
{
	if (AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>())
	{
		return ShooterGameState->RedTeamScore >= 10 || ShooterGameState->BlueTeamScore >= 10;
	}
	return false;
}

void AShooterGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
    
	if (AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>())
	{
		// Determine winner and send alert
		if (ShooterGameState->RedTeamScore >= 10)
		{
			ShooterGameState->MulticastOnAlert(TEXT("RED TEAM WINS"), FLinearColor::Red, 5.0f);
		}
		else if (ShooterGameState->BlueTeamScore >= 10)
		{
			ShooterGameState->MulticastOnAlert(TEXT("BLUE TEAM WINS"), FLinearColor::Blue, 5.0f);
		}
        
		// Set 5 second timer to restart the game
		GetWorld()->GetTimerManager().SetTimer(RestartGameTimerHandle, this, 
			&AShooterGameMode::RestartGame, 5.0f, false);
	}
}

AShooterGameMode::AShooterGameMode()
{
	PlayerStateClass = AShooterPlayerState::StaticClass();
	GameStateClass = AShooterGameState::StaticClass();
	bDelayedStart = true;
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AShooterGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

	// Get the player state
	if (AShooterPlayerState* PlayerState = C->GetPlayerState<AShooterPlayerState>())
	{
		// Check if player is already on a team
		if (PlayerState->Team == EShooterTeam::None)
		{
			// Add to Red if Red has less players OR if it is a tie
			if (RedTeamCount <= BlueTeamCount)
			{
				PlayerState->Team = EShooterTeam::Red;
				RedTeamCount++;
			}
			// Otherwise, add them to Blue
			else
			{
				PlayerState->Team = EShooterTeam::Blue;
				BlueTeamCount++;
			}
		}
	}
}

void AShooterGameMode::IncrementTeamScore(uint8 TeamByte)
{
	// retrieve the team score if any
	int32 Score = 0;
	if (int32* FoundScore = TeamScores.Find(TeamByte))
	{
		Score = *FoundScore;
	}

	// increment the score for the given team
	++Score;
	TeamScores.Add(TeamByte, Score);
}
