//C stdlib
#include <stdio.h>
#include <math.h>

//Android GL headers
//#include <GLES/egl.h>
#include <GLES/gl.h>

//Android libui headers
//#include <ui/EventHub.h>


//androidgl helper functions
extern bool aglCreateWindow();
extern void aglSwapBuffers();
extern void aglShutdown();

//Getters
extern int aglGetScreenWidth();
extern int aglGetScreenHeight();
//extern int aglGetKeyState(int key);

//GLU functions
extern void gluPerspective(double fovy, double aspect, 
						   double zNear, double zFar);	 
						   
extern void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
          GLfloat centerx, GLfloat centery, GLfloat centerz,
          GLfloat upx, GLfloat upy, GLfloat upz);


