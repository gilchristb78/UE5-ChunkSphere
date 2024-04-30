// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SphereChunk.generated.h"

class ATriangleSphere;
class UCrater;

UCLASS()
class MOONS_API ASphereChunk : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASphereChunk();

	UPROPERTY(EditAnywhere, Category = "Moon")
	int RotationalSpeed = 0;

	UPROPERTY(EditAnywhere, Category = "Moon")
	FColor MoonColor = FColor::Black;

	UPROPERTY(EditAnywhere, Category = "Moon")
	int PlanetSubDivisions = 2;

	UPROPERTY(EditAnywhere, Category = "Moon")
	int ChunkSubDivisions = 2;

	UPROPERTY(EditAnywhere, Category = "Moon")
	float PlanetRadius = 800;

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material2;

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material3;

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material4;

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material5;

	UPROPERTY(EditAnywhere, Category = "Moon")
	FVector PlanetLocation;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float Frequency = 0.01f;

	UPROPERTY(EditAnywhere, Category = "Noise")
	int FractalOctaves = 3;

	UPROPERTY(EditAnywhere, Category = "Noise")
	int NoiseSeed = 0;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float FractalLacunarity = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float FractalGain = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float NoiseStrength = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float warpScale = 3;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float colorWarpScale = 2;

	UPROPERTY(EditAnywhere, Category = "Crater")
	int CraterNum = 1;

	UPROPERTY(EditAnywhere, Category = "Crater")
	float MinCraterRadius = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Crater")
	float MaxCraterRadius = 5000.0f;

	UPROPERTY(EditAnywhere, Category = "Crater")
	float CraterRadiusBias = 0.75;

	UPROPERTY(EditAnywhere, Category = "Crater")
	float RimSteepness = 0.23f;

	UPROPERTY(EditAnywhere, Category = "Crater")
	float RimHeight = 0.81f;

	UPROPERTY(EditAnywhere, Category = "Crater")
	float Smoothfactor = 0.2f;

	
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

	float GetCraterRadius();
	float GetCraterFloor();

	UPROPERTY()
	TArray<UCrater*> Craters;

	TArray<TArray<FVector>> BaseTriangles = {
		{FVector::UpVector, FVector::ForwardVector, FVector::RightVector},
		{FVector::UpVector, FVector::RightVector, FVector::BackwardVector},
		{FVector::UpVector, FVector::LeftVector, FVector::ForwardVector},
		{FVector::UpVector, FVector::BackwardVector, FVector::LeftVector},
		{FVector::DownVector, FVector::ForwardVector, FVector::RightVector},
		{FVector::DownVector, FVector::RightVector, FVector::BackwardVector},
		{FVector::DownVector, FVector::LeftVector, FVector::ForwardVector},
		{FVector::DownVector, FVector::BackwardVector, FVector::LeftVector},
		
	};

	TArray<TArray<FVector>> ChunkTriangles;

	void Subdivide(int SubDivisions);
	void SubDivideTriangle(int SubDivisions, TArray<FVector> Triangle);

	TArray<int> GetVerticeRow(int RowNum, int Resolution);
	int GetTriangleNum(int x);

	TArray<ATriangleSphere*> Chunks;
	TArray<TArray<ATriangleSphere*>> ChunkRows; //could remove eventually

	bool isPointInChunk(ATriangleSphere* Chunk, FVector Point);
	bool isPointInTriangle3D(FVector Corner1, FVector Corner2, FVector Corner3, FVector Point);

	ATriangleSphere* ChunkIn;

	ATriangleSphere* GetChunkAt(FVector NormalizedPoint);
	int GetRow(int index); //could also add a get index (row / col) and remove rows array
	int GetCol(int index);
	void SetHalf(int row, int col);

	float PlanetDist(FVector Point1, FVector Point2);
	FVector getCentroid(ATriangleSphere* Chunk);
};
