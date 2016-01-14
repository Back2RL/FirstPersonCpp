// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "FloatingActor.generated.h"

UCLASS()
class FIRSTPERSONCPP_API AFloatingActor : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AFloatingActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	/** Activate Fancy New Project-Specific System Created in C++ ! */
	UFUNCTION(BlueprintCallable, Category = "SunShine")
		void ActivateSunShine();

	/** What is the current Sun Shine Rotation? ! */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SunShine")
		FRotator GetSunShineRotation();

private:
	float RunningTime;
	
};

