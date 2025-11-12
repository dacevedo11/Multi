// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterPlayerState.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "ShooterBulletCounterUI.generated.h"

/**
 *  Simple bullet counter UI widget for a first person shooter game
 */
UCLASS(abstract)
class MULTI_API UShooterBulletCounterUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Allows Blueprint to update sub-widgets with the new bullet count */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "UpdateBulletCounter"))
	void BP_UpdateBulletCounter(int32 MagazineSize, int32 BulletCount);

	/** Allows Blueprint to update sub-widgets with the new life total and play a damage effect on the HUD */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "Damaged"))
	void BP_Damaged(float LifePercent);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> RedScore;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> BlueScore;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MyScore;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Timer;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> AlertText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DeathMessage;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> RespawnTimer;

	/** Chat Bind Widgets */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> ChatBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> ChatScrollBox;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> ChatMessages;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> ChatEntry;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ChatTeam;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> ChatTextBox;
	
	UFUNCTION()
	void ShowAlert(const FString& AlertMessage, FLinearColor Color, float Duration);

	UFUNCTION()
	void HideAlert();

	UFUNCTION()
	void ShowDeathScreen(float RespawnTime);

	UFUNCTION()
	void HideDeathScreen();

	UFUNCTION()
	void AddChatMessage(EShooterTeam Team, const FString& SenderName, const FString& Message);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<class UChatMessageWidget> ChatMessageWidgetClass;

	UFUNCTION()
	void StartChatInput(bool bIsTeamChat);

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class UCheckBox> ReadyCheckBox;

private:
	FTimerHandle AlertTimerHandle;

	FTimerHandle RespawnCountdownHandle;

	float TimeUntilRespawn;
    
	UFUNCTION()
	void UpdateRespawnCountdown();

protected:

	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnReadyCheckStateChanged(bool bIsChecked);
};
