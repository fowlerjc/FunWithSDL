/*
===========================================================================
File:		main.c
Author: 	Clinton Freeman
Created on:  	Feb 7, 2011
Description:	Texturing demo - you will need to change the path to the texture
		before this will work...
===========================================================================
*/

#include "headers/SDL/SDL.h"
#include "headers/SDL/SDL_main.h"
#include "headers/SDL/SDL_opengl.h"

#include "headers/mathlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768

static int user_exit = 0;

//INPUT DECLARATIONS

static void input_keyDown(SDLKey k);
static void input_keyUp(SDLKey k);
static void input_mouseMove(int dx, int dy);
static void input_update();

//CAMERA DECLARATIONS

typedef struct
{
	vec3_t	position;
	vec3_t	angles_deg;
	vec3_t	angles_rad;
} camera_t;

static camera_t camera;

static void camera_init();
static void camera_rotateX(float degree);
static void camera_rotateY(float degree);
static void camera_rotateZ(float degree);
static void camera_translateForward(float dist);
static void camera_translateStrafe(float dist);

//RENDERER DECLARATIONS

//NEW TEXTURE STUFF
static GLbyte* r_image_loadTGA(char *name, GLuint *glTexID, int *width, int *height, int *bpp, GLuint *type);

static void r_init();
static void r_setupProjection();
static void r_setupModelview();
static void r_drawFrame();

//Texture Globals
//My textures.
static GLuint face1, face1Type;
static int face1Width, face1Height, face1BPP;
static GLuint face2, face2Type;
static int face2Width, face2Height, face2BPP;
static GLuint face3, face3Type;
static int face3Width, face3Height, face3BPP;
static GLuint face4, face4Type;
static int face4Width, face4Height, face4BPP;
static GLuint face5, face5Type;
static int face5Width, face5Height, face5BPP;
static GLuint face6, face6Type;
static int face6Width, face6Height, face6BPP;

static GLbyte *face1data, *face2data, *face3data, *face4data, *face5data, *face6data;


/*
 * SDL_main
 * Program entry point.
 */
int SDL_main(int argc, char* argv[]){
	SDL_Event	event;
	SDL_Surface	*screen;

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
	{
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	SDL_WM_SetCaption("Camera Demo", "Camera Demo");
	SDL_ShowCursor(SDL_DISABLE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_OPENGL);
	if(!screen)
	{
		printf("Unable to set video mode: %s\n", SDL_GetError());
		return 1;
	}

	r_init();

	while(!user_exit)
	{
		//Handle input
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_KEYDOWN:
				input_keyDown(event.key.keysym.sym);
				break;
			case SDL_KEYUP:
				input_keyUp(event.key.keysym.sym);
				break;
			case SDL_MOUSEMOTION:
				input_mouseMove(event.motion.x, event.motion.y);
				break;
			case SDL_QUIT:
				user_exit = 1;
			}
		}

		input_update();
		r_drawFrame();
	}

	SDL_Quit();
	return 0;
}

/*
===========================================================================
	INPUT
===========================================================================
*/

static int keys_down[256];

static void input_keyDown(SDLKey k) { keys_down[k] = 1; if(k == SDLK_ESCAPE || k == SDLK_q) user_exit = 1; }
static void input_keyUp  (SDLKey k) { keys_down[k] = 0; }

/*
 * input_mouseMove
 */
void input_mouseMove(int dx, int dy)
{
	float halfWinWidth, halfWinHeight;

	halfWinWidth  = (float)WINDOW_WIDTH  / 2.0;
	halfWinHeight = (float)WINDOW_HEIGHT / 2.0;

	dx -= halfWinWidth; dy -= halfWinHeight;

	//Feed the deltas to the camera
	camera_rotateX(-dy/2.0);
	camera_rotateY(-dx/2.0);

	//Reset cursor to center
	SDL_WarpMouse(halfWinWidth, halfWinHeight);
}

/*
 * input_update
 */
static void input_update()
{
	//WASD
	//The input values are arbitrary
	if(keys_down[SDLK_w])
		camera_translateForward(0.05);
	if(keys_down[SDLK_s])
		camera_translateForward(-0.05);
	if(keys_down[SDLK_a])
		camera_translateStrafe(0.05);
	if(keys_down[SDLK_d])
		camera_translateStrafe(-0.05);

	//Reset, sometimes you can get pretty lost...
	if(keys_down[SDLK_r])
	{
		VectorClear(camera.angles_deg);
		VectorClear(camera.angles_rad);
	}
}

