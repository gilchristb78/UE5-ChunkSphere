// Fill out your copyright notice in the Description page of Project Settings.


#include "Crater.h"

UCrater::UCrater()
{
}

UCrater::UCrater(float PlanetRadius)
{

}

UCrater::UCrater(float PlanetRadius, float Radius, float Floor, float RimSteep, float Height, float Smooth)
{

}

float UCrater::GetHeight(FVector Location)
{
	float Distance = GetDist(CraterCenter, Location) / CraterRadius;

	float MainShape = Distance * Distance - 1;
	float Cutoff = FMath::Min(Distance - 1 - RimHeight, 0);
	float CraterRim = Cutoff * Cutoff * RimSteepness;


	return FMath::Max(SmoothMin(MainShape, CraterRim, SmoothFactor), CraterFloor) * CraterRadius;
}

float UCrater::GetDist(FVector Point1, FVector Point2)
{
	FVector v1 = Point1.GetSafeNormal() * PlanetRadius;
	FVector v2 = Point2.GetSafeNormal() * PlanetRadius;
	return PlanetRadius * FMath::Acos(((v1.X * v2.X) + (v1.Y * v2.Y) + (v1.Z * v2.Z)) / (PlanetRadius * PlanetRadius));
}

float UCrater::SmoothMin(float a, float b, float k)
{
	float h = FMath::Clamp((b - a + k) / (2 * k), 0, 1);

	return a * h + b * (1 - h) - k * h * (1 - h);
}
