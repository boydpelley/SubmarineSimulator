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

typedef GLubyte ColorTexture[3];

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
	int groupcount;
} ObjValues;

typedef struct
{
	Face* faces;
	int faceCount;
} Group;

typedef struct
{
	ObjValues values;
	Group groups[100];
} Object;

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
GLboolean isDrawingFog = GL_TRUE;

// Submarine varaibles
Object submarine;
GLfloat submarineSpeed = 1.0f;
GLfloat submarineX = 0.0f;
GLfloat submarineY = -150.0f;
GLfloat submarineZ = 150.0f;

// Coral Variables
Object coral[14];
Vertex3 coralPositions[14];

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

// Wave Variables
GLfloat subdivisionSize = 25.0f;
GLfloat waveHeightOffset = 450.0f;
GLfloat wavePhase = 50.0f;
GLfloat waveTimeValue = 0.0f;
GLfloat waveAmplitude = 40.0f;
GLfloat waveVelocity = 0.025f;
GLfloat waveLength = 450.0f;

// Textures
GLuint sandTexture;

/*
* This method generates a random float between any two random number inclusively.
*/
GLfloat generateRandomFloat(GLfloat minValue, GLfloat maxValue)
{
	GLfloat random = rand() / (float)RAND_MAX;
	return (GLfloat)(minValue + random * (maxValue - minValue));
}

/*
* A helper function to return the normal of three vectors. It calculates the cross
* product of the two vectors, and returns the normal
*/
Vertex3 calculateNormal(Vertex3 v1, Vertex3 v2, Vertex3 v3) 
{
	GLfloat edge1[3] = { v2.position[0] - v1.position[0], v2.position[1] - v1.position[1], v2.position[2] - v1.position[2] };
	GLfloat edge2[3] = { v3.position[0] - v1.position[0], v3.position[1] - v1.position[1], v3.position[2] - v1.position[2] };

	GLfloat x = edge1[1] * edge2[2] - edge1[2] * edge2[1];
	GLfloat y = edge1[2] * edge2[0] - edge1[0] * edge2[2];
	GLfloat z = edge1[0] * edge2[1] - edge1[1] * edge2[0];

	Vertex3 normal = { x, y, z };
	return normal;
}

/*
* Helper function that normalizes a vector.
*/
void normalizeVector(Vertex3* vector)
{
	GLfloat length = sqrtf(vector->position[0] * vector->position[0] +
		vector->position[1] * vector->position[1] + vector->position[2] * vector->position[2]);

	vector->position[0] /= length;
	vector->position[1] /= length;
	vector->position[2] /= length;
}

// Helper function to set the material of a surface
void setMaterial(GLfloat ambient[], GLfloat diffuse[], GLfloat specular[], GLfloat shininess)
{
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
}

// Helper method to initialize the values and allocate memory for Object structs
void allocateObject(Object** object)
{
	*object = (Object*)malloc(sizeof(Object));
	if (!*object)
	{
		printf("Error allocating memory for Object\n");
		exit(1);
	}

	(*object)->values.vertexCount = 0;
	(*object)->values.normalCount = 0;
	(*object)->values.groupcount = 0;

	for (GLint i = 0; i < 10; i++)
	{
		(*object)->groups[i].faceCount = 0;
		(*object)->groups[i].faces = NULL;
	}
}

// Method that reads through a file, counting the pieces of a certain object, 
// whether it's coral or the submarine pieces. It increases trh groupCount 
// for each object
void countElements(FILE* file, Object* object)
{
	char line[128];
	int currentGroup = -1;

	while (fgets(line, sizeof(line), file) != NULL)
	{
		// If the line starts with g, then we have a sub piece
		if (line[0] == 'g')
		{
			currentGroup++;

			object->groups[currentGroup].faceCount = 0;
			
			object->values.groupcount++;
		}
		// If we are on a vertex line
		else if (line[0] == 'v' && line[1] != 'n')
		{
			object->values.vertexCount++;
		}
		// If we are on a normal line
		else if (line[0] == 'v' && line[1] == 'n')
		{
			object->values.normalCount++;
		}
		// If we are on a face line
		else if (line[0] == 'f')
		{
			object->groups[currentGroup].faceCount++;
		}
	}

	// Go back to the start of the file after we increase the counts
	rewind(file);
}

