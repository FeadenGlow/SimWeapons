// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "DroneCombatControllerComponent.generated.h"

class USimDroneCarrier;
class UMK1EyeballComponent;
class USimWeaponMountComponent;

struct FCollisionQueryParams;

UENUM(BlueprintType)
enum class EDroneCombatState : uint8
{
	Disabled UMETA(DisplayName = "Disabled"),
	Idle UMETA(DisplayName = "Idle"),
	Scanning UMETA(DisplayName = "Scanning"),
	TargetFound UMETA(DisplayName = "Target Found"),
	LostSight UMETA(DisplayName = "Lost Sight"),
	InvestigatingLastKnownPosition UMETA(DisplayName = "Investigating Last Known Position"),
	Approaching UMETA(DisplayName = "Approaching"),
	AvoidingObstacle UMETA(DisplayName = "Avoiding Obstacle"),
	InAttackRange UMETA(DisplayName = "In Attack Range"),
	AttackLineBlocked UMETA(DisplayName = "Attack Line Blocked"),
	PositioningForAttack UMETA(DisplayName = "Positioning For Attack"),
	Aiming UMETA(DisplayName = "Aiming"),
	ReadyToFire UMETA(DisplayName = "Ready To Fire"),
	Fired UMETA(DisplayName = "Fired"),
	RecoveringFromCollision UMETA(DisplayName = "Recovering From Collision")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDroneCombatTargetChangedSignature, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDroneCombatStateChangedSignature, EDroneCombatState, NewState);

