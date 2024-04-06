// Fill out your copyright notice in the Description page of Project Settings.


#include "SphereChunk.h"
#include "TriangleSphere.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASphereChunk::ASphereChunk()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASphereChunk::BeginPlay()
{
	Super::BeginPlay();

	Subdivide(PlanetSubDivisions);

	for(int i = 0; i < ChunkTriangles.Num(); i++)
	{
		TArray<FVector> ChunkTriangle = ChunkTriangles[i];
		FTransform transform = FTransform(FRotator::ZeroRotator, GetActorLocation(), FVector::OneVector);
		ATriangleSphere* Chunk = GetWorld()->SpawnActorDeferred<ATriangleSphere>(ATriangleSphere::StaticClass(), transform, this);
		Chunk->Corners = ChunkTriangle;
		Chunk->SubDivisions = ChunkSubDivisions;
		Chunk->Material = Material;
		Chunk->PlanetRadius = PlanetRadius;
		Chunk->FinishSpawning(transform);

		Chunks.Add(Chunk);
	}

	ChunkIn = Chunks[0];
	
}

// Called every frame
void ASphereChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<AActor*> DefaultPawns;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), DefaultPawns);

	for (AActor* DefaultPawn : DefaultPawns)
	{
		if (DefaultPawn)
		{
			FVector StartLocation = GetActorLocation();
			FVector EndLocation = DefaultPawn->GetActorLocation();
			FVector Normal = (EndLocation - StartLocation).GetSafeNormal();


			if (!isPointInChunk(ChunkIn, Normal))
			{
				ChunkIn->SetMaterial(Material);
				ChunkIn = GetChunkAt(Normal);
				ChunkIn->SetMaterial(Material2);
			}
			

			FVector DebugLoc = StartLocation + Normal * PlanetRadius;
			DrawDebugSphere(GetWorld(), DebugLoc, 30, 30, FColor::Green);

		}
	}
}

void ASphereChunk::Subdivide(int SubDivisions)
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

void ASphereChunk::SubDivideTriangle(int SubDivisions, TArray<FVector> Triangle)
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

TArray<int> ASphereChunk::GetVerticeRow(int RowNum, int Resolution)
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

int ASphereChunk::GetTriangleNum(int x)
{
	if (x < 1)
		return 0;

	return (x * (x + 1)) / 2;
}

bool ASphereChunk::isPointInChunk(ATriangleSphere* Chunk, FVector Point)
{
	return isPointInTriangle3D(Chunk->Corners[0].GetSafeNormal(), Chunk->Corners[1].GetSafeNormal(), Chunk->Corners[2].GetSafeNormal(), Point);
}

bool ASphereChunk::isPointInTriangle3D(FVector Corner1, FVector Corner2, FVector Corner3, FVector Point)
{
	FVector v0 = Corner3 - Corner1;
	FVector v1 = Corner2 - Corner1;
	FVector v2 = Point - Corner1;

	// Compute dot products
	float dot00 = FVector::DotProduct(v0, v0);
	float dot01 = FVector::DotProduct(v0, v1);
	float dot02 = FVector::DotProduct(v0, v2);
	float dot11 = FVector::DotProduct(v1, v1);
	float dot12 = FVector::DotProduct(v1, v2);

	// Compute barycentric coordinates
	float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is inside triangle
	return (u >= 0) && (v >= 0) && (u + v < 1);
}

ATriangleSphere* ASphereChunk::GetChunkAt(FVector NormalizedPoint)
{

	int BaseTriangle = 4 * (NormalizedPoint.Z < 0) + 2 * (NormalizedPoint.Y < 0) + (NormalizedPoint.X < 0);
	int ChunksPerDivision = 1 << PlanetSubDivisions * 1 << PlanetSubDivisions;

	int StartIndex =  BaseTriangle * ChunksPerDivision;
	int EndIndex = StartIndex + ChunksPerDivision;
	
	for (int i = StartIndex; i < EndIndex; i++)
	{
		if (isPointInChunk(Chunks[i], NormalizedPoint))
		{
			
			UE_LOG(LogTemp, Warning, TEXT("Row: %d, Col: %d"), GetRow(i), GetCol(i));
			return Chunks[i];
		}
	}

	return ChunkIn;

}

int ASphereChunk::GetRow(int index)
{
	int ChunksPerDivision = 1 << (2 * PlanetSubDivisions); //how many chunks are in each of the 8 divisions
	int modulus = FMath::Floor(index % ChunksPerDivision); //forget which division we are in, we just want the row
	int row = FMath::Floor(FMath::Sqrt((float)modulus));
	if (index >= ChunksPerDivision * 4)
		row = 7 - row;
	return row;
}

int ASphereChunk::GetCol(int index)
{

	int ChunksPerDivision = 1 << (2 * PlanetSubDivisions); //how many chunks are in each of the 8 divisions
	int modulus = FMath::Floor(index % ChunksPerDivision); //forget which division we are in, we just want the row
	int row = FMath::Floor(FMath::Sqrt((float)modulus));
	if (index >= ChunksPerDivision * 4)
		row = 7 - row;

	int BaseCol = (index % ChunksPerDivision) - pow(row,2);
	TArray<int> DivisionMap = { 0,1,3,2,0,1,3,2};
	int divisionmodifier = DivisionMap[floor(index / ChunksPerDivision)];
	if (row > 3)
	{
		BaseCol = (index % ChunksPerDivision) - pow(7 - row, 2);
		divisionmodifier *= (2 * (7 - row)) + 1;
	}
	else
	{
		divisionmodifier *= (2 * row) + 1;
	}

	return BaseCol + divisionmodifier;
}



