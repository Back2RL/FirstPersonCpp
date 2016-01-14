// Fill out your copyright notice in the Description page of Project Settings.

#include "FirstPersonCpp.h"
#include "FloatingActor.h"


// Sets default values
AFloatingActor::AFloatingActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AFloatingActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFloatingActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
	
	float DeltaTimeMultiplier = 100.0f;
	float DeltaHightMultiplier = 100.0f;

	FVector NewLocation = GetActorLocation();
	NewLocation.Z += DeltaTime * DeltaHightMultiplier;
	float DeltaHeight = (FMath::Sin(RunningTime + DeltaTime * DeltaTimeMultiplier) - FMath::Sin(RunningTime));
	NewLocation.Z += DeltaHeight * DeltaHightMultiplier;       //Scale our height by a factor of 100
	RunningTime += DeltaTime * DeltaTimeMultiplier;
	SetActorLocation(NewLocation);
}

void AFloatingActor::ActivateSunShine()
{

}


FRotator AFloatingActor::GetSunShineRotation()
{
	return	FRotator(0, 0, 0);
}
