// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "FastNoiseLite.h"
#include "KismetProceduralMeshLibrary.h"
#include "GameFramework/Actor.h"
#include "PlanetChunk.generated.h"

USTRUCT()
struct FChunkMeshDataPlanet
{
	GENERATED_BODY();

public:
	TArray<FVector> Vertices;
	TArray<FVector> WaterVertices;
	TArray<int> Triangles;
	TArray<FVector> Normals;
	TArray<FVector> WaterNormals;
	TArray<FVector2D> UV0;
	TArray<FVector2D> UVX;
	TArray<FVector2D> UVY;
	TArray<FVector2D> UVZ;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	/*TODO make this
	* Put Border stuff in here
	* Calc UV's and such with Vertices + BorderVertices
	* Will remove TriangleNum and the weird setNum
	TArray<FVector> BorderVertices;
	TArray<int> BorderTriangles;*/

	int TriangleNum;
	int VerticeNum;
};

UCLASS()
class MOONS_API APlanetChunk : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APlanetChunk();

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> WaterMaterial;

	UPROPERTY(EditAnywhere, Category = "Moon")
	int SubDivisions = 2;

	float PlanetRadius;
	float WarpScale = 80;
	float maxCraterRadius = 1000.0f;
	float NoiseStrength = 2000.0f;



	bool debug = false;

	TArray<FVector> Corners;

	void SetMaterial(UMaterialInterface* Mat);
	void SetNoiseVariables(float Freq, int Octaves, int Seed, float Lac, float Gain, float warp);
	void SetRidgeNoiseVariables(float Freq, int Octaves, int Seed, float Lac, float Gain, float warp);

	void RefreshChunk();

	
	void SetRendered(bool brender, int subdiv = 0);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


private:

	FChunkMeshDataPlanet MeshData;
	TObjectPtr<UProceduralMeshComponent> Mesh;

	FastNoiseLite* Noise;
	FastNoiseLite* RidgeNoise;
	FastNoiseLite* TemperatureNoise;

	void AddBorder(int Resolution);
	void SetFinalMaterialValues();
	void SetNormalsForMesh(const TArray<FVector>& Vertices, const TArray<int>& Triangles, TArray<FVector>& Normals);

	void RefreshVertices(int Resolution);
	TArray<int> GetVerticeRow(int RowNum, int Resolution);

	void SetWater();

	void RefreshTriangles(int Resolution);
	void AddTriangle(int a, int b, int c);

	bool Rendered = true;

	float GetDist(FVector Point1, FVector Point2);
	FVector GetCentroid();
};
