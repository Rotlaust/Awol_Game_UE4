// Fill out your copyright notice in the Description page of Project Settings.

#include "Awol.h"
#include "SkateboardTune.h"


// Sets default values for this component's properties
USkateboardTune::USkateboardTune()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = false;

	// Set reasonable default values
	MaxSpeed = 600.0f;
	MinMaxTurnAngleDeg = 10.0f;
	DeckHeight = 10.0f;
	TruckSpacing = 55.0f;
	AxleLength = 20.0f;
}


// Called when the game starts
void USkateboardTune::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

