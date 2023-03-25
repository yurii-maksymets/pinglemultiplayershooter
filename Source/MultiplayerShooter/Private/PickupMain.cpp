// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupMain.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

// Sets default values
APickupMain::APickupMain()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PickupArea = CreateDefaultSubobject<USphereComponent>(TEXT("Area"));
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	PickupArea->SetupAttachment(RootComponent);
	PickupMesh->SetupAttachment(PickupArea);
	PickupArea->SetRelativeLocation(FVector::ZeroVector);
	PickupMesh->SetRelativeLocation(FVector::ZeroVector);
	PickupArea->SetSphereRadius(200);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupArea->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupArea->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	TScriptDelegate OverlapDeleg;
	OverlapDeleg.BindUFunction(this, FName("OnOverlapped"));
	PickupArea->OnComponentBeginOverlap.Add(OverlapDeleg);
}

// Called when the game starts or when spawned
void APickupMain::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APickupMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickupMain::OnOverlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(GetLocalRole() == ENetRole::ROLE_Authority)
	{
		UE_LOG(LogTemp, Warning, TEXT("Overlapped!"));
	}
}