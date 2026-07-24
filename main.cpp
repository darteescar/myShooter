

#include<stdio.h>
#include<stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>

#include <IL/il.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

unsigned int t, tw, th;
unsigned char *imageData;
float *vertexB;  // Array de vértices
GLuint VBO;       // Vertex Buffer Object

// Função auxiliar: retorna a altura de um pixel (coluna i, linha j)
float h(int i, int j) {
	// i é coluna (x), j é linha (z)
	if (i < 0 || i >= tw || j < 0 || j >= th) {
		return 0.0f;
	}
	unsigned char intensity = imageData[j * tw + i];
	// Escalar de 0-255 para 0-30 metros
	return (intensity / 255.0f) * 50.0f;
}

float height(int px, int pz){
	float x1 = floor(px);
	float x2 = x1+1;
	float z1 = floor(pz);
	float z2 = z1+1;
	int fz = pz - z1;
	int fx = px - x1;
	float h_x1_z = h(x1,z1) * (1-fz) + h(x1,z2) * fz;
	float h_x2_z = h(x2,z1) * (1-fz) + h(x2,z2) * fz;
	float height_xz = h_x1_z * (1 - fx) + h_x2_z * fx;
	return height_xz;
}

float camX = 0, camZ = 0;
float camY = 0;

float alpha = 0; // direção (rotação horizontal)
float beta = 0;  // rotação vertical
float eyeHeight = 2.0f;

int startX, startY, tracking = 0;

bool keys[256] = {false};

void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window with zero width).
	if(h == 0)
		h = 1;

	// compute window's aspect ratio 
	float ratio = w * 1.0 / h;

	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the correct perspective
	gluPerspective(45,ratio,1,1000);

	// return to the model view matrix mode
	glMatrixMode(GL_MODELVIEW);
}