/*
===========================================================================
	CAMERA
===========================================================================
*/

//Maintain a matrix for each rotation and one for translation
static float xRotMatrix[16], yRotMatrix[16], zRotMatrix[16], translateMatrix[16];

static void camera_init()
{
	glmatrix_identity(xRotMatrix);
	glmatrix_identity(yRotMatrix);
	glmatrix_identity(zRotMatrix);
	glmatrix_identity(translateMatrix);
}

//Rotations just increase/decrease the angle and compute a new radian value.
static void camera_rotateX(float degree)
{
	camera.angles_deg[_X] += degree;
	camera.angles_rad[_X] = camera.angles_deg[_X] * M_PI_DIV180;
}

static void camera_rotateY(float degree)
{
	camera.angles_deg[_Y] += degree;
	camera.angles_rad[_Y] = camera.angles_deg[_Y] * M_PI_DIV180;
}

static void camera_rotateZ(float degree)
{
	camera.angles_deg[_Z] += degree;
	camera.angles_rad[_Z] = camera.angles_deg[_Z] * M_PI_DIV180;
}

static void camera_translateForward(float dist)
{
	float sinX, cosX, sinY, cosY, dx, dy, dz;

	sinY = sin(camera.angles_rad[_Y]);
	cosY = cos(camera.angles_rad[_Y]);

	sinX = sin(camera.angles_rad[_X]);
	cosX = cos(camera.angles_rad[_X]);

	//Free
	dx =  -sinY * cosX * dist;
	dy =  sinX * dist;
	dz =  -cosY * cosX * dist;

	//Person
	//dx =  -sinY * dist;
	//dy =  0.0;
	//dz =  -cosY * dist;

	camera.position[_X] += dx;
	camera.position[_Y] += dy;
	camera.position[_Z] += dz;
}

static void camera_translateStrafe(float dist)
{
	float sinX, cosX, sinY, cosY, dx, dy, dz, yPlus90;

	yPlus90 = (camera.angles_deg[_Y] + 90.0) * M_PI_DIV180;

	sinY = sin(yPlus90);
	cosY = cos(yPlus90);

	sinX = sin(camera.angles_rad[_X]);
	cosX = cos(camera.angles_rad[_X]);

	//Free
	//dx =  -sinY * cosX * dist;
	//dy =  0.0;
	//dz =  -cosY * cosX * dist;

	//Person
	dx =  -sinY * dist;
	dy =  0.0;
	dz =  -cosY * dist;

	camera.position[_X] += dx;
	camera.position[_Y] += dy;
	camera.position[_Z] += dz;
}

/*
===========================================================================
	TGA LOADING
===========================================================================
*/

#define HEADER_SIZE 18

typedef unsigned char byte;

typedef struct
{
	unsigned char 	idLength, colormapType, imageType;
	unsigned char	colormapSize;
	unsigned short	colormapIndex, colormapLength;
	unsigned short	xOrigin, yOrigin, width, height;
	unsigned char	pixelSize, attributes;
}
tgaHeader_t;

/*
 * Function: renderer_img_loadTGA
 * Description: Loads a TARGA image file, uploads to GL, and returns the
 * texture ID. Only supports 24/32 bit.
 */