UCLASS(BlueprintType, Blueprintable, ClassGroup = (SimWeapons), meta = (BlueprintSpawnableComponent))
class SIMWEAPONS_API UDroneCombatControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneCombatControllerComponent();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat")
	void StartCombatController();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat")
	void StopCombatController();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat")
	void ScanForTarget();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat")
	void ConsumeReceiverEnergy();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat")
	AActor* GetCurrentTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat")
	bool HasCurrentTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat")
	EDroneCombatState GetCombatState() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat|Movement")
	float GetCurrentTargetHorizontalDistance() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat|Attack")
	bool HasClearAttackLineToCurrentTarget();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat|Memory")
	bool HasValidTargetMemory() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone Combat|Memory")
	FVector GetLastKnownTargetLocation() const;

	UPROPERTY(BlueprintAssignable, Category = "Sim Weapons|Drone Combat")
	FDroneCombatTargetChangedSignature OnTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "Sim Weapons|Drone Combat")
	FDroneCombatStateChangedSignature OnCombatStateChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Scan", meta = (ClampMin = "0.1"))
	float ScanInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Energy")
	bool bConsumeReceiverEnergy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Energy", meta = (ClampMin = "0.1"))
	float ReceiverEnergyInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Targeting")
	bool bSelectClosestTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Memory")
	bool bUseTargetMemory = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Memory", meta = (ClampMin = "0.0"))
	float TargetMemoryDurationSeconds = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Memory", meta = (ClampMin = "0.0"))
	float LastKnownPositionAcceptanceRadiusCm = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Movement")
	bool bMoveToCurrentTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Movement", meta = (ClampMin = "0.0"))
	float ApproachSpeedCmPerSecond = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Movement", meta = (ClampMin = "0.0"))
	float AttackRangeCm = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Movement", meta = (ClampMin = "0.0"))
	float MoveAcceptanceToleranceCm = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Movement", meta = (ClampMin = "0.0"))
	float RotationInterpSpeed = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Movement")
	bool bKeepDroneLevelWhileRotating = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Movement")
	bool bUseSweepMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Ground Height")
	bool bUseGroundHeightControl = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Ground Height")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Ground Height", meta = (ClampMin = "0.0"))
	float DesiredAltitudeAboveGroundCm = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Ground Height", meta = (ClampMin = "0.0"))
	float GroundTraceDistanceCm = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Ground Height", meta = (ClampMin = "0.0"))
	float AltitudeInterpSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Ground Height", meta = (ClampMin = "0.0"))
	float MaxAltitudeCorrectionSpeedCmPerSecond = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Obstacle Avoidance")
	bool bUseObstacleAvoidance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Obstacle Avoidance")
	TEnumAsByte<ECollisionChannel> MovementObstacleTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Obstacle Avoidance", meta = (ClampMin = "0.0"))
	float ObstacleCheckDistanceCm = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Obstacle Avoidance", meta = (ClampMin = "1.0"))
	float ObstacleSphereRadiusCm = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Obstacle Avoidance", meta = (ClampMin = "0.0"))
	float ObstacleSideStepStrength = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Obstacle Avoidance", meta = (ClampMin = "0.0"))
	float ObstacleUpStepStrength = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision")
	bool bUseDroneBodySafetyCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision")
	TEnumAsByte<ECollisionChannel> DroneBodySafetyTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision", meta = (ClampMin = "1.0"))
	FVector DroneBodySafetyBoxHalfExtentCm = FVector(180.0f, 180.0f, 80.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision", meta = (ClampMin = "0.0"))
	float DroneBodySafetyMarginCm = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision", meta = (ClampMin = "0.0"))
	float DroneBodySafetyExtraTopHeightCm = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision Recovery")
	bool bUseCollisionRecovery = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision Recovery")
	bool bUseRotationSafetyCheck = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision Recovery")
	bool bUseSmoothCollisionRecovery = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision Recovery", meta = (ClampMin = "1.0"))
	float CollisionRecoveryMoveSpeedCmPerSecond = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision Recovery", meta = (ClampMin = "1.0"))
	float CollisionRecoveryAcceptanceRadiusCm = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision Recovery", meta = (ClampMin = "1.0"))
	float UnstuckSearchStepCm = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision Recovery", meta = (ClampMin = "1"))
	int32 UnstuckSearchSteps = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Collision Recovery", meta = (ClampMin = "0.0"))
	float UnstuckUpPriorityMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack")
	FName RocketWeaponMountComponentName = TEXT("WeaponMount_Rocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack")
	TEnumAsByte<ECollisionChannel> AttackLineTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack")
	bool bUseTargetBoundsForAim = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack")
	float TargetAimVerticalOffsetCm = -80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack")
	bool bUseAttackPreparation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "0.0"))
	float MinimumSafeAttackDistanceCm = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "0.0"))
	float PreferredAttackDistanceCm = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "0.0"))
	float AttackPositionDistanceToleranceCm = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "0.0"))
	float AimToleranceDegrees = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "0.0"))
	float AimHeightToleranceCm = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "0.0"))
	float AttackAltitudeInterpSpeed = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "0.0"))
	float MaxAttackAltitudeCorrectionSpeedCmPerSecond = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack")
	bool bUseTargetPrediction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "1.0"))
	float RocketSpeedEstimateCmPerSecond = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack", meta = (ClampMin = "0.0"))
	float MaxTargetPredictionTimeSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack")
	bool bHasFiredPrimaryWeapon = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Orbit")
	bool bUseAttackOrbiting = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Orbit", meta = (ClampMin = "0.0"))
	float AttackOrbitMoveSpeedCmPerSecond = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Orbit", meta = (ClampMin = "1.0"))
	float AttackPositionObstacleSphereRadiusCm = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Orbit", meta = (ClampMin = "0.0"))
	float AttackOrbitRadialCorrectionStrength = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots")
	bool bUseAttackSlotRepositioning = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "1"))
	int32 AttackSlotBlockThreshold = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "10.0"))
	float AttackSlotAcceptanceRadiusCm = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "50.0"))
	float AttackSlotBadLocationRadiusCm = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "0.1"))
	float AttackSlotBadLocationMemorySeconds = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "1"))
	int32 AttackSlotMaxBadLocations = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "4"))
	int32 AttackSlotRadialCandidateCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "10.0"))
	float AttackSlotVerticalSearchStepCm = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "0"))
	int32 AttackSlotVerticalSearchLevels = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "50.0"))
	float AttackSlotInitialPathProbeDistanceCm = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots")
	bool bRequireAttackLineForAttackSlot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots")
	bool bUseTwoPhaseAttackSlotMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "10.0"))
	float AttackSlotHorizontalAcceptanceRadiusCm = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "0.1"))
	float AttackSlotNoProgressTimeoutSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Attack|Slots", meta = (ClampMin = "1.0"))
	float AttackSlotProgressToleranceCm = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bPrintDebugMessages = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugTargetLine = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugMovementLine = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugObstacleTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugAttackLine = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugLastKnownTargetLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugGroundTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugAimLine = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugAttackOrbit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugDroneBodySafetyCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug")
	bool bDrawDebugCollisionRecovery = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone Combat|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawDuration = 1.0f;