// Function to allocate memory for all of the groups of a specific object to be
// rendered.
void allocateMemory(Object* object)
{
	object->values.vertices = (Vertex3*)malloc(sizeof(Vertex3) * (object->values.vertexCount));
	if (!object->values.vertices)
	{
		printf("Error allocating memory for the vertices\n");
		exit(1);
	}

	object->values.normals = (Vertex3*)malloc(sizeof(Vertex3) * (object->values.normalCount));
	if (!object->values.normals)
	{
		printf("Error allocating memory for the normals\n");
		exit(1);
	}
	for (int i = 0; i < object->values.groupcount; i++)
	{
		
		object->groups[i].faces = (Face*)malloc(sizeof(Face) * (object->groups[i].faceCount));
		if (!object->groups[i].faces)
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
void setValues(FILE* file, Object* object)
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
				&object->values.vertices[vertexCounter].position[0],
				&object->values.vertices[vertexCounter].position[1],
				&object->values.vertices[vertexCounter].position[2]) == 3)
			{
				vertexCounter++;
				//printf("Group: %d, Vertex: %d\n", currentGroup, vertexCounter);
			}
		}
		else if (line[0] == 'v' && line[1] == 'n')
		{
			if (sscanf_s(line, "vn %f %f %f",
				&object->values.normals[normalCounter].position[0],
				&object->values.normals[normalCounter].position[1],
				&object->values.normals[normalCounter].position[2]) == 3)
			{
				normalCounter++;
				//printf("Group: %d, Normal: %d\n", currentGroup, normalCounter);
			}
		}
		else if (line[0] == 'f')
		{
			if (sscanf_s(line, "f %d//%d %d//%d %d//%d",
				&object->groups[currentGroup].faces[faceCounter].v[0],
				&object->groups[currentGroup].faces[faceCounter].vn[0],
				&object->groups[currentGroup].faces[faceCounter].v[1],
				&object->groups[currentGroup].faces[faceCounter].vn[1],
				&object->groups[currentGroup].faces[faceCounter].v[2],
				&object->groups[currentGroup].faces[faceCounter].vn[2]) == 6)
			{

				object->groups[currentGroup].faces[faceCounter].v[0] -= 1;
				object->groups[currentGroup].faces[faceCounter].vn[0] -= 1;
				object->groups[currentGroup].faces[faceCounter].v[1] -= 1;
				object->groups[currentGroup].faces[faceCounter].vn[1] -= 1;
				object->groups[currentGroup].faces[faceCounter].v[2] -= 1;
				object->groups[currentGroup].faces[faceCounter].vn[2] -= 1;
				faceCounter++;
				//printf("Group: %d, Face: %d\n", currentGroup, faceCounter);
				for (int i = 0; i < 3; i++)
				{
					//printf("Group: %d, Face: %d, V: %d, VN: %d\n", currentGroup, faceCounter, object->groups[currentGroup].faces[faceCounter -1].v[i], object->groups[currentGroup].faces[faceCounter -1].v[i]);
				}
			}
		}
	}
}

// Helper method to count, allocate, and set the values for the object to be
// rendered.
void allocateAndPopulateHelper(FILE* file, Object* object)
{
	countElements(file, object);
	allocateMemory(object);
	setValues(file, object);
}

/*
* This method is used to render an object based on their values that were previously
* initialized, and their groups where memory was previously allocated. This method 
* sets an objects normal and vertex for each triangle that gets drawn
*/
void renderObject(Object* object)
{
	glBegin(GL_TRIANGLES);
	for (GLint i = 0; i < object->values.groupcount; i++)
	{
		for (GLint j = 0; j < object->groups[i].faceCount; j++)
		{
			Face* currentFace = &object->groups[i].faces[j];

			for (GLint k = 0; k < 3; k++)
			{
				GLint vertexIndex = currentFace->v[k];
				GLint normalIndex = currentFace->vn[k];

				Vertex3* vertex = &object->values.vertices[vertexIndex];
				Vertex3* normal = &object->values.normals[normalIndex];

				glNormal3f(normal->position[0], normal->position[1], normal->position[2]);
				glVertex3f(vertex->position[0], vertex->position[1], vertex->position[2]);
			}
		}
	}
	

	glEnd();
}

