// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetworkedGameplayPPPCharacter.h"
#include "NetworkedGameplayPPPProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"


//////////////////////////////////////////////////////////////////////////
// ANetworkedGameplayPPPCharacter

ANetworkedGameplayPPPCharacter::ANetworkedGameplayPPPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	TurnRateGamepad = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	maxHealth = 100.0f;
	currentHealth = maxHealth;

	projectileClass = ANetworkedGameplayPPPProjectile::StaticClass();
	fireRate = 0.25f;
	isFiringWeapon = false;
}

void ANetworkedGameplayPPPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkedGameplayPPPCharacter, currentHealth);
}

void ANetworkedGameplayPPPCharacter::SetCurrentHealth(float healthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		currentHealth = FMath::Clamp(healthValue, 0.0f, maxHealth);
		OnHealthUpdate();
	}
}

float ANetworkedGameplayPPPCharacter::TakeDamage(float damageTaken, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser)
{
	float damageApplied = currentHealth - damageTaken;
	SetCurrentHealth(damageApplied);
	return damageApplied;
}

void ANetworkedGameplayPPPCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

}

//////////////////////////////////////////////////////////////////////////// Input

void ANetworkedGameplayPPPCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ANetworkedGameplayPPPCharacter::StartFire);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ANetworkedGameplayPPPCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ANetworkedGameplayPPPCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "Mouse" versions handle devices that provide an absolute delta, such as a mouse.
	// "Gamepad" versions are for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &ANetworkedGameplayPPPCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &ANetworkedGameplayPPPCharacter::LookUpAtRate);
}

void ANetworkedGameplayPPPCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void ANetworkedGameplayPPPCharacter::OnHealthUpdate()
{
	if (IsLocallyControlled())
	{
		FString healthMessage = FString::Printf(TEXT("%f health remaining"), currentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, healthMessage);

		if (currentHealth <= 0)
		{
			FString deathMessage = FString::Printf(TEXT("Deadge"));
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, deathMessage);
		}
	}
	
	if (GetLocalRole() == ROLE_Authority)
	{
		FString healthMessage = FString::Printf(TEXT("%s has %f health remaining"), *GetFName().ToString(), currentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, healthMessage);
	}
}

void ANetworkedGameplayPPPCharacter::StartFire()
{
	if (!isFiringWeapon)
	{
		isFiringWeapon = true;
		UWorld* world = GetWorld();
		world->GetTimerManager().SetTimer(FiringTimer, this, &ANetworkedGameplayPPPCharacter::StopFire, fireRate, false);
		HandleFire();
	}
}

void ANetworkedGameplayPPPCharacter::StopFire()
{
	isFiringWeapon = false;
}

void ANetworkedGameplayPPPCharacter::HandleFire_Implementation()
{
	FVector spawnLocation = GetActorLocation() + (GetControlRotation().Vector() * 100.0f) + (GetActorUpVector() * 50.0f);
	FRotator spawnRotation = GetControlRotation();

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	ANetworkedGameplayPPPProjectile* spawnProjectile = GetWorld()->SpawnActor<ANetworkedGameplayPPPProjectile>(spawnLocation, spawnRotation, spawnParameters);
}

void ANetworkedGameplayPPPCharacter::OnPrimaryAction()
{
	// Trigger the OnItemUsed Event
	OnUseItem.Broadcast();
}

void ANetworkedGameplayPPPCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnPrimaryAction();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void ANetworkedGameplayPPPCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void ANetworkedGameplayPPPCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ANetworkedGameplayPPPCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ANetworkedGameplayPPPCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ANetworkedGameplayPPPCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

bool ANetworkedGameplayPPPCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &ANetworkedGameplayPPPCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &ANetworkedGameplayPPPCharacter::EndTouch);

		return true;
	}
	
	return false;
}
