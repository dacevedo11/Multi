// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShooterPlayerState.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EShooterTeam : uint8
{
	/* Team not assigned yet*/
	None,
	/* On red team */
	Red,
	/* On blue team */
	Blue
};

UCLASS()
class MULTI_API AShooterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	//* Constructor */
	AShooterPlayerState();
	
	UPROPERTY(Replicated)
	EShooterTeam Team = EShooterTeam::None;

	//** Get the current kill streak */
	int32 GetKillStreak() const { return KillStreak; }

	/** Add to the current kill streak */
	void AddToKillStreak();

	/** Reset the kill streak to 0*/
	void ResetKillStreak();

	bool ShouldSendStreakAlert(int32 Streak) const;

	void MarkStreakAlertSent(int32 Streak);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayKillStreakSound(int32 StreakCount);
	
	UFUNCTION(Server, Reliable)
    void ServerSendChatMessage(EShooterTeam ChatTeam, const FString& SenderName, const FString& Message);
    
    UFUNCTION(Client, Reliable)
    void ClientReceiveChatMessage(EShooterTeam ChatTeam, const FString& SenderName, const FString& Message);

	UFUNCTION(Server, Reliable)
	void ServerSetReadyState (bool bIsReady);

	UPROPERTY(EditDefaultsOnly)
	TMap<int32, FString> KillstreakMessages;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** Track the current kill streak */
	UPROPERTY()
	int32 KillStreak = 0;

	UPROPERTY()
	TArray<int32> SentStreakAlerts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TMap<int32, USoundBase*> KillStreakSounds;	
	
};
