// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/Pickup.h"
#include "Character/MainCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
	
	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Pickup Collision"));
	PickupCollision->SetupAttachment(RootComponent);
	PickupCollision->AddWorldOffset(FVector(0.f, 0.f, 85.f));
	PickupCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup Mesh"));
	PickupMesh->SetupAttachment(PickupCollision);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara Component"));
	NiagaraComponent->SetupAttachment(PickupCollision);
	
	// Stencil -- Outline effect
	PickupMesh->SetRenderCustomDepth(true);
	PickupMesh->SetCustomDepthStencilValue(DEPTH_BLUE);
	bReplicates = true;
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickup::Destroyed()
{
	Super::Destroyed();
	
	if (SoundPickup)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundPickup, GetActorLocation());
	}
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereBeginOverlap);
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &APickup::Turn, TurnTimerRate, true);
}

void APickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMainCharacter* MainCharacter = Cast<AMainCharacter>(OtherActor);
	if (MainCharacter && GetLocalRole() == ENetRole::ROLE_Authority)
	{
		SpawnBuffEffectAttached(MainCharacter);
	}
	Destroy();
}

void APickup::SpawnBuffEffectAttached_Implementation(AMainCharacter* AttachedCharacter) const
{
	if (NiagaraEffect && AttachedCharacter && AttachedCharacter->GetMesh())
	{
		const FVector BuffSocketLocation = AttachedCharacter->GetMesh()->GetSocketLocation(FName("BuffEffect"));
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraEffect,
			AttachedCharacter->GetMesh(),
			FName(),
			BuffSocketLocation,
			FRotator::ZeroRotator,
			EAttachLocation::KeepWorldPosition,
			true
		);
	}
}

void APickup::Turn()
{
	AddActorWorldRotation(FRotator(0.f, BaseTurnRate * TurnTimerRate, 0.f));
}
