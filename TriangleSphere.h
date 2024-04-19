// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "FastNoiseLite.h"
#include "Crater.h"
#include "KismetProceduralMeshLibrary.h"
#include "GameFramework/Actor.h"
#include "TriangleSphere.generated.h"

USTRUCT()
struct FChunkMeshData
{
	GENERATED_BODY();

public:
	TArray<FVector> Vertices;
	TArray<int> Triangles;
	TArray<FVector> Normals;
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
class MOONS_API ATriangleSphere : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATriangleSphere();

	//input variables
	UPROPERTY(EditAnywhere, Category = "Moon")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, Category = "Moon")
	int SubDivisions = 2;

	float PlanetRadius;
	float WarpScale = 80;
	float maxCraterRadius = 1000.0f;
	float NoiseStrength = 2000.0f;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	bool debug = false;

	TArray<FVector> Corners;

	UPROPERTY()
	TArray<UCrater*> Craters;
	void TryAddCrater(UCrater* Crater);
	void TryAddCraters(TArray<UCrater*> craters);

	void SetMaterial(UMaterialInterface* Mat);
	void SetNoiseVariables(float Freq, int Octaves, int Seed, float Lac, float Gain, float warp);

	void RefreshMoon();


	void SetRendered(bool brender, int subdiv = 0);
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

	FChunkMeshData MeshData;
	TObjectPtr<UProceduralMeshComponent> Mesh;
	
	FastNoiseLite* Noise;
	
	void AddBorder(int Resolution);
	void SetFinalMaterialValues();

	void RefreshVertices(int Resolution);
	TArray<int> GetVerticeRow(int RowNum, int Resolution);

	void RefreshTriangles(int Resolution);
	void AddTriangle(int a, int b, int c);
	
	bool Rendered = true;

	float GetDist(FVector Point1, FVector Point2);
	FVector GetCentroid();

};
