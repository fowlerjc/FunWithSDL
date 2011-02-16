/*
===========================================================================
File:		 camera.c
Author: 	 Clinton Freeman
Created on:  Feb 7, 2011
Description: This file is designed to help you create a simple first person
			 camera in your program. You are free to use the entire file
			 as a base, or to pick and choose bits that you find useful.
			 The main functions to pay attention to are:
			 	 - input_mouseMove
			 	 - input_update
			 	 - camera_translateForward
			 	 - camera_translateStrafe
			 	 - r_setupModelview
			 Of course, you should spend some time looking at how each
			 function interacts to create the effect of a camera. Here are
			 some ideas that might be worthy of extra credit (to be done
			 after you finish the required part):
			 	 - learn about quaternions and use them instead of Euler
			 	   angles to implement the same functionality. (recommended!)
			 	 - create an isometric or third person camera.
			 	 - investigate splines and how they are related to cinematic
			 	   camera paths, produce a simple demo.
===========================================================================
*/

#include "headers/SDL/SDL.h"
#include "headers/SDL/SDL_main.h"
#include "headers/SDL/SDL_opengl.h"

#include "headers/mathlib.h"
#include <stdio.h>

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

static void r_init();
static void r_setupProjection();
static void r_setupModelview();
static void r_drawFrame();

/*
 * SDL_main
 * Program entry point.
 */
int SDL_main(int argc, char* argv[])
{
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
	camera_rotateX(dy/2.0);
	camera_rotateY(dx/2.0);

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
		camera_translateForward(-0.01);
	if(keys_down[SDLK_s])
		camera_translateForward(0.01);
	if(keys_down[SDLK_a])
		camera_translateStrafe(-0.01);
	if(keys_down[SDLK_d])
		camera_translateStrafe(0.01);

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
	//If we wish to move forward, we must first calculate
	//a movement vector to add to our current position.
	//Depending upon the type of camera, this behavior
	//will differ. "Free" cameras typically allow the
	//user to fly around wherever they are looking.
	//"Person" cameras generally do not take into
	//account elevation, since in real life when you look
	//up you do not fly upward (although this would be awesome).

	//Since the only information we're storing about the
	//camera is the orientation in terms of degrees of rotation
	//about each cardinal axis, this representation must be converted
	//to a cartesian vector.

	//The conversion will follow a similar pattern to your standard
	//polar -> cartesian conversion, although this can be a little
	//confusing since we are looking down the negative Z axis.

	//Since we know that the Y axis points "up", the angle of rotation
	//around this axis will correspond to the theta found in typical polar
	//coordinates. You will need to play around with making terms negative
	//or positive, etc. in order to figure out exactly what the correct
	//conversion is. Although you could figure this out a priori using
	//diagrams and such, it is much easier and faster to use a guess
	//and check method, since you will know you've found the correct
	//configuration when it behaves how you want.

	//Once you have the "person" camera working, you can then begin
	//thinking about how to add in the Y (up) component. You know that
	//if the angle between the XZ plane and the Y axis is zero, the
	//vector should not have a Y component, and but the vector inside
	//the XZ plane should have a magnitude of one. Additionally, you
	//know that when the angle is 90 (i.e. you're looking "up"), the
	//Y component should be 1.0 and the magnitude of the X and Z components
	//should be 0.0. Given this, think of a term that you could multiply
	//each component by to achieve this effect. What is sin(0degrees)?
	//sin(90degrees)? cos(0degrees)? cos(90degrees)?
	float dx, dy, dz;

	//Free
	//dx =  ???;
	//dy =  ???;
	//dz =  ???;

	//Person
	//dx =  ???;
	//dy =  ???;
	//dz =  ???;

	camera.position[_X] += dx;
	camera.position[_Y] += dy;
	camera.position[_Z] += dz;
}

