// Fill out your copyright notice in the Description page of Project Settings.

#include "Awol.h"
#include "SkateboardSimPawn.h"
#include "SkateboardTune.h"
#include "GroundStateComponent.h"


// Sets default values
ASkateboardSimPawn::ASkateboardSimPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create root component, which will be a static mesh.
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	// Set the shape:
	const ConstructorHelpers::FObjectFinder<UStaticMesh> MeshObj(TEXT("/Game/StarterContent/Shapes/Shape_NarrowCapsule"));
	MeshComp->SetStaticMesh(MeshObj.Object);
	// Set the physical material:
	const ConstructorHelpers::FObjectFinder<UPhysicalMaterial> physMtlObj(TEXT("/Game/AWOL/Character/Mover/MoverPhysMtl"));
	BoundPhysMtl = physMtlObj.Object;
	if (GetWorld())
	{
		// Wrapping this call inside if (GetWorld()) prevents an unhandled excption when launching the Unreal Editor.
		// https://answers.unrealengine.com/questions/361842/ustaticmeshcomponent-setphysmaterialoverride-crash.html
		MeshComp->SetPhysMaterialOverride(physMtlObj.Object);
	}
	// Turn on physics:
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetEnableGravity(true);
	// Lock rotation
	MeshComp->BodyInstance.bLockXRotation = true;
	MeshComp->BodyInstance.bLockYRotation = true;
	MeshComp->BodyInstance.bLockZRotation = true;
	// Set as our root component:
	RootComponent = MeshComp;

	// Create camera spring arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 80.0f), FRotator(-60.0f, 0.0f, 0.0f));
	SpringArm->TargetArmLength = 300.0f;

	// Create a camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	// Attach our camera to our spring arm.  Offset and rotate the camera
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// Create a SkateboardTune component
	SkateboardTune = CreateDefaultSubobject<USkateboardTune>(TEXT("SkateboardTune"));
	AddOwnedComponent(SkateboardTune);

	// Create a GroundStateComponent:
	GroundStateComp = CreateDefaultSubobject<UGroundStateComponent>(TEXT("GroundState"));
	GroundStateComp->SetSkateboardTune(SkateboardTune);
	AddOwnedComponent(GroundStateComp);

	// Create visible skateboard pivot, which will drive its orientation:
	SkateboardModelPivot = CreateDefaultSubobject<USceneComponent>(TEXT("Skateboard Model Pivot"));
	SkateboardModelPivot->SetupAttachment(RootComponent);
	SkateboardModelPivot->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));

	// Create visible rider pivot, which will drive its orientation:
	RiderModelPivot = CreateDefaultSubobject<USceneComponent>(TEXT("Rider Model Pivot"));
	RiderModelPivot->SetupAttachment(RootComponent);
	RiderModelPivot->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));

	BindInputInCode = true;

	DebugDrawEnabled = false;
}

// Called when the game starts or when spawned
void ASkateboardSimPawn::BeginPlay()
{
	Super::BeginPlay();

	m_PrevVelocity = FVector::ZeroVector;

	m_LongitudinalVector = GetActorForwardVector();
	m_LateralVector = GetActorRightVector();
	m_Reverse = false;
	
	if (MeshComp != nullptr)
	{
		if (BoundPhysMtl != nullptr)
		{
			MeshComp->SetPhysMaterialOverride(BoundPhysMtl);
		}

		// Enable and add OnActorHit callback
		MeshComp->SetNotifyRigidBodyCollision(true);
		OnActorHit.AddUniqueDynamic(this, &ASkateboardSimPawn::OnActorBump);

		// Hide the visible sphere model:
		MeshComp->SetHiddenInGame(true);
		MeshComp->SetVisibility(false);
	}
}

// Called every frame
void ASkateboardSimPawn::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (GroundStateComp != nullptr)
	{
		FVector fwd = GetForwardVector();
		FVector right = GetRightVector();
		GroundStateComp->ProbeGround(GetActorLocation(), fwd, right);
	}

	UpdateOrientation();
	UpdateSteering(DeltaTime);
	UpdateMovement(DeltaTime);
	UpdateCamera();

	UpdateSkateboardModel();
	UpdateRiderModel();

	UpdatePrevVelocity();

	if (DebugDrawEnabled)
		DebugDraw();
}

void ASkateboardSimPawn::UpdatePrevVelocity()
{
	m_PrevVelocity = GetVelocity();
}

