// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "SkateboardTune.generated.h"


/**
* This component holds all tuning information having to do with the the skateboard.
*
* @see ASkateboardSimPawn
*/
UCLASS( ClassGroup=(Custom), HideCategories=(ComponentTick, Tags, Activation, Variable, Collision))
class AWOL_API USkateboardTune : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkateboardTune();

	// Called when the game starts
	virtual void BeginPlay() override;

	// The maximum speed, in cm/second
	UPROPERTY(EditAnywhere)
	float MaxSpeed;

	// The min/max turn angle, in degrees
	UPROPERTY(EditAnywhere)
	float MinMaxTurnAngleDeg;

	// The height of the deck off the ground, in cm.
	UPROPERTY(EditAnywhere)
	float DeckHeight;

	// The distance between the centers of the two trucks (axles), in cm.
	UPROPERTY(EditAnywhere)
	float TruckSpacing;

	// The length of the axle, in cm.  Or, the distance between the two wheels on the same truck.
	UPROPERTY(EditAnywhere)
	float AxleLength;
};
