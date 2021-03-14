#include <windows.h>
#include <Fl/gl.h>
#include <gl/glu.h>

#include "camera.h"
#include "vec.h"


#pragma warning(push)
#pragma warning(disable : 4244)

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502
#endif 

const float kMouseRotationSensitivity		= 1.0f/90.0f;
const float kMouseTranslationXSensitivity	= 0.03f;
const float kMouseTranslationYSensitivity	= 0.03f;
const float kMouseZoomSensitivity			= 0.08f;
const float kMouseTwistSensitivity			= 0.03f;

void MakeDiagonal(Mat4f &m, float k)
{
	register int i,j;

	for (i=0; i<4; i++)
		for (j=0; j<4; j++)
			m[i][j] = (i==j) ? k : 0.0f;
}

void MakeHScale(Mat4f &m, const Vec3f &s)	
{
	MakeDiagonal(m,1.0f);
	m[0][0] = s[0]; m[1][1] = s[1];	m[2][2] = s[2];
}

void MakeHTrans(Mat4f &m, const Vec3f &s)
{
	MakeDiagonal(m,1.0f);
	m[0][3] = s[0]; m[1][3] = s[1]; m[2][3] = s[2];
}

void MakeHRotX(Mat4f &m, float theta)
{
	MakeDiagonal(m,1.0f);
	float cosTheta = cos(theta);
	float sinTheta = sin(theta);
	m[1][1] = cosTheta;
	m[1][2] = -sinTheta;
	m[2][1] = sinTheta;
	m[2][2] = cosTheta;
}

void MakeHRotY(Mat4f &m, float theta)
{
	MakeDiagonal(m,1.0f);
	float cosTheta = cos(theta);
	float sinTheta = sin(theta);
	m[0][0] = cosTheta;
	m[2][0] = -sinTheta;
	m[0][2] = sinTheta;
	m[2][2] = cosTheta;
}

void MakeHRotZ(Mat4f &m, float theta)
{
	MakeDiagonal(m,1.0f);
	float cosTheta = cos(theta);
	float sinTheta = sin(theta);
	m[0][0] = cosTheta;
	m[0][1] = -sinTheta;
	m[1][0] = sinTheta;
	m[1][1] = cosTheta;
}


void Camera::calculateViewingTransformParameters() 
{
	Mat4f dollyXform;
	Mat4f azimXform;
	Mat4f elevXform;
	Mat4f twistXform;
	Mat4f originXform;

	Vec3f upVector;

	MakeHTrans(dollyXform, Vec3f(0,0,mDolly));
	MakeHRotY(azimXform, mAzimuth);
	MakeHRotX(elevXform, mElevation);
	MakeDiagonal(twistXform, 1.0f);
	MakeHTrans(originXform, mLookAt);
	
	mPosition = Vec3f(0,0,0);
	// grouped for (mat4 * vec3) ops instead of (mat4 * mat4) ops
	mPosition = originXform * (azimXform * (elevXform * (dollyXform * mPosition)));

	if (fmod((double)mElevation, 2.0 * M_PI) < 3 * M_PI / 2 && fmod((double)mElevation, 2.0 * M_PI) > M_PI / 2)
		mUpVector = Vec3f(sin(mTwist), -cos(mTwist), 0);
	else
		mUpVector = Vec3f(sin(mTwist), cos(mTwist), 0);

	mDirtyTransform = false;
}

Camera::Camera() 
{
	mElevation = mAzimuth = mTwist = 0.0f;
	mDolly = -20.0f;
	mElevation = 0.2f;
	mAzimuth = (float)M_PI;

	mLookAt = Vec3f( 0, 0, 0 );
	mCurrentMouseAction = kActionNone;

	calculateViewingTransformParameters();
}

void Camera::clickMouse( MouseAction_t action, int x, int y )
{
	mCurrentMouseAction = action;
	mLastMousePosition[0] = x;
	mLastMousePosition[1] = y;
}