void ASkateboardSimPawn::UpdateOrientation()
{
	if (GroundStateComp != nullptr && GroundStateComp->IsOnGround())
	{
		// NOTE: THIS NEEDS TO BE REVISITED!
		// For now, pop the forward vector to point in the direction of motion, after
		// subtracting out velocity toward ground normal.

		FVector currVel = MeshComp->GetPhysicsLinearVelocity();
		// What's our speed relative to the ground normal?
		float normalSpeed = FVector::DotProduct(currVel, GroundStateComp->GetGroundNormal());
		// Subtract out speed pointing toward the ground:
		currVel -= GroundStateComp->GetGroundNormal() * normalSpeed;

		float speed = currVel.Size();
		float speedThresh = 5.0f;
		if (speed > speedThresh)
		{
			// Use the dot product to determine whether we're moving in reverse
			m_Reverse = (FVector::DotProduct(currVel, m_LongitudinalVector) < 0.0f);

			m_LongitudinalVector = currVel;
			m_LongitudinalVector.Normalize();
			if (m_Reverse)
				m_LongitudinalVector *= -1.0f;

			m_LateralVector = FVector::CrossProduct(GroundStateComp->GetGroundNormal(), m_LongitudinalVector);
		}
		else
		{
			m_LongitudinalVector = FVector::CrossProduct(m_LateralVector, GroundStateComp->GetGroundNormal());
		}
	}
}

void ASkateboardSimPawn::UpdateSteering(float deltaTime)
{
	// TODO: approach desired steering over time.
	m_Steering = FMath::Clamp(m_MovementInput.Y, -1.0f, 1.0f);
}

void ASkateboardSimPawn::UpdateMovement(float deltaTime)
{
	FVector currVel = MeshComp->GetPhysicsLinearVelocity();
	float speed = currVel.Size();
	float forceScale = 800.0f * MeshComp->GetMass();

	if (speed > SkateboardTune->MaxSpeed)
	{
		currVel.Normalize();
		currVel *= (SkateboardTune->MaxSpeed - speed);
		MeshComp->AddImpulse(currVel);

		// Still apply steering even if at max speed.
		FVector totalForce = ComputeRightForce(deltaTime) * forceScale;
		MeshComp->AddForce(totalForce);
	}
	else if (!m_MovementInput.IsZero())
	{
		FVector totalForce = (ComputeForwardForce() + ComputeRightForce(deltaTime)) * forceScale;

		// Apply force
		MeshComp->AddForce(totalForce);
	}


	// TODO:
	// [ ] Add drag for braking
	// [ ] Align heading to direction of travel
	// [ ] Add friction compensation forces to prevent lateral sliding

}

void ASkateboardSimPawn::UpdateCamera()
{
	if (SpringArm != nullptr)
	{
		FRotator newRotation = SpringArm->GetComponentRotation();
		newRotation.Yaw += m_CameraInput.X;
		newRotation.Pitch = FMath::Clamp(newRotation.Pitch - m_CameraInput.Y, -80.0f, 10.0f);
		SpringArm->SetWorldRotation(newRotation);
	}
}

void ASkateboardSimPawn::UpdateSkateboardModel()
{
	FMatrix rotMatrix(GetForwardVector(), GetRightVector(), GetUpVector(), FVector::ZeroVector);
	SkateboardModelPivot->SetWorldRotation(rotMatrix.Rotator());
}

void ASkateboardSimPawn::UpdateRiderModel()
{
	float deckHeight = 10.0f;
	if (SkateboardTune != nullptr)
	{
		deckHeight = SkateboardTune->DeckHeight;
	}
	// For now, don't apply deckHeight offset, because it's baked into the
	// animations.  Once the animations are fixed, we can change this back.
	// m_RiderModelPivot->SetRelativeLocation(FVector(0.0f, 0.0f, deckHeight));
	RiderModelPivot->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	FVector riderForward = GetForwardVector();
	FVector riderUp = GetRiderUpVector();
	FVector riderRight = FVector::CrossProduct(riderUp, riderForward);
	riderForward = FVector::CrossProduct(riderRight, riderUp);
	FMatrix rotMatrix(riderForward, riderRight, riderUp, FVector::ZeroVector);
	RiderModelPivot->SetWorldRotation(rotMatrix.Rotator());
}

float ASkateboardSimPawn::ComputeSteerAngleDeg() const
{
	float minMaxAngle = 10.0f;
	if (SkateboardTune != nullptr)
	{
		minMaxAngle = SkateboardTune->MinMaxTurnAngleDeg;
	}
	float rotDeg = FMath::Lerp(-minMaxAngle, minMaxAngle, (m_MovementInput.Y + 1.0f) * 0.5f);
	return rotDeg;
}

