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

#if 0
struct Plane
{
    v3 n;
    f32 d;
};

union FrustumPlanes
{
    struct
    {
        Plane left;
        Plane right;
        Plane top;
        Plane bottom;
        Plane near;
        Plane far;
    };
    Plane planes[6];
};
Plane Normalize(Plane p)
{
    Plane result;
    auto denom = 1.0f / Length(p.n);
    result.n = p.n * denom;
    result.d = p.d * denom;
    return result;
}

// NOTE: Reference http://www8.cs.umu.se/kurser/5DV051/HT12/lab/plane_extraction.pdf
FrustumPlanes ExtractPlanesFromFrustum(const m4x4* m)
{
    FrustumPlanes result;

    result.left.n.x = m->_41 + m->_11;
    result.left.n.y = m->_42 + m->_12;
    result.left.n.z = m->_43 + m->_13;
    result.left.d = m->_44 + m->_14;

    result.right.n.x = m->_41 - m->_11;
    result.right.n.y = m->_42 - m->_12;
    result.right.n.z = m->_43 - m->_13;
    result.right.d = m->_44 - m->_14;

    result.top.n.x = m->_41 - m->_21;
    result.top.n.y = m->_42 - m->_22;
    result.top.n.z = m->_43 - m->_23;
    result.top.d = m->_44 - m->_24;

    result.bottom.n.x = m->_41 + m->_21;
    result.bottom.n.y = m->_42 + m->_22;
    result.bottom.n.z = m->_43 + m->_23;
    result.bottom.d = m->_44 + m->_24;

    result.near.n.x = m->_41 + m->_31;
    result.near.n.y = m->_42 + m->_32;
    result.near.n.z = m->_43 + m->_33;
    result.near.d = m->_44 + m->_34;

    result.far.n.x = m->_41 - m->_31;
    result.far.n.y = m->_42 - m->_32;
    result.far.n.z = m->_43 - m->_33;
    result.far.d = m->_44 - m->_34;

    for (u32x i = 0; i < ArrayCount(result.planes); i++)
    {
        result.planes[i] = Normalize(result.planes[i]);
    }

    return result;
}
#endif
