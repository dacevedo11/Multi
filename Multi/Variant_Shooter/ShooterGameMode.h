// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"	
#include "GameFramework/GameModeBase.h"
#include "ShooterPlayerState.h"
#include "GameFramework/GameMode.h"
#include "ShooterGameMode.generated.h"

class UShooterUI;

/**
 *  Simple GameMode for a first person shooter game
 *  Manages game UI
 *  Keeps track of team scores
 */
UCLASS(abstract)
class MULTI_API AShooterGameMode : public AGameMode
{
	GENERATED_BODY()

protected:
	/** Map of scores by team ID */
	TMap<uint8, int32> TeamScores;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	virtual void GenericPlayerInitialization(AController* C) override;

	int32 RedTeamCount = 0;

	int32 BlueTeamCount = 0;

	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual bool ReadyToEndMatch_Implementation() override;

	virtual void HandleMatchHasEnded() override;

public:
	/** Constructor */
	AShooterGameMode();

	/** Increases the score for the given team */
	void IncrementTeamScore(uint8 TeamByte);

	// Time before the match wills tart
	UPROPERTY(EditDefaultsOnly)
	float WaitingToStartDuration = 5.0f;

private:
	FTimerHandle RestartGameTimerHandle;

};