FVector ASkateboardSimPawn::ComputeForwardForce() const
{
	if (GroundStateComp != nullptr && GroundStateComp->IsOnGround() && m_MovementInput.X > 0.0f)
	{
		FVector newForward = GetForwardVector();
		float rotDeg = ComputeSteerAngleDeg();
		newForward = newForward.RotateAngleAxis(rotDeg, GetUpVector());
		return newForward * FMath::Clamp(m_MovementInput.X, 0.0f, 1.0f);
	}
	return FVector::ZeroVector;
}

FVector ASkateboardSimPawn::ComputeRightForce(float deltaTime) const
{
	if (GroundStateComp != nullptr && GroundStateComp->IsOnGround())
	{
		FVector currVel = MeshComp->GetPhysicsLinearVelocity();
		float speed = currVel.Size();

		if (speed > 1.0f)
		{
			float rotDeg = ComputeSteerAngleDeg();
			FVector tgtVel = currVel.RotateAngleAxis(rotDeg, GetUpVector());
			return (tgtVel - currVel) * deltaTime;
		}
	}
	return FVector::ZeroVector;
}

FVector ASkateboardSimPawn::ComputeCentripetalAccel() const
{
	FVector currVel = GetVelocity();
	FVector toCenter = ComputeTurnPivot(GetTopOfDeckPos(), m_PrevVelocity, currVel);
	if (toCenter.IsNearlyZero())
		return FVector::ZeroVector;

	float turnRadius = toCenter.Size();
	FVector centripAccel = toCenter;
	centripAccel.Normalize();

	// A = v^2 / r
	centripAccel *= (currVel.SizeSquared() / turnRadius);

	return centripAccel;
}

FVector ASkateboardSimPawn::ComputeTurnPivot(FVector pos, FVector v1, FVector v2) const
{
	float v1mag = v1.Size();
	float v2mag = v2.Size();
	
	// If either speed is less than threshold, consider the radius to be zero
	float thresh = 0.01f;
	if (v1mag < thresh || v2mag < thresh)
		return FVector::ZeroVector;

	float dot = FVector::DotProduct(v1, v1);
	float cosTheta = dot / (v1mag * v2mag);

	float theta = FMath::Acos(cosTheta);

	// r = chordlength / 2 * sin (theta/2)

	float radius = v2mag / (2.0f * FMath::Sin(theta * 0.5f));
	radius = FMath::Abs(radius);

	FVector upDown = FVector::CrossProduct(v1, v2);
	upDown.Normalize();

	FVector toCenter = FVector::CrossProduct(upDown, v2);
	toCenter.Normalize();
	toCenter *= radius;

	return toCenter;
}


FVector ASkateboardSimPawn::GetRiderUpVector() const
{
	FVector up = GetCenterOfMassPos() - GetTopOfDeckPos();

	// Protect against weirdness if these two positions are the same:
	if (up.SizeSquared() < 0.001)
		up = FVector::UpVector;
	else
		up.Normalize();

	return up;
}

FVector ASkateboardSimPawn::GetCenterOfMassPos() const
{
	// Compute an up vector, factoring in lean:
	FVector up = FVector::UpVector;
	/*
	// WIP : CENTRIPETAL FORCES AFFECTING LEAN
	// Scale by gravity, so we can add lateral accel to find lean:
	up *= 980.0f;
	// Add centripetal acceleration:
	FVector centripAccel = ComputeCentripetalAccel();
	up += centripAccel;
	// TODO: factor in crouch height based on vertical accel.
	// For now, keep at a constant height:
	up.Normalize();
	*/
	float comHeight = 100.0f;
	return GetActorLocation() + (up * comHeight);
}

FVector ASkateboardSimPawn::GetTopOfDeckPos() const
{
	// NOTE: This will eventually require more logic.  For now, just return a position above our actor location.
	float deckHeight = 10.0f;
	if (SkateboardTune != nullptr)
	{
		deckHeight = SkateboardTune->DeckHeight;
	}
	FVector groundPos = GetActorLocation();
	FVector offsetVector = FVector::UpVector;
	if (GroundStateComp != nullptr && GroundStateComp->IsOnGround())
	{
		offsetVector = GroundStateComp->GetGroundNormal();
		groundPos = GroundStateComp->GetGroundPosition();
	}
	return groundPos + (offsetVector * deckHeight);
}

