// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterComponents/CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Weapon/Weapon.h"
#include "Character/MainCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/ShooterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/ShooterPlayerController.h"
#include "Weapon/Projectile.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize the CarriedAmmoMap.
	InitCarriedAmmoMap();
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// MainCharacter is initialized in MainCharacter.cpp, PostInitializeComponents() function.
	if (MainCharacter)
	{
		UpdateCharacterSpeed();
		DefaultFOV = MainCharacter->GetFollowCamera()->FieldOfView;
		InterpFOV = DefaultFOV;
		if (EquippedWeapon) CrosshairSpread = EquippedWeapon->CrosshairsMinSpread;
	}
	// Initialize the CarriedAmmoMap.
	InitCarriedAmmoMap();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
	
	UpdateHUDCrosshairs(DeltaTime);
	AimZooming(DeltaTime);
	
}

void UCombatComponent::UpdateCharacterSpeed()
{
	if (MainCharacter && MainCharacter->GetCharacterMovement())
	{
		MainCharacter->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
		MainCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = bAiming ? AimCrouchWalkSpeed : BaseCrouchWalkSpeed;
	}
}

//void UCombatComponent::EquipWeapon_Implementation(AWeapon* WeaponToEquip)
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!MainCharacter || !WeaponToEquip) return;

	// Drop equipped weapon.
	if (EquippedWeapon) EquippedWeapon->Dropped();

	// Set weapon and its state.
	EquippedWeapon = WeaponToEquip;
	bAutomaticFire = EquippedWeapon->GetCanAutoFire();
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	// Automatically propagated to the clients, that's why we don't need to do attachment on the client again.
	AttachWeaponToRightHand();
	
	EquippedWeapon->SetOwner(MainCharacter);

	// Show the HUD: weapon type, ammo amount, carried ammo amount.
	SetHUDWeaponType();
	EquippedWeapon->SetHUDAmmo();
	SetCarriedAmmoFromMap(EquippedWeapon->GetWeaponType());	// Set carried ammo and display the HUD.
	
	// Play equip sound.
	if (MainCharacter->IsLocallyControlled())
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquippedSound, MainCharacter->GetActorLocation(), FRotator::ZeroRotator);
	}
	
	MainCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	MainCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::SetCombatState(const ECombatState State)
{
	CombatState = State;
	HandleCombatState();
}

void UCombatComponent::SetCarriedAmmo(int32 Amount)
{
	CarriedAmmo = Amount;
	HandleCarriedAmmo();
}

void UCombatComponent::HandleCarriedAmmo()
{
	if (!EquippedWeapon) return;
	
	SetHUDCarriedAmmo();
	UpdateCarriedAmmoMap({EquippedWeapon->GetWeaponType(), CarriedAmmo});

	// Jump to the end section of animation when the carried ammo is not enough to fulfill the clip or the clip has been fulfilled during reloading.
	if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		(EquippedWeapon->IsAmmoFull() || IsCarriedAmmoEmpty() && !EquippedWeapon->IsAmmoFull()))
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (!MainCharacter || !EquippedWeapon) return;
	
	bAiming = bIsAiming;
	UpdateCharacterSpeed();

	// Sniper scope effect when aiming. Be aware of IsLocallyControlled check.
	if (MainCharacter->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		MainCharacter->ShowSniperScopeWidget(bIsAiming);
	}
}

void UCombatComponent::Fire()
{
	// Be aware that sequence is important, because when we fire out the last ammo, the ammo will be 0
	// and reload will be executed immediately.
	if (EquippedWeapon && EquippedWeapon->IsAmmoEmpty())
	{
		Reload();
	}
	if (CanFire())
	{
		if (!MainCharacter || !EquippedWeapon || CombatState != ECombatState::ECS_Unoccupied) return;

		AimFactor += EquippedWeapon->GetRecoilFactor();
		MainCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(HitTarget);

		StartFireTimer();
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	// Everytime we call the RPC, the data will be sent across the network. And with multiplayer game, the less data we sent, the better.
	// It's only for things which are very important in the game such as shooting will need RPC.
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed) Fire();
}

bool UCombatComponent::CanFire() const
{
	return bRefireCheck && EquippedWeapon && !EquippedWeapon->IsAmmoEmpty();
}

void UCombatComponent::StartFireTimer()
{
	if (!MainCharacter || !EquippedWeapon) return;

	bRefireCheck = false;
	MainCharacter->GetWorldTimerManager().SetTimer(FireTimer, this, &ThisClass::FireTimerFinished, EquippedWeapon->GetFireRate(), false);
}

void UCombatComponent::FireTimerFinished()
{
	bRefireCheck = true;
	if (bAutomaticFire && bFireButtonPressed) Fire();
}

