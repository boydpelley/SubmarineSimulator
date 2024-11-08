#include <freeglut.h>

typedef struct
{
	GLfloat position[3];
} Vertex3;

typedef struct
{
	GLfloat rgb[3];
} Color;

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

	gluLookAt(cameraPosition[0], cameraPosition[1], cameraPosition[2],
		cameraLookAt[0], cameraLookAt[1], cameraLookAt[2],
		0, 1, 0);

	drawUnitVectors();

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
	glClearColor(0, 0, 0, 1.0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (float)windowWidth / (float)windowHeight, 1.0f, 1000.0f);
	glMatrixMode(GL_MODELVIEW);
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
	glutKeyboardFunc(handleKeyboard);
	glutSpecialFunc(handleSpecialKeyboard);

	glutIdleFunc(idleScene);

	initializeGL();
	glutMainLoop();
}