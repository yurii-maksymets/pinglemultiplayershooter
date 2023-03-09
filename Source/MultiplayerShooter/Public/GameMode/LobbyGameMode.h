// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSHOOTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable)
	void SetupDedicated(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = TEXT("DeathMatch"),
		int32 MaxNumberOfSearchResults = 10000, FString PathOfLobby = FString(TEXT("/Game/Maps/Lobby")));

	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	UFUNCTION()
		void OnStartSession(bool bWasSuccessful);

	UFUNCTION()
		void OnDestroySession(bool bWasSuccessful);

	// Member Variables
	int32 NumPublicConnections{ 4 };
	FString MatchType{ TEXT("DeathMatch") };
	int32 MaxSearchResults{ 10000 };
	FString LobbyPath{ "/Game/Maps/Lobby" };
	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	TSet <class AShooterPlayerController*> PControllers;
};
