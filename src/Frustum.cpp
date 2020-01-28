namespace soko
{
    struct Basis
    {
        // NOTE: Assumed normalized
        v3 p;
        v3 xAxis;
        v3 yAxis;
        v3 zAxis;
    };

    union FrustumCorners
    {
        struct
        {
            v3 nlt; // near left top
            v3 nrt;
            v3 nlb; // near left botom
            v3 nrb;
            v3 flt; // far left top
            v3 frt;
            v3 flb;
            v3 frb;
        };
        v3 corners[8];
    };

    Basis BasisFromMatrix(m4x4 m)
    {
        Basis result;
        result.xAxis = m.columns[0].xyz;
        result.yAxis = m.columns[1].xyz;
        result.zAxis = m.columns[2].xyz;
        result.p = m.columns[3].xyz;
        return result;
    }

    FrustumCorners GetFrustumCorners(Basis basis, f32 fovDeg, f32 aspectRatio, f32 n, f32 f)
    {
        FrustumCorners result;
        f32 tanHalfFov2 = Tan(ToRadians(fovDeg / 2.0f)) * 2.0f;

        f32 halfNearHeight = tanHalfFov2 * n * 0.5f;
        f32 halfNearWidth = halfNearHeight * aspectRatio;

        f32 halfFarHeight = tanHalfFov2 * f * 0.5f;
        f32 halfFarWidth = halfFarHeight * aspectRatio;

        v3 nearCenter = basis.p + basis.zAxis * n;
        v3 farCenter = basis.p + basis.zAxis * f;

        result.nlt = nearCenter + basis.yAxis * halfNearHeight - basis.xAxis * halfNearWidth;
        result.nrt = nearCenter + basis.yAxis * halfNearHeight + basis.xAxis * halfNearWidth;
        result.nlb = nearCenter - basis.yAxis * halfNearHeight - basis.xAxis * halfNearWidth;
        result.nrb = nearCenter - basis.yAxis * halfNearHeight + basis.xAxis * halfNearWidth;
        result.flt = farCenter + basis.yAxis * halfFarHeight - basis.xAxis * halfFarWidth;
        result.frt = farCenter + basis.yAxis * halfFarHeight + basis.xAxis * halfFarWidth;
        result.flb = farCenter - basis.yAxis * halfFarHeight - basis.xAxis * halfFarWidth;
        result.frb = farCenter - basis.yAxis * halfFarHeight + basis.xAxis * halfFarWidth;

        return result;
    }
}
