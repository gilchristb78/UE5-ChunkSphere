// Fill out your copyright notice in the Description page of Project Settings.


#include "TriangleSphere.h"


// Sets default values
ATriangleSphere::ATriangleSphere()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;

	Noise = new FastNoiseLite();
}

// Called when the game starts or when spawned
void ATriangleSphere::BeginPlay()
{
	Super::BeginPlay();

	if (MeshData.Vertices.IsEmpty()) //Only Set Variables and refresh moon if we havent done it yet.
	{
		RefreshMoon();
	}
		
}

/* If Values Are Changed within the editor, update the appropriate values */
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

/* Recalculate All Vertices and Triangles then Set the Mesh */
void ATriangleSphere::RefreshMoon()
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
	Mesh->CreateMeshSection(0, MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UV0, MeshData.UVX, MeshData.UVY, MeshData.UVZ, TArray<FColor>(), MeshData.Tangents, true);

}

/* Given Resolution, Reset Vertices Array and Repopulate */
void ATriangleSphere::RefreshVertices(int Resolution)
{
	//Reset Array
	MeshData.Vertices = TArray<FVector>();

	//Make the edges of the Triangle (0 -> 1, 0 -> 2, 1 -> 2)
	for (int Corner1 = 0; Corner1 < 2; Corner1++)
	{
		for (int Corner2 = Corner1 + 1; Corner2 <= 2; Corner2++)
		{

			for (int i = 1; i < Resolution; i++)
			{
				MeshData.Vertices.Add(FMath::LerpStable(Corners[Corner1], Corners[Corner2], float(i) / Resolution));
			}
		}
	}

	//Add the Corners at the right spot
	MeshData.Vertices.Insert(Corners[0], 0);
	MeshData.Vertices.Insert(Corners[1], Resolution);
	MeshData.Vertices.Insert(Corners[2], Resolution * 2);

	// Fill in Inner Triangle
	for (int i = 2; i < Resolution; i++)
	{
		for (int j = 1; j < i; j++)
		{
			MeshData.Vertices.Add(FMath::LerpStable(MeshData.Vertices[i], MeshData.Vertices[Resolution + i], float(j) / float(i)));
		}
	}
}

/* Given a Row and Resolution return the Indeces of all vertices in a given row */
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

	int TriangleNum = (RowNum - 2) < 1 ? 0 : ((RowNum - 2) * ((RowNum - 2) + 1)) / 2;
	for (int i = 0; i < RowNum - 1; i++)
	{	
		Row.Add(Resolution * 3 + TriangleNum + i);
	}

	Row.Add(RowNum + Resolution);

	return Row;
}

