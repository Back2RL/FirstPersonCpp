// Fill out your copyright notice in the Description page of Project Settings.

#include "FirstPersonCpp.h"
#include "Missile.h"
#include "UnrealNetwork.h"


// Sets default values
AMissile::AMissile()
{
	bReplicates = false/*temporarily*/;                                    // Set the missile to be replicated	
	PrimaryActorTick.bCanEverTick = true;                  // enable Tick

}

void AMissile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(AMissile, bFlag);
	DOREPLIFETIME(AMissile, IntegerArray);
	DOREPLIFETIME(AMissile, CurrentTarget);
	DOREPLIFETIME(AMissile, bSomeBool);
	DOREPLIFETIME(AMissile, MissileTransformOnAuthority);
	DOREPLIFETIME(AMissile, AdvancedHoming);

}

void AMissile::BeginPlay()                                 // Called when the game starts or when spawned
{
	Super::BeginPlay();

	if (Role == ROLE_Authority && bReplicates) {           // check if current actor has authority
		/* start a timer that executes a function (multicast)
		*/
		FTimerHandle Timer;	                               // timername/-handle
		const FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &AMissile::RunsOnAllClients);
		GetWorldTimerManager().SetTimer(Timer, TimerDelegate, NetUpdateFrequency, true, 0.0f);

		// calculate max missile liftime (v = s/t <=> t = s/v)
		MaxLifeTime = Range / (Velocity * 0.01f);
	}
}


void AMissile::Tick(float DeltaTime)                       // Called every frame
{
	Super::Tick(DeltaTime);

	// store lifetime
	Lifespan += DeltaTime;

	// perform homing to the target by rotating, both clients and server
	Homing(DeltaTime);

	// the distance the missile will be moved at the end of the curren tick
	MovementVector = GetActorForwardVector() * DeltaTime * Velocity;

	if (Role == ROLE_Authority) {                          // is server

		if (Lifespan > MaxLifeTime) {
			//  reached max lifetime -> explosion etc.
			if (ExplosionEffect) ExplosionEffect->Activate(true);
			Destroy();  // temp
		}

		// store current missile transform of client (replicated)
		MissileTransformOnAuthority = FTransform(GetActorRotation(), GetActorLocation(), GetActorScale3D());
		if (GEngine) GEngine->AddOnScreenDebugMessage(1, DeltaTime/*seconds*/, FColor::Red, "Authority");

		// check if missile can move to next location without colliding
		//if (TraceNextMovementForCollision()) {
		//	// missile will collide when performing next movement
		//	SetActorLocation(ExplosionLocation);
		//	// prevent missile movement
		//	MovementVector = FVector(0.0f, 0.0f, 0.0f);
		//	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f/*seconds*/, FColor::Red, "Hit");
		//	//Destroy();
		//}

	}
	if (Role < ROLE_Authority) {                           // is client

		// get ping
		if (GetWorld()->GetFirstPlayerController()) {
			State = Cast<APlayerState>(GetWorld()->GetFirstPlayerController()->PlayerState);
			if (State) {
				Ping = float(State->Ping) * 0.001f;
				// debug display ping on screen
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::Green, FString::SanitizeFloat(Ping));
			}
			// client has now the most recent ping in seconds

		}


	}


	/*GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("This is an on screen message!"));
	UE_LOG(LogTemp, Log, TEXT("Log text %f"), 0.1f);
	UE_LOG(LogTemp, Warning, TEXT("Log warning"));
	UE_LOG(LogTemp, Error, TEXT("Log error"));
	FError::Throwf(TEXT("Log error"));
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Dialog message")));
	UE_LOG(LogTemp, Warning, TEXT("Your message"));*/



	// continue moving forwards
	AddActorWorldOffset(MovementVector);
}


// return current lifetime of Missile in seconds
float AMissile::GetMissileLifetime()
{
	return	Lifespan;
}

/**	 return current lifetime of the missile in seconds*/
bool AMissile::TraceNextMovementForCollision()
{
	//The trace data is stored here
	FHitResult HitData(ForceInit);

	FCollisionQueryParams TraceParams(FName(TEXT("SingleVisibilityTrace")), true);
	TraceParams.bTraceComplex = true;
	//TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = false;

	//Ignore Actors
	//TraceParams.AddIgnoredActor();
	if (ActorLineTraceSingle(HitData, GetActorLocation(), GetActorLocation() + MovementVector, ECC_Visibility, TraceParams)) {
		ExplosionLocation = GetActorLocation() + GetActorForwardVector()*HitData.Distance;
		return true;
	}
	return false;
}


