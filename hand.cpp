// The sample model.  You should build a file
// very similar to this for when you make your model.
#include "modelerview.h"
#include "modelerapp.h"
#include "modelerdraw.h"
#include <FL/gl.h>
#include <vector>

#include "vec.h"
#include "marchingcubesconst.h"
#include "modelerglobals.h"

// To make a HandModel, we inherit off of ModelerView
class HandModel : public ModelerView
{
public:
	HandModel(int x, int y, int w, int h, char* label)
		: ModelerView(x, y, w, h, label) {
		verticesList = new vector<Vec3f>();
		MARCHING_CUBES_THRESHOLD = 0.28;
	}

	~HandModel() {
		delete verticesList;
	}

	virtual void draw();

	void addVertex(Vec3f ver);

	void clearVerticesList();

	void updateMarchingCubesMap();

private:
	static const int GRID_NUM = 50;

	double MARCHING_CUBES_THRESHOLD;

	bool marchingCubesMap[GRID_NUM + 1][GRID_NUM + 1][GRID_NUM + 1];
	vector<Vec3f>* verticesList;
};

// We need to make a creator function, mostly because of
// nasty API stuff that we'd rather stay away from.
ModelerView* createHandModel(int x, int y, int w, int h, char* label)
{
	return new HandModel(x, y, w, h, label);
}

// Adds a vertext to the vertices list that defines the model
void HandModel::addVertex(Vec3f ver) {
	verticesList->push_back(ver);
}

// Clears the vertices list
void HandModel::clearVerticesList() {
	verticesList->clear();
}

// Updates the Marching Cubes map 
void HandModel::updateMarchingCubesMap() {
	// Select number of tests according to quality setting
	int steps;
	switch (ModelerDrawState::Instance()->m_quality) {
	case HIGH:
		steps = 1; break;
	case MEDIUM:
		steps = 1; break;
	case LOW:
		steps = 1; break;
	case POOR:
		steps = 1; break;
	}

	double cubeSize = 1.0 / GRID_NUM * 10 * steps;
	double halfCubeSize = cubeSize / 2.0;

	for (int i = 0; i < GRID_NUM + 1; i += steps) {
		for (int j = 0; j < GRID_NUM + 1; j += steps) {
			for (int k = 0; k < GRID_NUM + 1; k += steps) {
				// Surface level is the summation of all 1/r^2, where r is the current point's distance from a vertex in the vertices list
				double surfaceLevel = 0;

				for (int n = 0; n < verticesList->size(); ++n) {
					double x = i * cubeSize - verticesList->at(n)[0];
					double y = j * cubeSize - verticesList->at(n)[1];
					double z = k * cubeSize - verticesList->at(n)[2];
					surfaceLevel += (1 / (x * x + y * y + z * z));
				}

				if (surfaceLevel >= MARCHING_CUBES_THRESHOLD) {
					marchingCubesMap[i][j][k] = true;
				} else {
					marchingCubesMap[i][j][k] = false;
				}
			}
		}
	}
}

// We are going to override (is that the right word?) the draw()
// method of ModelerView to draw out HandModel
void HandModel::draw()
{
	// Select number of tests according to quality setting
	int steps;
	switch (ModelerDrawState::Instance()->m_quality) {
	case HIGH:
		steps = 1; break;
	case MEDIUM:
		steps = 1; break;
	case LOW:
		steps = 1; break;
	case POOR:
		steps = 1; break;
	}

	double cubeSize = 1.0 / GRID_NUM * 10 * steps;
	double halfCubeSize = cubeSize / 2.0;

	// This call takes care of a lot of the nasty projection 
	// matrix stuff.  Unless you want to fudge directly with the 
	// projection matrix, don't bother with this ...
	ModelerView::draw();

	// draw the floor
	setAmbientColor(.1f, .1f, .1f);
	setDiffuseColor(COLOR_RED);
	glPushMatrix();
	glTranslated(-5, 0, -5);
	drawBox(10, 0.01f, 10);
	glPopMatrix();

	clearVerticesList();
	addVertex(Vec3f(7, 7, 3));
	addVertex(Vec3f(3, 3, 3));
	addVertex(Vec3f(3, 7, 7));

	updateMarchingCubesMap();

	// Draw metaballs
	setDiffuseColor(COLOR_GREEN);
	for (int i = 0; i < GRID_NUM; i += steps) {
		for (int j = 0; j < GRID_NUM; j += steps) {
			for (int k = 0; k < GRID_NUM; k += steps) {
				int index = 0;	// 00000000, each bit representing the value of a corner of the current cube
				double x = i * cubeSize;
				double y = j * cubeSize;
				double z = k * cubeSize;

				// Perform bitwise-OR to manipulate the value of index, for fitting into EDGE_TABLE later
				if (marchingCubesMap[i][j][k])				index |= 1;		// v0
				if (marchingCubesMap[i + 1][j][k])			index |= 2;		// v1
				if (marchingCubesMap[i + 1][j][k + 1])		index |= 4;		// v2
				if (marchingCubesMap[i][j][k + 1])			index |= 8;		// v3
				if (marchingCubesMap[i][j + 1][k])			index |= 16;	// v4
				if (marchingCubesMap[i + 1][j + 1][k])		index |= 32;	// v5
				if (marchingCubesMap[i + 1][j + 1][k + 1])	index |= 64;	// v6
				if (marchingCubesMap[i][j + 1][k + 1])		index |= 128;	// v7	

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
							validTriangle = false;
							break;
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

}

// 

// Comment all other main() and uncomment this if you want the modeler to load this

int main()
{
	// Initialize the controls
	// Constructor is ModelerControl(name, minimumvalue, maximumvalue, 
	// stepsize, defaultvalue)
	ModelerControl controls[NUMCONTROLS];
	controls[XPOS] = ModelerControl("X Position", -5, 5, 0.1f, 0);
	controls[YPOS] = ModelerControl("Y Position", 0, 5, 0.1f, 0);
	controls[ZPOS] = ModelerControl("Z Position", -5, 5, 0.1f, 0);
	controls[HEIGHT] = ModelerControl("Height", 1, 2.5, 0.1f, 1);
	controls[ROTATE] = ModelerControl("Rotate", -135, 135, 1, 0);

	ModelerApplication::Instance()->Init(&createHandModel, controls, NUMCONTROLS);
	return ModelerApplication::Instance()->Run();
}
