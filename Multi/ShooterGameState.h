// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameFramework/GameStateBase.h"
#include "ShooterGameState.generated.h"

/**
 * 
 */
UCLASS()
class MULTI_API AShooterGameState : public AGameState
{
	GENERATED_BODY()

public:
	//** Constructor */
	AShooterGameState();

	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(Replicated)
	int32 RedTeamScore = 0;

	UPROPERTY(Replicated)
	int32 BlueTeamScore = 0;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	float WaitingToStartTime = 0.0f;

	virtual void HandleMatchIsWaitingToStart() override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnAlert(const FString& Text, FLinearColor Color, float Duration);

	UPROPERTY(Replicated)
	int32 PlayersReady = 0;
};