static void camera_translateStrafe(float dist)
{
	//This function will look very similar to your translateForward function.
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

	//Game Engine Architecture talks about row vector vs column vector
	//conventions on page 153. It chooses to use row vector
	//while OpenGL represents vectors as columns. Thus, the
	//canonical matrices that are given in the book (page 157)
	//must first be transposed to fit OpenGL's format.
	//A second complication is that OpenGL differs from C
	//in that matrices are stored column-major, not row-major.
	//The Red Book suggests using a flat, 1-dimensional
	//16 element array instead of the intuitive 4x4 2-dimensional
	//array you might naturally choose to use. If you choose to
	//do this, here are the correct indices:
	//0 4 8  12
	//1 5 9  13
	//2 6 10 14
	//3 7 11 15

	//X rotation matrix from book (for multiplying row vectors rM = r')
	//1  0      0      0
	//0  cos(r) sin(r) 0
	//0 -sin(r) cos(r) 0
	//0  0      0      1
	//Transpose (for multiplying column vectors Mc = c')
	//1 0       0      0
	//0 cos(r) -sin(r) 0
	//0 sin(r)  cos(r) 0
	//0 0       0      1
	//Indices
	//0 4 8  12
	//1 5 9  13
	//2 6 10 14
	//3 7 11 15
	sinX = sin(camera.angles_rad[_X]);
	cosX = cos(camera.angles_rad[_X]);

	//xRotMatrix[??] = ??;
	//xRotMatrix[??] = ??;
	//xRotMatrix[??] = ??;
	//xRotMatrix[??] = ??;

	//Y rotation matrix from book (for multiplying row vectors rM = r')
	//cos(r) 0 -sin(r) 0
	//0      1  0      0
	//sin(r) 0  cos(r) 0
	//0      0  0      1
	//Transpose (for multiplying column vectors Mc = c')
	// cos(r) 0 sin(r) 0
	// 0      1 0      0
	//-sin(r) 0 cos(r) 0
	// 0      0 0      1
	//Indices
	//0 4 8  12
	//1 5 9  13
	//2 6 10 14
	//3 7 11 15
	sinY = sin(camera.angles_rad[_Y]);
	cosY = cos(camera.angles_rad[_Y]);

	//yRotMatrix[??] = ??;
	//yRotMatrix[??] = ??;
	//yRotMatrix[??] = ??;
	//yRotMatrix[??] = ??;

	//Z rotation matrix from book (for multiplying row vectors rM = r')
	// cos(r) sin(r) 0 0
	//-sin(r) cos(r) 0 0
	// 0      0      1 0
	// 0      0      0 1
	//Transpose (for multiplying column vectors Mc = c')
	//cos(r) -sin(r) 0 0
	//sin(r)  cos(r) 0 0
	//0       0      1 0
	//0       0      0 1
	//Indices
	//0 4 8  12
	//1 5 9  13
	//2 6 10 14
	//3 7 11 15
	sinZ = sin(camera.angles_rad[_Z]);
	cosZ = cos(camera.angles_rad[_Z]);

	//zRotMatrix[??] = ??;
	//zRotMatrix[??] = ??;
	//zRotMatrix[??] = ??;
	//zRotMatrix[??] = ??;

	//Translation matrix from book (for multiplying row vectors rM = r')
	//1 0 0 0
	//0 1 0 0
	//0 0 1 0
	//x y z 1
	//Transpose (for multiplying column vectors Mc = c')
	//1 0 0 x
	//0 1 0 y
	//0 0 1 z
	//0 0 0 1
	//Indices
	//0 4 8  12
	//1 5 9  13
	//2 6 10 14
	//3 7 11 15

	//translateMatrix[??] = ??;
	//translateMatrix[??] = ??;
	//translateMatrix[??] = ??;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glMultMatrixf(??);
	//glMultMatrixf(??);
	//glMultMatrixf(??);
	//glMultMatrixf(??);
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

	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_TRIANGLES);
		glVertex3f(-0.5, -0.5, -8.0);
		glVertex3f( 0.5, -0.5, -8.0);
		glVertex3f( 0.0,  0.5, -8.0);
	glEnd();

	SDL_GL_SwapBuffers();
}
