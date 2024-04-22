// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"
#include "PlanetChunk.h"

// Sets default values
APlanet::APlanet()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APlanet::BeginPlay()
{
	Super::BeginPlay();

	if (Chunks.IsEmpty()) //incase this actor is culled, dont redo math
	{
		Subdivide(PlanetSubDivisions);

		ChunkRows.SetNum((1 << PlanetSubDivisions) * 2);

		for (int i = 0; i < ChunkRows.Num(); i++)
		{
			int rowSize;
			int MaxRows = (1 << PlanetSubDivisions) * 2;
			i < MaxRows / 2 ? rowSize = 4 * ((2 * i) + 1) : rowSize = 4 * (2 * (MaxRows - 1 - i) + 1);
			ChunkRows[i].SetNum(rowSize);
		}

		for (int i = 0; i < ChunkTriangles.Num(); i++)
		{
			TArray<FVector> ChunkTriangle = ChunkTriangles[i];
			FTransform transform = FTransform(FRotator::ZeroRotator, (ChunkTriangle[0].GetSafeNormal() * PlanetRadius) + PlanetLocation, FVector::OneVector);
			APlanetChunk* Chunk = GetWorld()->SpawnActorDeferred<APlanetChunk>(APlanetChunk::StaticClass(), transform, this);
			Chunk->Corners = ChunkTriangle;
			Chunk->SubDivisions = ChunkSubDivisions;
			Chunk->Material = Material;
			Chunk->PlanetRadius = PlanetRadius;
			Chunk->SetNoiseVariables(Frequency, FractalOctaves, NoiseSeed, FractalLacunarity, FractalGain, warpScale);
			Chunk->FinishSpawning(transform);
			Chunks.Add(Chunk);
			ChunkRows[GetRow(i)][GetCol(i)] = Chunk;//.Insert(Chunk, GetCol(i));
		}

	}

	ChunkIn = Chunks[0];
	
}

// Called every frame
void APlanet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APlanet::Subdivide(int SubDivisions)
{
	if (SubDivisions == 0)
	{
		for (TArray<FVector> BaseTriangle : BaseTriangles)
		{
			ChunkTriangles.Add(BaseTriangle);
		}
		return;
	}

	for (TArray<FVector> BaseTriangle : BaseTriangles)
	{
		SubDivideTriangle(SubDivisions, BaseTriangle);
	}
}

void APlanet::SubDivideTriangle(int SubDivisions, TArray<FVector> Triangle)
{
	TArray<FVector> Vertices = TArray<FVector>();
	int Resolution = 1 << SubDivisions;

	//Make the edges of the Triangle (0 -> 1, 0 -> 2, 1 -> 2)
	for (int Corner1 = 0; Corner1 < 2; Corner1++)
	{
		for (int Corner2 = Corner1 + 1; Corner2 <= 2; Corner2++)
		{
			for (int i = 1; i < Resolution; i++)
			{
				Vertices.Add(FMath::LerpStable(Triangle[Corner1], Triangle[Corner2], float(i) / Resolution));
			}
		}
	}

	//Add the Corners at the right spot
	Vertices.Insert(Triangle[0], 0);
	Vertices.Insert(Triangle[1], Resolution);
	Vertices.Insert(Triangle[2], Resolution * 2);


	// Fill in Inner Triangle
	for (int i = 2; i < Resolution; i++)
	{
		for (int j = 1; j < i; j++)
		{
			Vertices.Add(FMath::LerpStable(Vertices[i], Vertices[Resolution + i], float(j) / float(i)));
		}
	}

	//Make Rows of triangles 
	TArray<int> TopRow = { 0 };
	for (int i = 1; i <= Resolution; i++)
	{
		TArray<int> BottomRow = GetVerticeRow(i, Resolution);

		for (int j = 0; j < TopRow.Num(); j++)
		{

			if (Triangle[0] == FVector::UpVector)
			{
				ChunkTriangles.Add({ Vertices[TopRow[j]],
								Vertices[BottomRow[j]],
								Vertices[BottomRow[j + 1]] });
			}
			else
			{
				ChunkTriangles.Add({ Vertices[BottomRow[j + 1]],
								Vertices[BottomRow[j]],
								Vertices[TopRow[j]] });
			}


			//Rotated Triangles
			if (j + 1 < TopRow.Num())
			{

				if (Triangle[0] == FVector::UpVector)
				{
					ChunkTriangles.Add({ Vertices[TopRow[j + 1]],
								Vertices[TopRow[j]],
								Vertices[BottomRow[j + 1]] });
				}
				else
				{
					ChunkTriangles.Add({ Vertices[BottomRow[j + 1]],
								Vertices[TopRow[j]],
								Vertices[TopRow[j + 1]] });
				}
			}
		}


		TopRow = BottomRow;
	}
}