void drawTerrain() {

    	// Desenhar usando glDrawArrays com TRIANGLE_STRIP
	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	
	glColor3f(0.0f, 1.0f, 0.0f);
	
	// Desenhar cada linha como um triangle strip
	// Cada linha tem 2 * tw vértices (linha atual + linha seguinte)
	for (int i = 0; i < th - 1; i++) {
		int first = i * 2 * tw;  // Índice do primeiro vértice desta linha
		int count = 2 * tw;       // Número de vértices (2 linhas)
		glDrawArrays(GL_TRIANGLE_STRIP, first, count);
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
}

void processKeys() {

    	float speed = 0.5f;

	float dx = 0.0f;
	float dz = 0.0f;

	if (keys['w']) {
	dx += sin(alpha * M_PI / 180.0f);
	dz += cos(alpha * M_PI / 180.0f);
	}

	if (keys['s']) {
	dx -= sin(alpha * M_PI / 180.0f);
	dz -= cos(alpha * M_PI / 180.0f);
	}

	if (keys['a']) {
	dx += cos(alpha * M_PI / 180.0f);
	dz -= sin(alpha * M_PI / 180.0f);
	}

	if (keys['d']) {
	dx -= cos(alpha * M_PI / 180.0f);
	dz += sin(alpha * M_PI / 180.0f);
	}

	float len = sqrt(dx * dx + dz * dz);
	if (len > 0.0f) {
	dx /= len;
	dz /= len;
	}

	camX += dx * speed;
	camZ += dz * speed;
}

void renderScene(void) {

	float pos[4] = {-1.0, 1.0, 1.0, 0.0};

	processKeys();

	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	camY = height(camX + tw/2, camZ + th/2) + eyeHeight;

    // direção para onde olha (com rotação vertical)
    float lookX = camX + sin(alpha * M_PI / 180.0f) * cos(beta * M_PI / 180.0f);
    float lookY = camY + sin(beta * M_PI / 180.0f);
    float lookZ = camZ + cos(alpha * M_PI / 180.0f) * cos(beta * M_PI / 180.0f);

    	gluLookAt(camX, camY, camZ,
              lookX, lookY, lookZ,
              0.0f, 1.0f, 0.0f);

	drawTerrain();

	srand(67);
	int arvores = 0;
	while (arvores < 50) {
		float x = (rand() * 1.0f) / RAND_MAX * 200 - 100;
		float z = (rand() * 1.0f) / RAND_MAX * 200 - 100;
		if ( (x*x + z*z) < 50.0f*50.0f ) continue;

		glPushMatrix();
		glTranslatef(x, height(x + tw/2, z + th/2), z);
		glColor3f(0.588f, 0.294f, 0.0f);
		glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
		glutSolidCone(2.0f, 5.0f, 10, 10);
		glTranslatef(0.0f, 0.0f, 3.0f);
		glColor3f(0.0f, 1.0f, 0.0f);
		glutSolidCone(4.0f, 10.0f, 10, 10);
		glPopMatrix();
		arvores++;
	}

	glutSwapBuffers();
}

void processMouseMotion(int xx, int yy) {

	if (!tracking)
		return;

	int deltaX = xx - startX;
	int deltaY = yy - startY;

	if (tracking == 1) {
		// Rotação horizontal e vertical
		alpha -= deltaX * 0.5f;
		beta -= deltaY * 0.5f;

		// Limitar rotação vertical para não dar volta de cabeça
		if (beta > 85.0f)
			beta = 85.0f;
		else if (beta < -85.0f)
			beta = -85.0f;
	}

    startX = xx;
    startY = yy;
    glutPostRedisplay();
}

void processMouseButtons(int button, int state, int xx, int yy) {
	
	if (state == GLUT_DOWN)  {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else
			tracking = 0;
	}
	else if (state == GLUT_UP) {
		tracking = 0;
	}
}

void keyDown(unsigned char key, int x, int y) {
    keys[key] = true;
}

void keyUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

void init() {
	ilInit();

// 	Load the height map "terreno.jpg"
	ilGenImages(1,&t);
	ilBindImage(t);
	ilLoadImage((ILstring)"terreno.jpg");
	ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	tw = ilGetInteger(IL_IMAGE_WIDTH);
	th = ilGetInteger(IL_IMAGE_HEIGHT);
	if (tw != 256 || th != 256) {
		printf("The image should be 256x256 pixels\n");
		exit(0);
	}
	imageData = ilGetData();

// 	Build the vertex arrays
	// Calcular número total de vértices: (th-1) linhas, cada com 2*tw vértices
	int totalVertices = (th - 1) * 2 * tw;
	vertexB = new float[totalVertices * 3];  // 3 coordenadas por vértice
	int index = 0;
	
	for (int i = 0; i < th - 1; i++) {
		for (int j = 0; j < tw; j++) {
			// Vértice na linha atual (linha i)
			float x = j - tw/2.0f;
			float y = h(j, i);
			float z = i - th/2.0f;
			
			vertexB[index++] = x;
			vertexB[index++] = y;
			vertexB[index++] = z;

			// Vértice na linha seguinte (linha i+1)
			x = j - tw/2.0f;
			y = h(j, i + 1);
			z = (i + 1) - th/2.0f;

			vertexB[index++] = x;
			vertexB[index++] = y;
			vertexB[index++] = z;
		}
	}

	// Criar VBO
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, totalVertices * 3 * sizeof(float), vertexB, GL_STATIC_DRAW);

// 	OpenGL settings
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
}


int main(int argc, char **argv) {

// init GLUT and the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(320,320);
	glutCreateWindow("CG@DI-UM");
	
	glPolygonMode(GL_FRONT , GL_LINE);

	glewInit();

// Required callback registry 
	glutDisplayFunc(renderScene);
	glutIdleFunc(renderScene);
	glutReshapeFunc(changeSize);

// Callback registration for keyboard processing
	glutKeyboardFunc(keyDown);
	glutKeyboardUpFunc(keyUp);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);

	init();	

// enter GLUT's main cycle
	glutMainLoop();
	
	return 0;
}

