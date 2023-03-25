// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterMatchMode.h"
#include "PlayerController/ShooterPlayerController.h"
#include "GameState/ShooterGameState.h"
#include "HUD/ShooterHUD.h"
#include "HUD/AnnouncementWidget.h"
#include "Kismet/GameplayStatics.h"

AShooterMatchMode::AShooterMatchMode(): Super()
{
}

void AShooterMatchMode::StartPlay()
{
	Super::StartPlay();
	bUseSeamlessTravel = true;
	FTimerHandle TH;
	FTimerDelegate TD;
	FTimerHandle TH2;
	FTimerDelegate TD2;
	TD2.BindLambda([&]{
		AShooterGameState* GS = Cast<AShooterGameState>(GetWorld()->GetGameState());
		GS->StartMatchTimer(MatchPlayTime);
	});
	GetWorldTimerManager().SetTimer(TH2, TD2, 0.5f, false);
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
	FString map = MapsAllowed[0];
	MapsAllowed.RemoveAt(0);
	UE_LOG(LogTemp, Warning, TEXT("FINISH GAME %s"), *map);
	MapsAllowed.Shrink();
	MapsAllowed.Add(map);
	GetWorld()->ServerTravel(FString("/Game/Maps/").Append(map), false);
}

void AShooterMatchMode::Tick(float Delta)
{
	Super::Tick(Delta);
	float& Time = Cast<AShooterGameState>(GetWorld()->GetGameState())->MatchTimer;
	if (Time > 0)
		Time-=Delta;
}