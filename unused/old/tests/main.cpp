#include "androidgl.h"

const GLbyte vertex []=
{
	0,1,0,
	-1,0,0,
	1,0,0
};

const GLubyte color []=
{
	255,0,0,
	0,255,0,
	0,0,255
};

int iRot = 0;

/**************************************
	 Entry
**************************************/
int main(int argc, char **argv){
	
	//Create the GL context
	if(!aglCreateWindow()){
		return 1;
	}
	
	//Get the aspect ration
	float ar = (float)aglGetScreenWidth() / (float)aglGetScreenHeight();
		
	printf("About to enter mainloop\n");

	//Mainloop
	while(true){
		
		//Set initial state
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();	
		gluPerspective(70.0f, ar ,0.5f, 1000.0f);	
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		//Camera
		gluLookAt(0,0,5, 0,0,0, 0,1,0);
				
		//Draw a triangle
		glRotatef(iRot, 0, 1, 0);
	
		glEnableClientState (GL_VERTEX_ARRAY);
		glEnableClientState (GL_COLOR_ARRAY);
		glVertexPointer (3, GL_BYTE , 0, vertex);	
		glColorPointer (3, GL_UNSIGNED_BYTE , 0, color);	
		glDrawArrays (GL_TRIANGLES, 0, 3);	
		
		//All done!
		aglSwapBuffers();
			
			
		//Keyboard input	
		if(aglGetKeyState('a')){
			iRot++;
		}
		if(aglGetKeyState('d')){
			iRot--;
		}
		
	}
		
	//Shut down the context
	aglShutdown();	

	return 0;
}
