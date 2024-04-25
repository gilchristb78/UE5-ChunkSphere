// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Planet.generated.h"

class APlanetChunk;

UCLASS()
class MOONS_API APlanet : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APlanet();

	UPROPERTY(EditAnywhere, Category = "Moon")
	int PlanetSubDivisions = 2;

	UPROPERTY(EditAnywhere, Category = "Moon")
	int ChunkSubDivisions = 2;

	UPROPERTY(EditAnywhere, Category = "Moon")
	float PlanetRadius = 800;

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> MaterialWater;

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
	float warpScale = 80;

	UPROPERTY(EditAnywhere, Category = "RidgeNoise")
	float RidgeFrequency = 0.01f;

	UPROPERTY(EditAnywhere, Category = "RidgeNoise")
	int RidgeFractalOctaves = 3;

	UPROPERTY(EditAnywhere, Category = "RidgeNoise")
	int RidgeNoiseSeed = 0;

	UPROPERTY(EditAnywhere, Category = "RidgeNoise")
	float RidgeFractalLacunarity = 2.0f;

	UPROPERTY(EditAnywhere, Category = "RidgeNoise")
	float RidgeFractalGain = 0.5f;

	UPROPERTY(EditAnywhere, Category = "RidgeNoise")
	float RidgewarpScale = 80;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

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

	TArray<APlanetChunk*> Chunks;
	TArray<TArray<APlanetChunk*>> ChunkRows; //could remove eventually

	bool isPointInChunk(APlanetChunk* Chunk, FVector Point);
	bool isPointInTriangle(FVector Corner1, FVector Corner2, FVector Corner3, FVector Point);

	APlanetChunk* ChunkIn;

	APlanetChunk* GetChunkAt(FVector NormalizedPoint);
	int GetRow(int index); //could also add a get index (row / col) and remove rows array
	int GetCol(int index);
	//void SetHalf(int row, int col);

	float PlanetDist(FVector Point1, FVector Point2);
	FVector getCentroid(APlanetChunk* Chunk);
};
