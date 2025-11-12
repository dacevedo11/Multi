// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)


#include "AmmoPickup.h"

#include "ShooterCharacter.h"
#include "ShooterWeapon.h"

// Sets default values
AAmmoPickup::AAmmoPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Replicate
	bReplicates = true;

	// Create Sphere component as root
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetSphereRadius(25.0f);

	// Create Text render component
	TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRenderComponent"));
	TextRenderComponent->SetupAttachment(RootComponent);

	// Configure text render component
	TextRenderComponent->SetText(FText::FromString(TEXT("Ammo")));
	TextRenderComponent->SetHorizontalAlignment(EHTA_Center);
	TextRenderComponent->SetVerticalAlignment(EVRTA_TextCenter);
	TextRenderComponent->SetTextRenderColor(FColor::Green);
}

// Called when the game starts or when spawned
void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAmmoPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAmmoPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// If this happens and the local role is not ROLE_Authority, do nothing
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// Check if OtherActor is an AShooterCharacter
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (!ShooterCharacter)
	{
		return;
	}

	// Check if Shooter Character has Current Weapon
	AShooterWeapon* CurrentWeapon = ShooterCharacter->GetCurrentWeapon();
	if (!CurrentWeapon)
	{
		return;
	}

	// Check if Current Weapon is not already full on ammo
	if (!CurrentWeapon->bIsFullAmmo())
	{
		// Add ammo
		CurrentWeapon->AddAmmo(AmmoAmount);
		// Get rid of the ammo pickup
		Destroy();
	}

	
}
