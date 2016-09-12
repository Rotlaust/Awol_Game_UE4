// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "SkateboardSimPawn.generated.h"

class UGroundStateComponent;
class USkateboardTune;

/**
* The high-level pawn that handles the skateboard simulation.
*/
UCLASS()
class AWOL_API ASkateboardSimPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASkateboardSimPawn();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	// The root pivot of our skateboard model:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool BindInputInCode;

	UFUNCTION(BlueprintCallable, Category = "SkateboardSim")
	void Input_MoveForward(float AxisValue);
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim")
	void Input_MoveRight(float AxisValue);
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim")
	void Input_CameraYaw(float AxisValue);
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim")
	void Input_CameraPitch(float AxisValue);

	// Physics OnHit callback
	UFUNCTION()
	void OnActorBump(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	// Set the world rotation of the camera's spring arm (to match the previous camera)
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim")
	void SetCameraBoomWorldRotation(FRotator cameraBoomWorldRotation);

	// Get the world rotation of the camera's spring arm
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim")
	FRotator GetCameraBoomWorldRotation() const;

	// Set the rider orientation
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim")
	void SetForwardVector(FVector forwardVector);

	// Get the rider orientation
	UFUNCTION(BlueprintCallable, Category = "SkateboardSim")
	FVector GetForwardVector2D() const;

	///// Blueprint-accessible components /////

	// The spring arm for the camera:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USpringArmComponent* SpringArm;

	// The camera we use when on a skateboard:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCameraComponent* Camera;

	// The mesh component that serves as our physical bound:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* MeshComp;

	// The root pivot of our skateboard model:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* SkateboardModelPivot;

	// The root pivot of our rider model:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* RiderModelPivot;

	// The physics material to use for our collision bound:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPhysicalMaterial* BoundPhysMtl;

	// Tuning values that pertain to the skateboard.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkateboardTune* SkateboardTune;

	// Our ground state component, which keeps track of our interaction with the ground:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UGroundStateComponent* GroundStateComp;

	// Enable/disable debug draw:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool DebugDrawEnabled;

private:
	void UpdateOrientation();
	void UpdateSteering(float deltaTime);
	void UpdateMovement(float deltaTime);
	void UpdateCamera();
	void UpdateSkateboardModel();
	void UpdateRiderModel();
	void UpdatePrevVelocity();

	// Compute desired steering angle, in degrees
	float ComputeSteerAngleDeg() const;

	// Compute unit force (mass==1) to apply in direction of GetForwardVector()
	FVector ComputeForwardForce() const;

	// Compute unit force (mass==1) to apply in direction of GetRightVector()
	FVector ComputeRightForce(float deltaTime) const;

	// Compute centripetal acceleration, in cm/s^2
	FVector ComputeCentripetalAccel() const;

	// Given current position, and previous and current velocity in cm/s,
	// compute the circular turn pivot.  (The magnitude of the return value is
	// the turn radius in cm.)
	FVector ComputeTurnPivot(FVector pos, FVector v1, FVector v2) const;

	// Returns a unit vector pointing in the direction of travel.
	// NOTE: this vector can abruptly reverse, e.g. at the apex of a slope.
	FVector GetForwardVector() const { return (m_Reverse ? -m_LongitudinalVector : m_LongitudinalVector); }

	// Returns a unit vector pointing perpendicular to the direction of travel.
	// NOTE: this vector can abruptly reverse, e.g. at the apex of a slope.
	FVector GetRightVector() const { return (m_Reverse ? -m_LateralVector : m_LateralVector); }

	FVector GetUpVector() const { return FVector::CrossProduct(GetForwardVector(), GetRightVector()); }

	FVector GetRiderUpVector() const;

	FVector GetCenterOfMassPos() const;

	FVector GetTopOfDeckPos() const;

	void ResetOrientation(const FVector up);

	void DebugDraw() const;

private:

	// Input variables
	FVector m_MovementInput;
	FVector m_CameraInput;

	// Physics state
	FVector m_PrevVelocity;

	// Orientation state
	FVector m_LateralVector; // Perpendicular to m_LongitudinalVector; m_LongitudinalVector X m_LateralVector = ground normal.
	FVector m_LongitudinalVector; // Direction board is pointing; velocity is +/- this vector.
	bool m_Reverse; // If true, velocity is negative along m_LongitudinalVector.

	// Normalized steering value
	float m_Steering;
};