void UCombatComponent::SwitchFireModeButtonPressed()
{
	if (!EquippedWeapon) return;
	
	if (bAutomaticFire && EquippedWeapon->GetCanSemiAutoFire() ||
		!bAutomaticFire && EquippedWeapon->GetCanAutoFire())
			bAutomaticFire = !bAutomaticFire;
}

void UCombatComponent::SetHUDCarriedAmmo()
{
	if (!MainCharacter) return;
	
	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(MainCharacter->Controller);
	if (ShooterPlayerController) ShooterPlayerController->UpdateCarriedAmmo(CarriedAmmo);
}

void UCombatComponent::InitCarriedAmmoMap()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, 10);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, 5);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, 30);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, 45);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, 5);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, 3);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, 4);
}

void UCombatComponent::SetCarriedAmmoFromMap(EWeaponType WeaponType)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		SetCarriedAmmo(CarriedAmmoMap[WeaponType]);
	}
}

int32 UCombatComponent::GetCarriedAmmoFromMap(EWeaponType WeaponType)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		return CarriedAmmoMap[WeaponType];
	}
	return -1;
}

void UCombatComponent::UpdateCarriedAmmoMap(const TPair<EWeaponType, int32>& CarriedAmmoPair)
{
	if (CarriedAmmoMap.Contains(CarriedAmmoPair.Key))
	{
		CarriedAmmoMap[CarriedAmmoPair.Key] = CarriedAmmoPair.Value;
	}
}

void UCombatComponent::SetHUDWeaponType()
{
	if (!MainCharacter || !EquippedWeapon) return;

	FString WeaponType;
	switch(EquippedWeapon->GetWeaponType())
	{
	case EWeaponType::EWT_AssaultRifle:
		WeaponType = FString("Assault Rifle");
		break;
	case EWeaponType::EWT_RocketLauncher:
		WeaponType = FString("Rocket Launcher");
		break;
	case EWeaponType::EWT_Pistol:
		WeaponType = FString("Pistol");
		break;
	case EWeaponType::EWT_SMG:
		WeaponType = FString("SMG");
		break;
	case EWeaponType::EWT_Shotgun:
		WeaponType = FString("Shotgun");
		break;
	case EWeaponType::EWT_SniperRifle:
		WeaponType = FString("Sniper Rifle");
		break;
	case EWeaponType::EWT_GrenadeLauncher:
		WeaponType = FString("Grenade Launcher");
		break;
	case EWeaponType::EWT_MAX:
		WeaponType = FString("");
		break;
	}
	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(MainCharacter->Controller);
	if (ShooterPlayerController) ShooterPlayerController->UpdateWeaponType(WeaponType);
}

void UCombatComponent::ReloadAnimNotify()
{
	if (!MainCharacter || !EquippedWeapon) return;

	/* Rep notify problem. bFireButtonPressed and bAutomaticFire are local variables, so we should not check
	 * authority here, or the client will immediately call Fire() while the Ammo is not updated. */
	MainCharacter->SetCombatState(ECombatState::ECS_Unoccupied);
	ReloadAmmoAmount();	// Ammo and CarriedAmmo Rep Notify triggered.
	HandleCarriedAmmo();
	// Local variable, so we needn't check IsLocallyControlled().
	if (bFireButtonPressed && bAutomaticFire) Fire();
}

void UCombatComponent::ShotgunShellAnimNotify()
{
	if (MainCharacter)
	{
		if (!EquippedWeapon || !MainCharacter->GetMesh()) return;

		// Recover the Combat State, though we are still reloading, but the shotgun reload mechanism is special, it can be
		// interrupted by the fire button after reload an ammo, so we reset the combat state.
		MainCharacter->SetCombatState(ECombatState::ECS_Unoccupied);
		
		// For shotgun, change 1 ammo per reload.
		EquippedWeapon->SetAmmo(EquippedWeapon->GetAmmo() + 1);
		EquippedWeapon->SetHUDAmmo();
		SetCarriedAmmo(GetCarriedAmmo() - 1);
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = MainCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance && MainCharacter->GetReloadMontage())
	{
		// Interrupt the animation right now.
		AnimInstance->Montage_JumpToSection(FName("EndShotgun"));
	}
}

void UCombatComponent::ThrowGrenadeAnimNotify()
{
	if (MainCharacter)
	{
		SetCombatState(ECombatState::ECS_Unoccupied);

		AttachWeaponToRightHand();
	}
}

void UCombatComponent::LaunchGrenadeAnimNotify()
{
	if (MainCharacter && MainCharacter->IsLocallyControlled())
	{
		LaunchGrenade(HitTarget);
	}
	// Hide the grenade mesh on all machine.
	ShowGrenadeAttached(false);
}

