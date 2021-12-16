

#include "stdafx.h"

void utlMtx2Euler(int ord, ftype m[3][3], ftype rot[3]);
void utlMtx2Quat(ftype m[3][3], ftype quat[4]);

typedef struct {
    ftype    translate[3];
    ftype    scale[3];
    ftype    rotation[3];     /* Euler angles    */
    ftype    quaternion[4];   /* quaternion      */
    ftype    rotMatrix[3][3]; /* rotation matrix */
} DECOMP_MAT;


#define UTL_ROT_XYZ          0
#define UTL_ROT_XZY          1
#define UTL_ROT_YXZ          2
#define UTL_ROT_YZX          3
#define UTL_ROT_ZXY          4
#define UTL_ROT_ZYX          5

#define XROT                 'x'
#define YROT                 'y'
#define ZROT                 'z'

//  ========== utlDecompMatrix ==========
//
//  SYNOPSIS
//      Decompose a matrix to it's components, translate,
//      rotate ( a quaternion) and scale.
//
static void utlDecompMatrix( const ftype *mat, DECOMP_MAT *dmat, char *rotOrder)
{
    int         i, j,
    order;
    static ftype       Sxy, Sxz,
    rot[3], quat[4],
    det,
        m[3][3];

    dmat->translate[0] = mat[3*4+0];
    dmat->translate[1] = mat[3*4+1];
    dmat->translate[2] = mat[3*4+2];

    m[0][0] = mat[0*4+0];
    m[0][1] = mat[0*4+1];
    m[0][2] = mat[0*4+2];

    dmat->scale[0] = sqrt( m[0][0]*m[0][0] + m[0][1]*m[0][1] + m[0][2]*m[0][2]);

    /* Normalize second row */
    m[0][0] /= dmat->scale[0];
    m[0][1] /= dmat->scale[0];
    m[0][2] /= dmat->scale[0];

    /* Determine xy shear */
    Sxy = mat[0*4+0] * mat[1*4+0] + 
    mat[0*4+1] * mat[1*4+1] +
    mat[0*4+2] * mat[1*4+2];

    m[1][0] = mat[1*4+0] - Sxy * mat[0*4+0];
    m[1][1] = mat[1*4+1] - Sxy * mat[0*4+1];
    m[1][2] = mat[1*4+2] - Sxy * mat[0*4+2];

    dmat->scale[1] = sqrt( m[1][0]*m[1][0] + m[1][1]*m[1][1] + m[1][2]*m[1][2]);

    /* Normalize second row */
    m[1][0] /= dmat->scale[1];
    m[1][1] /= dmat->scale[1];
    m[1][2] /= dmat->scale[1];

    /* Determine xz shear */
    Sxz = mat[0*4+0] * mat[2*4+0] + 
    mat[0*4+1] * mat[2*4+1] +
    mat[0*4+2] * mat[2*4+2];

    m[2][0] = mat[2*4+0] - Sxz * mat[0*4+0];
    m[2][1] = mat[2*4+1] - Sxz * mat[0*4+1];
    m[2][2] = mat[2*4+2] - Sxz * mat[0*4+2];

    dmat->scale[2] = sqrt( m[2][0]*m[2][0] + m[2][1]*m[2][1] + m[2][2]*m[2][2]);

    /* Normalize third row */
    m[2][0] /= dmat->scale[2];
    m[2][1] /= dmat->scale[2];
    m[2][2] /= dmat->scale[2];

    det = (m[0][0]*m[1][1]*m[2][2]) + (m[0][1]*m[1][2]*m[2][0]) + (m[0][2]*m[1][0]*m[2][1]) -
    (m[0][2]*m[1][1]*m[2][0]) - (m[0][0]*m[1][2]*m[2][1]) - (m[0][1]*m[1][0]*m[2][2]);

    /* If the determinant of the rotation matrix is negative, */
    /* negate the matrix and scale factors.                   */
    
    if ( det < 0.0) {
    for ( i = 0; i < 3; i++) {
      for ( j = 0; j < 3; j++) 
        m[i][j] *= -1.0;
      dmat->scale[i] *= -1.0;
    }
    }

    // Copy the 3x3 rotation matrix into the decomposition 
    // structure.
    //
    memcpy( dmat->rotMatrix, m, sizeof( ftype)*9);

    /*rot[1] = asin( -m[0][2]);
    if ( fabsf( cos( rot[1])) > 0.0001) {
        rot[0] = asin( m[1][2]/cos( rot[1]));
        rot[2] = asin( m[0][1]/cos( rot[1]));
    } else {
        rot[0] = acos( m[1][1]);
        rot[2] = 0.0;
    }*/

    switch( rotOrder[2]) {
    case XROT:
      if ( rotOrder[1] == YROT) 
        order = UTL_ROT_XYZ;
      else
        order = UTL_ROT_XZY;
            break;

    case YROT:
      if ( rotOrder[1] == XROT) 
        order = UTL_ROT_YXZ;
      else
        order = UTL_ROT_YZX;
            break;

    case ZROT:
      if ( rotOrder[1] == XROT) 
        order = UTL_ROT_ZXY;
      else
        order = UTL_ROT_ZYX;
            break;

    default:
      order = UTL_ROT_XYZ;
      break;
    }

    utlMtx2Euler( order, m, rot);
    dmat->rotation[0] = rot[0];
    dmat->rotation[1] = rot[1];
    dmat->rotation[2] = rot[2];

    utlMtx2Quat(m,quat);
    dmat->quaternion[0] = quat[0];
    dmat->quaternion[1] = quat[1];
    dmat->quaternion[2] = quat[2];
    dmat->quaternion[3] = quat[3];
}


