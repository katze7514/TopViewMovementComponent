// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "TopViewPawnMovement.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTopViewOnStandSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTopViewOnMoveSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTopViewOnNavDirectionSignature,uint8,toward);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTopViewOnMoveFinishedSignature,bool,bAbort);

/**
 *  TopViewで8方向移動を行うMovementComponent
 *  NavMeshも8方向で辿る
 */
UCLASS(ClassGroup=Movement, meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UTopViewPawnMovement : public UFloatingPawnMovement
{
	GENERATED_BODY()

	enum MoveMode{
		NORMAL,
		LOCATION,
		DURATION,
		EASE,
	};
		
	UTopViewPawnMovement();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

public:
	// Navigation動作時に進むべき速度を設定するために呼ばれる
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;

public:
	// 指定した方向に移動する。DirectionはTOP(1)から時計回りに1ずつ増える。0は停止
	UFUNCTION(BlueprintCallable, Category="TopViewPawnMovement")
	void MoveToDirection(uint8 Direction);

	// 1フレームだけ指定した位置へ方向移動する。移動はDirectionに沿った移動になる
	// 移動した方向が返ってくる
	UFUNCTION(BlueprintCallable, Category="TopViewPawnMovement")
	uint8 MoveToLocation(const FVector2D& Location);
	
public:
	// 指定した方向に指定した時間だけ移動する
	UFUNCTION(BlueprintCallable, Category="TopViewPawnMovement")
	void MoveToDirectionDuration(uint8 Direction, float Duration);

	// 指定した位置に指定時間でEaseする
	UFUNCTION(BlueprintCallable, Category="TopViewPawnMovement")
	void MoveToLocationEase(const FVector& Location, float Duration, TEnumAsByte<EEasingFunc::Type> Easing, float BlendExp);

	// 指定した方向と距離に指定時間でEaseする
	UFUNCTION(BlueprintCallable, Category="TopViewPawnMovement")
	void MoveToDirectionDistanceEase(uint8 Direction, float Distance, float Duration, TEnumAsByte<EEasingFunc::Type> Easing, float BlendExp);

	// 指定した位置に移動し続ける
	UFUNCTION(BlueprintCallable, Category="TopViewPawnMovement")
	void MoveToLocationAuto(const FVector2D& Location, float InAcceptRadius=20.f);

	// TopViewPawnMoveに実装した特殊移動をストップする
	UFUNCTION(BlueprintCallable, Category="TopViewPawnMovement")
	void StopTopViewMovemnt();

public:
	UFUNCTION(BlueprintCallable, Category="TopViewPawnMovement")
	FVector GetNavVelocity()const{ return NavVelocity; }

public:
	// 速さ(0.0~1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TopViewPawnMovement", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0", UIMax="1"))
	float MoveSpeed;

public:
	UPROPERTY(BlueprintAssignable, Category="TopViewPawnMovement")
	FTopViewOnStandSignature OnStand;

	UPROPERTY(BlueprintAssignable, Category="TopViewPawnMovement")
	FTopViewOnMoveSignature OnMove;

	// Navigation中に向きが変わった時に呼ばれる
	UPROPERTY(BlueprintAssignable, Category="TopViewPawnMovement")
	FTopViewOnNavDirectionSignature OnNavDirection;

	// 移動が終了した時に呼ばれる。StopTopViewMovementで止まった時はbAbortがtrue
	UPROPERTY(BlueprintAssignable, Category="TopViewPawnMovement")
	FTopViewOnMoveFinishedSignature OnMoveFinished;

private:
	void MoveNavInner(float DeltaTime);
	void CalcVelcocity(float DeltaTime, bool bDelta, bool bCorrected=false);

	void MoveNormal(float DeltaTime);
	bool MoveDurationInner(float DeltaTime);
	bool MoveEaseInner(float DeltaTime);
	bool MoveLocationAutoInner(float DeltaTime);

protected:
	MoveMode	Mode;

	FVector		NavVelocity;
	uint8		NavDirection;

	// 特殊移動系の変数
	uint8		MoveDirection;
	FVector		MoveStartLocation;
	FVector		MoveGoalLocation;
	float		MoveDuration;
	float		MoveDeltaTimes;
	float		AcceptRadius;

	TEnumAsByte<EEasingFunc::Type>	MoveEasing;
	float							MoveBlendExp;
};
