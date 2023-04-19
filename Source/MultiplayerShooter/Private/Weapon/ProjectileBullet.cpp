// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"
#include "Weapon/ShooterMovementComponent.h"
#include "Character/MainCharacter.h"
#include "Kismet/GameplayStatics.h"

AProjectileBullet::AProjectileBullet()
{
	ShooterMovementComponent = CreateDefaultSubobject<UShooterMovementComponent>(TEXT("Bullet Movement Component"));
	ShooterMovementComponent->bRotationFollowsVelocity = true;
	ShooterMovementComponent->InitialSpeed = 15000.f;
	ShooterMovementComponent->MaxSpeed = 15000.f;
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("AProjectileBullet::OnHit%s: %d"), *OtherActor->GetName(), OtherActor->GetLocalRole());
	/*
	// The Owner/Instigator is set in SpawnParams when we spawn the projectile
	const APawn* ProjectileInstigator = GetInstigator();
	//if (!ProjectileInstigator) return;

	// If we hit ourselves, it'll not trigger the HitImpact.
	if (OtherActor == GetOwner()) return;

	// ApplyDamage logic
	UE_LOG(LogTemp, Warning, TEXT("Apply damage: %.2f, %d"), Damage, UGameplayStatics::GetPlayerPawn(GetWorld(), 0)==OtherActor);
	if(UGameplayStatics::GetPlayerPawn(GetWorld(), 0) != OtherActor)
		UGameplayStatics::ApplyDamage(OtherActor, Damage, nullptr, this, UDamageType::StaticClass());
	else
		return;
	
	// Destroy() will be called, so Super::OnHit should be called at last.
	*/
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

