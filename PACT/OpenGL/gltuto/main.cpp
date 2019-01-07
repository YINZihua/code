#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include <GL/glew.h>
#if defined(__DARWIN__) || defined(__APPLE__)

#include <OpenGL/gl.h>
#include <GL/glut.h>
#include <OpenGL/glext.h>

#else
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>
#endif


typedef enum
{
	OBJ_SQUARE=0,
	OBJ_CUBE,
	OBJ_TEAPOT,
} ObjMode;

#include <stdint.h>

//GlutWindow Parameters
	int width=500;
	int height=500;
	bool display_changed = false;
	ObjMode obj_mode = OBJ_SQUARE;

	float field_of_view_angle;
	float z_near;
	float z_far;
	int is_fullscreen=0;
	bool use_vbo=false;

	bool mouse_grab=false;
	int grab_x=0;
	int grab_y=0;
	float angle_x=0;
	float angle_y=0;

	int window_id = 0;
	int rgb_width=0, rgb_height=0;

	GLuint rgbTextureID=0;

	#define M_PI		3.1415926535898f

	int sleep_ms=100;

/*FPS compute*/
	int ms_since_start;
	long long unsigned int elapsed ;
	int nb_frames = 0;
	int time_spent = 0;
	int start_time = 0;

GLuint loadBmp(char *img_file, int *twidth, int *theight)
{
	GLuint texture_id;
	unsigned int i_width, i_height;
	FILE *img = fopen(img_file, "r");
	if (img == NULL) return 0;

	int read, nb_comp = 3;
	fseek(img, 18, SEEK_SET);

	fread((void *)&read, 4, 1, img);
	*twidth = i_width = read ;

	fread((void *)&read, 4, 1, img);
	*theight = i_height = read ;

	fread((void *)&read, 2, 1, img);
	fread((void *)&read, 2, 1, img);
	nb_comp = read;
	nb_comp /= 8;

	if (nb_comp==3) {
		fseek(img, 54, SEEK_SET); // skip the BMP header
	} else {
		fseek(img, 54 + 256 * 4, SEEK_SET); // skip the BMP header
	}
	fprintf(stderr, "Loading image %s - size %dx%d - %d comp\n", img_file, i_width, i_height, nb_comp);

	char *data = (char *) malloc(sizeof(char) * nb_comp * i_width * i_height);
	read = fread(data, 1, nb_comp * i_width * i_height, img);
	if (read != nb_comp * i_width * i_height) {
		fprintf(stderr, "error reading BMP bytes\n");
	}
	fclose(img);


	glGenTextures(1, (GLuint *) &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, nb_comp, i_width, i_height, 0, (nb_comp==3) ? GL_BGR : GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	free(data);

	return texture_id;
}

void sleep(int ms)
{
	#ifdef WIN32
		Sleep(ms);
	#else
		struct timeval tv;
		tv.tv_sec = ms/1000;
		tv.tv_usec = (ms%1000)*1000;
		select(0, NULL, NULL, NULL, &tv);
	#endif
}

/*******************************
********************************
******       SHADER        *****
********************************
*******************************/


GLuint vertexShader=0;
GLuint fragmentShader=0;
GLuint programID=0;

GLuint createShader(GLenum shaderType, char *ShaderTextFile)
{
	GLuint myShader;
	int size;
	char *shaderData;
	FILE *shaderFile = fopen(ShaderTextFile, "rt");
	if (!shaderFile) {
		fprintf(stderr, "Cannot open shader file %s\n", ShaderTextFile);
		return 0;
	}
	fseek(shaderFile, 0, SEEK_END);
	size = ftell(shaderFile);
	fseek(shaderFile, 0, SEEK_SET);

	shaderData = (char *)malloc(sizeof(char)*(size+1));
	fread(shaderData, 1, size, shaderFile);
	fclose(shaderFile);
	shaderData[size] = 0;
	myShader = glCreateShader(shaderType);

	glShaderSource(myShader, 1, (const GLchar**)	&shaderData, &size);

	glCompileShader(myShader);

	glGetShaderiv(myShader, GL_INFO_LOG_LENGTH, &size);

	if (size > 0) {
		int charsWritten  = 0;
		char *infoLog = (char *)malloc(size);
		glGetShaderInfoLog(myShader, size, &charsWritten, infoLog);
		if (charsWritten) {
			fprintf(stderr, "Error compiling shader %s: %s\n", ShaderTextFile, infoLog);
			//fprintf(stderr, "Shader code is %s\n", shaderData);
		} else {
			fprintf(stderr, "Shader %s compiled OK\n", ShaderTextFile);
		}
		free(infoLog);
	} else {
		fprintf(stderr, "Shader %s compiled OK\n", ShaderTextFile);
	}

	free(shaderData);
	return myShader;
}



bool createProgram(const char *vert_src, const char *frag_src)
{
	char *data;
	vertexShader = createShader(GL_VERTEX_SHADER, (char *)vert_src);
	fragmentShader = createShader(GL_FRAGMENT_SHADER, (char *) frag_src);

	programID = glCreateProgram();

	glAttachShader(programID, vertexShader);
	glAttachShader(programID, fragmentShader);

	glLinkProgram(programID);

	{
		int infologLength = 0;
		int charsWritten  = 0;
		char *infoLog;

		glGetProgramiv(programID, GL_INFO_LOG_LENGTH,&infologLength);

		if (infologLength > 0)
		{
			infoLog = (char *)malloc(infologLength);
			glGetProgramInfoLog(programID, infologLength, &charsWritten, infoLog);
			if (charsWritten)
				fprintf(stderr, "Error linking program: %s\n",infoLog);
			free(infoLog);

		}
	}
	GLint isLinked = 0;
	glGetProgramiv(programID, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		return false;
	}
	glUseProgram(0);
	return true;
}


void init_display()
{
	GLint baseImageLoc;
	glEnable(GL_TEXTURE_2D);

	//we use the program shader we compiled
	if (programID) {
		glUseProgram(programID);

		if (rgbTextureID) {
			baseImageLoc = glGetUniformLocation(programID, "texture");
			if (baseImageLoc>=0) {
				glActiveTexture(GL_TEXTURE0 + 0);
				glUniform1i(baseImageLoc, 0);
				glBindTexture(GL_TEXTURE_2D, rgbTextureID);
			}
			//after the loop activating the textures, reactivate texture 0
			glActiveTexture(GL_TEXTURE0);
		}
	}


	//  Clear screen and Z-buffer
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

static const GLfloat cube_vbo_vertex[] = {
	//back
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	// front
	1.0f,  1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,

	// left
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,

	// right
	1.0f,  1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,

	1.0f, -1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f, -1.0f,

	// top
	-1.0f,  1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f,  1.0f,  1.0f,

	1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	// bottom
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f, -1.0f, -1.0f
};

static const GLfloat cube_vbo_txcoord[] = {
	//back
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,

	//front
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,

	//left
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,

	//right
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,

	//top
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,

	//bottom
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
};

GLuint cube_vbo_vertex_id = 0;
GLuint cube_vbo_texcoord_id = 0;
void drawCube_VBO()
{

	//VBO not yet created, do it
	if (!cube_vbo_vertex_id) {
		glGenBuffers(1, &cube_vbo_vertex_id);
		glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_vertex_id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vbo_vertex), cube_vbo_vertex, GL_STATIC_DRAW);


		glGenBuffers(1, &cube_vbo_texcoord_id);
		glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_texcoord_id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vbo_txcoord), cube_vbo_txcoord, GL_STATIC_DRAW);
	}

	GLint loc_vertex_attrib = glGetAttribLocation(programID, "vertex");
	if (loc_vertex_attrib>=0) {
		glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_vertex_id);
		glVertexAttribPointer(loc_vertex_attrib, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), NULL);
		glEnableVertexAttribArray(loc_vertex_attrib);
	}


	GLint loc_txcoord_attrib = 0;
	if (rgbTextureID && cube_vbo_texcoord_id) {
		loc_txcoord_attrib = glGetAttribLocation(programID, "texCoord");
		if (loc_txcoord_attrib>=0) {
			glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_texcoord_id);
			glVertexAttribPointer(loc_txcoord_attrib, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), NULL);
			glEnableVertexAttribArray(loc_txcoord_attrib);

		} else {
			fprintf(stderr, "failed to locate texCoord\n");
		}
	}

	glDrawArrays(GL_TRIANGLES, 0, 36); // 6 faces, 2 triangles (3 vertext each)
	glDisableVertexAttribArray(loc_vertex_attrib);
	if (loc_txcoord_attrib)
		glDisableVertexAttribArray(loc_txcoord_attrib);
}

