// Fill out your copyright notice in the Description page of Project Settings.

#include "FirstPersonCpp.h"
#include "Missile.h"
#include "UnrealNetwork.h"

#define deg(a) (a * (180.f) / PI)

// Sets default values
AMissile::AMissile()
{
	bReplicates = true;                                    // Set the missile to be replicated	
	PrimaryActorTick.bCanEverTick = true;                  // enable Tick
	bAlwaysRelevant = true;

}


// replication of variables
void AMissile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(AMissile, bFlag);
	DOREPLIFETIME(AMissile, IntegerArray);
	DOREPLIFETIME(AMissile, CurrentTarget);
	DOREPLIFETIME(AMissile, bSomeBool);
	DOREPLIFETIME(AMissile, MissileTransformOnAuthority);
	DOREPLIFETIME(AMissile, AdvancedHoming);

}

// Called when the game starts or when spawned
void AMissile::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority && bReplicates) {           // check if current actor has authority
														   // start a timer that executes a function (multicast)
		FTimerHandle Timer;
		const FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &AMissile::RunsOnAllClients);
		GetWorldTimerManager().SetTimer(Timer, TimerDelegate, NetUpdateFrequency, true, 0.0f);

		MaxLifeTime = Range / (Velocity * 0.01f);          // calculate max missile liftime (t = s/v (SI units))
		InitialLifeSpan = MaxLifeTime + 5.0f;              // set missile lifetime
	}
	Acceleration = 1.0f / AccelerationTime;
}

// Called every frame
void AMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	LifeTime += DeltaTime;                                 // store lifetime
	Homing(DeltaTime);                                     // perform homing to the target by rotating, both clients and server

	// the distance the missile will be moved at the end of the current tick
	MovementVector = GetActorForwardVector() * DeltaTime * Velocity;


	if (Role == ROLE_Authority) {
		// is authority
		if (LifeTime > MaxLifeTime) {                      //  reached max lifetime -> explosion etc.

			//if (ExplosionEffect) ExplosionEffect->Activate(true); // not yet working
			Destroy();  // temp
		}

		// store current missile transform of client (replicated)
		MissileTransformOnAuthority = FTransform(GetActorRotation(),
			GetActorLocation() + MovementVector,
			GetActorScale3D());

		//if (GEngine) GEngine->AddOnScreenDebugMessage(1, DeltaTime/*seconds*/, FColor::Red, "Authority");

		// store current location for next Tick
		LastActorLocation = GetActorLocation();

	}
	else {
		// is NOT authority

		if (GetWorld()->GetFirstPlayerController()) {      // get ping
			State = Cast<APlayerState>(GetWorld()->GetFirstPlayerController()->PlayerState); // "APlayerState" hardcoded, needs to be changed for main project
			if (State) {
				Ping = float(State->Ping) * 0.001f;
				// debug display ping on screen
				//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::Green, FString::SanitizeFloat(Ping));

				// client has now the most recent ping in seconds
			}
		}
	}


	// perform movement


	if (bReachedMaxVelocity) {
		AddActorWorldOffset(MovementVector);
	}
	else {
		Velocity += Acceleration * DeltaTime * MaxVelocity;
		Turnspeed += Acceleration * DeltaTime * MaxTurnspeed;
		if (Velocity > MaxVelocity) {
			Velocity = MaxVelocity;
			Turnspeed = MaxTurnspeed;
			bReachedMaxVelocity = true;
		}
		bNotFirstTick = true;
		AddActorWorldOffset(MovementVector);
	}
}

// return current lifetime of Missile in seconds
float AMissile::GetMissileLifetime()
{
	return	LifeTime;
}

