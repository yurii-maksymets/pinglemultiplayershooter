// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnnouncementWidget.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSHOOTER_API UAnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* TimeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* GameTimeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* Announce_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* Announce_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* WinText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* WeaponType;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* WeaponAmmoAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* CarriedAmmoAmount;

	UFUNCTION(BlueprintImplementableEvent)
	void StartTimer(float StartTime = 0.f);

	UFUNCTION(BlueprintImplementableEvent)
	void StartMatchTimer(float CurrentTime);
};
