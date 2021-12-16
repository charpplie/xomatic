
#pragma once

#define FORWARD_MOTION

//#define ROUNDING_VAL	0.5
#define ROUNDING_VAL	0.0
//#ifdef FORWARD_MOTION
//#define	KEY_FRAME	0
//#else
#define	KEY_FRAME	0
//#endif

class CPhotoBump10View;
class CVideo;
class CUtils;
class CPhotoImage;
class CPhotoFrame;
class CPVertex;
class CPQuadtree;
class CtTriangle;
typedef struct _IplImage IplImage;

typedef std::vector<CPhotoFrame *> lstFrames;
typedef lstFrames::iterator		lstFramesIt;

typedef std::vector<CPVertex *> lstPhotoVertices;
typedef lstPhotoVertices::iterator	lstPhotoVerticesIt;
