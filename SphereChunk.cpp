// Fill out your copyright notice in the Description page of Project Settings.


#include "SphereChunk.h"
#include "TriangleSphere.h"
#include "Crater.h"
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

		Craters = TArray<UCrater*>();
		for (int i = 0; i < CraterNum; i++)
		{
			UCrater* newCrater = NewObject<UCrater>(this);
			newCrater->CraterCenter = FMath::VRand().GetSafeNormal() * PlanetRadius;
			newCrater->PlanetRadius = PlanetRadius;
			newCrater->CraterRadius = GetCraterRadius();
			newCrater->CraterFloor = GetCraterFloor();
			newCrater->RimSteepness = RimSteepness;
			newCrater->RimHeight = RimHeight;
			newCrater->SmoothFactor = Smoothfactor;
			Craters.Add(newCrater);
		}

		for (int i = 0; i < ChunkTriangles.Num(); i++)
		{
			TArray<FVector> ChunkTriangle = ChunkTriangles[i];
			FTransform transform = FTransform(FRotator::ZeroRotator, (ChunkTriangle[0].GetSafeNormal() * PlanetRadius) + PlanetLocation, FVector::OneVector);
			ATriangleSphere* Chunk = GetWorld()->SpawnActorDeferred<ATriangleSphere>(ATriangleSphere::StaticClass(), transform, this);
			Chunk->Corners = ChunkTriangle;
			Chunk->SubDivisions = ChunkSubDivisions;
			Chunk->Material = Material;
			Chunk->PlanetRadius = PlanetRadius;
			Chunk->SetNoiseVariables(Frequency, FractalOctaves, NoiseSeed, FractalLacunarity, FractalGain, warpScale);
			Chunk->NoiseStrength = NoiseStrength;
			Chunk->maxCraterRadius = (PlanetRadius / 4);
			Chunk->TryAddCraters(Craters);
			Chunk->FinishSpawning(transform);
			Chunks.Add(Chunk);
			ChunkRows[GetRow(i)][GetCol(i)] = Chunk;//.Insert(Chunk, GetCol(i));
		}

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
			FVector StartLocation = PlanetLocation;
			FVector EndLocation = DefaultPawn->GetActorLocation();
			FVector Normal = (EndLocation - StartLocation).GetSafeNormal();


			if (!isPointInChunk(ChunkIn, Normal))
			{
				ChunkIn->SetMaterial(Material);
				ChunkIn = GetChunkAt(Normal);
				ChunkIn->SetMaterial(Material3);
			}
			

			FVector DebugLoc = StartLocation + Normal * PlanetRadius;

			if (GEngine)
			{
				//GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Black, FString::Printf(TEXT("Dist: %f"), FVector::Dist(DebugLoc, EndLocation) / PlanetRadius));
					
			}
				

			DrawDebugSphere(GetWorld(), DebugLoc, 30, 30, FColor::Green);

		}
	}
}

void ASphereChunk::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, Material))
	{
		for (ATriangleSphere* Chunk : Chunks)
		{
			Chunk->SetMaterial(Material);
		}
	}

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, Frequency) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, FractalOctaves) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, NoiseSeed) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, FractalLacunarity) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, FractalGain) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, NoiseStrength) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, warpScale))
	{
		for (ATriangleSphere* Chunk : Chunks)
		{
			Chunk->SetNoiseVariables(Frequency, FractalOctaves, NoiseSeed, FractalLacunarity, FractalGain, warpScale);
			Chunk->RefreshMoon();
		}
	}

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, RimSteepness) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, RimHeight) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, Smoothfactor))
	{
		for (UCrater* Crater : Craters)
		{
			if (Crater)
			{
				Crater->RimSteepness = RimSteepness;
				Crater->RimHeight = RimHeight;
				Crater->SmoothFactor = Smoothfactor;
			}
		}
		for (ATriangleSphere* Chunk : Chunks)
		{
			if(Chunk->Craters.Num() > 0)
				Chunk->RefreshMoon();
		}
	}

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, MinCraterRadius) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, MaxCraterRadius) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, CraterRadiusBias))
	{
		for (UCrater* Crater : Craters)
		{
			if(Crater)
				Crater->CraterRadius = GetCraterRadius();
		}
		for (ATriangleSphere* Chunk : Chunks)
		{
			Chunk->maxCraterRadius = (PlanetRadius / 4);
			if (Chunk->Craters.Num() > 0)
				Chunk->RefreshMoon();
		}
	}

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASphereChunk, CraterNum))
	{
		if (Craters.Num() < CraterNum)
		{
			TArray<UCrater*> TempCraters;
			for (int i = Craters.Num(); i < CraterNum; i++)
			{
				UCrater* newCrater = NewObject<UCrater>();
				newCrater->CraterCenter = FMath::VRand().GetSafeNormal() * PlanetRadius;
				//DrawDebugSphere(GetWorld(), newCrater->CraterCenter + PlanetLocation, CraterRadius, 36, FColor::Blue, true);
				newCrater->PlanetRadius = PlanetRadius;
				newCrater->CraterRadius = GetCraterRadius();
				newCrater->CraterFloor = GetCraterFloor();
				newCrater->RimSteepness = RimSteepness;
				newCrater->RimHeight = RimHeight;
				newCrater->SmoothFactor = Smoothfactor;
				Craters.Add(newCrater);
				TempCraters.Add(newCrater);
			}

			for (ATriangleSphere* Chunk : Chunks)
			{
				Chunk->TryAddCraters(TempCraters);
				Chunk->RefreshMoon();
			}
		}
	}
}

