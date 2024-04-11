// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Crater.generated.h"

/**
 * 
 */
UCLASS()
class MOONS_API UCrater : public UObject
{
	GENERATED_BODY()

public:

	UCrater();
	UCrater(float PlanetRadius);
	UCrater(float PlanetRadius, float Radius, float Floor, float RimSteep, float Height, float Smooth);

	float GetHeight(FVector Location);
	

	FVector CraterCenter;
	float PlanetRadius;
	float CraterRadius = 250.0;
	float CraterFloor = -0.2f;
	float RimSteepness = 0.23;
	float RimHeight = 0.81f;
	float SmoothFactor = 0.2f;

private:

	float GetDist(FVector Point1, FVector Point2);
	float SmoothMin(float a, float b, float k);

};
