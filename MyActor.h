// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UCLASS()
class FIRSTPERSONCPP_API AMyActor : public AActor
{
	// public variables, can eventually be edited in Editor
public:
	GENERATED_BODY()


		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
			int32 TotalDamage;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
			float DamageTimeInSeconds;



		// property is a calculated value using the designer’s settings
		// VisibleAnywhere flag marks that property as viewable, but not editable in the Unreal Editor
		// Transient flag means that it won’t be saved or loaded from disk; it’s meant to be a derived, non-persistent value
		UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Damage")
			float DamagePerSecond;

		

	// Sets default values for this actor's properties
	AMyActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// called in editor for calcuation of missing values with dependencies
	void AMyActor::PostInitProperties();
	
	// void AMyActor::CalculateValues();
	UFUNCTION(BlueprintCallable, Category = "Damage")
		void CalculateValues();
		

	void AMyActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);


	UFUNCTION(BlueprintNativeEvent, Category = "Damage")
		void CalledFromCpp();
		virtual void CalledFromCpp_Implementation();



};