// perform homing to the target by rotating
void AMissile::Homing(float DeltaTime)
{
	if (!CurrentTarget) return;                            // no homing when there is no valid target	

	CurrentTargetLocation = CurrentTarget->GetComponentLocation();

	if (Role == ROLE_Authority) {

		DistanceToTarget = (GetActorLocation() - CurrentTargetLocation).Size();

		float PossibleGap = Velocity * DeltaTime;

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::White, FString::FromInt(PossibleGap));

		if (DistanceToTarget < ExplosionRadius + PossibleGap && bNotFirstTick) {              // is the target in explosion range
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3/*seconds*/, FColor::Green, FString::FromInt(DistanceToTarget));
			Destroy();  // temp
		}

		//if (DistanceToTarget < ((Velocity + (TargetVelocity).Size()) * DeltaTime)) {
		//	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::Blue, "RangeChecking");

		//	float MinDistance = DistanceLineLine(GetActorLocation(), CurrentTargetLocation,
		//		GetActorForwardVector(),
		//		TargetVelocity);
		//	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1/*seconds*/, FColor::Green, FString::FromInt(MinDistance));

		//	if (((MinDistance > 0.0f && MinDistance < ExplosionRadius)) || DistanceToTarget < ExplosionRadius) {              // is the target in explosion range

		//		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3/*seconds*/, FColor::Green, FString::FromInt(MinDistance));
		//		Destroy();  // temp
		//	}
		//}

		LastDistanceToTarget = (GetActorLocation() - CurrentTargetLocation).Size();
	}

	if (AdvancedHoming) {                                  // is target prediction active?
		// yes		
		TargetVelocity = (CurrentTargetLocation - LastTargetLocation) / DeltaTime; // A vector with v(x,y,z) = [cm/s]		
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3/*seconds*/, FColor::Green, FString::FromInt(TargetVelocity.Size()));

		PredictedTargetLocation = LinearTargetPrediction(CurrentTargetLocation, GetActorLocation(), TargetVelocity, Velocity);
		AdvancedHomingStrength = FMath::GetMappedRangeValueClamped(
			FVector2D(AdvancedMissileMaxRange, AdvancedMissileMinRange),
			FVector2D(0.0f, 1.0f),
			DistanceToTarget);

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::White, FString::SanitizeFloat(AdvancedHomingStrength));

		// calculate the new heading direction of the missile by taking the distance to the target into consideration
		DirectionToTarget = (CurrentTargetLocation + ((PredictedTargetLocation - CurrentTargetLocation) * FMath::Sqrt(AdvancedHomingStrength))) - GetActorLocation();
	}
	else {
		// normal homing
		DirectionToTarget = (CurrentTarget->GetComponentLocation() - GetActorLocation());
	}

	LastTargetLocation = CurrentTargetLocation;        // store current targetlocation for next recalculation
	DirectionToTarget.Normalize();                            // normalize the direction vector

	AngleToTarget = FMath::Clamp(deg(FMath::Acos(DirectionToTarget | GetActorForwardVector())), 0.0f, Turnspeed * DeltaTime);
	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::White, "AngleToTarget (deg) = " + FString::SanitizeFloat(AngleToTarget));
	// rotation axis for turning the missile towards the target
	RotationAxisForTurningToTarget = GetActorForwardVector() ^ DirectionToTarget;
	// rotate the missile forward vector towards target direction
	NewDirection = GetActorForwardVector().RotateAngleAxis(AngleToTarget, RotationAxisForTurningToTarget.GetSafeNormal());
	/* // Debug
	float newAngle = deg(FMath::Acos(FVector::DotProduct(NewDirection, DirectionToTarget)));
	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Blue, "newAngleToTarget = " + FString::SanitizeFloat(newAngle));
	*/
	SetActorRotation(NewDirection.Rotation());             // apply the new direction as rotation to the missile
}

//returns a location at which has to be aimed in order to hit the target
FVector AMissile::LinearTargetPrediction(
	const FVector &TargetLocation,
	const FVector &StartLocation,
	const FVector &TargetVelocity, // cm/s
	const float ProjectileVelocity) // cm/s	
{
	FVector AB = TargetLocation - StartLocation;
	AB.Normalize();
	FVector vi = TargetVelocity - (FVector::DotProduct(AB, TargetVelocity) * AB);
	return StartLocation + vi + AB * FMath::Sqrt(ProjectileVelocity * ProjectileVelocity - FMath::Pow((vi.Size()), 2.f));
}

// replication of the timercalled funtion
void AMissile::RunsOnAllClients() {
	if (Role == ROLE_Authority)
	{
		ServerRunsOnAllClients();
	}
}

// multicasted function
void AMissile::ServerRunsOnAllClients_Implementation() {
	if (Role < ROLE_Authority) {

		SetActorTransform(MissileTransformOnAuthority);
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f/*seconds*/, FColor::Red, "corrected missile transform");

		//if (GEngine) GEngine->AddOnScreenDebugMessage(2, 3.0f/*seconds*/, FColor::Green, "(Multicast)");
		// currentTime.UtcNow().ToString()
		int64 Milliseconds = currentTime.ToUnixTimestamp() * 1000 + currentTime.GetMillisecond();

		//if (GEngine) GEngine->AddOnScreenDebugMessage(3, 3.0f/*seconds*/, FColor::Green, FString::SanitizeFloat((float)Milliseconds));
	}
}
//----------------------------------------------------- TESTING ------------------------------------------------