void UCombatComponent::Reload()
{
	if (!MainCharacter || IsCarriedAmmoEmpty() || CombatState != ECombatState::ECS_Unoccupied ||
		!EquippedWeapon || EquippedWeapon->IsAmmoFull()) return;

	CombatState = ECombatState::ECS_Reloading;
	MainCharacter->PlayReloadMontage();
}

void UCombatComponent::ReloadAmmoAmount()
{
	if (!EquippedWeapon || !EquippedWeapon->IsAmmoValid()) return;

	// If the carried ammo is enough to full the clip after reloading.
	if (CarriedAmmo >= (EquippedWeapon->GetClipSize() - EquippedWeapon->GetAmmo()))
	{
		// Sequence is important
		const int32 AmmoSupplied = EquippedWeapon->GetClipSize() - EquippedWeapon->GetAmmo();
		SetCarriedAmmo(CarriedAmmo - AmmoSupplied);
		EquippedWeapon->SetAmmo(EquippedWeapon->GetClipSize());
	}
	// If the carried ammo cannot full the clip after reloading.
	else
	{
		// Sequence is important
		EquippedWeapon->SetAmmo(EquippedWeapon->GetAmmo() + CarriedAmmo);
		SetCarriedAmmo(0);
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (!MainCharacter || CombatState != ECombatState::ECS_Unoccupied || IsGrenadeEmpty()) return;

	SetCombatState(ECombatState::ECS_Throwing);
	SetGrenadeAmount(Grenade - 1);

	AttachWeaponToLeftHand();

	MainCharacter->PlayThrowGrenadeMontage();
}

void UCombatComponent::LaunchGrenade(const FVector_NetQuantize& Target)
{
	if (!MainCharacter || !MainCharacter->GetMesh()) return;
	
	// Spawn and launch grenade.
	if (const UStaticMeshComponent* GrenadeMesh = MainCharacter->GetGrenadeAttached())
	{
		const FVector Dir = Target - GrenadeMesh->GetComponentLocation();

		// The socket location is in the collision range of hand's capsule mesh, so we need to spawn a bit further from the
		// original location to avoid collision.
		constexpr float SafeDist = 50.f;
		const FVector SpawnLocation = GrenadeMesh->GetComponentLocation() + Dir.GetSafeNormal() * SafeDist;
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = MainCharacter;
		SpawnParams.Instigator = MainCharacter;
		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<AProjectile>(
				ProjectileClass,
				SpawnLocation,
				Dir.Rotation(),
				SpawnParams
			);
		}
	}
}


void UCombatComponent::AttachWeaponToLeftHand()
{
	if (!MainCharacter || !MainCharacter->GetMesh() || !EquippedWeapon) return;
	
	if (const USkeletalMeshSocket* HandSocket = MainCharacter->GetMesh()->GetSocketByName(FName("LeftHandSocket")))
	{
		HandSocket->AttachActor(EquippedWeapon, MainCharacter->GetMesh());
	}
}

void UCombatComponent::AttachWeaponToRightHand()
{
	if (!MainCharacter || !MainCharacter->GetMesh() || !EquippedWeapon) return;
		
	if (const USkeletalMeshSocket* HandSocket = MainCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		HandSocket->AttachActor(EquippedWeapon, MainCharacter->GetMesh());
	}
}

void UCombatComponent::ShowGrenadeAttached(bool IsVisible)
{
	if (!MainCharacter || !MainCharacter->GetGrenadeAttached()) return;
	
	MainCharacter->GetGrenadeAttached()->SetVisibility(IsVisible);
}

void UCombatComponent::SetGrenadeAmount(int32 Amount)
{
	Grenade = FMath::Clamp(Amount, 0, MaxGrenade);

	HandleGrenadeRep();
}

void UCombatComponent::HandleGrenadeRep()
{
	if (!MainCharacter || !MainCharacter->IsLocallyControlled()) return;
	
	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(MainCharacter->GetController());
	if (ShooterPlayerController)
	{
		ShooterPlayerController->UpdateGrenade(Grenade);
	}
}

void UCombatComponent::HandleCombatState()
{
	if (!MainCharacter) return;

	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		break;
	case ECombatState::ECS_Reloading:
		MainCharacter->PlayReloadMontage();
		break;
	case ECombatState::ECS_Throwing:
		MainCharacter->PlayThrowGrenadeMontage();
		ShowGrenadeAttached(true);
		break;
	case ECombatState::ECS_MAX:
		break;
	}
}




/**
 * Project a line trace from the center of the screen to the target.
 */
