// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "AIController.h"
#include "TopViewPawnMovement.h"

namespace
{
enum Direction : uint8
{
	NONE,
	TOP,
	TOP_RIGHT,
	RIGHT,
	RIGHT_BOTTOM,
	BOTTOM,
	BOTTOM_LEFT,
	LEFT,
	LEFT_TOP,
};

inline FVector DirectionToVector(uint8 Direction, float Length=1.0f)
{
	FVector Vel;
	float sin=0,cos=0;

	switch(Direction)
	{
	case TOP:
		Vel.Set(0.f, -Length, 0.f);
	break;

	case TOP_RIGHT:
		FMath::SinCos(&sin,&cos, FMath::DegreesToRadians(315.f));
		Vel.Set(cos*Length, sin*Length, 0.f);
	break;

	case RIGHT:
		Vel.Set(Length, 0.f, 0.f);
	break;

	case RIGHT_BOTTOM:
		FMath::SinCos(&sin,&cos, FMath::DegreesToRadians(45.f));
		Vel.Set(cos*Length, sin*Length, 0.f);
	break;

	case BOTTOM:
		Vel.Set(0.f, Length, 0.f);
	break;

	case BOTTOM_LEFT:
		FMath::SinCos(&sin,&cos, FMath::DegreesToRadians(135.f));
		Vel.Set(cos*Length, sin*Length, 0.f);
	break;

	case LEFT:
		Vel.Set(-Length, 0.f, 0.f);
	break;

	case LEFT_TOP:
		FMath::SinCos(&sin,&cos, FMath::DegreesToRadians(225.f));
		Vel.Set(cos*Length, sin*Length, 0.f);
	break;
	}

	return std::move(Vel);
}

inline uint8 VectorToDirection(const FVector& Vel)
{
	float angle = FMath::RadiansToDegrees(FMath::Atan2(Vel.Y, Vel.X));
	if(angle<0.f) angle +=360.f;

	if(22.5f<=angle && angle<67.5f)			return RIGHT_BOTTOM;
	else if(67.5f<=angle && angle<112.5f)	return BOTTOM;
	else if(112.5f<=angle && angle<157.5f)	return BOTTOM_LEFT;
	else if(157.5f<=angle && angle<202.5f)	return LEFT;
	else if(202.5f<=angle && angle<247.5f)	return LEFT_TOP;
	else if(247.5f<=angle && angle<292.5f)	return TOP;
	else if(292.5f<=angle && angle<337.5f)	return TOP_RIGHT;
	else 									return RIGHT;
}

}

UTopViewPawnMovement::UTopViewPawnMovement()
{
	// デフォルトでZ軸移動を制限しておく
	bConstrainToPlane			= 1;
	SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Z);

	MoveSpeed					= 1.0f;
	NavDirection				= 0;
	Mode						= MoveMode::NORMAL;

	MoveDirection				= 0;
	MoveDuration				= 0.0f;
	MoveDeltaTimes				= 0.0f;

	MoveEasing					= EEasingFunc::Type::ExpoOut;
	MoveBlendExp				= 2.f;

	AcceptRadius				= 20.f;
}

void UTopViewPawnMovement::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	NavVelocity = MoveVelocity;
}

// 基本的にFloatingPawnMovementのコピペだが、FollowPathの時の扱いが違う
void UTopViewPawnMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	if(ShouldSkipUpdate(DeltaTime)) return;

	// ↓の呼び方だと、なぜかSuper::TickCompと等価になってしまうので、直接呼んでいる
	//Super::Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UPawnMovementComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(!PawnOwner || !UpdatedComponent) return;

	const AController* Controller = PawnOwner->GetController();
	if(Controller && Controller->IsLocalController())
	{
		bool bDelta = false;
		bool bCorrected = false;

		switch(Mode)
		{
		default:
			MoveNormal(DeltaTime);
		break;

		case MoveMode::DURATION:
			bCorrected = MoveDurationInner(DeltaTime);
		break;

		case MoveMode::EASE:
			bCorrected = MoveEaseInner(DeltaTime);
			bDelta = true;
		break;

		case MoveMode::LOCATION:
			bCorrected = MoveLocationAutoInner(DeltaTime);
		break;
		}

		CalcVelcocity(DeltaTime, bDelta, bCorrected);
		UpdateComponentVelocity();

		// OnMove → OnMoveFinishedと呼ばれるべき
		if(bCorrected)
		{	OnMoveFinished.Broadcast(false);	}
	}
}

void UTopViewPawnMovement::CalcVelcocity(float DeltaTime, bool bDelta, bool bCorrected)
{
	LimitWorldBounds();
	bPositionCorrected = bCorrected;

	// bDeltaがtrueの場合は、VelocityはDelta化済み
	FVector Delta = bDelta ? Velocity : Velocity * DeltaTime;

	if (!Delta.IsNearlyZero(1e-6f))
	{
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat Rotation = UpdatedComponent->GetComponentQuat();

		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);

		if (Hit.IsValidBlockingHit())
		{
			HandleImpact(Hit, DeltaTime, Delta);
			// Try to slide the remaining distance along the surface.
			SlideAlongSurface(Delta, 1.f-Hit.Time, Hit.Normal, Hit, true);
		}

		// Update velocity
		// We don't want position changes to vastly reverse our direction (which can happen due to penetration fixups etc)
		if (!bPositionCorrected)
		{
			const FVector NewLocation = UpdatedComponent->GetComponentLocation();
			Velocity = ((NewLocation - OldLocation) / DeltaTime);
		}

		OnMove.Broadcast();
	}
	else
	{
		OnStand.Broadcast();
	}
}

void UTopViewPawnMovement::MoveToDirection(uint8 Direction)
{
	FVector Vel = DirectionToVector(Direction, MoveSpeed);
	AddInputVector(Vel);
}