/*
 *  ========== CapQuat2Euler ==========
 *
 *  SYNOPSIS
 *      Convert a quaternion to Euler angles.
 *
 *  PARAMETERS
 *      int                     The order of rotations
 *      ftype   mat[3][3]       rotation matrix  
 *      ftype   rot[3]          xyz-rotation values
 *
 *  DESCRIPTION
 *      This routine converts a mateix to Euler angles.
 *      There are a few caveats:
 *          The rotation order for the returned angles is always zyx.
 *      The derivation of this algorithm is taken from Ken Shoemake's
 *      paper:
 *          SIGGRAPH 1985, Vol. 19, # 3, pp. 253-254
 *
 *  RETURN VALUE
 *      None.
 */

#define M_PI_2 M_PI/2.0

void utlMtx2Euler(int ord, ftype m[3][3], ftype rot[3])
//void utlMtx2Euler(int ord, ftype *m, ftype *rot)
{
  /*
   *  Ken Shoemake's recommended algorithm is to convert the
   *  quaternion to a matrix and the matrix to Euler angles.
   *  We do this, of course, without generating unused matrix
   *  elements.
   */
    ftype        zr, sxr, cxr,
    yr, syr, cyr,
    xr, szr, czr;
    static ftype epsilon = 1.0e-5f;

    switch ( ord) {

        case UTL_ROT_ZYX:
            syr = -m[0][2];
            cyr = sqrt(1 - syr * syr);

            if (cyr < epsilon) {
                /* Insufficient accuracy, assume that yr = PI/2 && zr = 0 */
                xr = atan2(-m[2][1], m[1][1]);
                yr = (syr > 0) ? M_PI_2 : -M_PI_2;      /* +/- 90 deg */
                zr = 0.0;
            } else {
                xr = atan2(m[1][2], m[2][2]);
                yr = atan2(syr, cyr);
                zr = atan2(m[0][1], m[0][0]);
            }
            break;

        case UTL_ROT_YZX:
            szr = m[0][1];
            czr = sqrt(1 - szr * szr);
            if (czr < epsilon) {
                /* Insufficient accuracy, assume that zr = +/- PI/2 && yr = 0 */
                xr = atan2(m[1][2], m[2][2]);
                yr = 0.0;
                zr = (szr > 0) ? M_PI_2 : -M_PI_2;
            } else {
                xr = atan2(-m[2][1], m[1][1]);
                yr = atan2(-m[0][2], m[0][0]);
                zr = atan2(szr,  czr);
            }
            break;

        case UTL_ROT_ZXY:
            sxr = m[1][2];
            cxr = sqrt(1 - sxr * sxr);

            if (cxr < epsilon) {
                /* Insufficient accuracy, assume that xr = PI/2 && zr = 0 */
                xr = (sxr > 0) ? M_PI_2 : -M_PI_2;
                yr = atan2(m[2][0], m[0][0]);
                zr = 0.0;
            } else {
                xr = atan2( sxr, cxr);
                yr = atan2(-m[0][2], m[2][2]);
                zr = atan2(-m[1][0], m[1][1]);
            }
            break;

        case UTL_ROT_XZY:
            szr = -m[1][0];
            czr = sqrt(1 - szr * szr);
            if (czr < epsilon) {
                /* Insufficient accuracy, assume that zr = PI / 2 && xr = 0 */
                xr = 0.0;
                yr = atan2(-m[0][2], m[2][2]);
                zr = (szr > 0) ? M_PI_2 : -M_PI_2;
            } else {
                xr = atan2(m[0][2], m[1][1]);
                yr = atan2(m[2][0], m[0][0]);
                zr = atan2(szr, czr);
            }
            break;

        case UTL_ROT_YXZ:
            sxr = -m[2][1];
            cxr = sqrt(1 - sxr * sxr);

            if (cxr < epsilon) {
                /* Insufficient accuracy, assume that xr = PI/2 && yr = 0 */
                xr = (sxr > 0) ? M_PI_2 : -M_PI_2;
                yr = 0.0;
                zr = atan2(-m[1][0], m[0][0]);
            } else {
                xr = atan2(sxr, cxr);
                yr = atan2(m[2][0], m[2][2]);
                zr = atan2(m[0][1], m[1][1]);
            }
            break;

        case UTL_ROT_XYZ:
            syr = m[2][0];
            cyr = sqrt(1 - syr * syr);
            if (cyr < epsilon) {
                /* Insufficient accuracy, assume that yr = PI / 2 && xr = 0 */
                xr = 0.0;
                yr = (syr > 0) ? M_PI_2 : -M_PI_2;
                zr = atan2(m[0][1], m[1][1]);
            } else {
                xr = atan2(-m[2][1], m[2][2]);
                yr = atan2( syr, cyr);
                zr = atan2(-m[1][1], m[0][0]);
            }
            break;
    }

    rot[0] = xr;
    rot[1] = yr;
    rot[2] = zr;
}

