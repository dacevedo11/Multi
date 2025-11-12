// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "EnhancedInputComponent.h"
#include "ShooterCharacter.h"
#include "ShooterBulletCounterUI.h"
#include "Multi.h"
#include "ShooterPlayerState.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController())
	{
		// create the bullet counter widget and add it to the screen
		BulletCounterUI = CreateWidget<UShooterBulletCounterUI>(this, BulletCounterUIClass);

		if (BulletCounterUI)
		{
			BulletCounterUI->SetOwningPlayer(this);
			BulletCounterUI->AddToPlayerScreen(0);

			// set the input mode to game only
			FInputModeUIOnly InputMode;
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
		}
		else
		{
			UE_LOG(LogMulti, Error, TEXT("Could not spawn bullet counter widget."));
		}
	}
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			EnhancedInputComponent->BindAction(ChatAllAction, ETriggerEvent::Started, this, &AShooterPlayerController::OnChatAllPressed);
			EnhancedInputComponent->BindAction(ChatTeamAction, ETriggerEvent::Started, this, &AShooterPlayerController::OnChatTeamPressed);
		}
	}
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Call the client RPC to set the input mode
	ClientOnPossess();

	// Get the player state
	if (AShooterPlayerState* PS = InPawn->GetPlayerState<AShooterPlayerState>())
	{
		// Get the character
		if (AShooterCharacter* Char = Cast<AShooterCharacter>(InPawn))
		{
			// Update the character's team to match the player state
			Char->SetTeam(PS->Team);
		}
	}
	
	SetupDelegates();
}

void AShooterPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// reset the bullet counter HUD
	BulletCounterUI->BP_UpdateBulletCounter(0, 0);
}

void AShooterPlayerController::OnBulletCountUpdated(int32 MagazineSize, int32 Bullets)
{
	// update the UI
	if (BulletCounterUI)
	{
		BulletCounterUI->BP_UpdateBulletCounter(MagazineSize, Bullets);
	}
}

void AShooterPlayerController::OnPawnDamaged(float LifePercent)
{
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->BP_Damaged(LifePercent);
	}
}

void AShooterPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	
	SetupDelegates();
}

void AShooterPlayerController::OnAlert(const FString& Text, FLinearColor Color, float Duration)
{
	if (BulletCounterUI)
	{
		BulletCounterUI->ShowAlert(Text, Color, Duration);
	}
}

void AShooterPlayerController::OnChatAllPressed()
{
	if (BulletCounterUI)
	{
		BulletCounterUI->StartChatInput(false);
	}
}

void AShooterPlayerController::OnChatTeamPressed()
{
	if (BulletCounterUI)
	{
		BulletCounterUI->StartChatInput(true);
	}
}

void AShooterPlayerController::SetupDelegates()
{
	// Only mess with the delegates on the local controller (since they're for the UI)
	if (GetPawn() && IsLocalPlayerController())
	{
		// subscribe to the pawn's OnDestroyed delegate
		GetPawn()->OnDestroyed.AddUniqueDynamic(this, &AShooterPlayerController::OnPawnDestroyed);

		// is this a shooter character?
		if (AShooterCharacter* ShooterCharacter = GetPawn<AShooterCharacter>())
		{
			ShooterCharacter->OnBulletCountUpdated.AddUniqueDynamic(this, &AShooterPlayerController::OnBulletCountUpdated);
			ShooterCharacter->OnDamaged.AddUniqueDynamic(this, &AShooterPlayerController::OnPawnDamaged);

			ShooterCharacter->OnDeath.AddUniqueDynamic(BulletCounterUI, &UShooterBulletCounterUI::ShowDeathScreen);

			// force update the life bar
			ShooterCharacter->OnDamaged.Broadcast(1.0f);
		}
	}
}

void AShooterPlayerController::ClientOnPossess_Implementation()
{
	// Switch to game-only input mode when we possess a pawn (game starts)
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);
	SetShowMouseCursor(false);
}
