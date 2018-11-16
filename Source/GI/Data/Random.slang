uint RandXORShift(inout uint RNGState)
{
    // Xorshift algorithm from George Marsaglia's paper
    RNGState ^= (RNGState  << 13);
    RNGState ^= (RNGState >> 17);
    RNGState ^= (RNGState << 5);
    return RNGState;
}

uint RandomSeed(uint seed)
{
    // Wang hash
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float RandomFloat(uint seed)
{
    return float(RandXORShift(seed)) * (1.0 / 4294967296.0);
}