// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterMatchMode.h"
#include "PlayerController/ShooterPlayerController.h"
#include "GameState/ShooterGameState.h"
#include "HUD/ShooterHUD.h"
#include "HUD/AnnouncementWidget.h"

void AShooterMatchMode::StartPlay()
{
	Super::StartPlay();
	bUseSeamlessTravel = true;
	AShooterGameState* GS = Cast<AShooterGameState>(GetWorld()->GetGameState());
	if (GS)
	{
		GS->StartMatchTimer(MatchPlayTime);
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("No Game State yet"));
	FTimerHandle TH;
	FTimerDelegate TD;
	TD.BindLambda([this]{FinishGame();});
	GetWorldTimerManager().SetTimer(TH, TD, MatchPlayTime, false);
}

void AShooterMatchMode::PostLogin(APlayerController* PlayerController)
{
	Super::PostLogin(PlayerController);
	if (PlayerController)
	{
		AShooterPlayerController* ShooterPC = Cast<AShooterPlayerController>(PlayerController);
	}
	
}

void AShooterMatchMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);
	AShooterPlayerController* pc = Cast<AShooterPlayerController>(C);
	if (pc)
		pc->StartLocalMatchTimer();
}

void AShooterMatchMode::HandleStartingNewPlayer(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer(NewPlayer);

}

void AShooterMatchMode::FinishGame()
{
	FString map = MapsAllowed.Pop();
	UE_LOG(LogTemp, Warning, TEXT("FINISH GAME %s"), *map);
	MapsAllowed.Add(map);
	GetWorld()->ServerTravel(FString("/Game/Maps/").Append(map), false);
}