void Camera::dragMouse( int x, int y )
{
	Vec3f mouseDelta   = Vec3f(x,y,0.0f) - mLastMousePosition;
	mLastMousePosition = Vec3f(x,y,0.0f);

	switch(mCurrentMouseAction)
	{
	case kActionTranslate:
		{
			calculateViewingTransformParameters();

			double xTrack =  -mouseDelta[0] * kMouseTranslationXSensitivity;
			double yTrack =  mouseDelta[1] * kMouseTranslationYSensitivity;

			Vec3f transXAxis = mUpVector ^ (mPosition - mLookAt);
			transXAxis /= sqrt((transXAxis*transXAxis));
			Vec3f transYAxis = (mPosition - mLookAt) ^ transXAxis;
			transYAxis /= sqrt((transYAxis*transYAxis));

			setLookAt(getLookAt() + transXAxis*xTrack + transYAxis*yTrack);
			
			break;
		}
	case kActionRotate:
		{
			float dAzimuth		=   -mouseDelta[0] * kMouseRotationSensitivity;
			float dElevation	=   mouseDelta[1] * kMouseRotationSensitivity;
			
			setAzimuth(getAzimuth() + dAzimuth);
			setElevation(getElevation() + dElevation);
			
			break;
		}
	case kActionZoom:
		{
			float dDolly = -mouseDelta[1] * kMouseZoomSensitivity;
			setDolly(getDolly() + dDolly);
			break;
		}
	case kActionTwist:
		{
			float dTwist = -mouseDelta[0] * kMouseTwistSensitivity;
			setTwist(getTwist() + dTwist);
			break;
		}
	default:
		break;
	}

}

void Camera::releaseMouse( int x, int y )
{
	mCurrentMouseAction = kActionNone;
}


void Camera::applyViewingTransform() {
	if( mDirtyTransform )
		calculateViewingTransformParameters();

	
	// Place the camera at mPosition, aim the camera at
	// mLookAt, and twist the camera such that mUpVector is up
	//gluLookAt(	mPosition[0], mPosition[1], mPosition[2],
	//			mLookAt[0],   mLookAt[1],   mLookAt[2],
	//			mUpVector[0], mUpVector[1], mUpVector[2]);

	lookAt(mPosition, mLookAt, mUpVector);	
}

void Camera::lookAt(Vec3f eye, Vec3f at, Vec3f up) {
	// camForward vector specifies "where the x-axis of the camera should point at"
	Vec3f camForward = eye - at;
	camForward.normalize();

	// camLeft vector specifies "where the z-axis of the camera should point at"
	Vec3f camLeft = up ^ camForward;
	camLeft.normalize();

	// camUp vector specifies "where the y-axis of the camera should point at"
	Vec3f camUp = camForward ^ camLeft;
	camUp.normalize();


	// Define the modelview matrix (from http://songho.ca/opengl/gl_camera.html)
	// Which is the result of rotation matrix * translation matrix
	// Indexed as follows:
	//	0	4	8	12		This row handles rotation and translation along X-axis
	//	1	5	9	13		This row handles rotation and translation along Y-axis
	//	2	6	10	14		This row handles rotation and translation along Z-axis
	//	3	7	11	15		This row exists only for the sake of homogeneous coordinates

	// Note: For normal rotation, the 1st, 2nd and 3rd COLUMNS should be responsible for X-, Y- or Z-axes rotation
	// However, since OpenGL has a fixed camera and needs to transform the ENTIRE WORLD to achieve different viewplane,
	// All modelview transformations are done INVERSELY
	// This is why for the translation, we translate to -eye instead of eye
	// Likewise, this is why we TRANSPOSE (inverse for orthogonal matrix in this case) the orignal rotation matrix before use
	double matrix[16];
		matrix[0]	= camLeft[0];
		matrix[1]	= camUp[0];
		matrix[2]	= camForward[0];
		matrix[3]	= 0;
		matrix[4]	= camLeft[1];
		matrix[5]	= camUp[1];
		matrix[6]	= camForward[1];
		matrix[7]	= 0;
		matrix[8]	= camLeft[2];
		matrix[9]	= camUp[2];
		matrix[10]	= camForward[2];
		matrix[11]	= 0;
		matrix[12]	= - camLeft[0] * eye[0] - camLeft[1] * eye[1] - camLeft[2] * eye[2];
		matrix[13]	= - camUp[0] * eye[0] - camUp[1] * eye[1] - camUp[2] * eye[2];
		matrix[14]	= - camForward[0] * eye[0] - camForward[1] * eye[1] - camForward[2] * eye[2];
		matrix[15]	= 1;

	// Transform the current worldview according to the specified matrix
	glMultMatrixd(matrix);
}

#pragma warning(pop)