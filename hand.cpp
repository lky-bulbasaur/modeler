// The sample model.  You should build a file
// very similar to this for when you make your model.
#include "modelerview.h"
#include "modelerapp.h"
#include "modelerdraw.h"
#include <FL/gl.h>
#include <vector>

#include "ThreadPool.h"
#include "vec.h"
#include "mat.h"
#include "marchingcubesconst.h"
#include "modelerglobals.h"

#include <iostream>

// To make a HandModel, we inherit off of ModelerView
class HandModel : public ModelerView
{
public:
	HandModel(int x, int y, int w, int h, char* label)
		: ModelerView(x, y, w, h, label) {
		verticesList = new vector<Vec3f>();
		marchingCubesMap = NULL;
		MARCHING_CUBES_THRESHOLD = 17;
		FLOOR_SIZE = 20.0;
	}

	~HandModel() {
		delete verticesList;
		if (marchingCubesMap != NULL) delete3DArray(gridNum + 1, marchingCubesMap);
	}

	virtual void draw();

	void addVertex(Vec3f ver);

	void translateVertices(double x, double y, double z, vector<Vec3f>* list);

	void rotateVertices(double angle, bool x, bool y, bool z, vector<Vec3f>* list);

	void clearVerticesList();

	void updateMarchingCubesMap();

	template <typename T>
	void new3DArray(int size, T*** &list) {
		list = new T** [size];
		for (int i = 0; i < size; ++i) {
			list[i] = new T* [size];
			for (int j = 0; j < size; ++j) {
				list[i][j] = new T[size];
			}
		}
	}

	template <typename T>
	void delete3DArray(int size, T*** &list) {
		for (int i = 0; i < size; ++i) {
			for (int j = 0; j < size; ++j) {
				delete [] list[i][j];
			}
			delete [] list[i];
		}
		delete [] list;
	}

private:
	static const int GRID_NUM_HIGH = 120;
	static const int GRID_NUM_MEDIUM = 96;
	static const int GRID_NUM_LOW = 64;
	static const int GRID_NUM_POOR = 480;

	int gridNum = GRID_NUM_MEDIUM;

	double MARCHING_CUBES_THRESHOLD;
	double FLOOR_SIZE;

	double*** marchingCubesMap;
	vector<Vec3f>* verticesList;

	float thumb_tipXrootX_angle = 0;			//max72
	float thumb_tipXrootX_delta = 4;
	float thumb_tipYrootY_angle = 0;			//max36
	float thumb_tipYrootY_delta = 2;
	float index_tipXmidXrootX_angle = 0;		//max63
	float index_tipXmidXrootX_delta = 3.5;
	float rest_tipXmidXrootX_angle = 0;			//max9
	float rest_tipXmidXrootX_delta = 0.5;
};

// We need to make a creator function, mostly because of
// nasty API stuff that we'd rather stay away from.
ModelerView* createHandModel(int x, int y, int w, int h, char* label)
{
	return new HandModel(x, y, w, h, label);
}

// Adds a vertex to the vertices list that defines the model
void HandModel::addVertex(Vec3f ver) {
	verticesList->push_back(ver);
}

// Translate all vertices currently in a given vertices list
void HandModel::translateVertices(double x, double y, double z, vector<Vec3f>* list) {
	for (int i = 0; i < list->size(); ++i) {
		Mat4<double> translateMat(
			1,	0,	0,	x,
			0,	1,	0,	y,
			0,	0,	1,	z,
			0,	0,	0,	1
		);

		Mat4<double> origVec(
			list->at(i)[0],	0,	0,	0,
			list->at(i)[1],	0,	0,	0,
			list->at(i)[2],	0,	0,	0,
			1,				0,	0,	0
		);

		Mat4<double> resultVec = translateMat * origVec;
		list->at(i)[0] = resultVec[0][0];
		list->at(i)[1] = resultVec[1][0];
		list->at(i)[2] = resultVec[2][0];
	}
}

