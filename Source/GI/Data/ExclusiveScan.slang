StructuredBuffer<uint> Input;
RWStructuredBuffer<uint> Result;
RWStructuredBuffer<uint> BlockSums;

#define GROUP_SIZE 512
#define NUM_ELEMENTS_IN_GROUP (GROUP_SIZE * 2)

groupshared uint temp[NUM_ELEMENTS_IN_GROUP];

void ExclusiveScanBlock(uint dispatchIndex, uint groupIndex, uint groupId, bool storeSum)
{
    int offset = 1;

    temp[2 * groupIndex] = Input[2 * dispatchIndex];
    temp[2 * groupIndex + 1] = Input[2 * dispatchIndex + 1];

    [unroll]
    for (int d = NUM_ELEMENTS_IN_GROUP >> 1; d > 0; d >>= 1)
    {
        GroupMemoryBarrierWithGroupSync();
        
        if (groupIndex < d)
        {
            int ai = offset * (2 * groupIndex + 1) - 1;
            int bi = offset * (2 * groupIndex + 2) - 1;

            temp[bi] += temp[ai];
        }

        offset *= 2;
    }

    if (groupIndex == 0) 
    {
        if(storeSum)
        {
            BlockSums[groupId] = temp[NUM_ELEMENTS_IN_GROUP - 1];
        }
        temp[NUM_ELEMENTS_IN_GROUP - 1] = 0;
    }

    [unroll]
    for (int d = 1; d < NUM_ELEMENTS_IN_GROUP; d *= 2)
    {
        offset >>= 1;
        GroupMemoryBarrierWithGroupSync();

        if (groupIndex < d)
        {
            int ai = offset * (2 * groupIndex + 1) - 1;
            int bi = offset * (2 * groupIndex + 2) - 1;

            uint t = temp[ai];
            temp[ai] = temp[bi];
            temp[bi] += t;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    Result[2 * dispatchIndex] = temp[2 * groupIndex];
    Result[2 * dispatchIndex + 1] = temp[2 * groupIndex + 1];
}

[numthreads(GROUP_SIZE, 1, 1)]
void ExclusiveScanInBlocks(uint3 dispatchIndex : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 groupId : SV_GroupID)
{
    ExclusiveScanBlock(dispatchIndex.x, groupIndex, groupId.x, true);
}

[numthreads(GROUP_SIZE, 1, 1)]
void ExclusiveScanSumsOfBlocks(uint3 dispatchIndex : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 groupId : SV_GroupID)
{
    ExclusiveScanBlock(dispatchIndex.x, groupIndex, groupId.x, false);
}

[numthreads(GROUP_SIZE, 1, 1)]
void AddSumsToBlocks(uint3 dispatchIndex : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 groupId : SV_GroupID)
{
    Result[2 * dispatchIndex.x] = Result[2 * dispatchIndex.x] + Input[groupId.x];
    Result[2 * dispatchIndex.x + 1] = Result[2 * dispatchIndex.x + 1] + Input[groupId.x];
}

