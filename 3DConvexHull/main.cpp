#include <math.h>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include "Camera.hpp"
#include "Controller.hpp"
#include "EnvInc.hpp"
#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#include <windows.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "glew32.lib")
#endif


#include "../common/EsgiShader.h"
#include "../common/mat4.h"


// format des vertices : X, Y, Z, ?, ?, ?, ?, ? = 8 floats
//#include "../data/DragonData.h"

struct EdgeKob;
struct FaceKob;
void MenuFunction(int i);

struct PointKob
{
public:
	float x;
	float y;
	float z;

	std::vector<EdgeKob*> adjacentEdge;
	std::vector<FaceKob*> adjacentFace;

	PointKob() {};

	PointKob(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	PointKob operator +(PointKob* p)
	{
		return PointKob(this->x + p->x, this->y + p->y, this->z + p->z);
	}

	PointKob operator /(int i)
	{
		return PointKob(this->x / i, this->y / i, this->z / i);;
	}

	PointKob operator /(float i)
	{
		return PointKob(this->x / i, this->y / i, this->z / i);;
	}

	PointKob operator *(float i)
	{
		return PointKob(this->x * i, this->y * i, this->z * i);
	}
};

struct EdgeKob
{
public:
	std::vector<PointKob*> points;
	std::vector<FaceKob*> adjacentFace;

	EdgeKob(PointKob* p1, PointKob* p2)
	{
		points.push_back(p1);
		points.push_back(p2);
	}
};

struct FaceKob
{
public:
	std::vector<EdgeKob*> edges;
	PointKob* barycentre;
	Colore color;

