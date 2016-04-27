
// g++ -std=c++11 -o cylindermap cylindermap.cpp -framework OpenGL -lGLEW -lglfw -lglbinding

#include "global.h"

// main func
int main(int argc, char *argv[])
{
    //
    //---- interface
    //
    
    // input
    int w = 1440;
    int h = 360;
    
    // output
    int width = 608;
    int height = 684;
    
    // deformation RG32F
    string deformFile = "transformation/deform.bin";
    char outFile[] = "result/output.bin";
    
    //
    //----- init
    //
    
    bool b_debug = false;
    
    if(argc>1)
    {
        if (strcmp(argv[1], "debug") == 0)
        {
            b_debug = true;
            std::cout<<"debugging mode"<<std::endl;
        }
        if(argc>2)
            w = atoi(argv[2]);
        if(argc>3)
            h = atoi(argv[3]);
    }
    
    //
    GLFWwindow* window = NULL;
    const GLubyte *renderer, *version;
    
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    
    //
    if(b_debug)
    {
        window = glfwCreateWindow (w, h, "cylinder map: input", NULL, NULL);
    }
    else
    {
        window = glfwCreateWindow (width, height, "cylinder map", NULL, NULL);
    }
    if (!window) {
        fprintf (stderr, "ERROR: could not open window with GLFW3\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent (window);
    
    //
    int windowWidth = w;
    int windowHeight = h;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    
    // OpenGL version
    renderer = glGetString(GL_RENDERER);
    version = glGetString(GL_VERSION);
    printf ("Renderer: %s\n", renderer);
    printf ("OpenGL version supported %s\n", version);
    
    //
    //---- create input images
    //
    
    // define object to render
    initObject(windowWidth, windowHeight);
    //initCheckerboard(windowWidth, windowHeight, 32);
    
    //
    //---- screen
    //
    
    //
    init_ss_quad();
    
    if(b_debug)
    {
        //
        postvs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(postvs, 1, &post_vertex_shader, NULL);
        glCompileShader(postvs);
        if(check_shader_compile_status(postvs)==false)
        {
            std::cout<<"Fail to compile vertex shader"<<std::endl;
            return -1;
        }
        
        postfs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(postfs, 1, &post_fragment_shader, NULL);
        glCompileShader(postfs);
        if(check_shader_compile_status(postfs)==false)
        {
            std::cout<<"Fail to compile fragment shader"<<std::endl;
            return -1;
        }
        
        //
        postsp = glCreateProgram ();
        glAttachShader(postsp, postvs);
        glAttachShader(postsp, postfs);
        glLinkProgram(postsp);
    }
    
    //
    //---- load deformation
    //
    
    vsDeform = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsDeform, 1, &vsWarp, NULL);
    glCompileShader(vsDeform);
    if(check_shader_compile_status(vsDeform)==false)
    {
        std::cout<<"Fail to compile screen vertex shader"<<std::endl;
        return -1;
    }
    
    fsDeform = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsDeform, 1, &fsWarp, NULL);
    glCompileShader(fsDeform);
    if(check_shader_compile_status(fsDeform)==false)
    {
        std::cout<<"Fail to compile screen fragment shader"<<std::endl;
        return -1;
    }
    
    //
    spDeform = glCreateProgram();
    glAttachShader(spDeform, vsDeform);
    glAttachShader(spDeform, fsDeform);
    glLinkProgram(spDeform);
    
    GLuint locPos = glGetAttribLocation(spDeform, "vPos");
    GLuint locTex0  = glGetUniformLocation(spDeform, "tex0");
    GLuint locTex1  = glGetUniformLocation(spDeform, "tex1");
    GLuint locWidth  = glGetUniformLocation(spDeform, "w");
    GLuint locHeight  = glGetUniformLocation(spDeform, "h");
    
    //
    char *p = loadDeform(deformFile);
    float *deformMat = (float*)p;
    
    //initFramebuffer(width, height, 1, 2, &textures[DMTEX], (unsigned char*)deformMat);
    initTextureRG32F(width, height, &textures[DMTEX], deformMat);
    
    //
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    
    while(!glfwWindowShouldClose(window))
    {
        // FPS
        double currentTime = glfwGetTime();
        nbFrames++;
        if ( currentTime - lastTime >= 1.0 ){
            printf("%f ms/frame\n", 1000.0/double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }
        
        //
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        //glEnable(GL_CULL_FACE);
        
        // 1st pass: render to a texture
        glBindFramebuffer (GL_FRAMEBUFFER, fb[PJTEX]);
        glViewport(0, 0, windowWidth, windowHeight);
        
        //
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        //
        glUseProgram (shaderProgram);
        glBindVertexArray (vao);
        glDrawArrays (GL_TRIANGLES, 0, 6);
        
        // 2nd pass: render to screen
        if(b_debug)
        {
            glBindFramebuffer (GL_FRAMEBUFFER, 0);
            glViewport(0, 0, windowWidth, windowHeight);
            glClearColor (0.0, 0.0, 0.0, 1.0);
            glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            //
            glUseProgram(postsp);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[PJTEX]);
            
            glBindVertexArray (g_ss_quad_vao);
            glDrawArrays (GL_TRIANGLES, 0, 6);
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, width, height);
            glClearColor (0.0, 0.0, 0.0, 1.0);
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            //
            glUseProgram(spDeform);
            
            //
            glUniform1f(locWidth, windowWidth);
            glUniform1f(locHeight, windowHeight);
            
            //
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[PJTEX]);
            glUniform1i(locTex0, 0);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textures[DMTEX]);
            glUniform1i(locTex1, 1);
            
            //
            glBindVertexArray(g_ss_quad_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }
        
        // event loop
        glfwPollEvents ();
        
        //
        if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose (window, 1);
        }
        
        //
        glfwSwapBuffers (window);
    }
    
    // de-alloc resources
    clear();
    
    //
    glfwTerminate();
    return 0;
}