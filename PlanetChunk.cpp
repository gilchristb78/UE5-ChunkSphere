// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetChunk.h"


// Sets default values
APlanetChunk::APlanetChunk()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;

	Noise = new FastNoiseLite();
	RidgeNoise = new FastNoiseLite();
	TemperatureNoise = new FastNoiseLite(); 
	
	TemperatureNoise->SetFrequency(0.0004);
	TemperatureNoise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	TemperatureNoise->SetFractalType(FastNoiseLite::FractalType_FBm);
	TemperatureNoise->SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
	TemperatureNoise->SetFractalGain(0.2);
	TemperatureNoise->SetFractalLacunarity(7.5);
	TemperatureNoise->SetFractalOctaves(5);
}

// Called when the game starts or when spawned
void APlanetChunk::BeginPlay()
{
	Super::BeginPlay();
	
	if (MeshData.Vertices.IsEmpty()) //Only Set Variables and refresh moon if we havent done it yet.
	{
		RefreshChunk();
		SetWater();
	}
}

/* Recalculate All Vertices and Triangles then Set the Mesh */
void APlanetChunk::RefreshChunk()
{
	int Resolution = 1 << SubDivisions;

	if (Rendered)
	{
		RefreshVertices(Resolution);
		RefreshTriangles(Resolution);
		AddBorder(Resolution);
		SetFinalMaterialValues();
	}

	Mesh->SetMaterial(0, Material);
	Mesh->CreateMeshSection(0, MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UV0, MeshData.UVX, MeshData.UVY, MeshData.UVZ, MeshData.Colors, MeshData.Tangents, true);
}

/* Given Resolution, Reset Vertices Array and Repopulate */
void APlanetChunk::RefreshVertices(int Resolution)
{
	//Reset Array
	MeshData.Vertices = TArray<FVector>();
	FVector val;
	//Make the edges of the Triangle (0 -> 1, 0 -> 2, 1 -> 2)
	for (int Corner1 = 0; Corner1 < 2; Corner1++)
	{
		for (int Corner2 = Corner1 + 1; Corner2 <= 2; Corner2++)
		{

			for (int i = 1; i < Resolution; i++)
			{
				val = FMath::LerpStable(Corners[Corner1], Corners[Corner2], float(i) / Resolution);
				MeshData.Vertices.Add(val);
				MeshData.WaterVertices.Add(val);
			}
		}
	}

	//Add the Corners at the right spot
	MeshData.Vertices.Insert(Corners[0], 0);
	MeshData.Vertices.Insert(Corners[1], Resolution);
	MeshData.Vertices.Insert(Corners[2], Resolution * 2);
	MeshData.WaterVertices.Insert(Corners[0], 0);
	MeshData.WaterVertices.Insert(Corners[1], Resolution);
	MeshData.WaterVertices.Insert(Corners[2], Resolution * 2);


	// Fill in Inner Triangle
	for (int i = 2; i < Resolution; i++)
	{
		for (int j = 1; j < i; j++)
		{
			val = FMath::LerpStable(MeshData.Vertices[i], MeshData.Vertices[Resolution + i], float(j) / float(i));
			MeshData.Vertices.Add(val);
			MeshData.WaterVertices.Add(val);
		}
	}
}

/* Given a Row and Resolution return the Indeces of all vertices in a given row */
TArray<int> APlanetChunk::GetVerticeRow(int RowNum, int Resolution)
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

	int TriangleNum = (RowNum - 2) < 1 ? 0 : ((RowNum - 2) * ((RowNum - 2) + 1)) / 2;
	for (int i = 0; i < RowNum - 1; i++)
	{
		Row.Add(Resolution * 3 + TriangleNum + i);
	}

	Row.Add(RowNum + Resolution);

	return Row;
}