private:
	bool CacheRequiredComponents();

	void SetCurrentTarget(AActor* NewTarget);

	void SetCombatState(EDroneCombatState NewState);

	AActor* SelectTarget(const TArray<AActor*>& Targets) const;

	void MoveTowardCurrentTarget(float DeltaTime);

	bool CalculateAvoidanceMoveDirection(const FVector& DesiredMoveDirection, FVector& OutMoveDirection) const;

	bool IsMoveDirectionBlocked(
		const FVector& StartLocation,
		const FVector& Direction,
		float Distance,
		FHitResult& OutHit
	) const;

	bool TryApplyMovementMoveDelta(const FVector& MoveDelta, float DeltaTime, const FColor& DebugColor);

	bool BuildBlockedMovementRecoveryMoveDelta(
		const FVector& DesiredMoveDirection,
		const FVector& HorizontalMoveDelta,
		const FHitResult& BlockHit,
		float DeltaTime,
		FVector& OutMoveDelta
	) const;

	float CalculateMovementEnergyCost(float SpeedCmPerSecond, float DeltaTime) const;

	bool GetGroundLocationBelowDrone(FVector& OutGroundLocation) const;

	bool GetMinimumGroundSafeAltitudeZ(float& OutMinimumOwnerZ) const;

	bool GetDesiredOwnerAltitudeZ(float& OutDesiredOwnerZ) const;

	float GetRocketMountZOffsetFromOwner() const;

	FVector ApplyAltitudeCorrectionToMove(const FVector& CurrentMoveDelta, float DeltaTime) const;

	void ApplyStationaryAltitudeControl(float DeltaTime);

	FVector GetDroneBodySafetyBoxHalfExtentWithMargin() const;

	FVector GetDroneBodySafetyBoxCenterLocation(const FVector& OwnerLocation) const;

	bool IsDroneBodyMoveBlocked(const FVector& MoveDelta, FHitResult& OutHit) const;

	bool IsDroneBodyLocationBlocked(const FVector& Location, FHitResult& OutHit) const;

	bool IsDroneBodyTransformBlocked(const FTransform& Transform, FHitResult& OutHit) const;

	void UpdateLastSafeDroneTransform();

	bool TryRecoverFromBodyOverlap(float DeltaTime);

	bool FindNearestSafeDroneLocation(FVector& OutSafeLocation) const;

	bool TrySetDroneRotationSafely(const FRotator& NewRotation);

	void ClearCollisionRecoveryTarget();

	bool StartSmoothCollisionRecovery();

	bool ContinueSmoothCollisionRecovery(float DeltaTime);

	void UpdateAttackPreparation(float DeltaTime);

	bool ShouldRunAttackPreparation() const;

	void UpdateTargetVelocityEstimate(AActor* Target);

	FVector GetPredictedTargetAimLocation() const;

	FVector GetCurrentTargetAimLocation() const;

	bool IsRocketAimedAtLocation(const FVector& AimLocation) const;

	float GetRocketAimAngleToLocation(const FVector& AimLocation) const;

	float GetRocketHeightErrorToLocation(const FVector& AimLocation) const;

	bool ApplyAttackPositioningMove(const FVector& AimLocation, float DeltaTime);

	bool TryApplyAttackMoveDelta(const FVector& MoveDelta, float DeltaTime, const FColor& DebugColor);

	bool IsAttackMoveBlocked(const FVector& MoveDelta, FHitResult& OutHit) const;

	bool BuildAttackOrbitFallbackMoveDelta(
		const FVector& AimLocation,
		float DeltaTime,
		FVector& OutMoveDelta
	) const;

	bool BuildAttackObstacleAwareFallbackMoveDelta(
		const FVector& AimLocation,
		const FVector& DesiredMoveDelta,
		const FHitResult& BlockHit,
		float DeltaTime,
		FVector& OutMoveDelta
	) const;

	bool FindBestAttackSlot(const FVector& AimLocation, FVector& OutAttackSlotLocation) const;

	bool IsAttackSlotLocationValid(
		const FVector& SlotLocation,
		const FVector& AimLocation,
		bool bRequireHeightReady,
		bool bRequireAttackLine,
		FHitResult& OutHit
	) const;

	bool IsAttackSlotNearBadLocation(const FVector& SlotLocation) const;

	void RememberBadAttackSlot(const FVector& SlotLocation);

	void ClearExpiredBadAttackSlots();

	void ClearCurrentAttackSlot();

	void ResetAttackSlotProgressTracking();

	bool HasAttackSlotMovementTimedOut(float CurrentDistanceToSlot);

	bool TraceAttackLineToCurrentTarget(FHitResult& OutHit);

	USimWeaponMountComponent* FindWeaponMountByName(FName MountComponentName) const;

	FVector GetAttackTraceStartLocation() const;

	FVector GetTargetAimLocation() const;

	void RememberTarget(AActor* Target);

	void ClearTargetMemory();

	bool GetCurrentNavigationTargetLocation(FVector& OutLocation, bool& bOutUsingTargetMemory) const;

	void AddDroneIgnoredActors(FCollisionQueryParams& QueryParams, bool bIgnoreCurrentTarget) const;

	void PrintDebugMessage(const FString& Message) const;

	UPROPERTY(Transient)
	USimDroneCarrier* DroneCarrier = nullptr;

	UPROPERTY(Transient)
	UMK1EyeballComponent* VisionSensor = nullptr;

	UPROPERTY(Transient)
	USimWeaponMountComponent* RocketWeaponMount = nullptr;

	UPROPERTY(Transient)
	AActor* CurrentTarget = nullptr;

	UPROPERTY(Transient)
	EDroneCombatState CombatState = EDroneCombatState::Idle;

	UPROPERTY(Transient)
	AActor* LastKnownTargetActor = nullptr;

	UPROPERTY(Transient)
	FVector LastKnownTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	float LastSeenTargetTimeSeconds = -1.0f;

	UPROPERTY(Transient)
	bool bHasLastKnownTargetLocation = false;

	UPROPERTY(Transient)
	FVector LastObservedTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	float LastObservedTargetTimeSeconds = -1.0f;

	UPROPERTY(Transient)
	FVector EstimatedTargetVelocity = FVector::ZeroVector;

	UPROPERTY(Transient)
	FTransform LastSafeDroneTransform = FTransform::Identity;

	UPROPERTY(Transient)
	bool bHasLastSafeDroneTransform = false;

	UPROPERTY(Transient)
	FVector CollisionRecoveryTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bHasCollisionRecoveryTarget = false;

	UPROPERTY(Transient)
	FVector CurrentAttackSlotLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bHasCurrentAttackSlot = false;

	UPROPERTY(Transient)
	int32 RepeatedAttackPositioningBlockCount = 0;

	UPROPERTY(Transient)
	TArray<FVector> BadAttackSlotLocations;

	UPROPERTY(Transient)
	TArray<float> BadAttackSlotExpireTimeSeconds;

	UPROPERTY(Transient)
	float LastAttackSlotProgressDistanceCm = -1.0f;

	UPROPERTY(Transient)
	float LastAttackSlotProgressTimeSeconds = -1.0f;

	FTimerHandle ScanTimerHandle;

	FTimerHandle ReceiverEnergyTimerHandle;
};
