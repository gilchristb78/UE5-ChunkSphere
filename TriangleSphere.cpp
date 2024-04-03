// Fill out your copyright notice in the Description page of Project Settings.


#include "TriangleSphere.h"
#include "ProceduralMeshComponent.h"

// Sets default values
ATriangleSphere::ATriangleSphere()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;

	Corners = {
		FVector::DownVector,
		FVector::ForwardVector,
		FVector::LeftVector
	};

	for (FVector& Corner : Corners)
	{
		Corner *= 800;
	}

	Mesh->SetMaterial(0, Material);
	Mesh->CreateMeshSection(0, Corners, { 2, 1, 0 }, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
}

// Called when the game starts or when spawned
void ATriangleSphere::BeginPlay()
{
	Super::BeginPlay();


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
		vert = vert.GetSafeNormal() * 800; //set the radius
	}

	Mesh->CreateMeshSection(0, Vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
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

#if WITH_EDITOR
void ATriangleSphere::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ATriangleSphere, SubDivisions))
	{
		RefreshMoon(); // Call CreatePlanet() whenever SubDivisions changes
	}
}
#endif


