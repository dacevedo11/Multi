// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)


#include "MultiOnlineSubsystem.h"

#include "Online/Lobbies.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineSessionNames.h"
using namespace UE::Online;

void UMultiOnlineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	OnlineServices = GetServices();

	if (OnlineServices)
	{
		AuthInterface = OnlineServices->GetAuthInterface();
		LobbiesInterface = OnlineServices->GetLobbiesInterface();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find a valid online service"));
	}
}

void UMultiOnlineSubsystem::Login()
{
	LoginHelper(LoginCredentialsType::PersistentAuth);
}

bool UMultiOnlineSubsystem::IsLoggedIn() const
{
	if (AccountInfo.LoginStatus == ELoginStatus::LoggedIn)
	{
		return true;
	}
	
	return false;
}

bool UMultiOnlineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if WITH_EDITOR
	return false;
#else
	return true;
#endif
}

void UMultiOnlineSubsystem::HostSession()
{
	if (!LobbiesInterface || IsLoggedIn() == false)
	{
		return;
	}

	FCreateLobby::Params Params;
	Params.LocalAccountId = AccountInfo.AccountId;
	Params.LocalName = TEXT("MyGame");
	Params.SchemaId = FSchemaId(TEXT("GameLobby"));
	Params.bPresenceEnabled = true;
	Params.MaxMembers = 4;
	Params.JoinPolicy = ELobbyJoinPolicy::PublicAdvertised;
	Params.Attributes.Emplace(SETTING_MAPNAME, TEXT("Lvl_Shooter"));

	LobbiesInterface->CreateLobby(MoveTemp(Params)).OnComplete([this](const TOnlineResult<FCreateLobby>& Result)
	{
		if (Result.IsOk())
		{
			GetWorld()->ServerTravel("Lvl_Shooter?listen");
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create a lobby"));
		}
	});

}

void UMultiOnlineSubsystem::FindAndJoinSession()
{
	if (!LobbiesInterface || IsLoggedIn() == false)
	{
		return;
	}

	FFindLobbies::Params Params;
	Params.LocalAccountId = AccountInfo.AccountId;

	LobbiesInterface->FindLobbies(MoveTemp(Params)).OnComplete([this](const TOnlineResult<FFindLobbies>& Result)
	{
		if (Result.IsOk())
		{
			const FFindLobbies::Result& FindResults = Result.GetOkValue();
			for (auto Lobby : FindResults.Lobbies)
			{
				if (Lobby->OwnerAccountId.IsValid() && Lobby->Members.Num() > 0)
				{
					// Well there's someone here and the lobby owner is too, so let's join...
					JoinLobbyHelper(Lobby);
					break;  
				}
			}
		}
		else
		{
			FOnlineError Error = Result.GetErrorValue();
			UE_LOG(LogTemp, Error, TEXT("Failed to find lobbies: %s"), *Error.GetLogString());
		}
	});
}

void UMultiOnlineSubsystem::LoginHelper(FName CredentialsType)
{
	// Instantiate the parameters necessary to login the user
	FAuthLogin::Params Params;
	
	if (ULocalPlayer* Player = GetGameInstance()->GetLocalPlayerByIndex(0))
	{
		Params.PlatformUserId = Player->GetPlatformUserId();
	}

	Params.CredentialsType = CredentialsType;

	// Handle the Login.OnComplete callback by registering the account info upon successful login or processing the resulting error:
	AuthInterface->Login(MoveTemp(Params)).OnComplete([this, CredentialsType](const TOnlineResult<FAuthLogin>& Result)
	{
		if (Result.IsOk())
		{
			AccountInfo = Result.GetOkValue().AccountInfo.Get();
		}
		else
		{
			if (CredentialsType == LoginCredentialsType::PersistentAuth)
			{
				LoginHelper(LoginCredentialsType::AccountPortal);
			}
			else
			{
				FOnlineError Error = Result.GetErrorValue();
				UE_LOG(LogTemp, Error, TEXT("%s"), *Error.GetLogString());	
			}
		}
	});
	
}

void UMultiOnlineSubsystem::JoinLobbyHelper(TSharedRef<const FLobby> Lobby)
{
	FJoinLobby::Params Params;
	Params.LocalAccountId = AccountInfo.AccountId;
	Params.bPresenceEnabled = true;
	Params.LobbyId = Lobby->LobbyId;
	Params.LocalName = TEXT("MyGame");

	LobbiesInterface->JoinLobby(MoveTemp(Params)).OnComplete([this](const TOnlineResult<FJoinLobby>& JoinResult)
	{
		if (JoinResult.IsOk())
		{
			auto Lobby = JoinResult.GetOkValue().Lobby;
			TOnlineResult<FGetResolvedConnectString> ConnectResult = OnlineServices->GetResolvedConnectString({AccountInfo.AccountId, Lobby->LobbyId});
			if (ConnectResult.IsOk())
			{
				APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
				PC->ClientTravel(ConnectResult.GetOkValue().ResolvedConnectString, TRAVEL_Absolute);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to join lobby"));
		}
	});
}



