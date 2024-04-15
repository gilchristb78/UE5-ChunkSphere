// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriangleSphere.generated.h"

class UProceduralMeshComponent;
class FastNoiseLite;
class UCrater;

UCLASS()
class MOONS_API ATriangleSphere : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATriangleSphere();

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, Category = "Moon")
	int SubDivisions = 2;

	TArray<FVector> Corners;

	float PlanetRadius;

	void SetMaterial(UMaterialInterface* Mat);
	void SetNoiseValues(float Freq, int Octaves, int Seed, float Lac, float Gain, float Strength, float warp);

	float Frequency = 0.01f;
	int FractalOctaves = 3;
	int NoiseSeed;
	float FractalLacunarity = 2.0f;
	float FractalGain = 0.5f;
	float NoiseStrength = 2000.0f;

	float WarpScale = 80;
	float debug = false;
	float maxCraterRadius = 1000.0f;

	UPROPERTY()
	TArray<UCrater*> Craters;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void RefreshMoon();

	void TryAddCrater(UCrater* Crater);
	void TryAddCraters(TArray<UCrater*> craters);
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

	float GetDist(FVector Point1, FVector Point2);
	FVector GetCentroid();

	int VerticeNum;
	int TriangleNum;

	TObjectPtr<UProceduralMeshComponent> Mesh;
	
	TArray<FVector> Vertices;
	TArray<int> Triangles;
	FastNoiseLite* Noise;
	
	void RefreshVertices(int Resolution);
	void RefreshTriangles(int Resolution);
	void AddBorder(int Resolution);
	TArray<int> GetVerticeRow(int RowNum, int Resolution);
	int GetTriangleNum(int x);
	void SetNoiseVariables();
	
	FVector CalculateTriplanarUVs(const FVector& Vertex);
};
