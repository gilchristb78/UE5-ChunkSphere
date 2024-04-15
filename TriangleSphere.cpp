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
	AddBorder(Resolution);

	TArray<FVector2D> UVs;

	for (FVector& vert : Vertices)
	{
		//FVector PlanarUVs = CalculateTriplanarUVs(vert);

		FVector2D UVX = FVector2D(vert.Y * 0.5, vert.Z * 0.5);
		FVector2D UVY = FVector2D(vert.X * 0.5, vert.Z * 0.5);
		FVector2D UVZ = FVector2D(vert.X * 0.5, vert.Y * 0.5);

		FVector norm = FVector(FMath::Abs(vert.X), FMath::Abs(vert.Y), FMath::Abs(vert.Z));
		FVector Mask = FVector();
		Mask.X = (norm.X > norm.Y && norm.X > norm.Z);
		Mask.Y = (norm.Y > norm.X && norm.Y > norm.Z);
		Mask.Z = (norm.Z > norm.X && norm.Z > norm.Y);

		FVector2D UV = (UVX * Mask.X) +(UVY * Mask.Y) + (UVZ * Mask.Z);
		
		UVs.Add(UV);

		FVector PlanetCenter = Corners[0].GetSafeNormal() * PlanetRadius;
		FVector location = (vert.GetSafeNormal() * PlanetRadius);
		/*float noiseX = Noise->GetNoise(location.X, location.Y, location.Z);
		float noiseY = Noise->GetNoise(location.X + 52, location.Y + 13, location.Z + 7);
		float noiseZ = Noise->GetNoise(location.X + 15, location.Y + 8, location.Z + 4);*/
		FVector WarpedLoc = location;
		Noise->DomainWarp(WarpedLoc.X, WarpedLoc.Y, WarpedLoc.Z);
		float noise = Noise->GetNoise(WarpedLoc.X / 50 /*+ (noiseZ * WarpScale)*/, WarpedLoc.Y / 50/*+ (noiseY * WarpScale)*/, WarpedLoc.Z / 50/*+ (noiseZ * WarpScale)*/);

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

	//TODO get the edges of all touching chunks and use that for normals aswell
	//"fakeIt"
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents); //todo uv's

	Vertices.SetNum(VerticeNum);
	Triangles.SetNum(TriangleNum);
	UVs.SetNum(VerticeNum);
	Normals.SetNum(VerticeNum);
	Tangents.SetNum(VerticeNum);

	Mesh->SetMaterial(0, Material);
	Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, true);

}

