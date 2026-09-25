//
// XMUtil.h contains helper routines that should have been inside the Xbox Maths library, but for some reason are not
//
// At the moment only the VMX128 versions are written, so if we are going
// to use the VMX32 versions on the PS3, then we'll need to write those.
//



#ifndef MISC_SHARED_XMUTIL_H
#define MISC_SHARED_XMUTIL_H

#if _MSC_VER > 1000
#pragma once
#endif



#ifdef PLATFORM_XENON
#ifdef _VMX128_INTRINSICS_


// pack a 4-vector into an XMVECTOR as a DWORD
XMINLINE XMVECTOR XMVectorToDWORD(XMVECTOR V)
{
    XMVECTOR N;
    static CONST XMVECTOR PackScale = XM_PACK_UNSIGNEDN_SCALE(8, 8, 8, 8);

    N = XM_PACK_OFFSET;
    N = __vnmsubfp(V, PackScale, N);
    N = __vpkd3d(N, N, VPACK_D3DCOLOR, VPACK_32, 3);
    N = __vspltw(N, 0);

	return N;
}



// Store a vector as a scalar DWORD, for use in conjunction with XMVectorToDWORD
XMINLINE void XMStoreDWORD(DWORD *pDestination, XMVECTOR V)
{
	__stvewx(V, (void *) &pDestination, 0)
}



// faster version of XMMatrixRotationNormal, using estimates for sin and cos
XMINLINE XMMATRIX XMMatrixRotationNormalEst
(
    XMVECTOR NormalAxis, 
    FLOAT    Angle
)
{
    FLOAT    SinAngle;
    FLOAT    CosAngle;
    XMVECTOR N0, N1;
    XMVECTOR A;
    XMVECTOR V0;
    XMVECTOR R0, R1, R2;
    XMVECTOR C0, C1, C2;
    XMVECTOR ZO;
    XMMATRIX M;

    XMDUMMY_INITIALIZE_VECTOR(ZO);

    XMScalarSinCosEst(&SinAngle, &CosAngle, Angle);

    N0 = __vpermwi(NormalAxis, 0x63);
    N1 = __vpermwi(NormalAxis, 0x87);

    A.v[0] = SinAngle;
    A.v[1] = CosAngle;
    A.v[2] = 1.0f - CosAngle;
    A.v[3] = 0.0f;

    C2 = __vspltw(A, 2);
    C1 = __vspltw(A, 1);
    C0 = __vspltw(A, 0);
    R1 = __vspltw(A, 0);

    V0 = __vmulfp(C2, N0);

    R0 = __vmulfp(C2, NormalAxis);

    M.r[0] = __vspltisw(0);
    M.r[1] = __vspltisw(0);
    M.r[2] = __vspltisw(0);

    R2 = __vmulfp(V0, N1);

    R0 = __vmaddcfp(NormalAxis, R0, C1);

    R1 = __vmaddcfp(NormalAxis, R1, R2);
    R2 = __vnmsubfp(NormalAxis, C0, R2);

    M.r[0] = __vrlimi(M.r[0], R0, 0x8, 0);
    M.r[1] = __vrlimi(M.r[1], R2, 0x8, 2);
    M.r[2] = __vrlimi(M.r[2], R1, 0x8, 1);

    M.r[0] = __vrlimi(M.r[0], R1, 0x4, 1);
    M.r[1] = __vrlimi(M.r[1], R0, 0x4, 0);
    M.r[2] = __vrlimi(M.r[2], R2, 0x4, 3);

    ZO = __vupkd3d(ZO, VPACK_NORMSHORT2);

    M.r[0] = __vrlimi(M.r[0], R2, 0x2, 3); // <R0.x, R1.z, R2.y, 0>
    M.r[1] = __vrlimi(M.r[1], R1, 0x2, 2); // <R2.z, R0.y, R1.x, 0>
    M.r[2] = __vrlimi(M.r[2], R0, 0x2, 0); // <R1.y, R2.x, R0.z, 0>

    M.r[3] = __vpermwi(ZO, 0xAB);

    return M;
}



