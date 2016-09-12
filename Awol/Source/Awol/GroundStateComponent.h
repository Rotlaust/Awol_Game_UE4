// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "GroundStateComponent.generated.h"

class USkateboardTune;

/**
* This component is responsible for resolving a SkateboardSimPawn's interaction with the ground.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AWOL_API UGroundStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGroundStateComponent();

	// Called after construction
	void SetSkateboardTune(const USkateboardTune* tune) { m_SkateboardTune = tune; }

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	// Probe for the ground based on our current position and forward/right vectors, where the forward vector is our direction of motion.
	// Returns true if the ground was found, false otherwise.
	bool ProbeGround(const FVector &pos, const FVector &forward, const FVector &right);

	// Called to notify us that a collision has occurred
	void NotifyCollision(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	// Helper function to do a physics probe for rideable surfaces
	bool ProbeRideable(const FVector& start, const FVector& end, FHitResult& hitOut);

	// Is this pawn touching the ground?
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim|GroundState")
	bool IsOnGround() const;

	// Get the position, in world space, where we're making contact with the ground.  Only valid if IsOnGround() is true.
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim|GroundState")
	FVector GetGroundPosition() const;

	// Get the normal vector for our ground contact point.  Only valid if IsOnGround() is true.
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim|GroundState")
	FVector GetGroundNormal() const;

private:
	const USkateboardTune* GetSkateboardTune() const;

	// Reset our internal state
	void ResetState();

	void DebugDraw() const;

private:
	bool m_IsOnGround;
	FVector m_GroundPosition;
	FVector m_GroundNormal;

	// Non-custodial pointer
	const USkateboardTune* m_SkateboardTune;
};