	FaceKob(EdgeKob* e1, EdgeKob* e2, EdgeKob* e3)
	{
		edges.push_back(e1);
		edges.push_back(e2);
		edges.push_back(e3);
	}
	FaceKob()
	{
	}

};





int sizetab, sizeind;
enum State {INIT, KOBBELT};

int menu_Main;
//--------------------------------------------------------------------------------------------------------------------------------------------------------
EsgiShader g_BasicShader;

std::vector<Point> p3D, tmpVectorPoints;           // Tous les points en 3D
std::vector<PointKob*> kobbeltPoints;
float* tabPoints, *tmpPoints;         //Tous les points en 3D
GLushort* createInd(int);
GLushort* indi,*indTmp;			// Tab indice


GLuint VAO;
GLuint VBO0;	// identifiant du Vertex Buffer Object 0
GLuint VBO1;	// identifiant du Vertex Buffer Object 1
GLuint IBO,IBO1;	// identifiant du Index Buffer Object
GLuint TexObj; // identifiant du Texture Object


float colore[4];

std::vector<Face*> *tmpFace = new std::vector<Face*>();
std::vector<Colore> col;

State state = INIT;

std::vector<PointKob*> perturbatedPoints;
std::vector<EdgeKob*> newEdges;


float * structToTabColor(std::vector<Point> newPoints, std::vector<Colore> c)
{
	float* tabP = new float[newPoints.size() * 9];
	int j = 0;
	for (int i = 0; i < newPoints.size() * 9; i += 9)
	{
		tabP[i] = newPoints[j].x;
		tabP[i + 1] = newPoints[j].y;
		tabP[i + 2] = newPoints[j].z;

		tabP[i + 3] = newPoints[j].n1;
		tabP[i + 4] = newPoints[j].n2;
		tabP[i + 5] = newPoints[j].n3;

		if (c[j] == Colore(purple))
		{
			tabP[i + 6] = RandomFloat(0,1);
			tabP[i + 7] = RandomFloat(0, 1);
			tabP[i + 8] = RandomFloat(0, 1);
		}
		if (c[j] == Colore(red))
		{
			tabP[i + 6] = RandomFloat(0, 1);
			tabP[i + 7] = RandomFloat(0, 1);
			tabP[i + 8] = RandomFloat(0, 1);
		}
		if (c[j] == Colore(blue))
		{
			tabP[i + 6] = RandomFloat(0, 1);
			tabP[i + 7] = RandomFloat(0, 1);
			tabP[i + 8] = RandomFloat(0, 1);
		}
		j++;
	}

	return tabP;
}

float OrientedAngle(EdgeKob* e1, EdgeKob* e2, PointKob* p1)
{
	PointKob* pk1;
	PointKob* pk2;


	if (e1->points[0] == p1)
	{
		pk1 = e1->points[1];
	}
	else
	{
		pk1 = e1->points[0];
	}

	if (e2->points[0] == p1)
	{
		pk2 = e2->points[1];
	}
	else
	{
		pk2 = e2->points[0];
	}

	float angle = atan2(pk2->z - p1->z, pk2->x - p1->x) - atan2(pk1->z - p1->z, pk1->x - p1->x);

	if (angle < 0)
		angle += 2 * M_PI;

	return angle;
}

EdgeKob* foundEdge(EdgeKob* e1, EdgeKob* e2)
{
	PointKob* p, *pk1 = nullptr, *pk2 = nullptr;

	if (e1->points[0] == e2->points[0])
	{
		p = e1->points[0];
		pk1 = e1->points[1];
		pk2 = e2->points[1];
	}
	else if (e1->points[0] == e2->points[1])
	{
		p = e1->points[0];
		pk1 = e1->points[1];
		pk2 = e2->points[0];
	}
	else if (e1->points[1] == e2->points[0])
	{
		p = e1->points[1];
		pk1 = e1->points[0];
		pk2 = e2->points[1];
	}
	else if (e1->points[1] == e2->points[1])
	{
		p = e1->points[1];
		pk1 = e1->points[0];
		pk2 = e2->points[0];
	}

	for (int i = 0; i < newEdges.size(); ++i)
	{
		if ((pk1 == newEdges[i]->points[0] && pk2 == newEdges[i]->points[1]) ||
			(pk1 == newEdges[i]->points[1] && pk2 == newEdges[i]->points[0]))
		{
			return newEdges[i];
		}
	}

	return nullptr;
}

bool edgeAlreadyExist(EdgeKob* e1, std::vector<EdgeKob*> vec)
{
	for (int i = 0; i < vec.size(); ++i)
	{
		if (vec[i] == e1)
			return true;
	}

	return false;
}

void findOrientedAdjacentEdges(PointKob* p)
{
	std::map<EdgeKob*, float> oriented_angles;

	EdgeKob* aref = p->adjacentEdge.at(0);

	for (int i = 1; i < p->adjacentEdge.size(); ++i)
	{
		oriented_angles.insert(std::pair<EdgeKob*, float>(p->adjacentEdge.at(i), OrientedAngle(aref, p->adjacentEdge.at(i), p)));
	}

	typedef std::function<bool(std::pair<EdgeKob*, float>, std::pair<EdgeKob*, float>)> Comparator;

	Comparator compFunctor = [](std::pair<EdgeKob*, float> elem1, std::pair<EdgeKob*, float> elem2)
	{
		return elem1.second < elem2.second;
	};

	std::set<std::pair<EdgeKob*, float>, Comparator> setOfAngles(oriented_angles.begin(), oriented_angles.end(), compFunctor);

	p->adjacentEdge.clear();
	p->adjacentEdge.push_back(aref);

	for (std::pair<EdgeKob*, float> element : setOfAngles)
	{
		p->adjacentEdge.push_back(element.first);
	}
}

std::vector<PointKob*> letsPertubate(std::vector<PointKob*> points)
{
	std::vector<PointKob*> pertubatedPoints;

	for (auto it = points.begin(); it != points.end(); ++it)
	{
		float alpha = 1 / 9 * (4 - 2 * cos((2 * M_PI) / (*it)->adjacentEdge.size()));

		PointKob sumAdj(0.f, 0.f, 0.f);
		for (auto itAdj = (*it)->adjacentEdge.begin(); itAdj != (*it)->adjacentEdge.end(); itAdj++)
		{
			if ((*itAdj)->points[0] != (*it))
			{
				sumAdj = sumAdj + (*itAdj)->points[0];
			}
			else
			{
				sumAdj = sumAdj + (*itAdj)->points[1];
			}
		}

		PointKob perturbatePoint;
		perturbatePoint.x = (*(*it) * (1 - alpha)).x + sumAdj.x * (alpha / (float)((*it)->adjacentEdge.size()));
		perturbatePoint.y = (*(*it) * (1 - alpha)).y + sumAdj.y * (alpha / (float)((*it)->adjacentEdge.size()));
		perturbatePoint.z = (*(*it) * (1 - alpha)).z + sumAdj.z * (alpha / (float)((*it)->adjacentEdge.size()));

		pertubatedPoints.push_back(&perturbatePoint);
	}

	return pertubatedPoints;
}

void letsGoKobbelt(std::vector<FaceKob*> faces, std::vector<PointKob*>& points, std::vector<EdgeKob*>& edges)
{
	// Calcul barycentre de chaque face
	for (auto it = faces.begin(); it != faces.end(); ++it)
	{
		PointKob* p1 = (*it)->edges[0]->points[0];
		PointKob* p2 = (*it)->edges[0]->points[1];
		PointKob* p3 = (*it)->edges[1]->points[1];

		PointKob barycentre;

		barycentre.x = ((p1->x + p2->x) + p3->x) / 3;
		barycentre.y = ((p1->y + p2->y) + p3->y) / 3;
		barycentre.z = ((p1->z + p2->z) + p3->z) / 3;

		(*it)->barycentre = &barycentre;
	}

	// Flipping
	perturbatedPoints = letsPertubate(points);

	std::vector<PointKob*> newPoints;
	std::vector<FaceKob*> newFaces;

	for (int i = 0; i < perturbatedPoints.size(); ++i)
	{
		for (int j = 0; j < perturbatedPoints[i]->adjacentEdge.size(); j++)
		{
			if (perturbatedPoints[i]->adjacentEdge[j]->adjacentFace.size() == 2)
			{
				perturbatedPoints[i]->adjacentEdge.erase(perturbatedPoints[i]->adjacentEdge.begin() + j);
				j--;
			}
			else
			{
				bool found = false;

				for (int k = 0; k < newEdges.size(); ++k)
				{
					if (perturbatedPoints[i]->adjacentEdge[j] == newEdges[k])
					{
						found = true;
						break;
					}
				}

				if (!found)
					newEdges.push_back(perturbatedPoints[i]->adjacentEdge[j]);
			}
		}

		for (int j = 0; j < perturbatedPoints[i]->adjacentFace.size(); j++)
		{
			EdgeKob* baryEdge = new EdgeKob(perturbatedPoints[i], perturbatedPoints[i]->adjacentFace[j]->barycentre);
			perturbatedPoints[i]->adjacentEdge.push_back(baryEdge);
			perturbatedPoints[i]->adjacentFace[j]->barycentre->adjacentEdge.push_back(baryEdge);
			newEdges.push_back(baryEdge);
		}
	}

	for (int i = 0; i < edges.size(); ++i)
	{
		if (edges[i]->adjacentFace.size() == 2)
		{
			EdgeKob* newEdge = new EdgeKob(edges[i]->adjacentFace[0]->barycentre, edges[i]->adjacentFace[1]->barycentre);
			edges[i]->adjacentFace[0]->barycentre->adjacentEdge.push_back(newEdge);
			edges[i]->adjacentFace[1]->barycentre->adjacentEdge.push_back(newEdge);

			newEdges.push_back(newEdge);
		}
	}

	for (int i = 0; i < faces.size(); ++i)
	{
		perturbatedPoints.push_back(faces[i]->barycentre);
	}

	for (int i = 0; i < perturbatedPoints.size(); ++i)
	{
		findOrientedAdjacentEdges(perturbatedPoints[i]);
	}

	std::vector<EdgeKob*> borderEdge;

	for (int i = 0; i < points.size(); ++i)
	{
		for (int j = 0; j < perturbatedPoints[i]->adjacentEdge.size() - 1; ++j)
		{
			EdgeKob* e1 = perturbatedPoints[i]->adjacentEdge[j];
			EdgeKob* e2 = perturbatedPoints[i]->adjacentEdge[j + 1];

			bool found = false;
			for (int k = 0; k < borderEdge.size(); ++k)
			{
				if (e1 == borderEdge[k])
					found = true;
			}

			if (found)
				continue;

			found = false;
			for (int k = 0; k < borderEdge.size(); ++k)
			{
				if (e2 == borderEdge[k])
					found = true;
			}

			if (found)
				continue;

			if (edgeAlreadyExist(e1, edges))
			{
				borderEdge.push_back(e1);
				e1->adjacentFace.clear();
			}
			else if (edgeAlreadyExist(e2, edges))
			{
				borderEdge.push_back(e2);
				e2->adjacentFace.clear();
			}

			EdgeKob* e3 = foundEdge(e1, e2);
			FaceKob* newFace = new FaceKob(e1, e2, e3);

			newFaces.push_back(newFace);
			e1->adjacentFace.push_back(newFace);
			e2->adjacentFace.push_back(newFace);
			e3->adjacentFace.push_back(newFace);

			PointKob* p = nullptr, *pk1 = nullptr, *pk2 = nullptr;

			if (e1->points[0] == e2->points[0])
			{
				p = e1->points[0];
				pk1 = e1->points[1];
				pk2 = e2->points[1];
			}
			else if (e1->points[0] == e2->points[1])
			{
				p = e1->points[0];
				pk1 = e1->points[1];
				pk2 = e2->points[0];
			}
			else if (e1->points[1] == e2->points[0])
			{
				p = e1->points[1];
				pk1 = e1->points[0];
				pk2 = e2->points[1];
			}
			else if (e1->points[1] == e2->points[1])
			{
				p = e1->points[1];
				pk1 = e1->points[0];
				pk2 = e2->points[0];
			}

			p->adjacentFace.push_back(newFace);
			pk1->adjacentFace.push_back(newFace);
			pk2->adjacentFace.push_back(newFace);
		}
	}

	faces = newFaces;
	edges = newEdges;
	points = perturbatedPoints;
}

void CreateGlutMenu()
{
	menu_Main = glutCreateMenu(MenuFunction);
	if (state == INIT || state == KOBBELT)
	{
		glutAddMenuEntry("Kobbelt subdiviser", 0);
		state = KOBBELT;
	}
	glutAttachMenu(GLUT_RIGHT_BUTTON);	
	glutPostRedisplay();
	return;
}

void MenuFunction(int i)
{
	if (i == 0)
	{
		state == KOBBELT;
	}

	glutDestroyMenu(menu_Main);
	CreateGlutMenu();
}

bool Initialize()
{
	colore[0] = 0.0;
	colore[1] = 1.0;
	colore[2] = 1.0;
	colore[3] = 1.0;

	std::vector<Point> centerPoints3D = createRandomPoints(30);

	Graph * tmpGraph = new Graph();
	EnvInc testEnv = *new EnvInc(tmpGraph,centerPoints3D);
	testEnv.initializeGraph();
	testEnv.algo();
	tmpFace = testEnv.getGraph()->getFaceList();

	for (int i = 0; i < tmpFace->size(); i++)
	{
		tmpVectorPoints.push_back(tmpFace->at(i)->getPoints()[0]);
		col.push_back(tmpFace->at(i)->getColor());
		tmpVectorPoints.push_back(tmpFace->at(i)->getPoints()[1]);
		col.push_back(tmpFace->at(i)->getColor());
		tmpVectorPoints.push_back(tmpFace->at(i)->getPoints()[2]);
		col.push_back(tmpFace->at(i)->getColor());

	}
	
	std::vector<Colore> tmpColore;
	
	p3D = transformPointsToCube(centerPoints3D);

	for (int i = 0; i < p3D.size(); i++)
	{
		tmpColore.push_back(Colore(red));
	}

	tabPoints = structToTabColor(p3D,tmpColore);

	indi = createInd(centerPoints3D.size()*24);
	indTmp = createInd(tmpVectorPoints.size());
	tmpPoints = structToTabColor(tmpVectorPoints,col);

	glewInit();
	g_BasicShader.LoadVertexShader("basic.vs");
	g_BasicShader.LoadFragmentShader("basic.fs");
	g_BasicShader.CreateProgram();

	glGenTextures(1, &TexObj);
	glBindTexture(GL_TEXTURE_2D, TexObj);
	int w, h, c; //largeur, hauteur et # de composantes du fichier

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //GL_NEAREST)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenVertexArrays(1, &VBO0); // Créer le VAO
	glBindVertexArray(VBO0); // Lier le VAO pour l'utiliser
	glEnableVertexAttribArray(0);


	//glGenBuffers(1, &VBO0);

	glBindBuffer(GL_ARRAY_BUFFER, VBO0);
	glBufferData(GL_ARRAY_BUFFER, p3D.size() * 9 * sizeof(float), tabPoints, GL_STATIC_DRAW);
	//---
	glGenVertexArrays(1, &VBO1); // Créer le VAO
	glBindVertexArray(VBO1); // Lier le VAO pour l'utiliser
	glEnableVertexAttribArray(0);


	//glGenBuffers(1, &VBO0);

	glBindBuffer(GL_ARRAY_BUFFER, VBO1);
	glBufferData(GL_ARRAY_BUFFER, tmpVectorPoints.size() * 9 * sizeof(float), tmpPoints, GL_STATIC_DRAW);

	// rendu indexe
	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, p3D.size() * sizeof(GLushort), indi, GL_STATIC_DRAW);
	glGenBuffers(1, &IBO1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO1);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tmpVectorPoints.size() * sizeof(GLushort), indTmp, GL_STATIC_DRAW);

	// le fait de specifier 0 comme BO desactive l'usage des BOs
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ChangeCam(CamType);


	

	return true;
}

