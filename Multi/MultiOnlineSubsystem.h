// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)

#pragma once

#include "CoreMinimal.h"
#include "Online/Auth.h"
#include "Online/Lobbies.h"
#include "Online/OnlineServices.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiOnlineSubsystem.generated.h"
using namespace UE::Online;


/**
 * 
 */
UCLASS()
class MULTI_API UMultiOnlineSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable)
	void Login();

	UFUNCTION(BlueprintPure)
	bool IsLoggedIn() const;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UFUNCTION(BlueprintCallable)
	void HostSession();

	UFUNCTION(BlueprintCallable)
	void FindAndJoinSession();

protected:
	// Various online interface pointers
	UE::Online::IOnlineServicesPtr OnlineServices;
	UE::Online::IAuthPtr AuthInterface;
	UE::Online::ILobbiesPtr LobbiesInterface;

	// Stores info of the account that's logged in
	UE::Online::FAccountInfo AccountInfo;

	// Helper function for login functionality
	void LoginHelper(FName CredentialsType);

	// Helper function for joining lobby
	void JoinLobbyHelper(TSharedRef<const FLobby> Lobby);
};
