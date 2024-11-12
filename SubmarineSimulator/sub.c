#include <freeglut.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	GLfloat position[3];
} Vertex3;

typedef struct
{
	GLfloat rgb[3];
} Color;

typedef struct
{
	GLint v[3];
	GLint vn[3];
} Face;

typedef struct
{
	Vertex3* vertices;
	Vertex3* normals;
	int vertexCount;
	int normalCount;
} ObjValues;

typedef struct
{
	Face* faces;
	int faceCount;
} Group;

// Beginning camera position
GLfloat cameraPosition[] = { 0, -200, 20 };
GLfloat cameraLookAt[] = { 0.0f, 0.0f, 0.0f };

// Window variables
GLfloat windowWidth = 800;
GLfloat windowHeight = 600;
GLint windowPositionX = 100;
GLint windowPositionY = 100;

// Camera movement step size
GLfloat cameraStepSize = 0.5f;

// State variables
GLboolean isFullscreen = GL_FALSE;
GLboolean isDrawingWireFrame = GL_FALSE;

// Submarine varaibles
ObjValues submarineValues;
Group submarine[4];
GLint* submarineGroupCount = 0;

// Method that reads through a file, counting the pieces of a certain object, 
// whether it's coral or the submarine pieces. It increases a groupCount
// variable that gets passed into the function for use in other methods
void countElements(FILE* file, ObjValues* values, Group* groups, int* groupCount)
{
	char line[128];
	int currentGroup = -1;

	while (fgets(line, sizeof(line), file) != NULL)
	{
		// If the line starts with g, then we have a sub piece
		if (line[0] == 'g')
		{
			currentGroup++;

			groups[currentGroup].faceCount = 0;
			
			(*groupCount)++;
		}
		// If we are on a vertex line
		else if (line[0] == 'v' && line[1] != 'n')
		{
			values->vertexCount++;
		}
		// If we are on a normal line
		else if (line[0] == 'v' && line[1] == 'n')
		{
			values->normalCount++;
		}
		// If we are on a face line
		else if (line[0] == 'f')
		{
			groups[currentGroup].faceCount++;
		}
	}

	// Go back to the start of the file after we increase the counts
	rewind(file);
}

// Function to allocate memory for all of the groups of a specific object to be
// rendered.
void allocateMemory(ObjValues* values, Group* groups, GLint groupCount)
{
	values->vertices = (Vertex3*)malloc(sizeof(Vertex3) * (values->vertexCount));
	if (!values->vertices)
	{
		printf("Error allocating memory for the vertices\n");
		exit(1);
	}

	values->normals = (Vertex3*)malloc(sizeof(Vertex3) * (values->normalCount));
	if (!values->normals)
	{
		printf("Error allocating memory for the normals\n");
		exit(1);
	}
	for (int i = 0; i < groupCount; i++)
	{
		
		groups[i].faces = (Face*)malloc(sizeof(Face) * (groups[i].faceCount));
		if (!groups[i].faces)
		{
			printf("Error allocating memory for the faces at index: %d\n", i);
			exit(1);
		}
	}
}

/*
* Method used to read through a file, and set the values for the vertices, 
* normals, and faces for all of the groups of an object
*/
void setValues(FILE* file, ObjValues* values, Group* groups, GLint groupCount)
{
	int vertexCounter = 0, normalCounter = 0, faceCounter = 0;

	int currentGroup = -1;

	char line[128];

	while (fgets(line, sizeof(line), file) != NULL)
	{
		if (line[0] == 'g')
		{
			currentGroup++;
			faceCounter = 0;
		}
		else if (line[0] == 'v' && line[1] != 'n')
		{
			if (sscanf_s(line, "v %f %f %f",
				&values->vertices[vertexCounter].position[0],
				&values->vertices[vertexCounter].position[1],
				&values->vertices[vertexCounter].position[2]) == 3)
			{
				vertexCounter++;
				printf("Group: %d, Vertex: %d\n", currentGroup, vertexCounter);
			}
		}
		else if (line[0] == 'v' && line[1] == 'n')
		{
			if (sscanf_s(line, "vn %f %f %f",
				&values->normals[normalCounter].position[0],
				&values->normals[normalCounter].position[1],
				&values->normals[normalCounter].position[2]) == 3)
			{
				normalCounter++;
				printf("Group: %d, Normal: %d\n", currentGroup, normalCounter);
			}
		}
		else if (line[0] == 'f')
		{
			if (sscanf_s(line, "f %d//%d %d//%d %d//%d",
				&groups[currentGroup].faces[faceCounter].v[0],
				&groups[currentGroup].faces[faceCounter].vn[0],
				&groups[currentGroup].faces[faceCounter].v[1],
				&groups[currentGroup].faces[faceCounter].vn[1],
				&groups[currentGroup].faces[faceCounter].v[2],
				&groups[currentGroup].faces[faceCounter].vn[2]) == 6)
			{

				groups[currentGroup].faces[faceCounter].v[0] -= 1;
				groups[currentGroup].faces[faceCounter].vn[0] -= 1;
				groups[currentGroup].faces[faceCounter].v[1] -= 1;
				groups[currentGroup].faces[faceCounter].vn[1] -= 1;
				groups[currentGroup].faces[faceCounter].v[2] -= 1;
				groups[currentGroup].faces[faceCounter].vn[2] -= 1;
				faceCounter++;
				//printf("Group: %d, Face: %d\n", currentGroup, faceCounter);
				for (int i = 0; i < 3; i++)
				{
					printf("Group: %d, Face: %d, V: %d, VN: %d\n", currentGroup, faceCounter, groups[currentGroup].faces[faceCounter -1].v[i], groups[currentGroup].faces[faceCounter -1].v[i]);
				}
			}
		}
	}
}

