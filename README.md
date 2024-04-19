# UE5-ChunkSphere
In this project I will expand on icosphere by dividing the sphere into "chunks" that can be individually subdivided for added performance.


made a decision to just search all the chunks for close subdivisions (roughly caps divisions at like 4 but only need < 5 then subdivide chunks)

## Planetary Chunk
<sub> TriangleSphere.cpp/h </sub><br><br>
This class will create one "Chunk" of a spherical world. The Chunk is comprised of 3 Vectors that Represent the corners of the triangular chunk. This can then be subdivided into smaller and smaller triangles to increase resolution. The Chunk uses craters and noise to modify the surface of the chunk. 


### Refresh Moon
In this function the chunk is "refreshed" causing all points and triangles to be recalculated. This function also sets the remaining values such as UV's and Normals before finally setting the procedurally generated mesh.

### Refresh Vertices
This function will create the vertice array that contains all points for the mesh to render.
<br><br>
How it Works:
![How the Sausage is Made](https://github.com/gilchristb78/UE5-ChunkSphere/blob/main/MoonCapture/TrianglesAndArrays.png)
1. Add 3 Rows of Vertices Connecting the 3 Corners.
2. Add the Corners in the correct locations.
3. Fill in the Inner Vertices

This function leaves us with an array of verticies with unique properties.<br> 
- Firstly we know the corners appear at index [0], [Resolution], and [2Resolution].<br>
- Next we know the edges appear at indeces (0 -> Resolution), (Resolution -> 2Resolution), and (2Resolution -> 3Resolution) exclusively.<br>
- Finally all inner vertices fall within a row with the first vertice in that row having index [3Resolution + △]

     > with △ equal to the triangle number for that row (inner rows △ = 0,1,3,6...).

### Get Vertice Row
This function will return an array of indices that correspond to vertices forming a row within the triangle. If the row number is less than 1 (ie. 0) then just return corner 0. If the row number is equal to resolution then return the edge between corners 1 and 2. Finally if we want a row in the middle then we add the point on edge 0, then loop through the inner points with our triangle math △, finaly we add the point on edge 1.

### Refresh Triangles
This function will create the triangle array that contains all triangles for the mesh to render. 
<br><br>
How it Works: <br>
![Triangle animation](https://github.com/gilchristb78/UE5-ChunkSphere/blob/main/MoonCapture/TrianglesLoop.gif)

1. Get a row of vertices utilizing GetVerticeRow()
2. Get a row of vertices one below that row
3. Loop left to right along the top row
     -  Add a Triangle
     -  If we are not at the end of our row, add an "upside down" triangle
4. Continue looping through rows until our bottom row corresponds with the bottom edge 
