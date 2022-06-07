// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetworkedGameplayPPPGameMode.h"
#include "NetworkedGameplayPPPCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANetworkedGameplayPPPGameMode::ANetworkedGameplayPPPGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
