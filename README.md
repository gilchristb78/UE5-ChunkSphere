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

1. Add 3 Rows of Vertices Connecting the 3 Corners.

![Chunk Edges Highlighted](https://github.com/gilchristb78/UE5-ChunkSphere/blob/main/MoonCapture/TriangleEdges.png)
<br><br>
![Chunk Array with Edges](https://github.com/gilchristb78/UE5-ChunkSphere/blob/main/MoonCapture/ArrayEdges.png)

2. Add the Corners in the correct locations.

![Chunk Corners Highlighted](https://github.com/gilchristb78/UE5-ChunkSphere/blob/main/MoonCapture/TriangleCorners.png)
<br><br>
![Chunk Array with Corners](https://github.com/gilchristb78/UE5-ChunkSphere/blob/main/MoonCapture/ArrayCorners.png)

3. Fill in the Inner Vertices

![Chunk Inner Points Heighlighted](https://github.com/gilchristb78/UE5-ChunkSphere/blob/main/MoonCapture/TriangleInner.png)
![Chunk Array With Inner Points](https://github.com/gilchristb78/UE5-ChunkSphere/blob/main/MoonCapture/ArrayAll.png)
<!-- Same Triangle Again with inside -->

This function leaves us with an array of verticies with unique properties.<br> 
- Firstly we know the corners appear at index [0], [Resolution], and [2Resolution].<br>
- Next we know the edges appear at indeces (0 -> Resolution), (Resolution -> 2Resolution), and (2Resolution -> 3Resolution) exclusively.<br>
- Finally all inner vertices fall within a row with the first vertice in that row having index [3Resolution + △] 
> with △ equal to the triangle number for that row (inner rows △ = 0,1,3,6...).
