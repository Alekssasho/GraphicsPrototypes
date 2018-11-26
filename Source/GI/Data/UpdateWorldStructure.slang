import GICommon;

RWStructuredBuffer<WorldStructureChunk> gWorldStructure;

StructuredBuffer<uint> gNewSurfelsCount;
StructuredBuffer<uint> gScannedNewSurfelsCount;

StructuredBuffer<uint> gOldSurfelIndices;
RWStructuredBuffer<uint> gNewSurfelIndices;

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint worldIndex = FlattenWorldIndex(tid);
    uint oldStartIndex = gWorldStructure[worldIndex].StartIndex;
    uint oldCount = gWorldStructure[worldIndex].Count;
    uint newSurfelCount = gNewSurfelsCount[worldIndex];
    uint sumOfNewSurfelCount = gScannedNewSurfelsCount[worldIndex];

    for (uint i = oldStartIndex; i < oldStartIndex + oldCount; ++i)
    {
        gNewSurfelIndices[i + sumOfNewSurfelCount + newSurfelCount] = gOldSurfelIndices[i];
    }

    gWorldStructure[worldIndex].StartIndex += sumOfNewSurfelCount;
    gWorldStructure[worldIndex].Count += newSurfelCount;
}