/* Reset Triangle Array and Repopulate */
void ATriangleSphere::RefreshTriangles(int Resolution)
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
void ATriangleSphere::SetFinalMaterialValues()
{
	MeshData.UV0.Empty();
	MeshData.UVX.Empty();
	MeshData.UVY.Empty();
	MeshData.UVZ.Empty();
	MeshData.Normals.Empty();
	MeshData.Tangents.Empty();
	for (FVector& vert : MeshData.Vertices)
	{
		//FVector PlanarUVs = CalculateTriplanarUVs(vert);
		FVector2D UVX = FVector2D(vert.Y , vert.Z );
		FVector2D UVY = FVector2D(vert.X , vert.Z );
		FVector2D UVZ = FVector2D(vert.X , vert.Y );

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
		

		FVector PlanetCenter = Corners[0].GetSafeNormal() * PlanetRadius;
		FVector location = (vert.GetSafeNormal() * PlanetRadius);
		/*float noiseX = Noise->GetNoise(location.X, location.Y, location.Z);
		float noiseY = Noise->GetNoise(location.X + 52, location.Y + 13, location.Z + 7);
		float noiseZ = Noise->GetNoise(location.X + 15, location.Y + 8, location.Z + 4);*/
		FVector WarpedLoc = location / (PlanetRadius / 100);
		//Noise->DomainWarp(WarpedLoc.X, WarpedLoc.Y, WarpedLoc.Z);
		float noise = Noise->GetNoise(WarpedLoc.X /*+ (noiseZ * WarpScale)*/, WarpedLoc.Y/*+ (noiseY * WarpScale)*/, WarpedLoc.Z/*+ (noiseZ * WarpScale)*/);

		float craterheight = 0;
		for (UCrater* Crater : Craters)
		{
			float offset = Crater->GetHeight(location);
			craterheight += offset;
		}

		vert = location - PlanetCenter + (vert.GetSafeNormal() * noise * (PlanetRadius / 10)) + (vert.GetSafeNormal() * craterheight);
		
		
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
	MeshData.UV0.SetNum(MeshData.VerticeNum);
	MeshData.UVX.SetNum(MeshData.VerticeNum);
	MeshData.UVY.SetNum(MeshData.VerticeNum);
	MeshData.UVZ.SetNum(MeshData.VerticeNum);
	MeshData.Normals.SetNum(MeshData.VerticeNum);
	MeshData.Tangents.SetNum(MeshData.VerticeNum);


}

void ATriangleSphere::SetNormalsForMesh(const TArray<FVector>& Vertices, const TArray<int>& Triangles, TArray<FVector>& Normals)
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
void ATriangleSphere::AddBorder(int Resolution)
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

/* If the crater is nearby the chunk then add it to the chunk */
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

/* Try adding each crater in an array */
void ATriangleSphere::TryAddCraters(TArray<UCrater*> craters)
{
	for (int i = 0; i < craters.Num(); i++)
	{
		TryAddCrater(craters[i]);
	}
}

/* Set Noise Variables */
void ATriangleSphere::SetNoiseVariables(float Freq, int Octaves, int Seed, float Lac, float Gain, float warp)
{
	Noise->SetSeed(Seed);
	Noise->SetFrequency(Freq);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);
	Noise->SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
	Noise->SetDomainWarpAmp(WarpScale);
	Noise->SetFractalOctaves(Octaves);
	Noise->SetFractalLacunarity(Lac);
	Noise->SetFractalGain(Gain);
}

/* Set Material and update the mesh */
void ATriangleSphere::SetMaterial(UMaterialInterface* Mat)
{
		Material = Mat;
		Mesh->SetMaterial(0, Mat);
}



/* Helper function to increase readibility */
void ATriangleSphere::AddTriangle(int a, int b, int c)
{
	MeshData.Triangles.Add(a);
	MeshData.Triangles.Add(b);
	MeshData.Triangles.Add(c);
}

void ATriangleSphere::SetRendered(bool brender, int subdiv)
{

	Rendered = brender;
	if (brender)
	{
		if (subdiv != SubDivisions)
		{
			SubDivisions = subdiv;
			RefreshMoon();
		}
		else
		{
			Mesh->CreateMeshSection(0, MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UV0, MeshData.UVX, MeshData.UVY, MeshData.UVZ, TArray<FColor>(), MeshData.Tangents, true);
		}
	}
	else
	{
		Mesh->CreateMeshSection(0, TArray<FVector>(), TArray<int>(), TArray<FVector>(), TArray<FVector2d>(), TArray<FVector2d>(), TArray<FVector2d>(), TArray<FVector2d>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	}

}

/* Use Arc Cosine to Find the distance along a sphere */
float ATriangleSphere::GetDist(FVector Point1, FVector Point2)
{
	FVector v1 = Point1.GetSafeNormal() * PlanetRadius;
	FVector v2 = Point2.GetSafeNormal() * PlanetRadius;
	return PlanetRadius * FMath::Acos(((v1.X * v2.X) + (v1.Y * v2.Y) + (v1.Z * v2.Z)) / (PlanetRadius * PlanetRadius));
}

/* Get the "center" of the chunk */
FVector ATriangleSphere::GetCentroid()
{
	float X1 = (Corners[1].X + Corners[2].X + Corners[0].X) / 3;
	float Y1 = (Corners[1].Y + Corners[2].Y + Corners[0].Y) / 3;
	float Z1 = (Corners[1].Z + Corners[2].Z + Corners[0].Z) / 3;
	return FVector(X1, Y1, Z1);
}