// faster version of XMMatrixRotationNormal, using estimates for sin and cos and angle supplied as a vector
XMINLINE XMMATRIX XMMatrixRotationNormalEstV
(
    XMVECTOR NormalAxis, 
    XMVECTOR Angle
)
{
    XMVECTOR SinAngle;
	XMVECTOR CosAngle;
    XMVECTOR N0, N1;
    XMVECTOR V0;
    XMVECTOR R0, R1, R2;
    XMVECTOR C0, C1, C2;
    XMVECTOR ZO;
    XMMATRIX M;
	XMVECTOR One = XMVectorSplatOne();

    XMDUMMY_INITIALIZE_VECTOR(ZO);

    XMVectorSinCosEst(&SinAngle, &CosAngle, Angle);

    N0 = __vpermwi(NormalAxis, 0x63);
    N1 = __vpermwi(NormalAxis, 0x87);

    C2 = XMVectorSubtract(One, CosAngle);
    C1 = CosAngle;
    C0 = SinAngle;
    R1 = SinAngle;

    V0 = __vmulfp(C2, N0);

    R0 = __vmulfp(C2, NormalAxis);

    M.r[0] = __vspltisw(0);
    M.r[1] = __vspltisw(0);
    M.r[2] = __vspltisw(0);

    R2 = __vmulfp(V0, N1);

    R0 = __vmaddcfp(NormalAxis, R0, C1);

    R1 = __vmaddcfp(NormalAxis, R1, R2);
    R2 = __vnmsubfp(NormalAxis, C0, R2);

    M.r[0] = __vrlimi(M.r[0], R0, 0x8, 0);
    M.r[1] = __vrlimi(M.r[1], R2, 0x8, 2);
    M.r[2] = __vrlimi(M.r[2], R1, 0x8, 1);

    M.r[0] = __vrlimi(M.r[0], R1, 0x4, 1);
    M.r[1] = __vrlimi(M.r[1], R0, 0x4, 0);
    M.r[2] = __vrlimi(M.r[2], R2, 0x4, 3);

    ZO = __vupkd3d(ZO, VPACK_NORMSHORT2);

    M.r[0] = __vrlimi(M.r[0], R2, 0x2, 3); // <R0.x, R1.z, R2.y, 0>
    M.r[1] = __vrlimi(M.r[1], R1, 0x2, 2); // <R2.z, R0.y, R1.x, 0>
    M.r[2] = __vrlimi(M.r[2], R0, 0x2, 0); // <R1.y, R2.x, R0.z, 0>

    M.r[3] = __vpermwi(ZO, 0xAB);

    return M;
}



// returns a random vector distributed on a sphere - doesn't work at the moment
XMINLINE XMVECTOR XMVectorOnSphere()
{
	static XMVECTOR vSelectControl = XMVectorSelectControl(0, 1, 0, 0);
	static XMFLOAT4A16 randomNos(0.25f, 0.25f, 0.25f, 0.25f);
	randomNos.x = 0.0f; // replace with random number between -0.5 and 0.5
	randomNos.y = 0.0f; // replace with random number between -0.5 and 0.5
	randomNos.z = randomNos.y;

	XMVECTOR vRandomAngles	=	XMLoadFloat4A16(&randomNos);
	XMVECTOR vTwoPi			=	__vspltw(g_XMPiConstants0, 1);
	
	vRandomAngles = XMVectorMultiply(vRandomAngles, vTwoPi);

	XMVECTOR vSin, vCos;
	XMVectorSinCosEst(&vSin, &vCos, vRandomAngles);
	
	XMVECTOR vLHS = __vpermwi(vSin, VPERMWI_CONST(1, 0, 3, 3));
	XMVECTOR vRHS = XMVectorSelect(vCos, vSin, vSelectControl);

	return XMVectorMultiply(vLHS, vRHS);
}



#endif
#endif



#endif