void UCombatComponent::TraceUnderCrosshairs(FHitResult& HitResult)
{
	// 'TraceUnderCrosshair' is a machine-related function, so it shouldn't be called by another machine.
	if (!MainCharacter || !MainCharacter->IsLocallyControlled()) return;
	
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	FVector2D CrosshairScreenLocation(ViewportSize * 0.5f);
	FVector CrosshairWorldLocation;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairScreenLocation,
		CrosshairWorldLocation,
		CrosshairWorldDirection
	);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldLocation;
		// A little trick about the distance and direction
		const float DistanceToCharacter = (CrosshairWorldLocation - MainCharacter->GetActorLocation()).Size();
		Start += (DistanceToCharacter + 100.f) * CrosshairWorldDirection;
		const FVector End = CrosshairWorldLocation + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (!HitResult.bBlockingHit)
		{
			HitResult.ImpactPoint = End;
		}
		if (HitResult.GetActor())
		{
			const bool bIsImplemented = HitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>();
			CrosshairColor = bIsImplemented ? FColor::Red : FColor::White;
		}
		else
		{
			CrosshairColor = FColor::White;
		}
	}
	HitTarget = HitResult.ImpactPoint;
}

void UCombatComponent::UpdateHUDCrosshairs(float DeltaTime)
{
	if (!MainCharacter || !MainCharacter->IsLocallyControlled()) return;
	if (!MainCharacter->Controller) return;
	
	// Instanced by casting.
	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(MainCharacter->Controller);
	if (!ShooterPlayerController) return;

	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(ShooterPlayerController->GetHUD());

	UpdateCrosshairSpread(DeltaTime);
	SetHUDPackage();
	
}

void UCombatComponent::UpdateCrosshairSpread(float DeltaTime)
{
	if (!MainCharacter || !ShooterHUD || !EquippedWeapon) return;

	// Interpolate the spread when the player is moving
	if (MainCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f)
	{
		VelocityFactor = FMath::FInterpTo(VelocityFactor, 1.f, DeltaTime, VelocityFactor_InterpSpeed);
	}
	else
	{
		VelocityFactor = FMath::FInterpTo(VelocityFactor, 0.f, DeltaTime, VelocityFactor_InterpSpeed);
	}

	// Interpolate the spread when the player is jumping
	if (MainCharacter->GetCharacterMovement()->IsFalling())
	{
		AirFactor = FMath::FInterpTo(AirFactor, 1, DeltaTime, 1.f);
	}
	else
	{
		AirFactor = FMath::FInterpTo(AirFactor, 0, DeltaTime, AirFactor_InterpSpeed);
	}
	if (bAiming)
	{
		AimFactor = FMath::FInterpTo(AimFactor, EquippedWeapon->GetAimAccuracy(), DeltaTime, AimFactor_InterpSpeed);
	}
	else
	{
		AimFactor = FMath::FInterpTo(AimFactor, 1.f, DeltaTime, AimFactor_InterpSpeed);
	}
	const float FinalFactor = VelocityFactor + AirFactor;
	
	// The DrawHUD function will be automatically called when we set the default HUD as BP_ShooterHUD in BP_GameMode settings.
	CrosshairSpread = 
		FinalFactor * (EquippedWeapon->CrosshairsMaxSpread - EquippedWeapon->CrosshairsMinSpread) +
		AimFactor * EquippedWeapon->CrosshairsMinSpread;
}

void UCombatComponent::SetHUDPackage()
{
	if (ShooterHUD == nullptr) return;

	if (EquippedWeapon)
	{
		ShooterHUD->SetHUDPackage(
			FHUDPackage(
				EquippedWeapon->CrosshairsCenter,
				EquippedWeapon->CrosshairsLeft,
				EquippedWeapon->CrosshairsRight,
				EquippedWeapon->CrosshairsTop,
				EquippedWeapon->CrosshairsBottom,
				CrosshairSpread,
				CrosshairColor
			)
		);
	}
	// In cases that we change a weapon, we need to refresh first.
	else
	{
		ShooterHUD->SetHUDPackage(
			FHUDPackage( nullptr, nullptr, nullptr, nullptr, nullptr, 0.f)
		);
	}
}

void UCombatComponent::AimZooming(float DeltaTime)
{
	if (!MainCharacter || !EquippedWeapon || !MainCharacter->IsLocallyControlled()) return;
	
	if (bAiming)
	{
		InterpFOV = FMath::FInterpTo(InterpFOV, EquippedWeapon->GetAim_FOV(), DeltaTime, EquippedWeapon->GetZoomInSpeed());
	}
	else
	{
		InterpFOV = FMath::FInterpTo(InterpFOV, DefaultFOV, DeltaTime, DefaultZoomOutSpeed);
	}
	MainCharacter->GetFollowCamera()->FieldOfView = InterpFOV;
}
