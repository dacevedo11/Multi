// Copyright 2025 Daniel Acevedo (acevedod@usc.edu)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "ChatMessageWidget.generated.h"

/**
 * 
 */
UCLASS()
class MULTI_API UChatMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind widgets */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Channel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Sender;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Message;
};
