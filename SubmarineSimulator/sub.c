#include <freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI 3.1415926535

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
GLfloat cameraPosition[] = { 0.0f, -200.0f, 0.0f };
GLfloat cameraLookAt[] = { 0.0f, 0.0f, 0.0f };

// Window variables
GLfloat windowWidth = 800;
GLfloat windowHeight = 600;
GLint windowPositionX = 100;
GLint windowPositionY = 100;

// State variables
GLboolean isFullscreen = GL_FALSE;
GLboolean isDrawingWireFrame = GL_FALSE;

// Submarine varaibles
ObjValues submarineValues;
Group submarine[4];
GLfloat submarineSpeed = 0.1f;
GLint* submarineGroupCount = 0;
GLfloat submarineX = 0.0f;
GLfloat submarineY = 0.0f;
GLfloat submarineZ = 0.0f;

// Keyboard Varibales
GLboolean keyStates[256] = { GL_FALSE };
GLboolean specialKeyStates[256] = { GL_FALSE };

// Mouse Look Variables
GLint prevX = 0;
GLint prevY = 0;
GLfloat cameraDistance = 200.0f;
GLfloat horizontalMouseAngle = 0.0f;
GLfloat verticalMouseAngle = 0.0f;
GLfloat sensitivity = 0.5f;

// Scene Variables
GLint bottomDiscRadius = 500;
GLint bottomDiscSegments = 48;
GLint wallHeight = 500;

// Helper function to set the material of a surface
void setMaterial(GLfloat ambient[], GLfloat diffuse[], GLfloat specular[], GLfloat shininess)
{
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
}

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

/*
* This method is used to render an object based on their values that were previously
* initialized, and their groups where memory was previously allocated. This method 
* sets an objects normal and vertex for each triangle that gets drawn
*/
void renderObject(ObjValues* values, Group* group)
{
	glBegin(GL_TRIANGLES);
	for (GLint i = 0; i < group->faceCount; i++)
	{
		Face* currentFace = &group->faces[i];

		for (GLint j = 0; j < 3; j++)
		{
			GLint vertexIndex = currentFace->v[j];
			GLint normalIndex = currentFace->vn[j];

			Vertex3* vertex = &values->vertices[vertexIndex];
			Vertex3* normal = &values->normals[normalIndex];

			glNormal3f(normal->position[0], normal->position[1], normal->position[2]);
			glVertex3f(vertex->position[0], vertex->position[1], vertex->position[2]);
		}
	}

	glEnd();
}