// Helper method to count, allocate, and set the values for the object to be
// rendered.
void allocateAndPopulateHelper(FILE* file, ObjValues* values, Group* groups, GLint* groupCount)
{
	countElements(file, values, groups, groupCount);
	allocateMemory(values, groups, *groupCount);
	setValues(file, values, groups, *groupCount);

	/*printf("This is the groupCount: %d\n", *groupCount);

	for (int i = 0; i < *groupCount; i++)
	{
		printf("There are %d faces in this group\n", groups[i].faceCount);
	}*/
}

void renderObject(ObjValues* values, Group* group)
{
	glBegin(GL_TRIANGLES);
	//printf("There are %d faces in this loop iteration\n", group->faceCount);
	for (GLint i = 0; i < group->faceCount; i++)
	{
		Face* currentFace = &group->faces[i];

		// TEMPORARY
		GLfloat color = (GLfloat) i / group->faceCount;
		glColor3f(color, color, 0.2f);
		for (GLint j = 0; j < 3; j++)
		{
			GLint vertexIndex = currentFace->v[j];
			GLint normalIndex = currentFace->vn[j];

			//printf("Vertex index: %d, Normal Index: %d\n", vertexIndex, normalIndex);

			Vertex3* vertex = &values->vertices[vertexIndex];
			Vertex3* normal = &values->normals[normalIndex];

			//printf("Vertex %d: (%f, %f, %f)\n", vertexIndex, vertex->position[0], vertex->position[1], vertex->position[2]);

			glNormal3f(normal->position[0], normal->position[1], normal->position[2]);
			glVertex3f(vertex->position[0], vertex->position[1], vertex->position[2]);
		}
	}

	glEnd();
}

void drawSubmarine()
{
	glPushMatrix();

	// Increase the size by 10x
	glScalef(2.0f, 2.0f, 2.0f);

	// Move to the look at position
	glTranslatef(30, -30, 0);

	// Set submarineColor to yellow
	glColor3f(1.0f, 1.0f, 0.0f);

	// Call the draw helper
	for (GLint i = 0; i < submarineGroupCount; i++)
	{
		renderObject(&submarineValues, &submarine[i]);
	}
	
	glPopMatrix();
}

/*
* Method used to draw the unit vectors coming out of the origin. It uses for 
* loops to set the vector values, and draws an X, Y, and Z vector, with their
* respective colors being red, green, and blue. It also draws a little white
* sphere at the origin.
*/
void drawUnitVectors()
{
	GLint lineLegnth = 50;

	Vertex3 vertices[3];
	Color colors[3];
	// Initialize the vertices and colors to 0
	for (GLint i = 0; i < 3; i++) 
	{
		for (GLint j = 0; j < 3; j++)
		{
			vertices[i].position[j] = 0;
			colors[i].rgb[j] = 0.0;
		}
	}

	// Loop that sets each line and color
	for (GLint i = 0; i < 3; i++)
	{
		vertices[i].position[i] = lineLegnth;
		colors[i].rgb[i] = 1.0;
	}

	glBegin(GL_LINES);
	glLineWidth(5);
	for (GLint i = 0; i < 3; i++)
	{
		glColor3f(colors[i].rgb[0], colors[i].rgb[1], colors[i].rgb[2]);
		glVertex3f(0, 0, 0);
		glVertex3f(vertices[i].position[0], vertices[i].position[1], vertices[i].position[2]);
	}
	glEnd();

	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();
	glTranslatef(0, 0, 0);
	glColor3f(1.0, 1.0, 1.0);
	gluSphere(quad, 1, 30, 30);
	glPopMatrix();

	gluDeleteQuadric(quad);
}