void drawCube()
{
	//first cube
		glBegin(GL_TRIANGLES);

		// back
		
		// glColor3f(100,100,100);

		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-1.0f,  1.0f, -1.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex3f( 1.0f, -1.0f, -1.0f);

		//-------------------------
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f( 1.0f, -1.0f, -1.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex3f( 1.0f,  1.0f, -1.0f);

		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-1.0f,  1.0f, -1.0f);

		// front

		// glColor3f(0,100,0);
		
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f( 1.0f,  1.0f,  1.0f);
		
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f( 1.0f, -1.0f,  1.0f);
		
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-1.0f, -1.0f,  1.0f);

		//----------------------

		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-1.0f, -1.0f,  1.0f);
		
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(-1.0f,  1.0f,  1.0f);
		
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f( 1.0f,  1.0f,  1.0f);

		// left
		// glColor3f(100,100,100);

		glVertex3f(-1.0f,  1.0f,  1.0f);
		glVertex3f(-1.0f, -1.0f,  1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);

		glVertex3f(-1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f,  1.0f, -1.0f);
		glVertex3f(-1.0f,  1.0f,  1.0f);

		// right

		// glColor3f(0,100,100);

		glVertex3f( 1.0f,  1.0f, -1.0f);
		glVertex3f( 1.0f, -1.0f, -1.0f);
		glVertex3f( 1.0f, -1.0f,  1.0f);

		glVertex3f( 1.0f, -1.0f,  1.0f);
		glVertex3f( 1.0f,  1.0f,  1.0f);
		glVertex3f( 1.0f,  1.0f, -1.0f);

		// top

		// glColor3f(100,0,0);

		glVertex3f(-1.0f,  1.0f, -1.0f);
		glVertex3f( 1.0f,  1.0f, -1.0f);
		glVertex3f( 1.0f,  1.0f,  1.0f);

		glVertex3f( 1.0f,  1.0f,  1.0f);
		glVertex3f(-1.0f,  1.0f,  1.0f);
		glVertex3f(-1.0f,  1.0f, -1.0f);

		// bottom
		// glColor3f(100,100,100);

		glVertex3f( 1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f,  1.0f);

		glVertex3f(-1.0f, -1.0f,  1.0f);
		glVertex3f( 1.0f, -1.0f,  1.0f);
		glVertex3f( 1.0f, -1.0f, -1.0f);


		glEnd();
}

