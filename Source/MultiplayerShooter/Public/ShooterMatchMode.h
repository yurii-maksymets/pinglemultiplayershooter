// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ShooterMatchMode.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSHOOTER_API AShooterMatchMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* PlayerController) override;
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;
	void HandleStartingNewPlayer(APlayerController* NewPlayer);
	void FinishGame();

	UPROPERTY(EditDefaultsOnly)
	float MatchPlayTime;

	UPROPERTY(EditDefaultsOnly)
	TArray<FString> MapsAllowed;
};