// Rotate all vertices currently in a given vertices list
void HandModel::rotateVertices(double angle, bool x, bool y, bool z, vector<Vec3f>* list) {
	for (int i = 0; i < list->size(); ++i) {
		double theta = angle / 360.0 * 2 * M_PI;

		Mat4<double> rotateMat;
		if (x) {
			rotateMat = Mat4<double>(
				1,	0,			0,				0,
				0,	cos(theta), -sin(theta),	0,
				0,	sin(theta),	cos(theta),		0,
				0,	0,			0,				1
			);
		}

		if (y) {
			rotateMat = Mat4<double>(
				cos(theta),		0, sin(theta),	0,
				0,				1, 0,			0,
				-sin(theta),	0, cos(theta),	0,
				0,				0, 0,			1
			);
		}

		if (z) {
			rotateMat = Mat4<double>(
				cos(theta),	-sin(theta),	0,	0,
				sin(theta),	cos(theta),		0,	0,
				0,			0,				1,	0,
				0,			0,				0,	1
			);
		}

		Mat4<double> origVec(
			list->at(i)[0], 0, 0, 0,
			list->at(i)[1], 0, 0, 0,
			list->at(i)[2], 0, 0, 0,
			1, 0, 0, 0
		);

		Mat4<double> resultVec = rotateMat * origVec;
		list->at(i)[0] = resultVec[0][0];
		list->at(i)[1] = resultVec[1][0];
		list->at(i)[2] = resultVec[2][0];
	}
}

// Clears the vertices list
void HandModel::clearVerticesList() {
	verticesList->clear();
}

// Updates the Marching Cubes map 
void HandModel::updateMarchingCubesMap() {

	// Select number of tests according to quality setting
	int oldGridNum = gridNum;
	switch (ModelerDrawState::Instance()->m_quality) {
	case HIGH:
		gridNum = GRID_NUM_HIGH; break;
	case MEDIUM:
		gridNum = GRID_NUM_MEDIUM; break;
	case LOW:
		gridNum = GRID_NUM_LOW; break;
	case POOR:
		gridNum = GRID_NUM_POOR; break;
	}

	if (marchingCubesMap != NULL) delete3DArray(oldGridNum + 1, marchingCubesMap);
	new3DArray(gridNum + 1, marchingCubesMap);

	double cubeSize = 1.0 / gridNum * FLOOR_SIZE;
	double halfCubeSize = cubeSize / 2.0;
	double offset = FLOOR_SIZE / 2;

	for (int i = 0; i < gridNum + 1; ++i) {
		for (int j = 0; j < gridNum + 1; ++j) {
			for (int k = 0; k < gridNum + 1; ++k) {
				marchingCubesMap[i][j][k] = 0;
			}
		}
	}

	ThreadPool* pool = new ThreadPool(8);

	for (int n = 0; n < verticesList->size(); ++n) {
		pool->enqueue([this, cubeSize, offset, n]() {
			for (int i = 0; i < gridNum + 1; ++i) {
				for (int j = 0; j < gridNum * 3 / 5 + 1; ++j) {
					for (int k = gridNum * 2 / 5; k < gridNum + 1; ++k) {
						double x = i * cubeSize - verticesList->at(n)[0] - offset;
						double y = j * cubeSize - verticesList->at(n)[1];
						double z = k * cubeSize - verticesList->at(n)[2] - offset;
						
						marchingCubesMap[i][j][k] += 1 / (x * x + y * y + z * z);
					}
				}
			}
		});
	}

	delete pool;
}