/*
 *  ========= utlMtx2Quat ====================
 * 
 *  SYNOPSIS
 *  Returns the w,x,y,z coordinates of the quaternion
 *  given the rotation matrix.
 */
static void utlMtx2Quat(ftype m[3][3], ftype quat[4])
{
    // m stores the 3x3 rotation matrix.
    // Convert it to quaternion.
    ftype trace = m[0][0] + m[1][1] + m[2][2];
    ftype s; 
    if (trace > 0.0) {
        s = sqrt(trace + 1.0);
        quat[0] = s*0.5f;
        s = 0.5f/s;
        quat[1] = (m[1][2] - m[2][1])*s;
        quat[2] = (m[2][0] - m[0][2])*s;
        quat[3] = (m[0][1] - m[1][0])*s;

    }
    else {
        int i = 0; // i represents index of quaternion, so 0=scalar, 1=xaxis, etc.
        int nxt[3] = {1,2,0}; // next index for each component.
        if (m[1][1] > m[0][0]) i = 1;
        if (m[2][2] > m[i][i]) i = 2;
        int j = nxt[i]; int k = nxt[j];
        s = sqrt( (m[i][i] - (m[j][j] + m[k][k])) + 1.0);
        ftype q[4];
        q[i+1] = s*0.5f;
        s=0.5f/s;
        q[0] = (m[j][k] - m[k][j])*s;
        q[j+1] = (m[i][j]+m[j][i])*s;
        q[k+1] = (m[i][k]+m[k][i])*s;
        quat[0] = q[0];
        quat[1] = q[1];
        quat[2] = q[2];
        quat[3] = q[3];
    }
}

/*
 *  ========== DtMatrixGetTranslation ==========
 *
 *  SYNOPSIS
 *  Return the x,y,z translation components of the
 *  given matrix. The priority order is assumed to be ---.
 */

int  DtMatrixGetTranslation( ftype *matrix, ftype *xTrans, ftype *yTrans, ftype *zTrans)
{
    DECOMP_MAT dmat;
    if (matrix)
    {
        utlDecompMatrix( matrix, &dmat, "xyz" );
        *xTrans = dmat.translate[0];
        *yTrans = dmat.translate[1];
        *zTrans = dmat.translate[2];
    }
    else
    {
        *xTrans = *yTrans = *zTrans = 0.0;
    }
    return(1);

}  /* DtMatrixGetTranslation */

/*
 *  ========== DtMatrixGetQuaternion ==========
 *
 *  SYNOPSIS
 *  Return the quaternion (scalar, xAxis, yAxis, zAxis)
 *  defining the orientation represented in the given matrix.
 */
int  DtMatrixGetQuaternion(ftype *matrix, ftype *scalar, ftype *xAxis, ftype *yAxis, ftype *zAxis)
{
    DECOMP_MAT dmat;
    if (matrix)
    {
    utlDecompMatrix( matrix, &dmat, "xyz" );
    *scalar = dmat.quaternion[0];
    *xAxis = dmat.quaternion[1];
    *yAxis = dmat.quaternion[2];
    *zAxis = dmat.quaternion[3];
    } 
    else
    {
    *scalar = 1.0; *xAxis = *yAxis = *zAxis = 0.0;
    }
    return(1);
}  /* DtMatrixGetQuaternion */

/*
 *  ========== DtMatrixGetRotation ==========
 *
 *  SYNOPSIS
 *  Return the x,y,z rotation components of the
 *  given matrix. The priority order is assumed to be ---.
 */

void DtMatrixGetRotation(ftype *matrix, ftype *xRotation, ftype *yRotation, ftype *zRotation)
{
    DECOMP_MAT dmat;
    utlDecompMatrix( matrix, &dmat, "xyz" );
    *xRotation = dmat.rotation[0];
    *yRotation = dmat.rotation[1];
    *zRotation = dmat.rotation[2];

}  /* DtMatrixGetRotation */
