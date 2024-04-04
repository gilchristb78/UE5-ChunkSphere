// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SphereChunk.generated.h"

class ATriangleSphere;

UCLASS()
class MOONS_API ASphereChunk : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASphereChunk();

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
		{FVector::UpVector, FVector::BackwardVector, FVector::LeftVector},
		{FVector::UpVector, FVector::LeftVector, FVector::ForwardVector},
		{FVector::DownVector, FVector::ForwardVector, FVector::RightVector},
		{FVector::DownVector, FVector::RightVector, FVector::BackwardVector},
		{FVector::DownVector, FVector::BackwardVector, FVector::LeftVector},
		{FVector::DownVector, FVector::LeftVector, FVector::ForwardVector}
	};

	TArray<TArray<FVector>> ChunkTriangles;

	void Subdivide(int SubDivisions);
	void SubDivideTriangle(int SubDivisions, TArray<FVector> Triangle);

	TArray<int> GetVerticeRow(int RowNum, int Resolution);
	int GetTriangleNum(int x);

	TArray<ATriangleSphere*> Chunks;

	bool isPointInTriangle(FVector Corner0, FVector Corner1, FVector Corner2, FVector Point);

};