void APlanetChunk::SetWater()
{

	/*
	FVector PlanetCenter = Corners[0].GetSafeNormal() * PlanetRadius;
		FVector location = (vert.GetSafeNormal() * PlanetRadius);
		FVector WarpedLoc = location;
		Noise->DomainWarp(WarpedLoc.X, WarpedLoc.Y, WarpedLoc.Z);
		float noise = Noise->GetNoise(WarpedLoc.X, WarpedLoc.Y, WarpedLoc.Z);

		vert = location - PlanetCenter +(vert.GetSafeNormal() * noise * (PlanetRadius / 12));
	*/
	if (debug)
	{
		UE_LOG(LogTemp, Warning, TEXT("water"));
	}
	for (int i = 0; i < MeshData.WaterVertices.Num(); i++)
	{
		FVector Vertice = MeshData.WaterVertices[i];
		FVector PlanetCenter = Corners[0].GetSafeNormal() * PlanetRadius;
		FVector Worldlocation = Vertice.GetSafeNormal() * PlanetRadius;
		FVector WorldWaterLevel = Worldlocation - PlanetCenter;
		MeshData.WaterVertices[i] = WorldWaterLevel;// = (WorldWaterLevel);
		MeshData.WaterNormals.Add(Worldlocation.GetSafeNormal());
	}

	Mesh->SetMaterial(1, WaterMaterial);
	Mesh->CreateMeshSection(1, MeshData.WaterVertices, MeshData.Triangles, MeshData.WaterNormals, TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	
}

/* Reset Triangle Array and Repopulate */
void APlanetChunk::RefreshTriangles(int Resolution)
{
	//Reset Array
	MeshData.Triangles = TArray<int>();

	//Make Rows of triangles 
	TArray<int> TopRow = { 0 };

	for (int i = 1; i <= Resolution; i++)
	{
		TArray<int> BottomRow = GetVerticeRow(i, Resolution);

		for (int j = 0; j < TopRow.Num(); j++)
		{
			AddTriangle(BottomRow[j + 1], BottomRow[j], TopRow[j]);

			//Rotated Triangles
			if (j + 1 < TopRow.Num())
			{
				AddTriangle(BottomRow[j + 1], TopRow[j], TopRow[j + 1]);
			}
		}


		TopRow = BottomRow;
	}
}

/* Set the final values before setting the mesh
*
*  Set Vertice Locations with Noise and Craters
*  Calculate normal / Tangent / UV values
*
*  Still Have to fix triplanar so it properly blends border
*/
void APlanetChunk::SetFinalMaterialValues()
{
	MeshData.UV0.Empty();
	MeshData.UVX.Empty();
	MeshData.UVY.Empty();
	MeshData.UVZ.Empty();
	MeshData.Colors.Empty();
	MeshData.Normals.Empty();
	MeshData.Tangents.Empty();
	for (FVector& vert : MeshData.Vertices)
	{
		//FVector PlanarUVs = CalculateTriplanarUVs(vert);
		FVector2D UVX = FVector2D(vert.Y, vert.Z);
		FVector2D UVY = FVector2D(vert.X, vert.Z);
		FVector2D UVZ = FVector2D(vert.X, vert.Y);

		FVector norm = FVector(FMath::Abs(vert.X), FMath::Abs(vert.Y), FMath::Abs(vert.Z));
		FVector Mask = FVector();
		Mask.X = (norm.X > norm.Y && norm.X > norm.Z);
		Mask.Y = (norm.Y > norm.X && norm.Y > norm.Z);
		Mask.Z = (norm.Z > norm.X && norm.Z > norm.Y);

		FVector2D UV = (UVX * Mask.X) + (UVY * Mask.Y) + (UVZ * Mask.Z);

		MeshData.UV0.Add(UV);

		MeshData.UVX.Add(UVX);
		MeshData.UVY.Add(UVY);
		MeshData.UVZ.Add(UVZ);

		double color = (1 - FMath::Abs((vert.GetSafeNormal() * PlanetRadius).Z / (FVector::UpVector * PlanetRadius).Z));
		color += TemperatureNoise->GetNoise((vert.GetSafeNormal() * 10000).X, (vert.GetSafeNormal() * 10000).Y, (vert.GetSafeNormal() * 10000).Z) / 2;
		color = (color * 2000) + 200;

		/*if (color < 400)
		{
			MeshData.Colors.Add(FColor::White);
		}
		else if (color < 800)
		{
			MeshData.Colors.Add(FColor::Cyan);
		}
		else
			MeshData.Colors.Add(FColor::MakeFromColorTemperature(color));*/


		FVector PlanetCenter = Corners[0].GetSafeNormal() * PlanetRadius;
		FVector location = (vert.GetSafeNormal() * PlanetRadius);
		FVector WarpedLoc = location;
		Noise->DomainWarp(WarpedLoc.X, WarpedLoc.Y, WarpedLoc.Z);
		float noise = Noise->GetNoise(WarpedLoc.X, WarpedLoc.Y, WarpedLoc.Z);

		if (noise < 0)
		{
			MeshData.Colors.Add(FColor::White);
		}
		else if (noise < 0.05)
		{
			MeshData.Colors.Add(FColor(200,200,150));
		}
		else if (noise < 0.15)
		{
			MeshData.Colors.Add(FColor(50, 200, 25));
		}
		else if (noise < 0.3)
		{
			MeshData.Colors.Add(FColor(36, 18, 0));
		}
		else if (noise < 0.7)
		{
			MeshData.Colors.Add(FColor(99, 99, 99));
		}
		else
			MeshData.Colors.Add(FColor::White);

		float RNoise = (1 * FMath::Pow(FMath::Abs(RidgeNoise->GetNoise(WarpedLoc.X, WarpedLoc.Y, WarpedLoc.Z)),2));

		vert = location - PlanetCenter + (vert.GetSafeNormal() * noise * (PlanetRadius / 25)) +(vert.GetSafeNormal() * RNoise * (PlanetRadius / 10));
	}

	double s = FPlatformTime::Seconds();
	SetNormalsForMesh(MeshData.Vertices, MeshData.Triangles, MeshData.Normals);
	double e = FPlatformTime::Seconds();

	if (debug)
	{
		UE_LOG(LogTemp, Warning, TEXT("Time: %f"), e - s);
	}
	//takes the most time
	//UKismetProceduralMeshLibrary::CalculateTangentsForMesh(MeshData.Vertices, MeshData.Triangles, MeshData.UV0, MeshData.Normals, MeshData.Tangents); //todo uv's

	MeshData.Vertices.SetNum(MeshData.VerticeNum);
	MeshData.Triangles.SetNum(MeshData.TriangleNum);
	MeshData.Colors.SetNum(MeshData.VerticeNum);
	MeshData.UV0.SetNum(MeshData.VerticeNum);
	MeshData.UVX.SetNum(MeshData.VerticeNum);
	MeshData.UVY.SetNum(MeshData.VerticeNum);
	MeshData.UVZ.SetNum(MeshData.VerticeNum);
	MeshData.Normals.SetNum(MeshData.VerticeNum);
	MeshData.Tangents.SetNum(MeshData.VerticeNum);


}

void APlanetChunk::SetNormalsForMesh(const TArray<FVector>& Vertices, const TArray<int>& Triangles, TArray<FVector>& Normals)
{
	TArray<int> counts;
	counts.SetNum(Vertices.Num());

	Normals.Empty();
	Normals.SetNum(Vertices.Num());

	for (int i = 0; i < Triangles.Num() - 2; i += 3)
	{
		int c = Triangles[i];
		int b = Triangles[i + 1];
		int a = Triangles[i + 2];
		FVector normal = FVector::CrossProduct(Vertices[b] - Vertices[a], Vertices[c] - Vertices[a]).GetSafeNormal();
		counts[a] == 0 ? Normals[a] = normal : Normals[a] = ((Normals[a] * counts[a]) + normal) / (counts[a] + 1);
		counts[b] == 0 ? Normals[b] = normal : Normals[b] = ((Normals[b] * counts[b]) + normal) / (counts[b] + 1);
		counts[c] == 0 ? Normals[c] = normal : Normals[c] = ((Normals[c] * counts[c]) + normal) / (counts[c] + 1);
		counts[a]++;
		counts[b]++;
		counts[c]++;
	}
}

/* Add a border of triangles around chunk
*  To be used to calculate normals
*
*/
void APlanetChunk::AddBorder(int Resolution)
{


	float oneTriangle = 1.0f / Resolution;
	float SidePlusOne = oneTriangle + 1.0f;

	MeshData.VerticeNum = MeshData.Vertices.Num();
	MeshData.TriangleNum = MeshData.Triangles.Num();

	FVector Corner1 = Corners[0] - ((Corners[2] - Corners[0]) * oneTriangle);
	FVector Corner2 = Corners[1] - ((Corners[2] - Corners[1]) * oneTriangle);

	int top = MeshData.Vertices.Num();
	MeshData.Vertices.Add(Corner1);
	for (int i = 1; i <= Resolution; i++)
	{
		MeshData.Vertices.Add(FMath::LerpStable(Corner1, Corner2, float(i) / (Resolution + 1)));
	}
	MeshData.Vertices.Add(Corner2);

	for (int bot = 0; bot < Resolution; bot++)
	{
		AddTriangle(top + 1, top, bot);

		top++;

		AddTriangle(top, bot, bot + 1);
	}

	AddTriangle(Resolution, MeshData.Vertices.Num() - 1, MeshData.Vertices.Num() - 2);

	Corner1 = Corners[0] + ((Corners[1] - Corners[0]) * SidePlusOne);

	MeshData.Vertices.Add(Corner1);

	AddTriangle(MeshData.Vertices.Num() - 1, MeshData.Vertices.Num() - 2, Resolution);

	top = MeshData.Vertices.Num() - 1;
	Corner2 = Corners[0] + ((Corners[2] - Corners[0]) * SidePlusOne);
	for (int i = 1; i <= Resolution; i++)
	{
		MeshData.Vertices.Add(FMath::LerpStable(Corner1, Corner2, float(i) / (Resolution + 1)));
	}
	MeshData.Vertices.Add(Corner2);

	for (int bot = Resolution; bot < (3 * Resolution) - 1; bot++)
	{
		AddTriangle(top + 1, top, bot);

		top++;

		MeshData.Triangles.Add(top);
		MeshData.Triangles.Add(bot);

		if (bot == Resolution)
			bot = 2 * Resolution;

		MeshData.Triangles.Add(bot + 1);

	}

	AddTriangle(top + 1, top, (3 * Resolution) - 1);

	top++;

	AddTriangle(top, (3 * Resolution) - 1, 2 * Resolution);
	AddTriangle(top + 1, top, 2 * Resolution);

	Corner1 = Corners[1] + ((Corners[2] - Corners[1]) * SidePlusOne);
	MeshData.Vertices.Add(Corner1);

	AddTriangle(MeshData.Vertices.Num() - 1, MeshData.Vertices.Num() - 2, 2 * Resolution);

	top = MeshData.Vertices.Num() - 1;
	Corner2 = Corners[0] - ((Corners[1] - Corners[0]) * oneTriangle);
	for (int i = 0; i <= Resolution; i++)
	{
		MeshData.Vertices.Add(FMath::LerpStable(Corner1, Corner2, float(i) / (Resolution + 1)));
	}
	MeshData.Vertices.Add(Corner2);

	for (int bot = 2 * Resolution; bot > Resolution + 1; bot--)
	{
		AddTriangle(top + 1, top, bot);

		top++;

		AddTriangle(top, bot, bot - 1);
	}

	AddTriangle(top + 1, top, Resolution + 1);

	top++;

	AddTriangle(top, Resolution + 1, 0);
	AddTriangle(top + 1, top, 0);

	top++;

	AddTriangle(top + 1, top, 0);
	AddTriangle(MeshData.VerticeNum, top + 1, 0);
}

/* Set Noise Variables */
void APlanetChunk::SetNoiseVariables(float Freq, int Octaves, int Seed, float Lac, float Gain, float warp)
{
	Noise->SetSeed(Seed);
	Noise->SetFrequency(Freq / (PlanetRadius / 1000));
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);
	Noise->SetDomainWarpType(FastNoiseLite::DomainWarpType_BasicGrid);
	Noise->SetDomainWarpAmp(WarpScale);
	Noise->SetFractalOctaves(Octaves);
	Noise->SetFractalLacunarity(Lac);
	Noise->SetFractalGain(Gain);
	
	TemperatureNoise->SetSeed(Seed);

}