static GLbyte* r_image_loadTGA(char *name, GLuint *glTexID, int *width, int *height, int *bpp, GLuint *type)
{
	int				dataSize, rows, cols, i, j;
	GLbyte			*buf, *imageData, *pixelBuf, red, green, blue, alpha;

	FILE 			*file;
	tgaHeader_t		header;
	struct stat 	st;

	file = fopen(name, "rb");

	if(file == NULL)
	{
		printf("Loading TGA: %s, failed. Null file pointer.\n", name);
		return NULL;
	}

	if(stat(name, &st))
	{
		printf("Loading TGA: %s, failed. Could not determine file size.\n", name);
		return NULL;
	}

	if(st.st_size < HEADER_SIZE)
	{
		printf("Loading TGA: %s, failed. Header too short.\n", name);
		return NULL;
	}

	buf = (byte *)malloc(st.st_size);
	fread(buf, sizeof(byte), st.st_size, file);

	fclose(file);

	memcpy(&header.idLength, 	 	&buf[0],  1);
	memcpy(&header.colormapType, 	&buf[1],  1);
	memcpy(&header.imageType, 		&buf[2],  1);
	memcpy(&header.colormapIndex, 	&buf[3],  2);
	memcpy(&header.colormapLength,  &buf[5],  2);
	memcpy(&header.colormapSize, 	&buf[7],  1);
	memcpy(&header.xOrigin,			&buf[8],  2);
	memcpy(&header.yOrigin,			&buf[10], 2);
	memcpy(&header.width,			&buf[12], 2);
	memcpy(&header.height,			&buf[14], 2);
	memcpy(&header.pixelSize,		&buf[16], 1);
	memcpy(&header.attributes,		&buf[17], 1);

	//Advance past the header
	buf += HEADER_SIZE;

	if(header.pixelSize != 24 && header.pixelSize != 32)
	{
		printf("Loading TGA: %s, failed. Only support 24/32 bit images.\n", name);
		return NULL;
	}
	else if(header.pixelSize == 24)
		*type = GL_RGB;
	else
		*type = GL_RGBA;

	//Determine size of image data chunk in bytes
	dataSize = header.width * header.height * (header.pixelSize / 8);

	//Set up our texture
	*bpp 	 	= header.pixelSize;
	*width  	= header.width;
	*height 	= header.height;

	imageData = (byte *)malloc(dataSize);
	rows	  = *height;
	cols	  = *width;

	if(*type == GL_RGB)
	{
		for(i = 0; i < rows; i++)
		{
			pixelBuf = imageData + (i * cols * 3);
			for(j = 0; j < cols; j++)
			{
				blue 	= *buf++;
				green 	= *buf++;
				red		= *buf++;

				*pixelBuf++ = red;
				*pixelBuf++ = green;
				*pixelBuf++ = blue;
			}
		}
	}
	else
	{
		for(i = 0; i < rows; i++)
		{
			pixelBuf = imageData + (i * cols * 4);
			for(j = 0; j < cols; j++)
			{
				blue 	= *buf++;
				green 	= *buf++;
				red		= *buf++;
				alpha	= *buf++;

				*pixelBuf++ = red;
				*pixelBuf++ = green;
				*pixelBuf++ = blue;
				*pixelBuf++ = alpha;
			}
		}
	}

	/*Upload the texture to OpenGL
	glGenTextures(1, glTexID);
	glBindTexture(GL_TEXTURE_2D, *glTexID);

	//Default OpenGL settings have GL_TEXTURE_MAG/MIN_FILTER set to use
	//mipmaps... without these calls texturing will not work properly.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//Upload image data to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, type, *width, *height,
			0, type, GL_UNSIGNED_BYTE, imageData);
	*/

	//Header debugging
	/*
	printf("Attributes: %d\n", 				header.attributes);
	printf("Colormap Index: %d\n", 			header.colormapIndex);
	printf("Colormap Length: %d\n", 		header.colormapLength);
	printf("Colormap Size: %d\n", 			header.colormapSize);
	printf("Colormap Type: %d\n", 			header.colormapType);
	printf("Height: %d\n", 					header.height);
	printf("Identification Length: %d\n",	header.idLength);
	printf("Image Type: %d\n", 				header.imageType);
	printf("Pixel Size: %d\n", 				header.pixelSize);
	printf("Width: %d\n", 					header.width);
	printf("X Origin: %d\n", 				header.xOrigin);
	printf("Y Origin: %d\n", 				header.yOrigin);
	*/
	return imageData;
}

/*
===========================================================================
	RENDERER
===========================================================================
*/

/*
 * r_init
 * Perform any one-time GL state changes.
 */
