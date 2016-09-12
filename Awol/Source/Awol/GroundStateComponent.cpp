// Fill out your copyright notice in the Description page of Project Settings.

#include "Awol.h"
#include "GroundStateComponent.h"
#include "SkateboardTune.h"


// Sets default values for this component's properties
UGroundStateComponent::UGroundStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UGroundStateComponent::BeginPlay()
{
	Super::BeginPlay();

	ResetState();

	// DEBUG DRAW probes
	// const FName traceTag("Rideable");
	// GetWorld()->DebugDrawTraceTag = traceTag;
}


// Called every frame
void UGroundStateComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	DebugDraw();
}

bool UGroundStateComponent::ProbeGround(const FVector & pos, const FVector & forward, const FVector & right)
{
	bool retval = true;

	// NOTE: UE4 does LEFT-handed cross products!
	FVector up = FVector::CrossProduct(forward, right);

	/*
	DrawDebugDirectionalArrow(GetWorld(), pos, pos + forward * 150.0f, 10.0f, FColor::Green, false, 1.0f, (uint8)'\000', 2.0f);
	DrawDebugDirectionalArrow(GetWorld(), pos, pos + right * 150.0f, 10.0f, FColor::Red, false, 1.0f, (uint8)'\000', 2.0f);
	DrawDebugDirectionalArrow(GetWorld(), pos, pos + up * 150.0f, 10.0f, FColor::Blue, false, 1.0f, (uint8)'\000', 2.0f);
	*/

	float probeLength = 40.0f;
	float probeSpacingFwd = 55.0f;
	float probeSpacingLat = 20.0f;
	float deckHeight = 10.0f;
	if (m_SkateboardTune != nullptr)
	{
		deckHeight = m_SkateboardTune->DeckHeight;
		probeLength = deckHeight * 6.0f;
		probeSpacingFwd = m_SkateboardTune->TruckSpacing;
		probeSpacingLat = m_SkateboardTune->AxleLength;
	}

	FVector frontProbePos = pos + (forward * 0.5f * probeSpacingFwd);
	FVector rearProbePos = pos - (forward * 0.5f * probeSpacingFwd);
	FVector rightProbePos = pos + (right * 0.5f * probeSpacingLat);
	FVector leftProbePos = pos - (right * 0.5f * probeSpacingLat);

	FVector probeStart = up * (0.5f * probeLength);
	FVector probeEnd = up * (-0.5f * probeLength);

	FHitResult frontHitResult;
	FHitResult rearHitResult;
	FHitResult rightHitResult;
	FHitResult leftHitResult;

	// Debug draw probes, color-coded so we can tell them apart
	/*
	DrawDebugDirectionalArrow(GetWorld(), frontProbePos + probeStart, frontProbePos + probeEnd, 10.0f, FColor::Blue, false, 1.0f, (uint8)'\000', 2.0f);
	DrawDebugDirectionalArrow(GetWorld(), rearProbePos + probeStart, rearProbePos + probeEnd, 10.0f, FColor::Cyan, false, 1.0f, (uint8)'\000', 2.0f);
	DrawDebugDirectionalArrow(GetWorld(), rightProbePos + probeStart, rightProbePos + probeEnd, 10.0f, FColor::Red, false, 1.0f, (uint8)'\000', 2.0f);
	DrawDebugDirectionalArrow(GetWorld(), leftProbePos + probeStart, leftProbePos + probeEnd, 10.0f, FColor::Green, false, 1.0f, (uint8)'\000', 2.0f);
	*/

	bool frontProbeHit = ProbeRideable(frontProbePos + probeStart, frontProbePos + probeEnd, frontHitResult);
	bool rearProbeHit = ProbeRideable(rearProbePos + probeStart, rearProbePos + probeEnd, rearHitResult);
	bool rightProbeHit = ProbeRideable(rightProbePos + probeStart, rightProbePos + probeEnd, rightHitResult);
	bool leftProbeHit = ProbeRideable(leftProbePos + probeStart, leftProbePos + probeEnd, leftHitResult);

	FVector newFrontPos = (frontProbeHit ? frontHitResult.ImpactPoint : frontProbePos + probeEnd);
	FVector newRearPos = (rearProbeHit ? rearHitResult.ImpactPoint : rearProbePos + probeEnd);
	FVector newRightPos = (rightProbeHit ? rightHitResult.ImpactPoint : rightProbePos + probeEnd);
	FVector newLeftPos = (leftProbeHit ? leftHitResult.ImpactPoint : leftProbePos + probeEnd);

	m_GroundPosition = FMath::Lerp(newFrontPos, newRearPos, 0.5f);

	FVector newFwd = newFrontPos - newRearPos;
	newFwd.Normalize();
	FVector newRight = newRightPos - newLeftPos;
	newRight.Normalize();

	m_GroundNormal = FVector::CrossProduct(newFwd, newRight);
	m_GroundNormal.Normalize();

	m_IsOnGround = (frontProbeHit || rearProbeHit || rightProbeHit || leftProbeHit);

	return m_IsOnGround;
}

void UGroundStateComponent::NotifyCollision(AActor * SelfActor, AActor * OtherActor, FVector NormalImpulse, const FHitResult & Hit)
{
	// TODO
}

bool UGroundStateComponent::ProbeRideable(const FVector& start, const FVector& end, FHitResult& hitOut)
{
	if (!GetWorld())
		return false;

	const FName traceTag("Rideable");
	FCollisionQueryParams cqp;
	cqp.TraceTag = traceTag;
	cqp.AddIgnoredActor(GetOwner());
	FCollisionResponseParams crp;

	return GetWorld()->LineTraceSingleByChannel(hitOut, start, end, ECollisionChannel::ECC_WorldStatic, cqp, crp);
}

const USkateboardTune * UGroundStateComponent::GetSkateboardTune() const
{
	return m_SkateboardTune;
}

void UGroundStateComponent::ResetState()
{
	m_IsOnGround = false;
}

void UGroundStateComponent::DebugDraw() const
{

}

bool UGroundStateComponent::IsOnGround() const
{
	return m_IsOnGround;
}

FVector UGroundStateComponent::GetGroundPosition() const
{
	return m_GroundPosition;
}

FVector UGroundStateComponent::GetGroundNormal() const
{
	return m_GroundNormal;
}