void set_light()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	GLfloat amb_light[] = { 0.2, 0.2, 0.2, 1.0 };
	GLfloat diffuse[] = { 1.0, 0.0, 0.0, 1 };
	GLfloat specular[] = { 1.0, 1.0, 1.0, 1 };
	GLfloat mat_shininess[] = { 50.0 };
	GLint LightPos[4] = {3,0,0,1};
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, amb_light );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, diffuse );
	glLightfv( GL_LIGHT0, GL_SPECULAR, specular );
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightiv(GL_LIGHT0,GL_POSITION,LightPos);
	glEnable( GL_COLOR_MATERIAL );
	glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );

}

void display3D()
{
	GLfloat aspect = (GLfloat) (width) / height;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glPushMatrix();

								//void gluPerspective(fovy, aspect, zNear, zFar)
								//	fovy 	: Specifies the field of view angle, in degrees, in the y direction
								//	aspect 	: Specifies the aspect ratio (ratio of x(width) to y(height))that
								//			  dertermines the field of view in the x direction 	
								//	zNear	: Specifies the distance from the viewer to the near clipping plane (positive)
								//	zFar	: Specifies the distance from the viewer to the far clipping plane (positive)
	gluPerspective(field_of_view_angle, aspect, z_near, z_far); 	

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
								//void gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ)
	gluLookAt(0, 0, 10,
			  0, 0, 1, 
			  0, 1, 0);


	glDisable(GL_LIGHTING);

	if (mouse_grab) {
		glRotatef(angle_x, 0, 1, 0);
		glRotatef(angle_y, 1, 0, 0);
		// glTranslatef(3,0,0);
		// glTranslatef(0,0,5);
	}

	if (!programID) {
		if (rgbTextureID)
			glBindTexture(GL_TEXTURE_2D, rgbTextureID);
		else
			//single color
			glColor3f(1.0, 0.0, 0.0);
	}

	switch (obj_mode) {
	case OBJ_CUBE:
		if (use_vbo) {
			drawCube_VBO();
		} else {
			drawCube();
		}
		break;
	case OBJ_TEAPOT:
		glutSolidTeapot(2.0f);
		break;
	default:
		break;
	}
}