static void r_init()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	//NEW TEXTURE STUFF
	glEnable(GL_TEXTURE_2D);
	//You might want to play with changing the modes
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//These functions load my custom textures.
	//r_image_loadTGA("C:/Users/Cory/workspace/FunWithSDL/Debug/textures/uparrow.tga",
	//		&myGLTexture, &myTexWidth, &myTexHeight, &myTexBPP);
	face1data = r_image_loadTGA("C:/Users/Cory/workspace/FunWithSDL/Debug/textures/face1.tga",
			&face1, &face1Width, &face1Height, &face1BPP, &face1Type);
	face2data = r_image_loadTGA("C:/Users/Cory/workspace/FunWithSDL/Debug/textures/face2.tga",
			&face2, &face2Width, &face2Height, &face2BPP, &face2Type);
	face3data = r_image_loadTGA("C:/Users/Cory/workspace/FunWithSDL/Debug/textures/face3.tga",
			&face3, &face3Width, &face3Height, &face3BPP, &face3Type);
	face4data = r_image_loadTGA("C:/Users/Cory/workspace/FunWithSDL/Debug/textures/face4.tga",
			&face4, &face4Width, &face4Height, &face4BPP, &face4Type);
	face5data = r_image_loadTGA("C:/Users/Cory/workspace/FunWithSDL/Debug/textures/face5.tga",
			&face5, &face5Width, &face5Height, &face5BPP, &face5Type);
	face6data = r_image_loadTGA("C:/Users/Cory/workspace/FunWithSDL/Debug/textures/face6.tga",
			&face6, &face6Width, &face6Height, &face6BPP, &face6Type);


	//*****************************************************************************
		glGenTextures(1, &face1);
		glBindTexture(GL_TEXTURE_2D, face1);

		//Default OpenGL settings have GL_TEXTURE_MAG/MIN_FILTER set to use
		//mipmaps... without these calls texturing will not work properly.
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		//Upload image data to OpenGL
		glTexImage2D(GL_TEXTURE_2D, 0, face1Type, face1Width, face1Height,
				0, face1Type, GL_UNSIGNED_BYTE, face1data);


	//*****************************************************************************
	glGenTextures(1, &face2);
	glBindTexture(GL_TEXTURE_2D, face2);

	//Default OpenGL settings have GL_TEXTURE_MAG/MIN_FILTER set to use
	//mipmaps... without these calls texturing will not work properly.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//Upload image data to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, face2Type, face2Width, face2Height,
			0, face2Type, GL_UNSIGNED_BYTE, face2data);

	//*************************************************************************
	glGenTextures(1, &face3);
	glBindTexture(GL_TEXTURE_2D, face3);

	//Default OpenGL settings have GL_TEXTURE_MAG/MIN_FILTER set to use
	//mipmaps... without these calls texturing will not work properly.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//Upload image data to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, face3Type, face3Width, face3Height,
		0, face3Type, GL_UNSIGNED_BYTE, face3data);

	//*************************************************************************
	glGenTextures(1, &face4);
	glBindTexture(GL_TEXTURE_2D, face4);

	//Default OpenGL settings have GL_TEXTURE_MAG/MIN_FILTER set to use
	//mipmaps... without these calls texturing will not work properly.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//Upload image data to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, face4Type, face4Width, face4Height,
		0, face4Type, GL_UNSIGNED_BYTE, face4data);

	//*************************************************************************
	glGenTextures(1, &face5);
	glBindTexture(GL_TEXTURE_2D, face5);

	//Default OpenGL settings have GL_TEXTURE_MAG/MIN_FILTER set to use
	//mipmaps... without these calls texturing will not work properly.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//Upload image data to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, face5Type, face5Width, face5Height,
		0, face5Type, GL_UNSIGNED_BYTE, face5data);
	//*************************************************************************
	glGenTextures(1, &face6);
	glBindTexture(GL_TEXTURE_2D, face6);

	//Default OpenGL settings have GL_TEXTURE_MAG/MIN_FILTER set to use
	//mipmaps... without these calls texturing will not work properly.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//Upload image data to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, face6Type, face6Width, face6Height,
		0, face6Type, GL_UNSIGNED_BYTE, face6data);

	camera_init();

	r_setupProjection();
}

/*
 * r_setupProjection
 * Calculates the GL projection matrix. Only called once.
 */
static void r_setupProjection()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 1.33, 0.5, 1024.0);
}

/*
 * r_setupModelview
 * Calculates the GL modelview matrix. Called each frame.
 */