/*
* Method to read PPM files to set a TextureID to it. It reads PPM files of most widths
* and heights, allocating memory dynamically. It reads through the PPM file and
* creates a mipmap to render the texture on top of. Handles errors if the file
* cannot be properly read or manipulated. Returns an unsigned integer representing
* the ID of the created texture.
*/
GLuint readPPM(char * filename)
{
	FILE* file = fopen(filename, "rb");
	if (!file)
	{
		printf("Could not open file %s\n", filename);
		return 1;
	}

	char line[256];
	char header[3];
	GLint width, height, maxColor;

	if (!fgets(line, sizeof(line), file))
	{
		printf("Error with reading the header\n");
		fclose(file);
		return 1;
	}

	sscanf_s(line, "%s2", header, (unsigned)_countof(header));
	if (header[0] != 'P' || header[1] != '6')
	{
		printf("Error with file, make sure it's a valid format\n");
		fclose(file);
		return 1;
	}

	if (!fgets(line, sizeof(line), file))
	{
		printf("Error reading sizes\n");
		fclose(file);
		return 1;
	}
	sscanf_s(line, "%d %d", &width, &height);

	if (!fgets(line, sizeof(line), file))
	{
		printf("Error reading max color\n");
		fclose(file);
		return 1;
	}
	sscanf_s(line, "%d", &maxColor);

	GLubyte *textureData = (GLubyte *)malloc(width * height * 3);
	if (!textureData) 
	{
		printf("Error allocating memory for textureData\n");
		fclose(file);
		return 1;
	}

	fread(textureData, 3, width * height, file);
	fclose(file);

	GLuint textureID = 1;
	glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_2D, textureID);

	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, textureData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	
	free(textureData);

	return textureID;
}

