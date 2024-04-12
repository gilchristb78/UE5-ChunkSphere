// Fill out your copyright notice in the Description page of Project Settings.


#include "TriangleSphere.h"
#include "ProceduralMeshComponent.h"
#include "FastNoiseLite.h"
#include "Crater.h"
#include "KismetProceduralMeshLibrary.h"

// Sets default values
ATriangleSphere::ATriangleSphere()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;

	Noise = new FastNoiseLite();
}

// Called when the game starts or when spawned
void ATriangleSphere::BeginPlay()
{
	Super::BeginPlay();

	SetNoiseVariables();

	if(Vertices.IsEmpty())
		RefreshMoon();
	
	
}

// Called every frame
void ATriangleSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATriangleSphere::RefreshMoon()
{
	int Resolution = 1 << SubDivisions;
	RefreshVertices(Resolution);
	RefreshTriangles(Resolution);

	for (FVector& vert : Vertices)
	{
		FVector PlanetCenter = Corners[0].GetSafeNormal() * PlanetRadius;
		FVector location = (vert.GetSafeNormal() * PlanetRadius);
		float noiseX = Noise->GetNoise(location.X, location.Y, location.Z);
		float noiseY = Noise->GetNoise(location.X + 520, location.Y + 130, location.Z + 70);
		float noiseZ = Noise->GetNoise(location.X + 150, location.Y + 80, location.Z + 40);
		float noise = Noise->GetNoise(location.X + noiseZ * (PlanetRadius / 12), location.Y + noiseY* (PlanetRadius / 12), location.Z + noiseZ * (PlanetRadius / 12));
		

		float craterheight = 0;
		for(UCrater* Crater : Craters)
		{
			float offset = Crater->GetHeight(location);
			craterheight += offset;
			
		}

		vert = location - PlanetCenter + (vert.GetSafeNormal() * noise * NoiseStrength) + (vert.GetSafeNormal() * craterheight);
	}
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;

	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, TArray<FVector2D>(), Normals, Tangents); //todo uv's

	Mesh->SetMaterial(0, Material);
	Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
}

void ATriangleSphere::TryAddCrater(UCrater* Crater)
{
	FVector Centroid = GetCentroid();
	float centroidDist = GetDist(Corners[0], Centroid);
	float MainDist = GetDist(Crater->CraterCenter, Centroid);
	//UE_LOG(LogTemp, Warning, TEXT("CD: %f, MD: %f, CR: %f"), centroidDist, MainDist, Crater->CraterRadius);
	if (MainDist < centroidDist + 2 * Crater->CraterRadius)
	{
		Craters.Add(Crater);
	}

}

void ATriangleSphere::TryAddCraters(TArray<UCrater*> craters)
{
	for (int i = 0; i < craters.Num(); i++)
	{
		TryAddCrater(craters[i]);
	}
}

void ATriangleSphere::RefreshVertices(int Resolution)
{
	
	//Reset Array
	Vertices = TArray<FVector>();

	//Make the edges of the Triangle (0 -> 1, 0 -> 2, 1 -> 2)
	for (int Corner1 = 0; Corner1 < 2; Corner1++)
	{
		for (int Corner2 = Corner1 + 1; Corner2 <= 2; Corner2++)
		{
			for (int i = 1; i < Resolution; i++)
			{
				Vertices.Add(FMath::LerpStable(Corners[Corner1], Corners[Corner2], float(i) / Resolution));
			}
		}
	}

	//Add the Corners at the right spot
	Vertices.Insert(Corners[0], 0);
	Vertices.Insert(Corners[1], Resolution);
	Vertices.Insert(Corners[2], Resolution * 2);


	// Fill in Inner Triangle
	for (int i = 2; i < Resolution; i++)
	{
		for (int j = 1; j < i; j++)
		{
			Vertices.Add(FMath::LerpStable(Vertices[i], Vertices[Resolution + i], float(j) / float(i)));
		}
	}
}

