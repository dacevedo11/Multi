// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)

#include "ShooterBPLibrary.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "ShooterCharacter.h"
#include "ShooterPlayerController.h"
#include "ShooterGameMode.h"
#include "ShooterGameState.h"

AShooterCharacter* UShooterBPLibrary::GetShooterCharacter(const UObject* WorldContextObject, int32 PlayerIndex)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			return Cast<AShooterCharacter>(PC->GetPawn());
		}
	}
	return nullptr;
}

AShooterPlayerController* UShooterBPLibrary::GetShooterController(const UObject* WorldContextObject, int32 PlayerIndex)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		return Cast<AShooterPlayerController>(World->GetFirstPlayerController());
	}
	return nullptr;
}

AShooterGameMode* UShooterBPLibrary::GetShooterGameMode(const UObject* WorldContextObject, int32 PlayerIndex)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		return Cast<AShooterGameMode>(World->GetAuthGameMode());
	}
	return nullptr;
}

AShooterGameState* UShooterBPLibrary::GetShooterGameState(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		return Cast<AShooterGameState>(World->GetGameState());
	}
	return nullptr;
}