TArray<int> APlanet::GetVerticeRow(int RowNum, int Resolution)
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
		Row.Add(Resolution * 3 + GetTriangleNum(RowNum - 2) + i);
	}

	Row.Add(RowNum + Resolution);

	return Row;
}

int APlanet::GetTriangleNum(int x)
{
	if (x < 1)
		return 0;

	return (x * (x + 1)) / 2;
}

bool APlanet::isPointInChunk(APlanetChunk* Chunk, FVector Point)
{
	return isPointInTriangle(Chunk->Corners[0].GetSafeNormal(), Chunk->Corners[1].GetSafeNormal(), Chunk->Corners[2].GetSafeNormal(), Point);
}

bool APlanet::isPointInTriangle(FVector Corner1, FVector Corner2, FVector Corner3, FVector Point)
{
	return false;
}

int APlanet::GetRow(int index)
{
	int MaxRows = (1 << PlanetSubDivisions) * 2;
	int ChunksPerDivision = 1 << (2 * PlanetSubDivisions); //how many chunks are in each of the 8 divisions
	int modulus = FMath::Floor(index % ChunksPerDivision); //forget which division we are in, we just want the row
	int row = FMath::Floor(FMath::Sqrt((float)modulus));
	if (index >= ChunksPerDivision * 4)
		row = MaxRows - 1 - row;
	return row;
}

int APlanet::GetCol(int index)
{

	int MaxRows = (1 << PlanetSubDivisions) * 2;
	int ChunksPerDivision = 1 << (2 * PlanetSubDivisions); //how many chunks are in each of the 8 divisions
	int modulus = FMath::Floor(index % ChunksPerDivision); //forget which division we are in, we just want the row
	int row = FMath::Floor(FMath::Sqrt((float)modulus));
	if (index >= ChunksPerDivision * 4)
		row = MaxRows - row - 1;

	int BaseCol = (index % ChunksPerDivision) - pow(row, 2);
	TArray<int> DivisionMap = { 0,1,3,2,0,1,3,2 };
	int divisionmodifier = DivisionMap[floor(index / ChunksPerDivision)];
	if (row >= MaxRows / 2)
	{
		BaseCol = (index % ChunksPerDivision) - pow(MaxRows - 1 - row, 2);
		divisionmodifier *= (2 * (MaxRows - 1 - row)) + 1;
	}
	else
	{
		divisionmodifier *= (2 * row) + 1;
	}

	return BaseCol + divisionmodifier;
}

float APlanet::PlanetDist(FVector Point1, FVector Point2)
{
	FVector v1 = Point1.GetSafeNormal() * PlanetRadius;
	FVector v2 = Point2.GetSafeNormal() * PlanetRadius;
	return PlanetRadius * FMath::Acos(((v1.X * v2.X) + (v1.Y * v2.Y) + (v1.Z * v2.Z)) / (PlanetRadius * PlanetRadius));
}

FVector APlanet::getCentroid(APlanetChunk* Chunk)
{
	float X1 = (Chunk->Corners[1].X + Chunk->Corners[2].X + Chunk->Corners[0].X) / 3;
	float Y1 = (Chunk->Corners[1].Y + Chunk->Corners[2].Y + Chunk->Corners[0].Y) / 3;
	float Z1 = (Chunk->Corners[1].Z + Chunk->Corners[2].Z + Chunk->Corners[0].Z) / 3;

	return FVector(X1, Y1, Z1);
}
