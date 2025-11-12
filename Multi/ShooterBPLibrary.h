// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShooterBPLibrary.generated.h"

/**
 * 
 */
UCLASS()
class MULTI_API UShooterBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	static class AShooterCharacter* GetShooterCharacter(const UObject* WorldContextObject, int32 PlayerIndex = 0);

	UFUNCTION(BlueprintPure)
	static class AShooterPlayerController* GetShooterController(const UObject* WorldContextObject, int32 PlayerIndex = 0);

	UFUNCTION(BlueprintPure)
	static class AShooterGameMode* GetShooterGameMode(const UObject* WorldContextObject, int32 PlayerIndex = 0);

	UFUNCTION(BlueprintPure)
	static class AShooterGameState* GetShooterGameState(const UObject* WorldContextObject);
};