float ASphereChunk::GetCraterRadius()
{


	float k = FMath::Pow(1 - CraterRadiusBias, 3);
	float randVal = FMath::FRand();
	float val = (randVal * k) / (randVal * k - randVal + 1);

	return val * ((PlanetRadius / 4) - (PlanetRadius / 200)) + (PlanetRadius / 200);
}

float ASphereChunk::GetCraterFloor()
{
	return -(FMath::Pow(FMath::FRand(),2));
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
			SetHalf(GetRow(i), GetCol(i));
			return Chunks[i];
		}
	}

	return ChunkIn;

}

int ASphereChunk::GetRow(int index)
{
	int MaxRows = (1 << PlanetSubDivisions) * 2;
	int ChunksPerDivision = 1 << (2 * PlanetSubDivisions); //how many chunks are in each of the 8 divisions
	int modulus = FMath::Floor(index % ChunksPerDivision); //forget which division we are in, we just want the row
	int row = FMath::Floor(FMath::Sqrt((float)modulus));
	if (index >= ChunksPerDivision * 4)
		row = MaxRows - 1 - row;
	return row;
}

int ASphereChunk::GetCol(int index)
{

	int MaxRows = (1 << PlanetSubDivisions) * 2;
	int ChunksPerDivision = 1 << (2 * PlanetSubDivisions); //how many chunks are in each of the 8 divisions
	int modulus = FMath::Floor(index % ChunksPerDivision); //forget which division we are in, we just want the row
	int row = FMath::Floor(FMath::Sqrt((float)modulus));
	if (index >= ChunksPerDivision * 4)
		row = MaxRows - row - 1;

	int BaseCol = (index % ChunksPerDivision) - pow(row,2);
	TArray<int> DivisionMap = { 0,1,3,2,0,1,3,2};
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

void ASphereChunk::SetHalf(int row, int col)
{
	for (ATriangleSphere* Chunk : HalfPlanetChunks)
	{
		Chunk->SetMaterial(Material);
	}
	HalfPlanetChunks = TArray<ATriangleSphere*>();

	ATriangleSphere* MiddleChunk = ChunkRows[row][col];

	for(TArray<ATriangleSphere*> Row : ChunkRows)
	{
		for (ATriangleSphere* Chunk : Row)
		{
			float MaxDist = (PI * PlanetRadius);
			float dist = PlanetDist(getCentroid(ChunkRows[row][col]), getCentroid(Chunk));
			if (dist < MaxDist / 2)
			{
				Chunk->SetMaterial(Material4);
				HalfPlanetChunks.Add(Chunk);
			}

			if (dist < MaxDist / 3)
			{
				Chunk->SetMaterial(Material3);
			}

			if (dist < MaxDist / 5)
			{
				Chunk->SetMaterial(Material5);
			}

			if (dist < MaxDist / 10)
			{
				Chunk->SetMaterial(Material2);
			}
		}

	}

}

FVector ASphereChunk::getCentroid(ATriangleSphere* Chunk)
{
	float X1 = (Chunk->Corners[1].X + Chunk->Corners[2].X + Chunk->Corners[0].X) / 3;
	float Y1 = (Chunk->Corners[1].Y + Chunk->Corners[2].Y + Chunk->Corners[0].Y) / 3;
	float Z1 = (Chunk->Corners[1].Z + Chunk->Corners[2].Z + Chunk->Corners[0].Z) / 3;

	return FVector(X1, Y1, Z1);
}

float ASphereChunk::PlanetDist(FVector Point1, FVector Point2)
{
	FVector v1 = Point1.GetSafeNormal() * PlanetRadius;
	FVector v2 = Point2.GetSafeNormal() * PlanetRadius;
	return PlanetRadius * FMath::Acos(((v1.X * v2.X) + (v1.Y * v2.Y) + (v1.Z * v2.Z)) / (PlanetRadius * PlanetRadius));
}