GLuint quad_vbo = 0;
void displayQuad_VBO()
{
	static const GLfloat g_vertex_quad_fan_data[] = {
		-0.5f,-0.5f,0.0f, // triangle 1 : begin
		0.5f,-0.5f, 1.0f,
		0.5f, 0.5f, 1.0f, // triangle 1 : end
		-0.5f, 0.5f,-1.0f // triangle 2 fan, using first vertext and previous one
	};
	//VBO not yet created, do it
	if (!quad_vbo) {
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_quad_fan_data), g_vertex_quad_fan_data, GL_STATIC_DRAW);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
	}
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // Starting from vertex 0; 4 vertices total -> 2 triangles
	glDisableVertexAttribArray(0);
}

void display_quad()
{
	float hw = 1;
	float hh = 1;
	if (use_vbo) {
		displayQuad_VBO();
	} else {

		glBegin(GL_TRIANGLES);				//glBegin(GLenum mode) : treat each point/pair/triplet as ...
		
		
		glTexCoord2f(0, 0);					//glTexCoord_x_type(numbers) :  specifies texture coordinates 
											//								in some dimensions
											//								x : 0 		left
											//									0.5		middle horizontal
											//									1		right
											//								y : 0		bottom
											//									0.5		middle vertical
											//									1		top
		glColor3f(50,0,0);
		glVertex3f(-hw/2,-hh/2, 0 );	//glVertex_x_type(numbers) :	used within glBegin/glEnd to 
											//								specify point line, and polygon 
											//								vertices, don't change color, 
											//								normal, texture coordinates and 
											//								fog coordinates configuration
		

		glTexCoord2f(0, 1);					
		glColor3f(50,50,0);
		glVertex3f(-hw/2, hh/2, 0 );


		glTexCoord2f(1, 1);
		glColor3f(0,0,100);
		glVertex3f( hw/2, hh/2, 0 );

		glColor3f(100,0,0);
		glTexCoord2f(1, 1);
		glVertex3f( hw/2, hh/2, 0 );
		
		glColor3f(0,0,100);
		glTexCoord2f(1, 0);
		glVertex3f( hw/2,-hh/2, 0 );
		
		glColor3f(0,100,0);
		glTexCoord2f(0, 0);
		glVertex3f(-hw/2,-hh/2, 0 );
		
		// /////////////////////////////////////////draw a trapeze
		// glTexCoord2f(0, 1);
		// glVertex3f(-hw/2, hh/2, 0);

		// glTexCoord2f(0, 0);
		// glVertex3f(-hw/2, -hh/2, 0);
		
		// glTexCoord2f(1, 1);
		// glVertex3f(hw/2, hh/4, 0);
		
		// glTexCoord2f(1, 0);
		// glVertex3f(hw/2, -hh/4, 0);


		// //////////////////////////////////////////draw a cercle
		// glTexCoord2d(0,0);

		// int derivate = 60;
		// int i = derivate;
		// while(i--)
		// {
		// 	glVertex3f(0, 0, 0);
		// 	glVertex3f(cos(2*3.1415*i/derivate), 		sin(2*3.1415*i/derivate), 		0);
		// 	glVertex3f(cos(2*3.1415*(i+1)/derivate), 	sin(2*3.1415*(i+1)/derivate), 	0);
		// }
		

		glEnd();

	}
}

