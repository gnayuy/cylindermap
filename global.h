
//
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>

//
#include <GL/glew.h>

//
#include <GLFW/glfw3.h>
GLFWwindow* window;

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"
using namespace glm;

#include "shaders.h"

//
GLuint fb[2] = {std::numeric_limits<GLuint>::max(), std::numeric_limits<GLuint>::max()}; //framebuffers
GLuint rb[2] = {std::numeric_limits<GLuint>::max(), std::numeric_limits<GLuint>::max()}; //renderbuffers, color and depth
GLuint textures[2] = {std::numeric_limits<GLuint>::max(), std::numeric_limits<GLuint>::max()};

const int PJTEX = 0; // project texture
const int DMTEX = 1; // deformation texture

// screen
GLuint g_ss_quad_vao = 0, g_ss_quad_vbo = 0;
GLuint postvs=0, postfs=0;
GLuint postsp=0;

// obj
GLuint vao = 0, vbo = 0;
GLuint vs=0, fs=0;
GLuint shaderProgram=0;

//
GLuint vsDeform = 0, fsDeform = 0;
GLuint spDeform = 0;

// check for shader compiler errors
bool check_shader_compile_status(GLuint obj) {
    GLint status;
    glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length);
        glGetShaderInfoLog(obj, length, &length, &log[0]);
        std::cerr << &log[0];
        return false;
    }
    return true;
}

// load deformation matrix
char* loadDeform(string fn)
{
    FILE *file = fopen(fn.c_str(), "rb");
    char* p = NULL;
    
    if(file!=NULL)
    {
        fseek(file , 0 , SEEK_END);
        long size = ftell(file);
        rewind(file);
        
        try {
            p = new char [size];
        } catch (...) {
            return NULL;
        }
        
        fread (p,1,size,file);
    }
    fclose(file);
    
    return p;
}

//
bool* stimulation(int n)
{
    bool* p = NULL;
    
    try {
        p = new bool [n];
    } catch (...) {
        std::cout<<"Fail to allocate array for stimulation.\n";
        return NULL;
    }
    
    std::default_random_engine generator;
    std::bernoulli_distribution distribution(0.5);
    
    for (int i=0; i<n; ++i)
        p[i] = distribution(generator);
    
    return p;
}

//
bool initTextureRGB(int g_gl_width, int g_gl_height, GLuint* tex, unsigned char *data)
{
    //
    glGenTextures (1, tex);
    glBindTexture (GL_TEXTURE_2D, *tex);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, g_gl_width, g_gl_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    //glGenerateMipmap(GL_TEXTURE_2D);
    
    // free data here
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //
    return true;
}

bool initTextureRG32F(int g_gl_width, int g_gl_height, GLuint* tex, float *data)
{
    //
    glGenTextures (1, tex);
    glBindTexture (GL_TEXTURE_2D, *tex);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RG32F, g_gl_width, g_gl_height, 0, GL_RG, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    //glGenerateMipmap(GL_TEXTURE_2D);
    
    // free data here
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //
    return true;
}

// init a framebuffer
bool initFramebuffer(int g_gl_width, int g_gl_height, int n, int type, GLuint* tex, unsigned char *data)
{
    if(!(n==0 || n==1))
    {
        printf("Invalid n\n");
        return false;
    }
    
    //
    glGenFramebuffers (1, &fb[n]);
    glGenTextures (1, tex);
    glBindTexture (GL_TEXTURE_2D, *tex);
    
    if(type==1)
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, g_gl_width, g_gl_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    else if(type==2)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, g_gl_width, g_gl_height, 0, GL_RG, GL_FLOAT, (float*)data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    //
    glBindFramebuffer (GL_FRAMEBUFFER, fb[n]);
    
    if(n==0)
        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    else if(n==1)
        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, *tex, 0);
    
    //
    glGenRenderbuffers (1, &rb[n]);
    glBindRenderbuffer (GL_RENDERBUFFER, rb[n]);
    glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_gl_width, g_gl_height);
    
    glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb[n]);
    
    //
    //    GLenum draw_bufs[] = { GL_COLOR_ATTACHMENT0 };
    //    glDrawBuffers (1, draw_bufs);
    
    //
    GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
    if (GL_FRAMEBUFFER_COMPLETE != status) {
        fprintf (stderr, "ERROR: incomplete framebuffer\n");
        if (GL_FRAMEBUFFER_UNDEFINED == status) {
            fprintf (stderr, "GL_FRAMEBUFFER_UNDEFINED\n");
        } else if (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT == status) {
            fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
        } else if (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT == status) {
            fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");
        } else if (GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER == status) {
            fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n");
        } else if (GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER== status) {
            fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n");
        } else if (GL_FRAMEBUFFER_UNSUPPORTED == status) {
            fprintf (stderr, "GL_FRAMEBUFFER_UNSUPPORTED\n");
        } else if (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE == status) {
            fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n");
        } else if (GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS == status) {
            fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n");
        } else {
            fprintf (stderr, "unspecified error\n");
        }
        return false;
    }
    
    //
    glBindFramebuffer (GL_FRAMEBUFFER, 0);
    return true;
}