// Method used to draw the submarine
void drawSubmarine()
{
	glEnable(GL_LIGHTING);
	glPushMatrix();

	// Move to the look at position
	glTranslatef(submarineX, submarineY, submarineZ);

	// Rotate the submarine so that it is rotated to the right axis
	glRotatef(90.0f, 1, 0, 0);
	glRotatef(-90.0f, 0, 1, 0);

	// Increase the size by 0.1x
	glScalef(0.2f, 0.2f, 0.2f);

	// Submarine lighting variables so that the submarine is yellow, and slightly shiny
	GLfloat ambient[] = { 0.5f, 0.5f, 0.0f, 1.0f };
	GLfloat diffuse[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	GLfloat specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat shininess = 50.0f;

	setMaterial(ambient, diffuse, specular, shininess);

	// Call the draw helper
	renderObject(&submarine);
	
	glPopMatrix();
	glDisable(GL_LIGHTING);
}

void drawCoral()
{
	glEnable(GL_LIGHTING);

	GLfloat ambient[] = { 0.05f, 0.7f, 0.1f, 1.0f };
	GLfloat diffuse[] = { 0.0f, 1.0f, 0.1f, 1.0f };
	GLfloat specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat shininess = 50.0f;
	
	for (GLint i = 0; i < 14; i++)
	{
		glPushMatrix();
		
		// Move each coral to their random position
		glTranslatef(coralPositions[i].position[0], coralPositions[i].position[1], 0);

		// Scale each coral to 100 times its size
		glScalef(200.0f, 200.0f, 200.0f);

		// Rotate the coral so it lies on the proper axis
		glRotatef(90.0f, 1, 0, 0);
		glRotatef(-90.0f, 0, 1, 0);

		// Set the material to the green color
		setMaterial(ambient, diffuse, specular, shininess);

		// Render each coral
		renderObject(&coral[i]);

		glPopMatrix();
	}

	glDisable(GL_LIGHTING);
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
* It does so by using gluDisk, and fills the circle with the spongebob sand 
* ppm texture.
*/
void drawBottomDisc()
{
	GLfloat emission[] = {0.2f, 0.2f, 0.2f, 1.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, sandTexture);

	glColor3f(1.0f, 1.0f, 1.0f);

	GLUquadric* quadric = gluNewQuadric();
	gluQuadricTexture(quadric, GL_TRUE);

	gluDisk(quadric, 0.0, bottomDiscRadius + 1, bottomDiscSegments, 1);

	gluDeleteQuadric(quadric);

	glDisable(GL_TEXTURE_2D);
}

/*
* Method to draw the walls of the scene using the same texture as the floor.
* It uses gluQuadrics to draw a gluCylinder.
*/
void drawCylinderWall()
{
	GLfloat emission[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, sandTexture);

	glColor3f(1.0f, 1.0f, 1.0f);

	GLUquadric* quadric = gluNewQuadric();
	gluQuadricTexture(quadric, GL_TRUE);

	gluCylinder(quadric, bottomDiscRadius, bottomDiscRadius, wallHeight, bottomDiscSegments, bottomDiscSegments);

	gluDeleteQuadric(quadric);

	glDisable(GL_TEXTURE_2D);
}

/*
* Function that's used to draw fog, which simulates the look of being under
* water. It draws a blue fog and draws it exponentially. The fog is toggled on
* and off with the 'b' key.
*/
void drawFog()
{
	if (isDrawingFog)
	{
		glEnable(GL_FOG);
	}
	else
	{
		glDisable(GL_FOG);
	}
	GLfloat fogColor[] = { 0.01f, 0.2f, 0.4f, 1.0f };

	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_MODE, GL_EXP);
	glFogf(GL_FOG_DENSITY, 0.0025f);
}

/*
* Function that's used to draw a wave. It starts from -600, -600 and goes all the
* way to 600, 600, with a fixed height; the variance coming from the heights 
* calculated in the function. It draws the waves diagonally, has lighting set to 
* it so it resembles water, and uses GL_TRIANGLES to draw the surface. It sets the
* normals of the triangles to properly reflect lights hitting the surface.
*/
void drawWave()
{
	glEnable(GL_LIGHTING);

	// Used to draw more or less waves based on the wavelength
	GLfloat frequency = 2.0f * PI / waveLength;

	// Set the lighting properties of the wave
	GLfloat ambient[] = { 0.02f, 0.25f, 0.5f, 1.0f };
	GLfloat diffuse[] = { 0.0f, 0.03f, 0.5f, 1.0f };
	GLfloat specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat shininess = 25.0f;

	setMaterial(ambient, diffuse, specular, shininess);

	// Loop from -600 to 600
	for (GLfloat x = -600; x < 600; x += subdivisionSize)
	{
		for (GLfloat y = -600; y < 600; y += subdivisionSize)
		{
			// The calculation of the x points is based on the following pseudocode:
			//heightAtVertex = sin(valueBasedOnPosition + phase + timeValue) * waveAmplitude

			// Multiply the frequency by the x + z (+ subDivisionSize) to properly set the wavelength
			GLfloat z1 = (sinf((x + y) * frequency + wavePhase + waveTimeValue) * waveAmplitude) + waveHeightOffset;
			GLfloat z2 = (sinf((x + subdivisionSize + y) * frequency + wavePhase + waveTimeValue) * waveAmplitude) + waveHeightOffset;
			GLfloat z3 = (sinf((x + subdivisionSize + y + subdivisionSize) * frequency + wavePhase + waveTimeValue) * waveAmplitude) + waveHeightOffset;
			GLfloat z4 = (sinf((x + y + subdivisionSize) * frequency + wavePhase + waveTimeValue) * waveAmplitude) + waveHeightOffset;

			// Set the vertices to the x, y, and z values with their respective offsets
			Vertex3 v1 = { x, y, z1 };
			Vertex3 v2 = { x + subdivisionSize, y, z2 };
			Vertex3 v3 = { x + subdivisionSize, y + subdivisionSize, z3 };
			Vertex3 v4 = { x, y + subdivisionSize, z4 };

			// Find the normals between the two triangles we are drawing
			Vertex3 normal1 = calculateNormal(v1, v2, v3);
			normalizeVector(&normal1);

			Vertex3 normal2 = calculateNormal(v1, v3, v4);
			normalizeVector(&normal2);

			// Draw the two triangles, setting the normal each triangle
			glBegin(GL_TRIANGLES);
			glNormal3f(normal1.position[0], normal1.position[1], normal1.position[2]);
			glVertex3f(v1.position[0], v1.position[1], v1.position[2]);
			glVertex3f(v2.position[0], v2.position[1], v2.position[2]);
			glVertex3f(v3.position[0], v3.position[1], v3.position[2]);
			
			glNormal3f(normal2.position[0], normal2.position[1], normal2.position[2]);
			glVertex3f(v1.position[0], v1.position[1], v1.position[2]);
			glVertex3f(v3.position[0], v3.position[1], v3.position[2]);
			glVertex3f(v4.position[0], v4.position[1], v4.position[2]);
			glEnd();
		}
	}
	glDisable(GL_LIGHTING);
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

	//printf("Camera: ( %.2f, %.2f, %.2f ); Submarine: ( %.2f, %.2f, %.2f )\n", newCamX, newCamY, newCamZ, submarineX, submarineY, submarineZ);

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
	if (key == 'b' || key == 'B')
	{
		isDrawingFog = !isDrawingFog;
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

	waveTimeValue += waveVelocity;
	if (waveTimeValue > 100000) waveTimeValue = 0;

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

	drawFog();

	drawSubmarine();

	drawBottomDisc();
	drawCylinderWall();

	drawCoral();

	drawWave();

	drawUnitVectors();

	glutSwapBuffers();
}

/*
* Function that handles window reshaping by calling gluPerspective when the 
* window is resized.
*/
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
	allocateObject(&submarine);

	FILE* file = fopen("submarine - updated.obj", "r");
	if (!file)
	{
		printf("No file found\n");
		return 1;
	}

	allocateAndPopulateHelper(file, &submarine);
	fclose(file);

	printf("Success allocating for submarine\n");
}

void initCoral()
{
	char* coralFilePaths[14] = { "coral/coral_1.obj", "coral/coral_2.obj", "coral/coral_3.obj" ,
	"coral/coral_4.obj", "coral/coral_5.obj", "coral/coral_6.obj", "coral/coral_7.obj", 
	"coral/coral_8.obj", "coral/coral_9.obj", "coral/coral_10.obj", "coral/coral_11.obj", 
	"coral/coral_12.obj", "coral/coral_13.obj", "coral/coral_14.obj" };
	for (GLint i = 0; i < 14; i++)
	{
		allocateObject(&coral[i]);

		FILE* file = fopen(coralFilePaths[i], "r");
		if (!file)
		{
			printf("No coral file found for index: %d\n", i);
			return 1;
		}

		allocateAndPopulateHelper(file, &coral[i]);
		fclose(file);
		
		printf("Success allocating for coral at %d\n", i);

		// Set the coral positions to some random position
		GLint x = (int)generateRandomFloat(-400, 400);
		GLint y = (int)generateRandomFloat(-400, 400);

		coralPositions[i].position[0] = x;
		coralPositions[i].position[1] = y;
		coralPositions[i].position[2] = 0;
	}
}

// Method to initialize data and textures
void init()
{
	initSub();
	initCoral();

	sandTexture = readPPM("spongebob-sand.ppm");
	printf("Initialized sand texture with ID: %u\n", sandTexture);
}

// Method to free the memory of all of the objects we allocated memory for
void freeObjects()
{
	free(submarine.groups->faceCount);
	free(submarine.groups->faces);
	free(submarine.values.vertices);
	free(submarine.values.normals);
}

// The main method that ties everything together
int main(int argc, char** argv)
{
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

	init();

	glutIdleFunc(idleScene);

	initializeGL();
	glutMainLoop();

	freeObjects();
}