static void r_setupModelview()
{
	float sinX, cosX, sinY, cosY, sinZ, cosZ;

	sinX = sin(-camera.angles_rad[_X]);
	cosX = cos(-camera.angles_rad[_X]);

	xRotMatrix[5]  = cosX;
	xRotMatrix[6]  = sinX;
	xRotMatrix[9]  = -sinX;
	xRotMatrix[10] = cosX;

	sinY = sin(-camera.angles_rad[_Y]);
	cosY = cos(-camera.angles_rad[_Y]);

	yRotMatrix[0]  =  cosY;
	yRotMatrix[2]  = -sinY;
	yRotMatrix[8]  =  sinY;
	yRotMatrix[10] =  cosY;

	sinZ = sin(-camera.angles_rad[_Z]);
	cosZ = cos(-camera.angles_rad[_Z]);

	zRotMatrix[0] = cosZ;
	zRotMatrix[1] = sinZ;
	zRotMatrix[4] = -sinZ;
	zRotMatrix[5] = cosZ;

	translateMatrix[12] = -camera.position[_X];
	translateMatrix[13] = -camera.position[_Y];
	translateMatrix[14] = -camera.position[_Z];

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(xRotMatrix);
	glMultMatrixf(yRotMatrix);
	glMultMatrixf(zRotMatrix);
	glMultMatrixf(translateMatrix);
}

/*
 * r_drawFrame
 * Perform any drawing and setup necessary to produce a single frame.
 */
static void r_drawFrame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Orient and position the camera
	r_setupModelview();

	glColor3f(1.0, 1.0, 1.0);

	glBindTexture(GL_TEXTURE_2D, face1);
	glBegin(GL_QUADS);
		//Set up appropriate texture for the forward face.

		//I've been visualizing this the wrong way. Each four is considered a square.
		//Face 1
		glTexCoord2f(0.0, 0.0);
			glVertex3f(-0.5, -0.5, -8.0);
		glTexCoord2f(1.0, 0.0);
			glVertex3f( 0.5, -0.5, -8.0);
		glTexCoord2f(1.0, 1.0);
			glVertex3f( 0.5,  0.5, -8.0);
		glTexCoord2f(0.0, 1.0);
			glVertex3f(-0.5,  0.5, -8.0);
	glEnd();

		//Face 2
		glBindTexture(GL_TEXTURE_2D, face4);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
			glVertex3f(-0.5, -0.5, -9.0);
		glTexCoord2f(1.0, 0.0);
			glVertex3f(-0.5, -0.5, -8.0);
		glTexCoord2f(1.0, 1.0);
			glVertex3f(-0.5, 0.5, -8.0);
		glTexCoord2f(0.0, 1.0);
			glVertex3f(-0.5, 0.5, -9.0);
	glEnd();

		//Face 3
		glBindTexture(GL_TEXTURE_2D, face3);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
			glVertex3f(0.5, -0.5, -8.0);
		glTexCoord2f(1.0, 0.0);
			glVertex3f(0.5, -0.5, -9.0);
		glTexCoord2f(1.0, 1.0);
			glVertex3f(0.5, 0.5, -9.0);
		glTexCoord2f(0.0, 1.0);
			glVertex3f(0.5, 0.5, -8.0);
		glEnd();

		//Face 4
		glBindTexture(GL_TEXTURE_2D, face6);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
			glVertex3f(0.5, -0.5, -9.0);
		glTexCoord2f(1.0, 0.0);
			glVertex3f(-0.5, -0.5, -9.0);
		glTexCoord2f(1.0, 1.0);
			glVertex3f(-0.5, 0.5, -9.0);
		glTexCoord2f(0.0, 1.0);
			glVertex3f(0.5, 0.5, -9.0);
			glEnd();

		//Face 5
		glBindTexture(GL_TEXTURE_2D, face5);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
			glVertex3f(-0.5, 0.5, -8.0);
		glTexCoord2f(1.0, 0.0);
			glVertex3f(0.5, 0.5, -8.0);
		glTexCoord2f(1.0, 1.0);
			glVertex3f(0.5, 0.5, -9.0);
		glTexCoord2f(0.0, 1.0);
			glVertex3f(-0.5, 0.5, -9.0);
		glEnd();

		//Face 6
		glBindTexture(GL_TEXTURE_2D, face2);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
			glVertex3f(0.5, -0.5, -8.0);
		glTexCoord2f(1.0, 0.0);
			glVertex3f(-0.5, -0.5, -8.0);
		glTexCoord2f(1.0, 1.0);
			glVertex3f(-0.5, -0.5, -9.0);
		glTexCoord2f(0.0, 1.0);
			glVertex3f(0.5, -0.5, -9.0);
	glEnd();

	SDL_GL_SwapBuffers();
}
