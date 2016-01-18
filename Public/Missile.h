// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Engine.h"
#include "GameFramework/Actor.h"
#include "Missile.generated.h"


UCLASS()
class FIRSTPERSONCPP_API AMissile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMissile();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	/**	 return current lifetime of the missile in seconds*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Missile")
		float GetMissileLifetime();

	/**	 Perform target location prediction*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Missile")
		FVector LinearTargetPrediction(
			const FVector &TargetLocation,
			const FVector &StartLocation,
			const FVector &TargetVelocity,
			const float ProjectileVelocity);

	/** sets missile turnspeed in deg/s*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float MaxTurnspeed = 40.0f;

	/** missile range in m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float Range = 4000.0f;

	/** missile explosioneffect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		class UParticleSystemComponent* ExplosionEffect;
		 
	/** sets missile velocity in cm/s*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float Velocity = 2000.0f;

	/** stores how many degree the missile is currently turning when homing*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float AngleToTarget;

	/** the current target (scenecpmponent) the missile is homing towards */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Missile")
	class USceneComponent* CurrentTarget;

	/** is advanced homing active */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Missile")
		bool AdvancedHoming = true;

	/**	 check if missile will collide when performing next movement*/
	UFUNCTION(BlueprintCallable,  Category = "Missile")
		bool TraceNextMovementForCollision();

	///** A Replicated Boolean Flag */
	//UPROPERTY(Replicated)
	//	uint32 bFlag : 1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Missile")
		FTransform MissileTransformOnAuthority;



	/** A Replicated Array Of Integers */
	UPROPERTY(Replicated)
		TArray<uint32> IntegerArray;


	/** A Replicated Boolean Flag */
	UPROPERTY(ReplicatedUsing = OnRep_Flag)
		uint32 bFlag : 1;

	void ServerSetFlag();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_RunsOnServer();

	UPROPERTY()
	APlayerState* State;

	UFUNCTION(NetMulticast, Reliable)
		void ServerDealing();
	virtual void Dealing();

	UFUNCTION(NetMulticast, Unreliable)
		void ServerRunsOnAllClients();
	virtual void RunsOnAllClients();



	////// example for function replication------------------------
	UPROPERTY(Replicated)
	bool bSomeBool = false;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetSomeBool(bool bNewSomeBool);
	virtual void SetSomeBool(bool bNewSomeBool); // executed on client
	
	////// end: example for function replication------------------------

	////// example for function replication

	UFUNCTION(Client, Reliable)
		void Client_RunsOnOwningClientOnly();
	virtual void RunsOnOwningClientOnly();

	////// end: example for function replication





private:

	UFUNCTION()
		void OnRep_Flag();

	float MaxLifeTime;
	float Lifespan;
	FVector ExplosionLocation;
	FVector RotationAxisTarget;
	FVector NewDirection;
	FVector MovementVector;
	float Dot;
	FVector DirectionToTarget;
	FVector CurrentTargetLocation;
	FVector LastTargetLocation;
	FVector TargetVelocity;
	FVector PredictedTargetLocation;
	int FramesSinceLastVelocityCheck;
	/** perform homing to the target by rotating*/
	void Homing(float DeltaTime);

	FDateTime currentTime;
	float Ping;



};