void ATriangleSphere::TryAddCrater(UCrater* Crater)
{
	FVector Centroid = GetCentroid();
	float centroidDist = GetDist(Corners[0], Centroid);
	float MainDist = GetDist(Crater->CraterCenter, Centroid);
	//UE_LOG(LogTemp, Warning, TEXT("CD: %f, MD: %f, CR: %f"), centroidDist, MainDist, Crater->CraterRadius);
	float multiplier = 1 + Crater->RimHeight * 2; //2 for fudgibility
	if (MainDist < (1.4 * centroidDist) + (multiplier * maxCraterRadius))//1.4 = fudgability number
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
	//FVector tester2 = FMath::LerpStable(Corners[Corner0], Corners[Corner1], float(1) / 2);
	//FVector tester = Corners[Corner1] - (tester2 - Corners[Corner1]);
	
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

void ATriangleSphere::AddBorder(int Resolution)
{

		//these are the "extended" corners connect 2 nearest a corner and the corner for a negative triangle
		//also make strips between them for negative edge triangles
		//use this for normal calcs
		float test = 1.0f / Resolution;
		float test2 = test + 1.0f;
		
		VerticeNum = Vertices.Num();
		TriangleNum = Triangles.Num();

		FVector Corner1 = Corners[0] - ((Corners[2] - Corners[0]) * test);
		FVector Corner2 = Corners[1] - ((Corners[2] - Corners[1]) * test);
		
		int top = Vertices.Num();
		Vertices.Add(Corner1);
		for (int i = 1; i <= Resolution; i++)
		{
			Vertices.Add(FMath::LerpStable(Corner1, Corner2, float(i) / (Resolution + 1)));
		}
		Vertices.Add(Corner2);

		for (int bot = 0; bot < Resolution; bot++)
		{
			
			Triangles.Add(top + 1);
			Triangles.Add(top);
			Triangles.Add(bot);
			top++;

			Triangles.Add(top);
			Triangles.Add(bot);
			Triangles.Add(bot + 1);
			
		}

		Triangles.Add(Resolution);
		Triangles.Add(Vertices.Num() - 1);
		Triangles.Add(Vertices.Num() - 2);

		Corner1 = Corners[0] + ((Corners[1] - Corners[0]) * test2);
		
		Vertices.Add(Corner1);
		
		Triangles.Add(Vertices.Num() - 1);
		Triangles.Add(Vertices.Num() - 2);
		Triangles.Add(Resolution);



		top = Vertices.Num() - 1;
		Corner2 = Corners[0] + ((Corners[2] - Corners[0]) * test2);
		for (int i = 1; i <= Resolution; i++)
		{
			Vertices.Add(FMath::LerpStable(Corner1, Corner2, float(i) / (Resolution + 1)));
		}
		Vertices.Add(Corner2);
		
		for (int bot = Resolution; bot < (3 * Resolution) - 1; bot++)
		{

			Triangles.Add(top + 1);
			Triangles.Add(top);
			Triangles.Add(bot);
			top++;

			Triangles.Add(top);
			Triangles.Add(bot);
			if (bot == Resolution)
				bot = 2 * Resolution;

			Triangles.Add(bot + 1);

		}

		Triangles.Add(top + 1);
		Triangles.Add(top);
		Triangles.Add((3 * Resolution) - 1);
		top++;

		Triangles.Add(top);
		Triangles.Add((3 * Resolution) - 1);
		Triangles.Add(2 * Resolution);


		
		Triangles.Add(top + 1);
		Triangles.Add(top);
		Triangles.Add(2 * Resolution);


		Corner1 = Corners[1] + ((Corners[2] - Corners[1]) * test2);
		Vertices.Add(Corner1);

		Triangles.Add(Vertices.Num() - 1);
		Triangles.Add(Vertices.Num() - 2);
		Triangles.Add(2 * Resolution);

		top = Vertices.Num() - 1;
		Corner2 = Corners[0] - ((Corners[1] - Corners[0]) * test);
		for (int i = 0; i <= Resolution; i++)
		{
			Vertices.Add(FMath::LerpStable(Corner1, Corner2, float(i) / (Resolution + 1)));
		}
		Vertices.Add(Corner2);

		for (int bot = 2 * Resolution; bot > Resolution + 1; bot--)
		{

			Triangles.Add(top + 1);
			Triangles.Add(top);
			Triangles.Add(bot);
			top++;

			Triangles.Add(top);
			Triangles.Add(bot);
			Triangles.Add(bot - 1);

		}

		Triangles.Add(top + 1);
		Triangles.Add(top);
		Triangles.Add(Resolution + 1);
		top++;

		Triangles.Add(top);
		Triangles.Add(Resolution + 1);
		Triangles.Add(0);

		Triangles.Add(top + 1);
		Triangles.Add(top);
		Triangles.Add(0);
		top++;

		Triangles.Add(top + 1);
		Triangles.Add(top);
		Triangles.Add(0);

		Triangles.Add(VerticeNum);
		Triangles.Add(top + 1);
		Triangles.Add(0);


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
	Noise->SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
	Noise->SetDomainWarpAmp(WarpScale);
	Noise->SetFractalOctaves(FractalOctaves);
	Noise->SetFractalLacunarity(FractalLacunarity);
	Noise->SetFractalGain(FractalGain);
	
}

FVector ATriangleSphere::CalculateTriplanarUVs(const FVector& Vertex)
{
	FVector PlanarUVs;
	float ChunkSize = GetDist(Corners[0], Corners[1]);
	PlanarUVs.X = FMath::Abs(Vertex.Y / ChunkSize); // Calculate X component based on Y coordinate
	PlanarUVs.Y = FMath::Abs(Vertex.Z / ChunkSize); // Calculate Y component based on Z coordinate
	PlanarUVs.Z = FMath::Abs(Vertex.X / ChunkSize); // Calculate Z component based on X coordinate
	return PlanarUVs;
}

void ATriangleSphere::SetMaterial(UMaterialInterface* Mat)
{
	if (Mat != Material)
	{
		Material = Mat;
		Mesh->SetMaterial(0, Mat);
	}
	
}

void ATriangleSphere::SetNoiseValues(float Freq, int Octaves, int Seed, float Lac, float Gain, float Strength, float warp)
{
	Frequency = Freq;
	FractalOctaves = Octaves;
	NoiseSeed = Seed;
	FractalLacunarity = Lac;
	FractalGain = Gain;
	NoiseStrength = Strength;
	WarpScale = warp;
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