// perform homing to the target by rotating
void AMissile::Homing(float DeltaTime)
{
	// check if missile has a valid target	
	if (!CurrentTarget) return;
	// get the vector from missile to the current target

	//if (AdvancedHoming) { // is target prediction active?
		// yes
		CurrentTargetLocation = CurrentTarget->GetComponentLocation();

		TargetVelocity = (CurrentTargetLocation - LastTargetLocation) / DeltaTime;
		
		// store current targetlocation for next recalculation
		LastTargetLocation = CurrentTargetLocation;
		// calculate new direction to intercept flightpath
		DirectionToTarget = LinearTargetPrediction(CurrentTargetLocation, GetActorLocation(), TargetVelocity, Velocity) - GetActorLocation();
	//}
	//else {
	//	DirectionToTarget = (CurrentTarget->GetComponentLocation() - GetActorLocation());
	//}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::White, FString::FromInt(TargetVelocity.Size()));

	// ----------------------

	DirectionToTarget.Normalize();
	//get dotproduct with missile forward vector	
	AngleToTarget = FMath::Min(FMath::Acos(GetActorForwardVector() | DirectionToTarget) / DeltaTime /* increases turning at small angles */, MaxTurnspeed * DeltaTime);

	//AngleToTarget = (Dot <= 0.0f) ? (180.0f + FMath::RadiansToDegrees(Dot)) : FMath::RadiansToDegrees(Dot);

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::White, FString::SanitizeFloat(AngleToTarget / DeltaTime));

	RotationAxisTarget = GetActorForwardVector() ^ DirectionToTarget;
	RotationAxisTarget.Normalize();
	// rotate the missile forward vector towards target direction
	NewDirection = GetActorForwardVector().RotateAngleAxis(AngleToTarget, RotationAxisTarget);
	// apply the new direction as rotation to the missile

	SetActorRotation(NewDirection.Rotation());

}

// 
FVector AMissile::LinearTargetPrediction(
	const FVector &TargetLocation,
	const FVector &StartLocation,
	const FVector &TargetVelocity, // cm/s
	const float ProjectileVelocity) // cm/s
	//returns a location at which has to be aimed in order to hit the target
{
	FVector ABmag = TargetLocation - StartLocation;
	ABmag.Normalize();
	FVector vi = TargetVelocity - (FVector::DotProduct(ABmag, TargetVelocity) * ABmag);
	//FVector vj = ABmag * FMath::Sqrt(ProjectileVelocity * ProjectileVelocity - FMath::Pow((vi.Size()),2.f));
	//return StartLocation + vi + vj;

	//in short:
	return StartLocation + vi + ABmag * FMath::Sqrt(ProjectileVelocity * ProjectileVelocity - FMath::Pow((vi.Size()), 2.f));

}


void AMissile::ServerSetFlag()
{
	if (HasAuthority() && !bFlag) // Ensure Role == ROLE_Authority
	{
		bFlag = true;
		OnRep_Flag(); // Run locally since we are the server this won't be called automatically.
	}
}

void AMissile::OnRep_Flag()
{
	// When this is called, bFlag already contains the new value. This
	// just notifies you when it changes.
}




void AMissile::Server_RunsOnServer_Implementation()
{
	// Do something here that modifies game state.
}

bool AMissile::Server_RunsOnServer_Validate()
{
	// Optionally validate the request and return false if the function should not be run.
	return true;
}

void AMissile::Dealing() {
	if (Role == ROLE_Authority)
	{
		ServerDealing();
	}
}

void AMissile::ServerDealing_Implementation() {
	//
}

void AMissile::RunsOnAllClients() {
	if (Role == ROLE_Authority)
	{
		ServerRunsOnAllClients();
	}
}


// multicasted function
void AMissile::ServerRunsOnAllClients_Implementation() {
	//if (Role < ROLE_Authority) {

	//	SetActorTransform(MissileTransformOnAuthority);
	//	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f/*seconds*/, FColor::Red, "corrected missile transform");

	//	if (GEngine) GEngine->AddOnScreenDebugMessage(2, 3.0f/*seconds*/, FColor::Green, "(Multicast)");
	//	// currentTime.UtcNow().ToString()
	//	int64 Milliseconds = currentTime.ToUnixTimestamp() * 1000 + currentTime.GetMillisecond();

	//	if (GEngine) GEngine->AddOnScreenDebugMessage(3, 3.0f/*seconds*/, FColor::Green, FString::SanitizeFloat((float)Milliseconds));
	//}
}


////// example for function replication------------------------

void AMissile::SetSomeBool(bool bNewSomeBool)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(4, 20.0f/*seconds*/, FColor::White, FString("Client calls serverfunction to set a bool ").Append(FString(bSomeBool ? "True" : "False")));
	ServerSetSomeBool(bNewSomeBool);
}

bool AMissile::ServerSetSomeBool_Validate(bool bNewSomeBool)
{
	return true;
}

void AMissile::ServerSetSomeBool_Implementation(bool bNewSomeBool)
{
	bSomeBool = bNewSomeBool;
	if (GEngine) GEngine->AddOnScreenDebugMessage(5, 20.0f/*seconds*/, FColor::Blue, "bool was set on server");
	if (GEngine) GEngine->AddOnScreenDebugMessage(6, 20.0f/*seconds*/, FColor::Blue, FString(bSomeBool ? "true" : "false"));

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