void display2D()
{
	int loc;
	float hw = 1;
	float hh = 1;
	float angle;

	if (programID)
	 	glUseProgram(programID);

	//move to ortho projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-hw, hw, -hh, hh, -20, 20);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (!programID) {
		glDisable(GL_LIGHTING);
		if (rgbTextureID)
			glBindTexture(GL_TEXTURE_2D, rgbTextureID);
		else
			//single color
			glColor3f(1.0, 0.0, 0.0);
	}

	switch (obj_mode) {
	case OBJ_SQUARE:
		display_quad();
		break;
	default:
		break;
	}

	// reload original matrix
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	if (programID)
	 	glUseProgram(0);
}

void display()
{
	int do_sleep=0;
	struct timeval time_before,time_after;

	if (display_changed) {
		glViewport(0, 0, width, height);
		display_changed = false;
	}

	gettimeofday(&time_before, NULL);
	if (!start_time) {
		start_time = time_before.tv_sec*1000 + time_before.tv_usec/1000;
	}

	init_display();

	switch (obj_mode) {
	case OBJ_SQUARE:
		display2D();
		break;
	default:
		display3D();
	}

	glDisable(GL_TEXTURE_2D);

	//finally flush the last frame drawn
	glutSwapBuffers();


	gettimeofday(&time_after, NULL);
	elapsed = time_after.tv_sec*1000 + time_after.tv_usec/1000;
	elapsed -= time_before.tv_sec*1000 + time_before.tv_usec/1000;

	nb_frames++;
	time_spent += elapsed;
	ms_since_start = time_after.tv_sec*1000 + time_after.tv_usec/1000;
	ms_since_start -= start_time;

	int ms_since_start_frame = time_after.tv_sec*1000 + time_after.tv_usec/1000 - ( time_before.tv_sec*1000 + time_before.tv_usec/1000 );

	fprintf(stderr, "FPS %02d - frame draw time %02d ms (avg %02d)\r", (nb_frames * 1000) / time_spent, (unsigned int) elapsed, time_spent/nb_frames);

	if (do_sleep)
		sleep(sleep_ms);
}



void initialize ()
{
	glShadeModel( GL_SMOOTH );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

	glEnable( GL_BLEND );
}

void print_help()
{
	fprintf(stderr, " f: toggle fullscreen\n");
	fprintf(stderr, " q: quit\n");
	fprintf(stderr, " h: print this message\n");

	fprintf(stderr, "\n");
}

void keyboard ( unsigned char key, int x, int y )
{
	switch ( key ) {
	case 'f':
		if (!is_fullscreen) {
			glutFullScreen();
			is_fullscreen=1;
		} else {
			glutReshapeWindow(2*rgb_width, rgb_height);
			is_fullscreen=0;
		}
		glutPostRedisplay();
	    break;
	case 'q':
		glutDestroyWindow(window_id);
	    return;
	default:
		printf("unknown key mapping %c - press h for help\n",key);
		return;
	}
	glutPostRedisplay();
}


void sendInfoToShader()
{
	glUseProgram(programID);
	int loc;
	loc = glGetUniformLocation(programID, "width");
	glUniform1f(loc, (float) rgb_width);
	loc = glGetUniformLocation(programID, "height");
	glUniform1f(loc, (float) rgb_height);
	glUseProgram(0);
}

void reshape_window(int w, int h)
{
	width = w;
	height = h;
	int loc;

	if (programID) {
		//update uniforms
		glUseProgram(programID);
		sendInfoToShader();
		glUseProgram(0);
	}
	display_changed = true;
	glutPostRedisplay();
}