// We are going to override (is that the right word?) the draw()
// method of ModelerView to draw out HandModel
void HandModel::draw()
{
	double cubeSize = 1.0 / gridNum * FLOOR_SIZE;
	double halfCubeSize = cubeSize / 2.0;
	double offset = FLOOR_SIZE / 2;

	// This call takes care of a lot of the nasty projection 
	// matrix stuff.  Unless you want to fudge directly with the 
	// projection matrix, don't bother with this ...
	ModelerView::draw();


	if (ModelerApplication::Instance()->GetAnimateValue()) {
		thumb_tipXrootX_angle += thumb_tipXrootX_delta;
		thumb_tipYrootY_angle += thumb_tipYrootY_delta;
		index_tipXmidXrootX_angle += index_tipXmidXrootX_delta;
		rest_tipXmidXrootX_angle += rest_tipXmidXrootX_delta;
		//std::cout << "hi" << endl;
		if (thumb_tipYrootY_angle > 35 || thumb_tipYrootY_angle < 0) {
			thumb_tipXrootX_delta *= -1;
			thumb_tipYrootY_delta *= -1;
			index_tipXmidXrootX_delta *= -1;
			rest_tipXmidXrootX_delta *= -1;
		}
	}

	// Dynamic lighting
	GLfloat light0Pos[] = { VAL(LIGHT0_XPOS), VAL(LIGHT0_YPOS), VAL(LIGHT0_ZPOS), 0 };
	glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
	GLfloat light1Pos[] = { VAL(LIGHT1_XPOS), VAL(LIGHT1_YPOS), VAL(LIGHT1_ZPOS), 0 };
	glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);

	// draw the floor
	setAmbientColor(.1f, .1f, .1f);
	setDiffuseColor(COLOR_RED);
	glPushMatrix();
	glTranslated(-5, 0, -5);
	// drawBox(10, 0.01f, 10);	// Uncomment this if you want to see the hand clip through the floor
	glPopMatrix();

	clearVerticesList();

	// Define the hand metaball model with vertices
	// Also handle transformations here
	// Also since I'm dumb, I brute-forced the way non-graphic vectors are transformed
	// This means that for hierarchical modelling, we will be doing it in reversed order compared to the lecture slides lol
	// i.e. Transform the leaf parts first, then work the way up to the root

	// =====================================================================================================================
	//	THUMB
	// =====================================================================================================================
		// Thumb tip
		vector<Vec3f> thumbTip;
			thumbTip.push_back(Vec3f(0, 0, 0));
			thumbTip.push_back(Vec3f(0, 0.5, 0));
			thumbTip.push_back(Vec3f(0, 1, 0));

			// Move self only
			rotateVertices(VAL(THUMB_TIP_XROTATE) + thumb_tipXrootX_angle, 1, 0, 0, &thumbTip);
			rotateVertices(VAL(THUMB_TIP_YROTATE) + thumb_tipYrootY_angle, 0, 1, 0, &thumbTip);
			rotateVertices(VAL(THUMB_TIP_ZROTATE), 0, 0, 1, &thumbTip);
			translateVertices(0, 1.4, 0, &thumbTip);

		// Thumb root
		vector<Vec3f> thumbRoot;
			thumbRoot.push_back(Vec3f(0, 0, 0));
			thumbRoot.push_back(Vec3f(0, 0.5, 0));
			thumbRoot.push_back(Vec3f(0, 1, 0));

			// Also move children
			for (int i = 0; i < thumbTip.size(); ++i)	thumbRoot.push_back(thumbTip.at(i));
			rotateVertices(45, 0, 0, 1, &thumbRoot);
			rotateVertices(VAL(THUMB_ROOT_XROTATE) + thumb_tipXrootX_angle, 1, 0, 0, &thumbRoot);
			rotateVertices(VAL(THUMB_ROOT_YROTATE) + thumb_tipYrootY_angle, 0, 1, 0, &thumbRoot);
			rotateVertices(VAL(THUMB_ROOT_ZROTATE), 0, 0, 1, &thumbRoot);
			translateVertices(-2.5, 4, 0, &thumbRoot);

	// =====================================================================================================================
	//	INDEX FINGER
	// =====================================================================================================================
		// Index tip
		vector<Vec3f> indexTip;
			indexTip.push_back(Vec3f(0, 0, 0));
			indexTip.push_back(Vec3f(0, 0.5, 0));
			indexTip.push_back(Vec3f(0, 1, 0));

			// Move self only
			rotateVertices(VAL(INDEX_TIP_XROTATE) + index_tipXmidXrootX_angle, 1, 0, 0, &indexTip);
			rotateVertices(VAL(INDEX_TIP_YROTATE), 0, 1, 0, &indexTip);
			rotateVertices(VAL(INDEX_TIP_ZROTATE), 0, 0, 1, &indexTip);
			translateVertices(0, 1.4, 0, &indexTip);

		// Index mid
		vector<Vec3f> indexMid;
			indexMid.push_back(Vec3f(0, 0, 0));
			indexMid.push_back(Vec3f(0, 0.5, 0));
			indexMid.push_back(Vec3f(0, 1, 0));

			// Also move children
			for (int i = 0; i < indexTip.size(); ++i)	indexMid.push_back(indexTip.at(i));
			rotateVertices(VAL(INDEX_MID_XROTATE) + index_tipXmidXrootX_angle, 1, 0, 0, &indexMid);
			rotateVertices(VAL(INDEX_MID_YROTATE), 0, 1, 0, &indexMid);
			rotateVertices(VAL(INDEX_MID_ZROTATE), 0, 0, 1, &indexMid);
			translateVertices(0, 1.4, 0, &indexMid);

		// Index root
		vector<Vec3f> indexRoot;
			indexRoot.push_back(Vec3f(0, 0, 0));
			indexRoot.push_back(Vec3f(0, 0.5, 0));
			indexRoot.push_back(Vec3f(0, 1, 0));

			// Also move children
			for (int i = 0; i < indexMid.size(); ++i)	indexRoot.push_back(indexMid.at(i));
			rotateVertices(22.5, 0, 0, 1, &indexRoot);
			rotateVertices(VAL(INDEX_ROOT_XROTATE) + index_tipXmidXrootX_angle, 1, 0, 0, &indexRoot);
			rotateVertices(VAL(INDEX_ROOT_YROTATE), 0, 1, 0, &indexRoot);
			rotateVertices(VAL(INDEX_ROOT_ZROTATE), 0, 0, 1, &indexRoot);
			translateVertices(-1.25, 6, 0, &indexRoot);
	
	// =====================================================================================================================
	//	MIDDLE FINGER
	// =====================================================================================================================
		// Middle tip
		vector<Vec3f> middleTip;
			middleTip.push_back(Vec3f(0, 0, 0));
			middleTip.push_back(Vec3f(0, 0.5, 0));
			middleTip.push_back(Vec3f(0, 1, 0));

			// Move self only
			rotateVertices(VAL(MIDDLE_TIP_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &middleTip);
			rotateVertices(VAL(MIDDLE_TIP_YROTATE), 0, 1, 0, &middleTip);
			rotateVertices(VAL(MIDDLE_TIP_ZROTATE), 0, 0, 1, &middleTip);
			translateVertices(0, 1.4, 0, &middleTip);

		// Middle mid
		vector<Vec3f> middleMid;
			middleMid.push_back(Vec3f(0, 0, 0));
			middleMid.push_back(Vec3f(0, 0.5, 0));
			middleMid.push_back(Vec3f(0, 1, 0));

			// Also move children
			for (int i = 0; i < middleTip.size(); ++i)	middleMid.push_back(middleTip.at(i));
			rotateVertices(VAL(MIDDLE_MID_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &middleMid);
			rotateVertices(VAL(MIDDLE_MID_YROTATE), 0, 1, 0, &middleMid);
			rotateVertices(VAL(MIDDLE_MID_ZROTATE), 0, 0, 1, &middleMid);
			translateVertices(0, 1.8, 0, &middleMid);

		// Middle root
		vector<Vec3f> middleRoot;
			middleRoot.push_back(Vec3f(0, 0, 0));
			middleRoot.push_back(Vec3f(0, 0.5, 0));
			middleRoot.push_back(Vec3f(0, 1, 0));
			middleRoot.push_back(Vec3f(0, 1.5, 0));

			// Also move children
			for (int i = 0; i < middleMid.size(); ++i)	middleRoot.push_back(middleMid.at(i));
			rotateVertices(VAL(MIDDLE_ROOT_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &middleRoot);
			rotateVertices(VAL(MIDDLE_ROOT_YROTATE), 0, 1, 0, &middleRoot);
			rotateVertices(VAL(MIDDLE_ROOT_ZROTATE), 0, 0, 1, &middleRoot);
			translateVertices(0, 6.5, 0, &middleRoot);

	// =====================================================================================================================
	//	RING FINGER
	// =====================================================================================================================
		// Ring tip
		vector<Vec3f> ringTip;
			ringTip.push_back(Vec3f(0, 0, 0));
			ringTip.push_back(Vec3f(0, 0.5, 0));
			ringTip.push_back(Vec3f(0, 1, 0));

			// Move self only
			rotateVertices(VAL(RING_TIP_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &ringTip);
			rotateVertices(VAL(RING_TIP_YROTATE), 0, 1, 0, &ringTip);
			rotateVertices(VAL(RING_TIP_ZROTATE), 0, 0, 1, &ringTip);
			translateVertices(0, 1.4, 0, &ringTip);

		// Ring mid
		vector<Vec3f> ringMid;
			ringMid.push_back(Vec3f(0, 0, 0));
			ringMid.push_back(Vec3f(0, 0.5, 0));
			ringMid.push_back(Vec3f(0, 1, 0));

			// Also move children
			for (int i = 0; i < ringTip.size(); ++i)	ringMid.push_back(ringTip.at(i));
			rotateVertices(VAL(RING_MID_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &ringMid);
			rotateVertices(VAL(RING_MID_YROTATE), 0, 1, 0, &ringMid);
			rotateVertices(VAL(RING_MID_ZROTATE), 0, 0, 1, &ringMid);
			translateVertices(0, 1.4, 0, &ringMid);

		// Ring root
		vector<Vec3f> ringRoot;
			ringRoot.push_back(Vec3f(0, 0, 0));
			ringRoot.push_back(Vec3f(0, 0.5, 0));
			ringRoot.push_back(Vec3f(0, 1, 0));

			// Also move children
			for (int i = 0; i < ringMid.size(); ++i)	ringRoot.push_back(ringMid.at(i));
			rotateVertices(-22.5, 0, 0, 1, &ringRoot);
			rotateVertices(VAL(RING_ROOT_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &ringRoot);
			rotateVertices(VAL(RING_ROOT_YROTATE), 0, 1, 0, &ringRoot);
			rotateVertices(VAL(RING_ROOT_ZROTATE), 0, 0, 1, &ringRoot);
			translateVertices(1.25, 6, 0, &ringRoot);

	// =====================================================================================================================
	//	LITTLE FINGER
	// =====================================================================================================================
		// Little tip
		vector<Vec3f> littleTip;
			littleTip.push_back(Vec3f(0, 0, 0));
			littleTip.push_back(Vec3f(0, 0.5, 0));

			// Move self only
			rotateVertices(VAL(LITTLE_TIP_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &littleTip);
			rotateVertices(VAL(LITTLE_TIP_YROTATE), 0, 1, 0, &littleTip);
			rotateVertices(VAL(LITTLE_TIP_ZROTATE), 0, 0, 1, &littleTip);
			translateVertices(0, 1, 0, &littleTip);

		// Little mid
		vector<Vec3f> littleMid;
			littleMid.push_back(Vec3f(0, 0, 0));
			littleMid.push_back(Vec3f(0, 0.5, 0));

			// Also move children
			for (int i = 0; i < littleTip.size(); ++i)	littleMid.push_back(littleTip.at(i));
			rotateVertices(VAL(LITTLE_MID_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &littleMid);
			rotateVertices(VAL(LITTLE_MID_YROTATE), 0, 1, 0, &littleMid);
			rotateVertices(VAL(LITTLE_MID_ZROTATE), 0, 0, 1, &littleMid);
			translateVertices(0, 1, 0, &littleMid);

		// Little root
		vector<Vec3f> littleRoot;
			littleRoot.push_back(Vec3f(0, 0, 0));
			littleRoot.push_back(Vec3f(0, 0.5, 0));

			// Also move children
			for (int i = 0; i < littleMid.size(); ++i)	littleRoot.push_back(littleMid.at(i));
			rotateVertices(-45, 0, 0, 1, &littleRoot);
			rotateVertices(VAL(LITTLE_ROOT_XROTATE) + rest_tipXmidXrootX_angle, 1, 0, 0, &littleRoot);
			rotateVertices(VAL(LITTLE_ROOT_YROTATE), 0, 1, 0, &littleRoot);
			rotateVertices(VAL(LITTLE_ROOT_ZROTATE), 0, 0, 1, &littleRoot);
			translateVertices(2.5, 5, 0, &littleRoot);

	// =====================================================================================================================
	//	PALM
	// =====================================================================================================================
	vector<Vec3f> palm;
		palm.push_back(Vec3f(-2, 3.5, 0));
		palm.push_back(Vec3f(-2, 4, 0));
		palm.push_back(Vec3f(-1.5, 3, 0));
		palm.push_back(Vec3f(-1.5, 4, 0));
		palm.push_back(Vec3f(-1.25, 5.5, -0.25));
		palm.push_back(Vec3f(-0.75, 2, -0.5));
		palm.push_back(Vec3f(-0.75, 4, -0.25));
		palm.push_back(Vec3f(-1, 2.5, 0));
		palm.push_back(Vec3f(-1, 3, -0.25));
		palm.push_back(Vec3f(-1, 4, -0.25));
		palm.push_back(Vec3f(-0.5, 2, -0.5));
		palm.push_back(Vec3f(-0.5, 3, -0.5));
		palm.push_back(Vec3f(-0.5, 4, -0.25));
		palm.push_back(Vec3f(-0.5, 5.5, -0.1));
		palm.push_back(Vec3f(0, 1.5, -0.25));
		palm.push_back(Vec3f(0, 2, -0.25));
		palm.push_back(Vec3f(0, 3.5, -0.65));
		palm.push_back(Vec3f(0.5, 1.5, -0.25));
		palm.push_back(Vec3f(0.5, 2, -0.45));
		palm.push_back(Vec3f(0.5, 3, -0.65));
		palm.push_back(Vec3f(0.5, 4, -0.45));
		palm.push_back(Vec3f(0.5, 5, -0.25));
		palm.push_back(Vec3f(1, 1.5, -0.25));
		palm.push_back(Vec3f(1, 2, -0.25));
		palm.push_back(Vec3f(1, 3, -0.25));
		palm.push_back(Vec3f(1, 4, -0.25));
		palm.push_back(Vec3f(1, 5.5, -0.1));
		palm.push_back(Vec3f(1.5, 3, -0.25));
		palm.push_back(Vec3f(1.5, 3, -0.1));
		palm.push_back(Vec3f(1.5, 4, -0.1));
		palm.push_back(Vec3f(2, 5, -0.25));
		palm.push_back(Vec3f(2, 3.5, -0.25));
		palm.push_back(Vec3f(2, 4, 0));

		// Also move children
		for (int i = 0; i < thumbRoot.size(); ++i)	palm.push_back(thumbRoot.at(i));
		for (int i = 0; i < indexRoot.size(); ++i)	palm.push_back(indexRoot.at(i));
		for (int i = 0; i < middleRoot.size(); ++i)	palm.push_back(middleRoot.at(i));
		for (int i = 0; i < ringRoot.size(); ++i)	palm.push_back(ringRoot.at(i));
		for (int i = 0; i < littleRoot.size(); ++i)	palm.push_back(littleRoot.at(i));
		

	for (int i = 0; i < palm.size(); ++i) {
		addVertex(palm.at(i));
	}

	updateMarchingCubesMap();

	// Draw metaballs
	setAmbientColor(.1f, .1f, .1f);
	setDiffuseColor(1, 0.6, 0);
	glPushMatrix();
	glTranslated(VAL(XPOS), VAL(YPOS), VAL(ZPOS));
	glRotated(VAL(XROTATE), 1, 0, 0);
	glRotated(VAL(YROTATE), 0, 1, 0);
	glRotated(VAL(ZROTATE), 0, 0, 1);
	for (int i = 0; i < gridNum; ++i) {
		for (int j = 0; j < gridNum * 3 / 5; ++j) {
			for (int k = gridNum * 2 / 5; k < gridNum; ++k) {
				int index = 0;	// 00000000, each bit representing the value of a corner of the current cube
				double x = i * cubeSize - offset;
				double y = j * cubeSize;
				double z = k * cubeSize - offset;

				// Perform bitwise-OR to manipulate the value of index, for fitting into EDGE_TABLE later
				if (marchingCubesMap[i][j][k] >= MARCHING_CUBES_THRESHOLD)				index |= 1;		// v0
				if (marchingCubesMap[i + 1][j][k] >= MARCHING_CUBES_THRESHOLD)			index |= 2;		// v1
				if (marchingCubesMap[i + 1][j][k + 1] >= MARCHING_CUBES_THRESHOLD)		index |= 4;		// v2
				if (marchingCubesMap[i][j][k + 1] >= MARCHING_CUBES_THRESHOLD)			index |= 8;		// v3
				if (marchingCubesMap[i][j + 1][k] >= MARCHING_CUBES_THRESHOLD)			index |= 16;	// v4
				if (marchingCubesMap[i + 1][j + 1][k] >= MARCHING_CUBES_THRESHOLD)		index |= 32;	// v5
				if (marchingCubesMap[i + 1][j + 1][k + 1] >= MARCHING_CUBES_THRESHOLD)	index |= 64;	// v6
				if (marchingCubesMap[i][j + 1][k + 1] >= MARCHING_CUBES_THRESHOLD)		index |= 128;	// v7	

				if (index == 0) continue;

				for (int n = 0; n < 15; n += 3) {
					bool validTriangle = true;
					Vec3f vertices[3];
					for (int m = 0; m < 3; ++m) {
						int curEdge = TRI_TABLE[index][n + m];
						switch (curEdge) {
						case 0:
							vertices[m] = Vec3f(x + halfCubeSize, y, z); break;
						case 1:
							vertices[m] = Vec3f(x + cubeSize, y, z + halfCubeSize); break;
						case 2:
							vertices[m] = Vec3f(x + halfCubeSize, y, z + cubeSize); break;
						case 3:
							vertices[m] = Vec3f(x, y, z + halfCubeSize); break;
						case 4:
							vertices[m] = Vec3f(x + halfCubeSize, y + cubeSize, z); break;
						case 5:
							vertices[m] = Vec3f(x + cubeSize, y + cubeSize, z + halfCubeSize); break;
						case 6:
							vertices[m] = Vec3f(x + halfCubeSize, y + cubeSize, z + cubeSize); break;
						case 7:
							vertices[m] = Vec3f(x, y + cubeSize, z + halfCubeSize); break;
						case 8:
							vertices[m] = Vec3f(x, y + halfCubeSize, z); break;
						case 9:
							vertices[m] = Vec3f(x + cubeSize, y + halfCubeSize, z); break;
						case 10:
							vertices[m] = Vec3f(x + cubeSize, y + halfCubeSize, z + cubeSize); break;
						case 11:
							vertices[m] = Vec3f(x, y + halfCubeSize, z + cubeSize); break;
						default:
							validTriangle = false; break;
						}
					}

					if (validTriangle) {
						drawTriangle(vertices[0][0], vertices[0][1], vertices[0][2],
							vertices[1][0], vertices[1][1], vertices[1][2],
							vertices[2][0], vertices[2][1], vertices[2][2]);
					}
				}
			}
		}
	}
	glPopMatrix();

}

// Comment all other main() and uncomment this if you want the modeler to load this

int main()
{
	// Initialize the controls
	// Constructor is ModelerControl(name, minimumvalue, maximumvalue, 
	// stepsize, defaultvalue)
	ModelerControl controls[NUMCONTROLS];
	controls[LIGHT0_XPOS] = ModelerControl("Light 0 X Position", -20, 20, 0.1f, 4);
	controls[LIGHT0_YPOS] = ModelerControl("Light 0 Y Position", -20, 20, 0.1f, 2);
	controls[LIGHT0_ZPOS] = ModelerControl("Light 0 Z Position", -20, 20, 0.1f, -4);
	controls[LIGHT1_XPOS] = ModelerControl("Light 1 X Position", -20, 20, 0.1f, -2);
	controls[LIGHT1_YPOS] = ModelerControl("Light 1 Y Position", -20, 20, 0.1f, 1);
	controls[LIGHT1_ZPOS] = ModelerControl("Light 1 Z Position", -20, 20, 0.1f, 5);
	controls[XPOS] = ModelerControl("Hand X Position", -5, 5, 0.1f, 0);
	controls[YPOS] = ModelerControl("Hand Y Position", 0, 5, 0.1f, 0);
	controls[ZPOS] = ModelerControl("Hand Z Position", -5, 5, 0.1f, 0);
	controls[XROTATE] = ModelerControl("Hand X Rotation", -90, 90, 1, 0);
	controls[YROTATE] = ModelerControl("Hand Y Rotation", -90, 90, 1, 0);
	controls[ZROTATE] = ModelerControl("Hand Z Rotation", -90, 90, 1, 0);
	controls[THUMB_TIP_XROTATE] = ModelerControl("Thumb Tip X Rotation", 0, 90, 1, 0);
	controls[THUMB_TIP_YROTATE] = ModelerControl("Thumb Tip Y Rotation", -90, 90, 1, 0);
	controls[THUMB_TIP_ZROTATE] = ModelerControl("Thumb Tip Z Rotation", -90, 90, 1, 0);
	controls[THUMB_ROOT_XROTATE] = ModelerControl("Thumb Root X Rotation", -90, 90, 1, 0);
	controls[THUMB_ROOT_YROTATE] = ModelerControl("Thumb Root Y Rotation", -90, 90, 1, 0);
	controls[THUMB_ROOT_ZROTATE] = ModelerControl("Thumb Root Z Rotation", -90, 90, 1, 0);
	controls[INDEX_TIP_XROTATE] = ModelerControl("Index Finger Tip X Rotation", 0, 90, 1, 0);
	controls[INDEX_TIP_YROTATE] = ModelerControl("Index Finger Tip Y Rotation", -90, 90, 1, 0);
	controls[INDEX_TIP_ZROTATE] = ModelerControl("Index Finger Tip Z Rotation", -90, 90, 1, 0);
	controls[INDEX_MID_XROTATE] = ModelerControl("Index Finger Mid X Rotation", 0, 90, 1, 0);
	controls[INDEX_MID_YROTATE] = ModelerControl("Index Finger Mid Y Rotation", -90, 90, 1, 0);
	controls[INDEX_MID_ZROTATE] = ModelerControl("Index Finger Mid Z Rotation", -90, 90, 1, 0);
	controls[INDEX_ROOT_XROTATE] = ModelerControl("Index Finger Root X Rotation", -90, 90, 1, 0);
	controls[INDEX_ROOT_YROTATE] = ModelerControl("Index Finger Root Y Rotation", -90, 90, 1, 0);
	controls[INDEX_ROOT_ZROTATE] = ModelerControl("Index Finger Root Z Rotation", -90, 90, 1, 0);
	controls[MIDDLE_TIP_XROTATE] = ModelerControl("Middle Finger Tip X Rotation", 0, 90, 1, 0);
	controls[MIDDLE_TIP_YROTATE] = ModelerControl("Middle Finger Tip Y Rotation", -90, 90, 1, 0);
	controls[MIDDLE_TIP_ZROTATE] = ModelerControl("Middle Finger Tip Z Rotation", -90, 90, 1, 0);
	controls[MIDDLE_MID_XROTATE] = ModelerControl("Middle Finger Mid X Rotation", 0, 90, 1, 0);
	controls[MIDDLE_MID_YROTATE] = ModelerControl("Middle Finger Mid Y Rotation", -90, 90, 1, 0);
	controls[MIDDLE_MID_ZROTATE] = ModelerControl("Middle Finger Mid Z Rotation", -90, 90, 1, 0);
	controls[MIDDLE_ROOT_XROTATE] = ModelerControl("Middle Finger Root X Rotation", -90, 90, 1, 0);
	controls[MIDDLE_ROOT_YROTATE] = ModelerControl("Middle Finger Root Y Rotation", -90, 90, 1, 0);
	controls[MIDDLE_ROOT_ZROTATE] = ModelerControl("Middle Finger Root Z Rotation", -90, 90, 1, 0);
	controls[RING_TIP_XROTATE] = ModelerControl("Ring Finger Tip X Rotation", 0, 90, 1, 0);
	controls[RING_TIP_YROTATE] = ModelerControl("Ring Finger Tip Y Rotation", -90, 90, 1, 0);
	controls[RING_TIP_ZROTATE] = ModelerControl("Ring Finger Tip Z Rotation", -90, 90, 1, 0);
	controls[RING_MID_XROTATE] = ModelerControl("Ring Finger Mid X Rotation", 0, 90, 1, 0);
	controls[RING_MID_YROTATE] = ModelerControl("Ring Finger Mid Y Rotation", -90, 90, 1, 0);
	controls[RING_MID_ZROTATE] = ModelerControl("Ring Finger Mid Z Rotation", -90, 90, 1, 0);
	controls[RING_ROOT_XROTATE] = ModelerControl("Ring Finger Root X Rotation", -90, 90, 1, 0);
	controls[RING_ROOT_YROTATE] = ModelerControl("Ring Finger Root Y Rotation", -90, 90, 1, 0);
	controls[RING_ROOT_ZROTATE] = ModelerControl("Ring Finger Root Z Rotation", -90, 90, 1, 0);
	controls[LITTLE_TIP_XROTATE] = ModelerControl("Little Finger Tip X Rotation", 0, 90, 1, 0);
	controls[LITTLE_TIP_YROTATE] = ModelerControl("Little Finger Tip Y Rotation", -90, 90, 1, 0);
	controls[LITTLE_TIP_ZROTATE] = ModelerControl("Little Finger Tip Z Rotation", -90, 90, 1, 0);
	controls[LITTLE_MID_XROTATE] = ModelerControl("Little Finger Mid X Rotation", 0, 90, 1, 0);
	controls[LITTLE_MID_YROTATE] = ModelerControl("Little Finger Mid Y Rotation", -90, 90, 1, 0);
	controls[LITTLE_MID_ZROTATE] = ModelerControl("Little Finger Mid Z Rotation", -90, 90, 1, 0);
	controls[LITTLE_ROOT_XROTATE] = ModelerControl("Little Finger Root X Rotation", -90, 90, 1, 0);
	controls[LITTLE_ROOT_YROTATE] = ModelerControl("Little Finger Root Y Rotation", -90, 90, 1, 0);
	controls[LITTLE_ROOT_ZROTATE] = ModelerControl("Little Finger Root Z Rotation", -90, 90, 1, 0);

	ModelerApplication::Instance()->Init(&createHandModel, controls, NUMCONTROLS);
	return ModelerApplication::Instance()->Run();
}