void APlanetChunk::SetRidgeNoiseVariables(float Freq, int Octaves, int Seed, float Lac, float Gain, float warp)
{
	RidgeNoise->SetSeed(Seed);
	RidgeNoise->SetFrequency(Freq / (PlanetRadius / 1000));
	RidgeNoise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	RidgeNoise->SetFractalType(FastNoiseLite::FractalType_FBm);
	RidgeNoise->SetDomainWarpType(FastNoiseLite::DomainWarpType_BasicGrid);
	RidgeNoise->SetDomainWarpAmp(WarpScale);
	RidgeNoise->SetFractalOctaves(Octaves);
	RidgeNoise->SetFractalLacunarity(Lac);
	RidgeNoise->SetFractalGain(Gain);
}

/* Set Material and update the mesh */
void APlanetChunk::SetMaterial(UMaterialInterface* Mat)
{
	Material = Mat;
	Mesh->SetMaterial(0, Mat);
}



/* Helper function to increase readibility */
void APlanetChunk::AddTriangle(int a, int b, int c)
{
	MeshData.Triangles.Add(a);
	MeshData.Triangles.Add(b);
	MeshData.Triangles.Add(c);
}

void APlanetChunk::SetRendered(bool brender, int subdiv)
{

	Rendered = brender;
	if (brender)
	{
		if (subdiv != SubDivisions)
		{
			SubDivisions = subdiv;
			RefreshChunk();
		}
		else
		{
			Mesh->CreateMeshSection(0, MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UV0, MeshData.UVX, MeshData.UVY, MeshData.UVZ, MeshData.Colors, MeshData.Tangents, true);
		}
	}
	else
	{
		Mesh->CreateMeshSection(0, TArray<FVector>(), TArray<int>(), TArray<FVector>(), TArray<FVector2d>(), TArray<FVector2d>(), TArray<FVector2d>(), TArray<FVector2d>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	}

}

/* Use Arc Cosine to Find the distance along a sphere */
float APlanetChunk::GetDist(FVector Point1, FVector Point2)
{
	FVector v1 = Point1.GetSafeNormal() * PlanetRadius;
	FVector v2 = Point2.GetSafeNormal() * PlanetRadius;
	return PlanetRadius * FMath::Acos(((v1.X * v2.X) + (v1.Y * v2.Y) + (v1.Z * v2.Z)) / (PlanetRadius * PlanetRadius));
}

/* Get the "center" of the chunk */
FVector APlanetChunk::GetCentroid()
{
	float X1 = (Corners[1].X + Corners[2].X + Corners[0].X) / 3;
	float Y1 = (Corners[1].Y + Corners[2].Y + Corners[0].Y) / 3;
	float Z1 = (Corners[1].Z + Corners[2].Z + Corners[0].Z) / 3;
	return FVector(X1, Y1, Z1);
}