void mouse_motion(int x, int y)
{
	angle_x = x-grab_x;
	angle_x/=width;
	angle_x*=360;

	angle_y = y-grab_y;
	angle_y/=height;
	angle_y*=360;
	glutPostRedisplay();
}

void mouse_button(int button, int state, int x, int y)
{
	if (state==GLUT_DOWN) {
		mouse_grab=true;
		grab_x = x;
		grab_y = y;
	} else {
		mouse_grab=false;
	}
}

void print_usage()
{
	fprintf(stderr, "Usage: [OPTIONS...]\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-width W: set display width\n");
	fprintf(stderr, "-height H: set display height\n");
	fprintf(stderr, "-tx IMAGE_RGB.bmp: loads image and create texture \n");
	fprintf(stderr, "-mode VAL: sets object to draw, 0 is rectangle (default), 1 is cube, 2 is teapot\n");
	fprintf(stderr, "-shader: uses vertex shader in glsl_vert.txt and fragment shader in glsl_frag.txt, or gsls_vert_tx.txt / glsl_frag_tx.txt when texturing is enabled\n");
	fprintf(stderr, "-vert FILE: sets vertex shader file name\n");
}



/*******************************************************
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * 
 * 
 * 
 * main 
 * 
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * ******************************************************
 * */
int main(int argc, char **argv)
{
	char *image_rgb=NULL;
	const char *vert_src=NULL;
	const char *frag_src=NULL;
	width = height = 0;

	// set window values
	for (int n = 1;n<argc;n++) {
		if (!strcmp(argv[n], "-tx")) {
			image_rgb = argv[n+1];
			n++;
		}
		else if (!strcmp(argv[n], "-width")) {
			width = atoi(argv[n+1]);
			n++;
		}
		else if (!strcmp(argv[n], "-height")) {
			height = atoi(argv[n+1]);
			n++;
		}
		else if (!strcmp(argv[n], "-mode")) {
			obj_mode = (ObjMode) atoi(argv[n+1]);
			n++;
		}
		else if (!strcmp(argv[n], "-vbo")) {
			use_vbo = true;
		}
		else if (!strcmp(argv[n], "-shader")) {
			use_vbo = true;
			vert_src = "glsl_vert.txt";
			frag_src = "glsl_frag.txt";
		} else {
			fprintf(stderr, "unknown option\n");
			print_usage();
			return 1;
		}
	}

	if (vert_src && image_rgb) {
		vert_src = "glsl_vert_tx.txt";
		frag_src = "glsl_frag_tx.txt";
	}
	field_of_view_angle = 70;
	z_near = 0.1f;
	z_far = 100.0f;

	// initialize and run program
	glutInit(&argc, argv);                                                // GLUT initialization
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );            // Display Mode
	glutInitWindowSize(10,10);					// set window size
	window_id = glutCreateWindow("OpenGL PACT Tuto");			                	// create Window
	initialize();

	glewExperimental=1;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err));
	}
	if (GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader)
		fprintf(stderr, "Ready for GLSL - GLEW version %s - GL version %s\n",  glewGetString(GLEW_VERSION), glGetString( GL_VERSION ) );
	else {
		fprintf(stderr, "Shaders not supported :(  \n");
		exit(1);
	}

	if (frag_src && vert_src) {
		if (! createProgram(vert_src, frag_src) )
			return 1;
	}

	rgbTextureID = 0;
	if (image_rgb)
		rgbTextureID = loadBmp(image_rgb, &rgb_width, &rgb_height);

	if (!width || !height) {
		width=500;
		height=500;
	}
	if (programID)
		sendInfoToShader();


	glutDisplayFunc(display);			                	// register Display Function
	//  glutIdleFunc( display );				        	// register Idle Function
	glutKeyboardFunc( keyboard );						// register Keyboard Handler
	glutReshapeFunc(reshape_window);
	glutMotionFunc(mouse_motion);
	glutMouseFunc(mouse_button);
	glutReshapeWindow(width, height);

	if (programID)
		sendInfoToShader();

	glutMainLoop();
	return 0;
}
