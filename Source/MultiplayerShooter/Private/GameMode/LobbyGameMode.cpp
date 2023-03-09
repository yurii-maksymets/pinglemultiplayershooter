// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"
#include "GameFramework/GameState.h"
#include "MultiplayerSessionsSubsystem.h"
#include "PlayerController/ShooterPlayerController.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (Cast<AShooterPlayerController>(NewPlayer))
		PControllers.Add(Cast<AShooterPlayerController>(NewPlayer));

	const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	if (NumberOfPlayers == 2)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			bUseSeamlessTravel = true;
			for (AShooterPlayerController* pc : PControllers)
				if (pc)	pc->StartLocalTimer();

			FTimerHandle TH;
			FTimerDelegate TD;
			TD.BindLambda([this]
				{
					GetWorld()->ServerTravel(FString("/Game/Maps/Arena1"));
				}
			);
			GetWorld()->GetTimerManager().SetTimer(TH, TD, 7.f, false);
		}
	}
}

void ALobbyGameMode::SetupDedicated(int32 NumberOfPublicConnections, FString TypeOfMatch,
	int32 MaxNumberOfSearchResults, FString PathOfLobby)
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		NumPublicConnections = NumberOfPublicConnections;
		MatchType = TypeOfMatch;
		MaxSearchResults = MaxNumberOfSearchResults;
		LobbyPath = FString::Printf(TEXT("%s?listen"), *PathOfLobby);
		UGameInstance* GameInstance = GetGameInstance();
		if (GameInstance)
		{
			MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		}
		// Bind the callbacks to the custom delegates once the menu is set up
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
			MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		}
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
		}
	}
}

void ALobbyGameMode::OnCreateSession(bool bWasSuccessful)
{
	// Print the session created result
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				FString(TEXT("Created Successfully"))
			);
		}
		// Server Travel to the Lobby Level
		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel(LobbyPath);
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("Failed to create session!"))
			);
		}
	}
}

void ALobbyGameMode::OnStartSession(bool bWasSuccessful)
{

}

void ALobbyGameMode::OnDestroySession(bool bWasSuccessful)
{

}