// screen
void init_ss_quad ()
{
    //
    GLfloat ss_quad_pos[] = {
        -1.0, -1.0,
        1.0, -1.0,
        1.0,  1.0,
        1.0,  1.0,
        -1.0,  1.0,
        -1.0, -1.0
    };
    
    //
    glGenVertexArrays (1, &g_ss_quad_vao);
    glBindVertexArray (g_ss_quad_vao);
    glGenBuffers (1, &g_ss_quad_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, g_ss_quad_vbo);
    glBufferData (GL_ARRAY_BUFFER, sizeof (ss_quad_pos), ss_quad_pos, GL_STATIC_DRAW);
    glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray (0);
}

//
int initObject(int w, int h, int nTex)
{
    // ex: a triangle
//    GLfloat points[] = {
//        0.0f,	0.5f,	0.0f,
//        0.5f,  -0.5f,	0.0f,
//        -0.5f, -0.5f,	0.0f
//    };
    
    // ex: a rectangle
//    GLfloat points[] = {
//        -0.1, -0.65, 0.0f,
//        -0.03, -0.65, 0.0f,
//        -0.03, 0.65, 0.0f,
//        -0.03, 0.65, 0.0f,
//        -0.1,  0.65, 0.0f,
//        -0.1, -0.65, 0.0f
//    };
    
//    GLfloat points[] = {
//        -0.3, -0.65, 0.0f,
//        -0.25, -0.65, 0.0f,
//        -0.25, 0.65, 0.0f,
//        -0.25, 0.65, 0.0f,
//        -0.3,  0.65, 0.0f,
//        -0.3, -0.65, 0.0f
//    };
    
    GLfloat points[] = {
        -0.3, -0.65, 0.0f,
        -0.25, -0.65, 0.0f,
        -0.25, 0.65, 0.0f,
        -0.25, 0.65, 0.0f,
        -0.3,  0.65, 0.0f,
        -0.3, -0.65, 0.0f
    };
    
    for(int i=0; i<18; i+=3)
    {
        points[i] = 0.5*w*(points[i]+1);
        points[i+1] = 0.5*h*(points[i+1]+1);
    }
    
    //
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    //
    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);
    if(check_shader_compile_status(vs)==false)
    {
        std::cout<<"Fail to compile vertex shader"<<std::endl;
        return -1;
    }
    
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);
    if(check_shader_compile_status(fs)==false)
    {
        std::cout<<"Fail to compile fragment shader"<<std::endl;
        return -1;
    }
    
    //
    shaderProgram = glCreateProgram ();
    glAttachShader(shaderProgram, fs);
    glAttachShader(shaderProgram, vs);
    glLinkProgram(shaderProgram);
    
    //
    initFramebuffer(w, h, 0, 1, &textures[nTex], NULL);
    
    return 0;
}

//
int initCheckerboard(int w, int h, int step)
{
    int c = 3;
    int size = c*w*h;
    unsigned char *p = NULL;
    
    try
    {
        p = new unsigned char [size];
    }
    catch(...)
    {
        std::cout<<"Fail to allocate memory for checkerboard"<<std::endl;
        return -1;
    }
    
    int i_s = 1;
    int j_s = 1;
    
    for (int j = 0; j < h; j+=step)
    {
        int jend = j+step;
        if(jend>h) jend=h;
        for (int i = 0; i < w; i+=step)
        {
            int iend = i+step;
            if(iend>w) iend = w;
            
            //
            unsigned char val = ( i/step + j/step ) % 2 == 0 ? 0 : 255;
            
            //            printf("[%d,%d] [%d,%d] val %d \n", i,iend,j,jend,val);
            
            for(int m=j; m<jend; m++)
            {
                int offy = m*w*c;
                for(int n=i; n<iend; n++)
                {
                    int offx = offy+n*c;
                    
                    p[offx] = val;
                    p[offx+1] = val;
                    p[offx+2] = val;
                }
            }
        }
        
    }
    
    //
    //    FILE *fp=NULL;
    //    fp = fopen("checkerboard.txt", "w");
    //
    //    if (fp == NULL) {
    //        std::cout<<"Can't open output file %s!\n";
    //        return -1;
    //    }
    //
    //    for(size_t i=0; i<size; i+=c)
    //    {
    //        if(i%w==0)
    //            fprintf(fp, ";\n");
    //
    //        fprintf(fp, "%d ", p[i]);
    //
    //    }
    //    
    //    fclose(fp);
    
    
    initTextureRGB(w,h,&textures[PJTEX],p);
    return 0;
}

//
void clear()
{
    glDeleteTextures(2, textures);
    glDeleteFramebuffers(2, fb);
    glDeleteRenderbuffers(2, rb);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    
    glDeleteProgram(shaderProgram);
    glDeleteShader(fs);
    glDeleteShader(vs);
    
    glDeleteProgram(postsp);
    glDeleteShader(postfs);
    glDeleteShader(postvs);
    
    glDeleteProgram(spDeform);
    glDeleteShader(fsDeform);
    glDeleteShader(vsDeform);
}

