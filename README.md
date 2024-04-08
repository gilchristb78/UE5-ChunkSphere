# UE5-ChunkSphere
In this project I will expand on icosphere by dividing the sphere into "chunks" that can be individually subdivided for added performance.


made a decision to just search all the chunks for close subdivisions (roughly caps divisions at like 4 but only need < 5 then subdivide chunks)

need to eventually change actor location of chunks closer to the mesh 
(possibly at Corners[0] * PlanetRadius for easy math -> 
change actor location to GetPlanetLocation() -> 
{GetActorLocation - Corners[0] * PlanetRadius})

right now if planet is too big, doesnt render mesh because actor is too far away.
