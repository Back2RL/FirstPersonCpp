// Fill out your copyright notice in the Description page of Project Settings.

#include "FirstPersonCpp.h"
#include "MyActor.h"


// Sets default values
AMyActor::AMyActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Set default values
	TotalDamage = 200;
	DamageTimeInSeconds = 1.f;

}

	//allows calculation of missing values that have dependencies
void AMyActor::PostInitProperties()
{
	Super::PostInitProperties();
	CalculateValues();
}	

void AMyActor::CalculateValues()
{
	DamagePerSecond = TotalDamage / DamageTimeInSeconds;
}

#if WITH_EDITOR
void AMyActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	CalculateValues();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}


void AMyActor::CalledFromCpp_Implementation()
{
	// Do something cool here
}
