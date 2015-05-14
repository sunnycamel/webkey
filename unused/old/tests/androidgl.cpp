#include "androidgl.h"

//EGL globals
static EGLDisplay iEglDisplay; 	
static EGLConfig  iEglConfig;   	
static EGLContext iEglContext; 	
static EGLSurface iEglSurface;	

//Screen size. This can change from device to device - we autodetect the size
//that we need in aglCreateWindow(). The user application can then retrieve this
//with aglGetScreenWidth/Height.
static int iScreenX = 480;
static int iScreenY = 320;	

int id = 7;

EGLint attribList [] = 
{ 
	EGL_BUFFER_SIZE, 16, //16 bit color
	EGL_DEPTH_SIZE, 15,
	EGL_NONE 
};	



/**************************************
         Simple getters
**************************************/
int aglGetScreenWidth(){
	return iScreenX;
}

int aglGetScreenHeight(){
	return iScreenY;
}

/**************************************
 Creates the OpenGL window + context
**************************************/
bool aglCreateWindow(){
		
	//Start up the display
	iEglDisplay = eglGetDisplay (EGL_DEFAULT_DISPLAY);
	if(iEglDisplay == EGL_NO_DISPLAY){
		printf("Unable to find a  suitable EGLDisplay\n");
		return false;
	}
		
	if(!eglInitialize(iEglDisplay, 0, 0)){
		printf("Couldn't init display\n");
		return false;
	}
	
	EGLint numConfigs;
	
	if(!eglChooseConfig(iEglDisplay, attribList, &iEglConfig, 1, &numConfigs)){
		printf("Couldn't choose config\n");
		return false;
	}
	 
	iEglContext = eglCreateContext(iEglDisplay, iEglConfig, EGL_NO_CONTEXT, 0);
		
	if(iEglContext == 0){
		printf("Couldn't create context\n");
		return false;
	}
		
	NativeWindowType iWindow = android_createDisplaySurface();
	
	iEglSurface = eglCreateWindowSurface(iEglDisplay, iEglConfig, iWindow, 0);	
	
	if(iEglSurface == NULL){
		printf("Couldn't create surface\n");
		return false;
	}
	
	eglMakeCurrent(iEglDisplay, iEglSurface, iEglSurface, iEglContext);
	
	//Set some common GL state here
	glClearColor(0.0, 0.0, 0.0, 1.0);
	
	//Start up the eventhub
	mEventHub = new android::EventHub();
	mEventHub->incStrong(&id);
	
	if(mEventHub->errorCheck()){
		printf("Couldn't start the event listener (%d)\n", 
				mEventHub->errorCheck());
		return false;
	}
	
	return true;
}

/**************************************
	Swap the OpenGL buffer
**************************************/
void aglSwapBuffers(){

	glFlush();
		
	eglMakeCurrent(iEglDisplay, iEglSurface, iEglSurface, iEglContext);
	eglSwapBuffers(iEglDisplay, iEglSurface);
}


/**************************************
	Shuts down the window + context
**************************************/
void aglShutdown(){
	eglMakeCurrent(iEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(iEglDisplay, iEglSurface);
	eglDestroyContext(iEglDisplay, iEglContext);
	eglTerminate(iEglDisplay);
	
	//should shut down eventhub here...
	mEventHub->decStrong(&id);
}


/**************************************
	gluperspective implementation
**************************************/
void gluPerspective(double fovy, double aspect, double zNear, double zFar){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double xmin, xmax, ymin, ymax;
	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;
	glFrustumf(xmin, xmax, ymin, ymax, zNear, zFar);
}


/**************************************
	  glulookat implementation
**************************************/
void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
          GLfloat centerx, GLfloat centery, GLfloat centerz,
          GLfloat upx, GLfloat upy, GLfloat upz)
{
    GLfloat m[16];
    GLfloat x[3], y[3], z[3];
    GLfloat mag;
    
    /* Make rotation matrix */
    
    /* Z vector */
    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if (mag) {          /* mpichler, 19950515 */
        z[0] /= mag;
        z[1] /= mag;
        z[2] /= mag;
    }
    
    /* Y vector */
    y[0] = upx;
    y[1] = upy;
    y[2] = upz;
    
    /* X vector = Y cross Z */
    x[0] = y[1] * z[2] - y[2] * z[1];
    x[1] = -y[0] * z[2] + y[2] * z[0];
    x[2] = y[0] * z[1] - y[1] * z[0];
    
    /* Recompute Y = Z cross X */
    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = -z[0] * x[2] + z[2] * x[0];
    y[2] = z[0] * x[1] - z[1] * x[0];
    
    /* mpichler, 19950515 */
    /* cross product gives area of parallelogram, which is < 1.0 for
     * non-perpendicular unit-length vectors; so normalize x, y here
     */
    
    mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if (mag) {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }
    
    mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if (mag) {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }
    
#define M(row,col)  m[col*4+row]
    M(0, 0) = x[0];
    M(0, 1) = x[1];
    M(0, 2) = x[2];
    M(0, 3) = 0.0;
    M(1, 0) = y[0];
    M(1, 1) = y[1];
    M(1, 2) = y[2];
    M(1, 3) = 0.0;
    M(2, 0) = z[0];
    M(2, 1) = z[1];
    M(2, 2) = z[2];
    M(2, 3) = 0.0;
    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
#undef M
    glMultMatrixf(m);
    
    /* Translate Eye to Origin */
    glTranslatef(-eyex, -eyey, -eyez);
    
}
