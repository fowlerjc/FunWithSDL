// Now working on camera. Will keep this file for the time being.
#include "headers/SDL/SDL.h"
#include "headers/SDL/SDL_main.h"
#include "headers/SDL/SDL_opengl.h"
#include <stdio.h>

//New defines that we can use inside of mouse input handling to avoid magic numbers
#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768

//Means of the user exiting the main loop
static int user_exit = 0;

//Eventually these will be moved out of here into a "rendering" subsystem, which is why they are prefixed r_.
static void r_init();
static void r_drawFrame();

//Fancy new input declarations - we will also want to move this out of here.
static int keys_down[256];
static void input_update();
static void input_keyDown(SDLKey k);
static void input_keyUp(SDLKey k);
static void input_mouseMove(int xPos, int yPos);

//X and Y coordinates for vertices. These should most certainly not
//be global values. Ah well...
static float vx1 = -1.0, vx2 = 0.0, vx3 = -0.5;
static float vy1 = -0.5, vy2 = -0.5, vy3 = 0.5;

/*
 * SDL_main
 * Program entry point
 */

/*
int SDL_main(int argc, char* argv[])
{
	SDL_Event	event;
	SDL_Surface	*screen;

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
	{
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	//Second argument seems to make no difference.
	SDL_WM_SetCaption("Input Demo", NULL);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_OPENGL);
	if(!screen)
	{
		printf("Unable to set video mode: %s\n", SDL_GetError());
		return 1;
	}

	//Renderer
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
				exit(0);
			}
		}

		//Respond to keys being down
		input_update();

		//Draw the scene
		r_drawFrame();
	}

	SDL_Quit();
	return 0;
}
*/

//We simply flag whether a key is up or down when it actually occurs, and then
//do the logic in a single update later on. If you don't do it this way things like
//using input to move the camera ends up being super stuttery, due to whatever reason.

//My added input will use the arrow keys to move the object on screen.
//There HAS to be a better way to do this. However, I don't know it so...
//My suspicion is that a translation matrix could be employed but I couldn't
//figure out how.
static void input_keyDown(SDLKey k) {
	keys_down[k] = 1;
	if(k == SDLK_ESCAPE || k == SDLK_q)
		user_exit = 1;
	else if(k == SDLK_UP){
		vy1 += 1.0;
		vy2 += 1.0;
		vy3 += 1.0;
	}
	else if(k == SDLK_DOWN){
		vy1 -= 1.0;
		vy2 -= 1.0;
		vy3 -= 1.0;
	}
	else if(k == SDLK_LEFT){
		vx1 -= 1.0;
		vx2 -= 1.0;
		vx3 -= 1.0;
	}
	else if(k == SDLK_RIGHT){
		vx1 += 1.0;
		vx2 += 1.0;
		vx3 += 1.0;
	}
}
static void input_keyUp  (SDLKey k) { keys_down[k] = 0; }


//Mouse input is more complicated...
//The input event gives you where the mouse is located in screen coordinates.
//In our case, this means xPos will be between 0 and 1024, and yPos between
//0 and 768. Try printing out the values to get an idea of how the coordinates
//are oriented.
//Imagine how a first person camera typically works, and try to think of what is
//occurring in terms of matrix transformations. Since we know that the camera is
//looking down the -Z axis in the default OpenGL setup, and that we typically
//deal with rotations about axes, which rotations correspond to moving the mouse
//up/down or left/right?
//One other problem you will face is how to transform the screen coordinates of
//the mouse movement into a useful form to use inside of a rotation matrix.
//A simple way to achieve this is to reset the cursor to the middle of the window
//after each mouse movement. Then, you can get a delta movement out of the screen
//coordinates by simply subtracting half of the screen width/height. This can
//be translated into an angle of rotation directly, although you may need to scale
//the value (i.e. adjust the sensitivity).
static void input_mouseMove(int xPos, int yPos)
{
	float halfWinWidth, halfWinHeight,
		  dx, dy;

	halfWinWidth  = (float)WINDOW_WIDTH  / 2.0;
	halfWinHeight = (float)WINDOW_HEIGHT / 2.0;

	dx = xPos - halfWinWidth; dy = yPos - halfWinHeight;

	//Do something with deltas.

	//Reset cursor to center COMMENTED OUT TO COMBAT SEVERE IRRITATION
	//SDL_WarpMouse(halfWinWidth, halfWinHeight);
}

//This function is currently being called each time we run through the main loop.
//By the time it is executed, all keys will have been registered as down or up.
//You can use this information inside of a simple check to do things like
//move the camera in a certain direction.
static void input_update()
{
	if(keys_down['w'])
		printf("Move me forward please!");
}

/*
 * r_init
 * Initializes rendering state.
 */
static void r_init()
{
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 1.33, 1.0, 10.0);
}

/*
 * r_drawFrame
 * Produces a final image of the scene.
 */
static void r_drawFrame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Triangle Number 1
	glColor3f(1.0, 1.0, 0.0);
	glBegin(GL_TRIANGLES);
		//Leftmost vertex.
		glVertex3f(vx1, vy1, -5.0);
		//Rightmost vertex.
		glVertex3f(vx2, vy2, -5.0);
		//Center vertex.
		glVertex3f(vx3, vy3, -5.0);
	glEnd();

	SDL_GL_SwapBuffers();
}
