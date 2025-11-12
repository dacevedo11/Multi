// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "ShooterBPLibrary.h"
#include "ShooterBulletCounterUI.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "ShooterGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameMode.h"

void AShooterCharacter::OnRep_CurrentHP()
{
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));
}

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	SetCurrentHP(MaxHP);

	// update the HUD
	OnDamaged.Broadcast(1.0f);
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	SetCurrentHP(CurrentHP - Damage);

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		if (EventInstigator)
		{
			AShooterGameState* GameState = UShooterBPLibrary::GetShooterGameState(this);
			
			if (GameState && GameState->GetMatchState() == MatchState::InProgress)
			{
				// Increase PlayerState score by 1
				if (APlayerState* KillPlayerState = EventInstigator->GetPlayerState<APlayerState>())
				{
					KillPlayerState->SetScore(KillPlayerState->GetScore() + 1);

					// Add to kill streak
					if (AShooterPlayerState* ShooterPlayerState = Cast<AShooterPlayerState>(KillPlayerState))
					{
						ShooterPlayerState->AddToKillStreak();
					}
				}

				// Increase team score by 1
				if (GameState)
				{
					// Get the team
					if (AShooterCharacter* KillerCharacter = Cast<AShooterCharacter>(EventInstigator->GetPawn()))
					{
						if (KillerCharacter->Team == EShooterTeam::Red) // Red team
						{
							GameState->RedTeamScore++;
						}
						else if (KillerCharacter->Team == EShooterTeam::Blue) // Blue team
						{
							GameState->BlueTeamScore++;
						}
					}
				}
			}
		}

		if (AShooterPlayerState* DeadPlayerState = GetPlayerState<AShooterPlayerState>())
		{
			if (DeadPlayerState->GetKillStreak() >= 3)
			{
				if (AShooterPlayerState* KillerPlayerState = Cast<AShooterPlayerState>(EventInstigator->GetPlayerState<APlayerState>()))				{
					FString AlertMessage = FString::Printf(TEXT("%s ended %s's streak!"), 
						*KillerPlayerState->GetPlayerName(), *DeadPlayerState->GetPlayerName());
            
					// Get killer's team color
					FLinearColor TeamColor = FLinearColor::White;
					if (AShooterCharacter* KillerCharacter = Cast<AShooterCharacter>(EventInstigator->GetPawn()))
					{
						TeamColor = (KillerCharacter->Team == EShooterTeam::Red) ? FLinearColor::Red : FLinearColor::Blue;
					}
            
					if (AShooterGameState* GameState = UShooterBPLibrary::GetShooterGameState(this))
					{
						GameState->MulticastOnAlert(AlertMessage, TeamColor, 2.5f);
					}
				}
			}
		}

		
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoStartFiring()
{
	ServerDoStartFiring();
}

void AShooterCharacter::DoStopFiring()
{		
	ServerDoStopFiring();
}

void AShooterCharacter::DoSwitchWeapon()	
{
	ServerDoSwitchWeapon();
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	if (AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>())
	{
		ShooterPlayerState->ResetKillStreak();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls
	DisableInput(nullptr);

	MulticastOnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	if (Controller)
	{
		AController* OldController = Controller;
		// Tell the current controller it doesn't control this pawn anymore
		Controller->UnPossess();
		if (AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this))
		{
			// Force the old controller to restart
			GameMode->RestartPlayer(OldController);
		}
	}
	// destroy the character to force the PC to respawn
	Destroy();
}


void AShooterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Always replicate changes to CurrentHP
	DOREPLIFETIME(AShooterCharacter, CurrentHP);
	
	DOREPLIFETIME(AShooterCharacter, Team);

	DOREPLIFETIME(AShooterCharacter, ReplicatedControlRotation);
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetLocalRole() == ROLE_Authority)
	{
		ReplicatedControlRotation = GetControlRotation();
	}
}

void AShooterCharacter::OnRep_Team()
{
	BP_OnTeamSet();
}

AShooterWeapon* AShooterCharacter::GetCurrentWeapon()
{
	return CurrentWeapon;
}

void AShooterCharacter::SetTeam(EShooterTeam InTeam)
{
	Team = InTeam;
    
	if (GetNetMode() == NM_ListenServer)
	{
		OnRep_Team();
	}
}

void AShooterCharacter::SetCurrentHP(float InCurrentHP)
{
	CurrentHP = InCurrentHP;

	if (GetNetMode() == NM_ListenServer)
	{
		OnRep_CurrentHP();
	}
}

void AShooterCharacter::MulticastOnDeath_Implementation()
{
	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	if (IsLocallyControlled())
	{
		// Only the player who dies sees this
		OnDeath.Broadcast(RespawnTime); // shows death screen
	}
	
	// call the BP handler
	BP_OnDeath();
}

void AShooterCharacter::ServerDoStartFiring_Implementation()
{
	// fire the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::ServerDoStopFiring_Implementation()
{
	// stop firing the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::ServerDoSwitchWeapon_Implementation()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1)
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon();
	}
}