void Terminate()
{
	glDeleteTextures(1, &TexObj);
	glDeleteBuffers(1, &IBO);
	glDeleteBuffers(1, &VBO0);
	glDeleteBuffers(1, &IBO1);
	glDeleteBuffers(1, &VBO1);
	g_BasicShader.DestroyProgram();
}

void update()
{
	glutPostRedisplay();
}

void animate()
{


	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	// afin d'obtenir le deltatime actuel
	TimeSinceAppStartedInMS = glutGet(GLUT_ELAPSED_TIME);
	TimeInSeconds = TimeSinceAppStartedInMS / 1000.0f;
	DeltaTime = (TimeSinceAppStartedInMS - OldTime) / 1000.0f;
	OldTime = TimeSinceAppStartedInMS;

	glViewport(0, 0, width, height);
	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	//glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	auto program = g_BasicShader.GetProgram();
	glUseProgram(program);

	/*	uint32_t texUnit = 0;
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, TexObj);
	auto texture_location = glGetUniformLocation(program, "u_Texture");
	glUniform1i(texture_location, texUnit);
	*/
	// UNIFORMS
	Esgi::Mat4 worldMatrix;
	worldMatrix.MakeScale(1.0f, 1.0f, 1.0f);

	//  Camera Matrix
	Esgi::Mat4 cameraMatrix;
	switch (CamType)
	{
	case 0:	//FPS
		cameraMatrix = FPSCamera(posX, posY, posZ, rotX, rotY);
		break;
	case 1:	//Orbit
		cameraMatrix = OrbitCamera(posX, posY, posZ, distance, rotX, rotY);
		break;
	}

	//

	auto world_location = glGetUniformLocation(program, "u_WorldMatrix");
	glUniformMatrix4fv(world_location, 1, GL_FALSE, worldMatrix.m);

	Esgi::Mat4 projectionMatrix;
	float w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
	// ProjectionMatrix
	float aspectRatio = w / h;			// facteur d'aspect
	float fovy = 45.0f;					// degree d'ouverture
	float nearZ = 0.1f;
	float farZ = 10000.0f;
	projectionMatrix.Perspective(fovy, aspectRatio, nearZ, farZ);

	//projectionMatrix.MakeScale(1.0f / (0.5f*w), 1.0f / (0.5f*h), 1.0f);

	auto projection_location = glGetUniformLocation(program, "u_ProjectionMatrix");
	glUniformMatrix4fv(projection_location, 1, GL_FALSE, projectionMatrix.m);

	auto camera_location = glGetUniformLocation(program, "u_CameraMatrix");
	glUniformMatrix4fv(camera_location, 1, GL_FALSE, cameraMatrix.m);

	auto time_location = glGetUniformLocation(program, "u_Time");
	glUniform1f(time_location, TimeInSeconds);

	/*auto c_location = glGetUniformLocation(program, "color");
	glUniform4fv(c_location, 1, colore);*/

	// ATTRIBUTES
	auto normal_location = glGetAttribLocation(program, "a_Normal");
	auto position_location = glGetAttribLocation(program, "a_Position");
	auto color_location = glGetAttribLocation(program, "a_Color");
	//auto texcoords_location = glGetAttribLocation(program, "a_TexCoords");
	//glVertexAttrib3f(color_location, 0.0f, 1.0f, 0.0f);

	// Le fait de specifier la ligne suivante va modifier le fonctionnement interne de glVertexAttribPointer
	// lorsque GL_ARRAY_BUFFER != 0 cela indique que les donnees sont stockees sur le GPU
	glBindBuffer(GL_ARRAY_BUFFER, VBO0);

	//glBindVertexArray(VAO);

	glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<const void *>(0 * sizeof(float)));
	glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<const void *>(3 * sizeof(float)));
	glVertexAttribPointer(color_location, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<const void *>(6 * sizeof(float)));
	// on interprete les 3 valeurs inconnues comme RGB alors que ce sont les normales
	//glVertexAttribPointer(texcoords_location, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<const void *>(6 * sizeof(float)));

	//glEnableVertexAttribArray(texcoords_location);
	glEnableVertexAttribArray(position_location);
	glEnableVertexAttribArray(normal_location);
	glEnableVertexAttribArray(color_location);
	//glEnableVertexAttribArray(texcoords_location);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glDrawElements(GL_QUADS, p3D.size(), GL_UNSIGNED_SHORT, nullptr);

	//-----------

	glBindBuffer(GL_ARRAY_BUFFER, VBO1);

	//glBindVertexArray(VAO);

	glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<const void *>(0 * sizeof(float)));
	glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<const void *>(3 * sizeof(float)));
	glVertexAttribPointer(color_location, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<const void *>(6 * sizeof(float)));
	// on interprete les 3 valeurs inconnues comme RGB alors que ce sont les normales
	//glVertexAttribPointer(texcoords_location, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<const void *>(6 * sizeof(float)));

	//glEnableVertexAttribArray(texcoords_location);
	glEnableVertexAttribArray(position_location);
	glEnableVertexAttribArray(normal_location);
	glEnableVertexAttribArray(color_location);
	//glEnableVertexAttribArray(texcoords_location);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO1);
	glDrawElements(GL_TRIANGLES, tmpVectorPoints.size(), GL_UNSIGNED_SHORT, nullptr);

	//----------------
	//glPointSize(10);
	glDisableVertexAttribArray(position_location);
	glDisableVertexAttribArray(normal_location);
	glDisableVertexAttribArray(color_location);
	//glDisableVertexAttribArray(texcoords_location);
	glUseProgram(0);

	
	//Repositionnement du curseur 
	//glutWarpPointer(width*0.5f, height*0.5f);
	glEnd();


	glutSwapBuffers();

}

