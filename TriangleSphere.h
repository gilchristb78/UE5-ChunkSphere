// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriangleSphere.generated.h"

class UProceduralMeshComponent;

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

	TObjectPtr<UProceduralMeshComponent> Mesh;
	TArray<FVector> Corners;

	TArray<FVector> Vertices;
	TArray<int> Triangles;

	

	void RefreshMoon();
	void RefreshVertices(int Resolution);
	void RefreshTriangles(int Resolution);
	TArray<int> GetVerticeRow(int RowNum, int Resolution);
	int GetTriangleNum(int x);
};
