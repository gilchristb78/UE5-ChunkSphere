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

	for (TArray<FVector> ChunkTriangle : ChunkTriangles)
	{
		FTransform transform = FTransform(FRotator::ZeroRotator, GetActorLocation(), FVector::OneVector);
		ATriangleSphere* Chunk = GetWorld()->SpawnActorDeferred<ATriangleSphere>(ATriangleSphere::StaticClass(), transform, this);
		Chunk->Corners = ChunkTriangle;
		Chunk->SubDivisions = ChunkSubDivisions;
		Chunk->Material = Material;
		Chunk->PlanetRadius = PlanetRadius;
		Chunk->FinishSpawning(transform);

		Chunks.Add(Chunk);

		if (ChunkTriangle[0].Z > 0)
		{
			FVector DebugLoc = ChunkTriangle[0].GetSafeNormal() * PlanetRadius + GetActorLocation();
			DebugLoc.Z = GetActorLocation().Z + PlanetRadius + 10;
			DrawDebugSphere(GetWorld(), DebugLoc, 30, 15, FColor::Red, true);

			DebugLoc = ChunkTriangle[1].GetSafeNormal() * PlanetRadius + GetActorLocation();
			DebugLoc.Z = GetActorLocation().Z + PlanetRadius + 10;
			DrawDebugSphere(GetWorld(), DebugLoc, 30, 15, FColor::Red, true);

			DebugLoc = ChunkTriangle[2].GetSafeNormal() * PlanetRadius + GetActorLocation();
			DebugLoc.Z = GetActorLocation().Z + PlanetRadius + 10;
			DrawDebugSphere(GetWorld(), DebugLoc, 30, 15, FColor::Red, true);
		}
	}
	
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

			int Incrementer = 1 << PlanetSubDivisions * 1 << PlanetSubDivisions;

			//Figure out which Resoultion^2 set of chunks its in (right now just do top or bottom
			//need math later to figure out rest

			if (Normal.Z > 0) //up
			{
				if (Normal.Y > 0) //right
				{
					if (Normal.X > 0) //forward
					{
						Chunks[0]->SetMaterial(Material2);
					}
					else //back
					{
						Chunks[Incrementer]->SetMaterial(Material2);
					}
				}
				else //left
				{
					if (Normal.X > 0) //forward
					{
						Chunks[Incrementer * 3]->SetMaterial(Material2);
					}
					else //back
					{
						Chunks[Incrementer * 2]->SetMaterial(Material2);
					}
				}
			}
			else // down
			{
				if (Normal.Y > 0) //right
				{
					if (Normal.X > 0) //forward
					{
						Chunks[Chunks.Num() - Incrementer * 4]->SetMaterial(Material2);
					}
					else //back
					{
						Chunks[Chunks.Num() - Incrementer * 3]->SetMaterial(Material2);
					}
				}
				else //left
				{
					if (Normal.X > 0) //forward
					{
						Chunks[Chunks.Num() - Incrementer]->SetMaterial(Material2);
					}
					else //back
					{
						Chunks[Chunks.Num() - Incrementer * 2]->SetMaterial(Material2);
					}
				}
			}

			//UE_LOG(LogTemp, Warning, TEXT("Normal: %f %f %f"), Normal.X, Normal.Y, Normal.Z);

			FVector DebugLoc = StartLocation + Normal * PlanetRadius;
			DebugLoc.Z = PlanetRadius + 10 + StartLocation.Z;

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

bool ASphereChunk::isPointInTriangle(FVector Corner0, FVector Corner1, FVector Corner2, FVector Point)
{

	float x1 = Corner0.X;
	float x2 = Corner1.X;
	float x3 = Corner2.X;

	float y1 = Corner0.Y;
	float y2 = Corner1.Y;
	float y3 = Corner2.Y;

	float xp = Point.X;
	float yp = Point.Y;



	float areaABC = 0.5f * ((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1));
	float areaBCP = 0.5f * ((x2 - xp) * (y3 - yp) - (x3 - xp) * (y2 - yp));
	float areaCAP = 0.5f * ((x3 - xp) * (y1 - yp) - (x1 - xp) * (y3 - yp));

	// Calculate barycentric coordinates
	float alpha = areaBCP / areaABC;
	float beta = areaCAP / areaABC;
	float gamma = 1.0f - alpha - beta;

	// Check if point is inside triangle
	return alpha >= 0.0f && beta >= 0.0f && gamma >= 0.0f && alpha + beta + gamma <= 1.0f;

}

