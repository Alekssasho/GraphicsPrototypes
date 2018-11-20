import GICommon;
import Random;

StructuredBuffer<uint2> gSurfelSpawnCoords;
Buffer<uint> gSurfelCount;

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint dim;
    uint stride;
    Data.Surfels.Storage.GetDimensions(dim, stride);

    uint currentCount = Data.Surfels.Count[0];
    if (tid.x == 0)
    {
        Data.Surfels.Count[0] = min(Data.Surfels.Count[0] + gSurfelCount[0], dim);
    }
    if (currentCount + tid.x >= dim)
        return;

    uint2 screenPos = gSurfelSpawnCoords[tid.x];

    uint seedState = RandomSeed(tid.x + currentCount * 16000);

    // Create new Surfel
    Surfel surfel;
    surfel.Position = GetWorldPosition(screenPos);
    surfel.Normal = GetNormal(screenPos);
    surfel.Color.r = RandomFloat(seedState);
    surfel.Color.g = RandomFloat(seedState);
    surfel.Color.b = RandomFloat(seedState);

    Data.Surfels.Storage[currentCount + tid.x] = surfel;
}