int main(int argc, const char* argv[])
{


	// passe les parametres de la ligne de commande a glut
	glutInit(&argc, (char**)argv);
	// defini deux color buffers (un visible, un cache) RGBA
	// GLUT_DEPTH alloue egalement une zone mémoire pour le depth buffer
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	// positionne et dimensionne la fenetre
	glutInitWindowPosition(100, 10);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Transformation");
	gluOrtho2D(-400, 400, -400, 400);						// Repère 2D délimitant les abscisses et les ordonnées
															// creation de la fenetre ainsi que du contexte de rendu

	glClearColor(1.0, 1.0, 1.0, 0.5);
	glColor3f(1.0, 1.0, 1.0);			     	 // couleur: blanc
	glPointSize(2.0);
#ifdef FREEGLUT
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif

#ifdef NO_GLEW
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)
		wglGetProcAddress("glVertexAttribPointer");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)
		wglGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)
		wglGetProcAddress("glDisableVertexAttribArray");
#else
	glewInit();
#endif
	Initialize();
	glutIdleFunc(update);
	glutDisplayFunc(animate);

	glutPassiveMotionFunc(mouse);
	glutSpecialFunc(SpecialInput);
	glutKeyboardFunc(keyboard);
	CreateGlutMenu();

	glutMainLoop();

	Terminate();

	return 1;
}

GLushort* createInd(int n)
{
	GLushort* tmp = new GLushort[n];
	for (int i = 0; i < n ; i++)
	{
		tmp[i] = i;
	}
	return tmp;
}