void idleScene(void)
{
	glutPostRedisplay();
}

// Display function that sets what the camera is looking at, draws the vectors,
// the scene, etc.
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();

	if (isDrawingWireFrame)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	gluLookAt(cameraPosition[0], cameraPosition[1], cameraPosition[2],
		cameraLookAt[0], cameraLookAt[1], cameraLookAt[2],
		0, 1, 0);

	drawSubmarine();

	//drawUnitVectors();

	glutSwapBuffers();
}

// Function to handle special keys on the keyboard. This method handles
// vertical movement of the camera / submarine
void handleSpecialKeyboard(unsigned char key, GLint x, GLint y)
{
	// Handle vertical movement
	if (key == GLUT_KEY_UP)
	{
		cameraPosition[2] += cameraStepSize;
		cameraLookAt[2] += cameraStepSize;
	}
	if (key == GLUT_KEY_DOWN)
	{
		cameraPosition[2] -= cameraStepSize;
		cameraPosition[2] -= cameraStepSize;
	}
}

// Function to handle standard keyboard keys, this method handles movement,
// state variables, and ending the application
void handleKeyboard(unsigned char key, GLint x, GLint y)
{
	// Handle lateral movement
	if (key == 'w' || key == 'W')
	{
		cameraPosition[1] += cameraStepSize;
		cameraLookAt[1] += cameraStepSize;
	}
	if (key == 'a' || key == 'A')
	{
		cameraPosition[0] -= cameraStepSize;
		cameraLookAt[0] -= cameraStepSize;
	}
	if (key == 's' || key == 'S')
	{
		cameraPosition[1] -= cameraStepSize;
		cameraLookAt[1] -= cameraStepSize;
	}
	if (key == 'd' || key == 'D')
	{
		cameraPosition[0] += cameraStepSize;
		cameraLookAt[0] += cameraStepSize;
	}

	// Toggle the state variables
	if (key == 'f' || key == 'F')
	{
		isFullscreen = !isFullscreen;
		if (isFullscreen)
		{
			glutFullScreen();
		}
		else
		{
			glutReshapeWindow(windowWidth, windowHeight);
			glutPositionWindow(windowPositionX, windowPositionY);
		}
	}
	if (key == 'u' || key == 'U')
	{
		isDrawingWireFrame = !isDrawingWireFrame;
		glutPostRedisplay();
	}

	if (key == 'q' || key == 'Q')
	{
		exit(1);
	}
}

void windowReshape(GLint width, GLint height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (float)width / (float)height, 1.0f, 1000.0f);
	glMatrixMode(GL_MODELVIEW);
}

void initializeGL(void)
{
	glEnable(GL_DEPTH_TEST);
	// Add back in when implmenting lighting
	// glEnable(GL_LIGHTING);
	// glEnable(GL_LIGHT0);
	// glEnable(GL_NORMALIZE);
	glClearColor(0, 0, 0, 1.0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (float)windowWidth / (float)windowHeight, 1.0f, 1000.0f);
	glMatrixMode(GL_MODELVIEW);
}

// Helper method to clean up the main method and leave the initialization of 
// the submarine in its own method
void initSub()
{
	FILE* file = fopen("submarine - updated.obj", "r");
	if (!file)
	{
		printf("No file found\n");
		return 1;
	}

	allocateAndPopulateHelper(file, &submarineValues, submarine, &submarineGroupCount);
	fclose(file);
}

// Method to free the memory of all of the objects we allocated memory for
void freeObjects()
{
	free(submarineValues.vertices);
	free(submarineValues.normals);
	for (GLint i = 0; i < submarineGroupCount; i++)
	{
		free(submarine[i].faces);
	}
}

// The main method that ties everything together
int main(int argc, char** argv)
{
	initSub();

	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(windowPositionX, windowPositionY);
	glutCreateWindow("Submarine Simulator");

	glutDisplayFunc(display);
	glutReshapeFunc(windowReshape);
	glutKeyboardFunc(handleKeyboard);
	glutSpecialFunc(handleSpecialKeyboard);

	glutIdleFunc(idleScene);

	initializeGL();
	glutMainLoop();

	freeObjects();
}