// testing
void AMissile::ServerSetFlag()
{
	if (HasAuthority() && !bFlag) // Ensure Role == ROLE_Authority
	{
		bFlag = true;
		OnRep_Flag(); // Run locally since we are the server this won't be called automatically.
	}
}

// testing
void AMissile::OnRep_Flag()
{
	// When this is called, bFlag already contains the new value. This
	// just notifies you when it changes.
}

// testing
void AMissile::Server_RunsOnServer_Implementation()
{
	// Do something here that modifies game state.
}

// testing
bool AMissile::Server_RunsOnServer_Validate()
{
	// Optionally validate the request and return false if the function should not be run.
	return true;
}

// testing
void AMissile::Dealing() {
	if (Role == ROLE_Authority)
	{
		ServerDealing();
	}
}

float AMissile::DistanceLineLine(const FVector& a1,
	const FVector& a2,
	const FVector& b1,
	const FVector& b2) {
	return ((a1 - a2)*(b1 ^ b2)).Size() / (b1 ^ b2).Size();
}

//returns the Distance or -1 / WIP
bool AMissile::ClosestPointsOnTwoLines(const FVector& LineStartA,
	const FVector& LineEndA,
	const FVector& LineStartB,
	const FVector& LineEndB,
	FVector& PointA,
	FVector& PointB) {
	const FVector u = LineEndA - LineStartA;
	const FVector v = LineEndB - LineStartB;

	const float X1 = (LineStartA - LineStartB) | u;
	const float X2 = (LineStartA - LineStartB) | v;

	const float Y1 = u | u;
	const float Y2 = v | u;

	const float Z1 = Y2;
	const float Z2 = v | v;

	//X1 + t * Y1 - s * Z1 = 0;
	//X2 + t * Y2 - s * Z2 = 0;

	//(Y2/Y1 * X1 - X2) - s * (Y2 / Y1 * Z1 - Z2) = 0
	float s = (Y2 / Y1 * X1 - X2) / (Y2 / Y1 * Z1 - Z2);
	float t = (-X1 + s * Z1) / Y1;

	PointA = LineStartA + t * u;
	PointB = LineStartB + s * v;

	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds()/*seconds*/, FColor::Red, "s = " + FString::SanitizeFloat(s) + " t = " + FString::SanitizeFloat(t));

	return (t*t < 1.0f && s*s < 1.0f);
}



// testing
void AMissile::ServerDealing_Implementation() {
	//
}

////// example for function replication------------------------
void AMissile::SetSomeBool(bool bNewSomeBool)
{
	//if (GEngine) GEngine->AddOnScreenDebugMessage(4, 20.0f/*seconds*/, FColor::White, FString("Client calls serverfunction to set a bool ").Append(FString(bSomeBool ? "True" : "False")));
	ServerSetSomeBool(bNewSomeBool);
}

bool AMissile::ServerSetSomeBool_Validate(bool bNewSomeBool)
{
	return true;
}

void AMissile::ServerSetSomeBool_Implementation(bool bNewSomeBool)
{
	bSomeBool = bNewSomeBool;
	//if (GEngine) GEngine->AddOnScreenDebugMessage(5, 20.0f/*seconds*/, FColor::Blue, "bool was set on server");
	//if (GEngine) GEngine->AddOnScreenDebugMessage(6, 20.0f/*seconds*/, FColor::Blue, FString(bSomeBool ? "true" : "false"));

}
////// end: example for function replication------------------------

////// example for function replication

void AMissile::RunsOnOwningClientOnly() {
	if (Role == ROLE_Authority)
	{
		Client_RunsOnOwningClientOnly();
	}
}
void AMissile::Client_RunsOnOwningClientOnly_Implementation()
{
	// Do something here to affect the client. This method was called by the server ONLY.
}

////// end: example for function replication


/*GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("This is an on screen message!"));
UE_LOG(LogTemp, Log, TEXT("Log text %f"), 0.1f);
UE_LOG(LogTemp, Warning, TEXT("Log warning"));
UE_LOG(LogTemp, Error, TEXT("Log error"));
FError::Throwf(TEXT("Log error"));
FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Dialog message")));
UE_LOG(LogTemp, Warning, TEXT("Your message"));*/