void ASkateboardSimPawn::ResetOrientation(const FVector up)
{
	// Reset our orientation to something reasonable, using whichever vector has the smallest "up" amount:
	if (FMath::Abs(m_LongitudinalVector.Z) > FMath::Abs(m_LateralVector.Z))
	{
		m_LongitudinalVector = FVector::CrossProduct(m_LateralVector, up);
		m_LateralVector = FVector::CrossProduct(up, m_LongitudinalVector);
	}
	else
	{
		m_LateralVector = FVector::CrossProduct(up, m_LongitudinalVector);
		m_LongitudinalVector = FVector::CrossProduct(m_LateralVector, up);
	}
}

void ASkateboardSimPawn::DebugDraw() const
{
	const UWorld* pWorld = GetWorld();
	if (pWorld != nullptr)
	{
		// Draw our center of mass, which is really just our actor's position.
		FColor comColor = (GroundStateComp->IsOnGround() ? FColor::Green : FColor::Yellow);
		DrawDebugSphere(pWorld, GetCenterOfMassPos(), 25.0f, 16, comColor);

		// Draw centripetal acceleration vector:
		// DrawDebugDirectionalArrow(pWorld, GetCenterOfMassPos(), GetCenterOfMassPos() + ComputeCentripetalAccel(), 10.0f, FColor::Red, false, -1.0f, (uint8)'\000', 3.0f);

		FVector topOfDeckPos = GetTopOfDeckPos();
		DrawDebugSphere(pWorld, topOfDeckPos, 5.0f, 4, FColor::Blue);

		// Debug draw axes:
		DrawDebugDirectionalArrow(GetWorld(), topOfDeckPos, topOfDeckPos + GetForwardVector() * 100.0f, 10.0f, FColor::Red, false, -1.0f, (uint8)'\000', 3.0f);
		DrawDebugDirectionalArrow(GetWorld(), topOfDeckPos, topOfDeckPos + GetRightVector() * 100.0f, 10.0f, FColor::Green, false, -1.0f, (uint8)'\000', 3.0f);
		DrawDebugDirectionalArrow(GetWorld(), topOfDeckPos, topOfDeckPos + GetUpVector() * 100.0f, 10.0f, FColor::Blue, false, -1.0f, (uint8)'\000', 3.0f);
	}
}


// Called to bind functionality to input
void ASkateboardSimPawn::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	if (BindInputInCode)
	{
		InputComponent->BindAxis("MoveForward", this, &ASkateboardSimPawn::Input_MoveForward);
		InputComponent->BindAxis("MoveRight", this, &ASkateboardSimPawn::Input_MoveRight);
		InputComponent->BindAxis("LookUp", this, &ASkateboardSimPawn::Input_CameraPitch);
		InputComponent->BindAxis("Turn", this, &ASkateboardSimPawn::Input_CameraYaw);
	}
}

void ASkateboardSimPawn::Input_MoveForward(float AxisValue)
{
	m_MovementInput.X = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void ASkateboardSimPawn::Input_MoveRight(float AxisValue)
{
	m_MovementInput.Y = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void ASkateboardSimPawn::Input_CameraYaw(float AxisValue)
{
	m_CameraInput.X = AxisValue;
}

void ASkateboardSimPawn::Input_CameraPitch(float AxisValue)
{
	m_CameraInput.Y = AxisValue;
}

void ASkateboardSimPawn::OnActorBump(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GroundStateComp)
	{
		GroundStateComp->NotifyCollision(SelfActor, OtherActor, NormalImpulse, Hit);

		if (!GroundStateComp->IsOnGround())
		{
			ResetOrientation(Hit.Normal);
		}
	}
}

void ASkateboardSimPawn::SetCameraBoomWorldRotation(FRotator cameraBoomWorldRotation)
{
	SpringArm->SetWorldRotation(cameraBoomWorldRotation);
}

FRotator ASkateboardSimPawn::GetCameraBoomWorldRotation() const
{
	return SpringArm->GetComponentRotation();
}


void ASkateboardSimPawn::SetForwardVector(FVector forwardVector)
{
	m_LongitudinalVector = forwardVector;
	m_LateralVector = FVector::CrossProduct(FVector::UpVector, m_LongitudinalVector);
	m_Reverse = false;
}

FVector ASkateboardSimPawn::GetForwardVector2D() const
{
	FVector fwd = GetForwardVector();
	fwd.Z = 0.0f;

	if (fwd.IsNearlyZero())
	{
		fwd = FVector::CrossProduct(GetRightVector(), FVector::UpVector);
		fwd.Z = 0.0f;
	}

	fwd.Normalize();

	return fwd;
}