uint8 UTopViewPawnMovement::MoveToLocation(const FVector2D& Location)
{
	const FVector Loc = UpdatedComponent->GetComponentLocation();
	const uint8 direction = VectorToDirection((FVector(Location, 0.f) - Loc));
	if(direction==0) return 0;

	FVector Vel = DirectionToVector(direction, MoveSpeed);
	AddInputVector(Vel);

	return direction;
}

///////// MoveNormal
void UTopViewPawnMovement::MoveNormal(float DeltaTime)
{
	const AController* Controller = PawnOwner->GetController();
	if(Controller->IsLocalPlayerController()==true || Controller->IsFollowingAPath()==false)
	{
		ApplyControlInputToVelocity(DeltaTime);
	}
	else
	{
		if(IsExceedingMaxSpeed(MaxSpeed)==true)
		{	Velocity = Velocity.GetUnsafeNormal()*MaxSpeed;	}

		if(Controller->IsFollowingAPath()==true)
		{
			MoveNavInner(DeltaTime);
		}
	}
}

void UTopViewPawnMovement::MoveNavInner(float DeltaTime)
{
	if(NavVelocity.SizeSquared2D()>0.f)
	{
		// 速度に合わせた方向に進む
		const uint8 direction = VectorToDirection(NavVelocity);
		MoveToDirection(direction);
		ApplyControlInputToVelocity(DeltaTime);

		if(NavDirection!=direction)
		{
			NavDirection=direction;
			OnNavDirection.Broadcast(NavDirection);
		}
	}
}

///////// MoveDuration
void UTopViewPawnMovement::MoveToDirectionDuration(uint8 Direction, float Duration)
{
	if(Mode==MoveMode::NORMAL)
		StopActiveMovement();

	MoveDirection	= Direction;
	MoveDuration	= Duration;
	MoveDeltaTimes  = 0.f;

	Mode = MoveMode::DURATION;
}

bool UTopViewPawnMovement::MoveDurationInner(float DeltaTime)
{
	MoveDeltaTimes+=DeltaTime;
	MoveToDirection(MoveDirection);
	ApplyControlInputToVelocity(DeltaTime);
	
	if(MoveDeltaTimes>=MoveDuration)
	{
		Mode = MoveMode::NORMAL;
		return true;
	}

	return false;
}

///////// MoveEase
void UTopViewPawnMovement::MoveToLocationEase(const FVector& Location, float Duration, TEnumAsByte<EEasingFunc::Type> Easing, float BlendExp)
{
	if(Mode==MoveMode::NORMAL)
		StopActiveMovement();

	MoveStartLocation	= UpdatedComponent->GetComponentLocation();
	MoveStartLocation.Z = 0.f;
	MoveGoalLocation	= Location;
	MoveGoalLocation.Z  = 0.f;
	MoveDuration		= Duration;
	MoveDeltaTimes		= 0.f;

	MoveEasing		= Easing;
	MoveBlendExp	= BlendExp;

	Mode = MoveMode::EASE;
}

void UTopViewPawnMovement::MoveToDirectionDistanceEase(uint8 Direction, float Distance, float Duration, TEnumAsByte<EEasingFunc::Type> Easing, float BlendExp)
{
	FVector Vel = DirectionToVector(Direction, Distance);
	Vel = UpdatedComponent->GetComponentLocation()+Vel;
	Vel.Z = 0.f;

	MoveToLocationEase(Vel, Duration, Easing, BlendExp);
}

bool UTopViewPawnMovement::MoveEaseInner(float DeltaTime)
{
	const FVector EaseStartVec = UKismetMathLibrary::VEase(MoveStartLocation, MoveGoalLocation, UKismetMathLibrary::FClamp(MoveDeltaTimes/MoveDuration, 0.f, 1.f), MoveEasing, MoveBlendExp);
	MoveDeltaTimes+=DeltaTime;
	const FVector EaseGoalVec = UKismetMathLibrary::VEase(MoveStartLocation, MoveGoalLocation, UKismetMathLibrary::FClamp(MoveDeltaTimes/MoveDuration, 0.f, 1.f), MoveEasing, MoveBlendExp);
	
	Velocity = EaseGoalVec-EaseStartVec;

	if(MoveDeltaTimes>=MoveDuration)
	{
		Mode = MoveMode::NORMAL;
		return true;
	}

	return false;
}

///////
void UTopViewPawnMovement::MoveToLocationAuto(const FVector2D& Location, float InAcceptRadius)
{
	if(Mode==MoveMode::NORMAL)
		StopActiveMovement();

	MoveDirection		= 0;
	MoveGoalLocation.X	= Location.X;
	MoveGoalLocation.Y	= Location.Y;
	MoveGoalLocation.Z	= 0.f;
	AcceptRadius		= InAcceptRadius;

	Mode				= MoveMode::LOCATION;
}

bool UTopViewPawnMovement::MoveLocationAutoInner(float DeltaTime)
{
	const FVector Loc = UpdatedComponent->GetComponentLocation();

	if(FVector::DistSquaredXY(Loc, MoveGoalLocation)<=AcceptRadius)
	{
		Mode = MoveMode::NORMAL;
		return true;
	}
	else
	{
		const uint8 direction = MoveToLocation(FVector2D(MoveGoalLocation));
		if(NavDirection!=direction)
		{	OnNavDirection.Broadcast(direction);	}

		NavDirection = direction;
		return false;
	}
}

///////
void UTopViewPawnMovement::StopTopViewMovemnt()
{
	if(Mode!=MoveMode::NORMAL)
	{
		Mode=MoveMode::NORMAL;
		OnMoveFinished.Broadcast(true);
	}
}