void ATriangleSphere::RefreshTriangles(int Resolution)
{
	//Reset Array
	Triangles = TArray<int>();

	//Make Rows of triangles 
	TArray<int> TopRow = { 0 };
	for (int i = 1; i <= Resolution; i++)
	{
		TArray<int> BottomRow = GetVerticeRow(i, Resolution);

		for (int j = 0; j < TopRow.Num(); j++)
		{
			//Normal Triangles in strip
			Triangles.Add(BottomRow[j + 1]);
			Triangles.Add(BottomRow[j]);
			Triangles.Add(TopRow[j]);

			//Rotated Triangles
			if (j + 1 < TopRow.Num())
			{
				Triangles.Add(BottomRow[j + 1]);
				Triangles.Add(TopRow[j]);
				Triangles.Add(TopRow[j + 1]);
			}
		}


		TopRow = BottomRow;
	}
}

TArray<int> ATriangleSphere::GetVerticeRow(int RowNum, int Resolution)
{
	TArray<int> Row;

	//First Row is just the "Top" Corner
	if (RowNum < 1)
	{
		return { 0 };
	}
	
	//Bottom Row is an Edge
	if (RowNum == Resolution)
	{
		Row.Add(RowNum);

		for (int i = 1; i < Resolution; i++)
		{
			Row.Add(2 * Resolution + i);
		}
		Row.Add(2 * Resolution);

		return Row;
	}

	//Rest need triangle math
	Row.Add(RowNum);

	for (int i = 0; i < RowNum - 1; i++)
	{
		Row.Add(Resolution * 3 + GetTriangleNum(RowNum-2) + i);
	}

	Row.Add(RowNum + Resolution);

	return Row;
}

int ATriangleSphere::GetTriangleNum(int x)
{
	if(x < 1)
		return 0;

	return (x * (x + 1)) / 2;
}

void ATriangleSphere::SetNoiseVariables()
{
	Noise->SetSeed(NoiseSeed);
	Noise->SetFrequency(Frequency);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);
	Noise->SetFractalOctaves(FractalOctaves);
	Noise->SetFractalLacunarity(FractalLacunarity);
	Noise->SetFractalGain(FractalGain);
	
}

void ATriangleSphere::SetMaterial(UMaterialInterface* Mat)
{
	if (Mat != Material)
	{
		Material = Mat;
		Mesh->SetMaterial(0, Mat);
	}
	
}

void ATriangleSphere::SetNoiseValues(float Freq, int Octaves, int Seed, float Lac, float Gain, float Strength)
{
	Frequency = Freq;
	FractalOctaves = Octaves;
	NoiseSeed = Seed;
	FractalLacunarity = Lac;
	FractalGain = Gain;
	NoiseStrength = Strength;
	SetNoiseVariables();
	RefreshMoon();
}

#if WITH_EDITOR
void ATriangleSphere::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ATriangleSphere, SubDivisions))
	{
		RefreshMoon(); // Call CreatePlanet() whenever SubDivisions changes
	}
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ATriangleSphere, Material))
	{
		SetMaterial(Material);
	}
}
#endif

float ATriangleSphere::GetDist(FVector Point1, FVector Point2)
{
	FVector v1 = Point1.GetSafeNormal() * PlanetRadius;
	FVector v2 = Point2.GetSafeNormal() * PlanetRadius;
	return PlanetRadius * FMath::Acos(((v1.X * v2.X) + (v1.Y * v2.Y) + (v1.Z * v2.Z)) / (PlanetRadius * PlanetRadius));
}
FVector ATriangleSphere::GetCentroid()
{
	float X1 = (Corners[1].X + Corners[2].X + Corners[0].X) / 3;
	float Y1 = (Corners[1].Y + Corners[2].Y + Corners[0].Y) / 3;
	float Z1 = (Corners[1].Z + Corners[2].Z + Corners[0].Z) / 3;
	return FVector(X1, Y1, Z1);
}