// Method used to draw the submarine
void drawSubmarine()
{
	glPushMatrix();

	// Move to the look at position
	glTranslatef(submarineX, submarineY, submarineZ);

	// Rotate the submarine so that it is rotated to the right axis
	glRotatef(90.0f, 1, 0, 0);
	glRotatef(-90.0f, 0, 1, 0);

	// Increase the size by 0.1x
	glScalef(0.1f, 0.1f, 0.1f);

	// Submarine lighting variables so that the submarine is yellow, and slightly shiny
	GLfloat ambient[] = { 0.5f, 0.5f, 0.0f, 1.0f };
	GLfloat diffuse[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	GLfloat specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat shininess = 50.0f;

	setMaterial(ambient, diffuse, specular, shininess);

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
	GLint lineLegnth = 25;

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

	glDisable(GL_LIGHTING);

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

	glEnable(GL_LIGHTING);
}

/*
* Method that's used to draw the bottom of the map, or the sandy sea floor.
* It does so by using a trangle fan, and fills the circle with the sand ppm
* texture.
*/
void drawBottomDisc()
{
	glDisable(GL_LIGHTING);

	glColor3f(0.8f, 0.8f, 0.2f);

	glBegin(GL_TRIANGLE_FAN);

	glVertex3f(0.0f, 0.0f, 0.0f);

	GLfloat increment = 2 * PI / bottomDiscSegments;

	for (GLint i = 0; i <= bottomDiscSegments; i++)
	{
		GLfloat angle = i * increment;

		// Make the X and Y just so every slightly larger
		GLfloat x = (bottomDiscRadius + 1) * cos(angle);
		GLfloat y = (bottomDiscRadius + 1) * sin(angle);

		glVertex3f(x, y, 0.0f);
	}

	glEnd();

	glEnable(GL_LIGHTING);
}

void drawCylinderWall()
{
	//glDisable(GL_LIGHTING);

	glColor3f(0.8f, 0.8f, 0.2f);

	glBegin(GL_QUAD_STRIP);

	GLfloat increment = 2 * PI / bottomDiscSegments;

	for (GLint i = 0; i <= bottomDiscSegments; i++)
	{
		GLfloat angle = i * increment;

		GLfloat x = bottomDiscRadius * cos(angle);
		GLfloat y = bottomDiscRadius * sin(angle);

		glVertex3f(x, y, 0.0f);
		
		glVertex3f(x, y, wallHeight);
	}

	glEnd();

	//glEnable(GL_LIGHTING);
}

/*
* Method that is used to handle the mouse movement across the screen to rotate 
* the camera. It uses global prevX and prevY variables so that it can keep 
* track of the mouse position. It sets the horizontal mouse angle (the azimuth)
* based on the sensitivity and the difference, and also sets the vertical based 
* on the same properties. It keeps the vertical within bounds, and makes sure
* the horizontal doesn't get some large (or negatively large) number.
*/
void moveMouse(GLint x, GLint y)
{
	GLint xDiff = x - prevX;
	GLint yDiff = y - prevY;

	horizontalMouseAngle += xDiff * sensitivity;
	verticalMouseAngle += yDiff * sensitivity;

	// Reset angles to avoid ridiculoualy large (or negatively large) horizontal values
	if (horizontalMouseAngle > 360.0f) horizontalMouseAngle -= 360.0f;
	else if (horizontalMouseAngle < 0.0f) horizontalMouseAngle += 360.0f;

	// Make sure that the vertical does not go above or below the top or the bottonm of the submarine
	if (verticalMouseAngle > 89.0f) verticalMouseAngle = 89.0f;
	else if (verticalMouseAngle < -89.0f) verticalMouseAngle = -89.0f;

	prevX = x;
	prevY = y;

	glutPostRedisplay();
}

/*
* This method calculates where the camera should be based on a sphere on where
* the submarine lies, sets the new camera based on this, and makes sure the 
* camera is looking at the position of the submarine. It first calculates the
* horizontal and vertical radians of the mouse angles, and then it calculates
* the new postion of the camera based on this web resource:
* https://mathinsight.org/spherical_coordinates, retrieved from relationship
* (1)l except I added the submarine position to properly offset the camera
*/
void moveCamera()
{
	GLfloat radianHorizontal = horizontalMouseAngle * (PI / 180.0f);
	GLfloat radianVertical = verticalMouseAngle * (PI / 180.0f);

	GLfloat newCamX = submarineX + cameraDistance * cosf(radianHorizontal) * cosf(radianVertical);
	GLfloat newCamY = submarineY + cameraDistance * sinf(radianHorizontal) * cosf(radianVertical);
	GLfloat newCamZ = submarineZ + cameraDistance * sinf(radianVertical);

	printf("Camera: ( %.2f, %.2f, %.2f ); Submarine: ( %.2f, %.2f, %.2f )\n", newCamX, newCamY, newCamZ, submarineX, submarineY, submarineZ);

	gluLookAt(newCamX, newCamY, newCamZ, submarineX, submarineY, submarineZ, 0, 0, 1);
}

// Function to handle standard key down presses. We handle the state varaibles
// in this function
void handleKeyboardDown(unsigned char key, GLint x, GLint y)
{
	keyStates[key] = GL_TRUE;

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

// Function to handle the release of standard keys
void handleKeyboardUp(unsigned char key, GLint x, GLint y)
{
	keyStates[key] = GL_FALSE;
}

// Function to handle special key down presses
void handleSpecialKeyboardDown(unsigned char key, GLint x, GLint y)
{
	specialKeyStates[key] = GL_TRUE;
}

// Function to handle the release of special keys
void handleSpecialKeyboardUp(unsigned char key, GLint x, GLint y)
{
	specialKeyStates[key] = GL_FALSE;
}

// Function that's used to move the submarine if any of the keys are pressed
void handleMovement()
{
	// Handle lateral movement
	if (keyStates['w'] || keyStates['W'])
	{
		submarineY += submarineSpeed;
	}
	if (keyStates['a'] || keyStates['A'])
	{
		submarineX -= submarineSpeed;
	}
	if (keyStates['s'] || keyStates['S'])
	{
		submarineY -= submarineSpeed;
	}
	if (keyStates['d'] || keyStates['D'])
	{
		submarineX += submarineSpeed;
	}

	// Handle vertical movement
	if (specialKeyStates[GLUT_KEY_UP])
	{
		submarineZ += submarineSpeed;
	}
	if (specialKeyStates[GLUT_KEY_DOWN])
	{
		submarineZ -= submarineSpeed;
	}
}

// Idle function to handle idle changes
void idleScene(void)
{
	handleMovement();

	glutPostRedisplay();
}

// Display function that sets what the camera is looking at, draws the vectors,
// the scene, etc.
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();

	glPolygonMode(GL_FRONT_AND_BACK, isDrawingWireFrame ? GL_LINE : GL_FILL);

	moveCamera();

	// Make sure the light comes from the top
	GLfloat lightPosition[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	drawSubmarine();

	drawBottomDisc();
	drawCylinderWall();

	drawUnitVectors();

	glutSwapBuffers();
}

void windowReshape(GLint width, GLint height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (float)width / (float)height, 1.0f, 2000.0f);
	glMatrixMode(GL_MODELVIEW);
}

void initializeGL(void)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glClearColor(0, 0, 0, 1.0);

	// Global ambient light
	GLfloat globalAmbient[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

	// Set up the sunlight
	GLfloat lightPosition[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	GLfloat lightDiffuse[] = { 1.0f, 1.0f, 0.8f, 1.0f };  
	GLfloat lightSpecular[] = { 1.0f, 1.0f, 0.8f, 1.0f }; 

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (float)windowWidth / (float)windowHeight, 1.0f, 2000.0f);
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

	glutKeyboardFunc(handleKeyboardDown);
	glutKeyboardUpFunc(handleKeyboardUp);
	glutSpecialFunc(handleSpecialKeyboardDown);
	glutSpecialUpFunc(handleSpecialKeyboardUp);

	glutPassiveMotionFunc(moveMouse);

	glutIdleFunc(idleScene);

	initializeGL();
	glutMainLoop();

	freeObjects();
}