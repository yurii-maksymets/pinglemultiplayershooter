// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupMain.generated.h"

UCLASS()
class MULTIPLAYERSHOOTER_API APickupMain : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickupMain();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnOverlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditDefaultsOnly)
	class USphereComponent* PickupArea;
	UPROPERTY(EditDefaultsOnly)
	class UStaticMeshComponent* PickupMesh;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
