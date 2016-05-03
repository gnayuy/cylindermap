// cylindermap
// Develop by Yang Yu (yuy@janelia.hhmi.org)

// compile on Mac
// g++ -std=c++11 -o cylindermap cylindermap.cpp -framework OpenGL -lGLEW -lglfw -lglbinding

// NOTE:
// texture 0, 1, 2 for r,g,b frames
// texture 3 for deformation matrix (transformation)
//

//
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
    bool b_input = false;
    
    if(argc>1)
    {
        if (strcmp(argv[1], "input") == 0)
        {
            b_input = true;
            std::cout<<"showing input image"<<std::endl;
        }
        else if(strcmp(argv[1], "debug") == 0)
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
    if(b_input)
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
    //glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    
    printf("win size: [%d, %d], framebuffer size: [%d, %d]\n", w,h, windowWidth, windowHeight);
    
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
    //---- Model, View, Projection
    //
    
    //
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix =  glm::lookAt(glm::vec3(0.0f,0.0f,2.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
    glm::mat4 projectionMatrix = glm::perspective((glm::pi<float>()*2*67)/360.0f, (float)w/(float)h, 0.1f, 100.0f);
    
    //glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
    
    glm::mat4 viewMatrixColor[3];
    
    //
    //---- screen
    //
    
    //
    init_ss_quad();
    
    if(b_input)
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
    GLuint locWidth  = glGetUniformLocation(spDeform, "w");
    GLuint locHeight  = glGetUniformLocation(spDeform, "h");
    
    GLuint locTex[4];
    locTex[0] = glGetUniformLocation(spDeform, "tex0");
    locTex[1] = glGetUniformLocation(spDeform, "tex1");
    locTex[2] = glGetUniformLocation(spDeform, "tex2");
    locTex[3] = glGetUniformLocation(spDeform, "texDeform");
    
    //
    char *p = loadDeform(deformFile);
    float *deformMat = (float*)p;
    
    initFramebuffer(width, height, 3, 2, &textures[3], (unsigned char*)deformMat);
    
    
    //
    //---- create input images
    //
    
    // define object to render
    initObject(windowWidth, windowHeight);
    //initCheckerboard(windowWidth, windowHeight, 32, 0);
    
    //
    float xtranslate = -0.001f;
    int count = 0, n = 300;
    bool *stimuli = stimulation(n);
    
//    for(int i=0; i<n; i++)
//        printf("%d ", stimuli[i]);
//    printf("\n");
//
//    int size = w*h*3;
//    std::vector<GLubyte> pix(size);
    
    // render
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    
    //
    bool b_DataChanged = false;
    
    //
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    //
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
        

        // 1st pass: render to a RGB texture
        
        //
        glUseProgram (shaderProgram);
        
        // RGB-CHANNEL
        for(int c=0; c<3; c++)
        {
            if(count<n)
            {
                b_DataChanged = true;
                
                // IF
                // performs the scaling FIRST, and THEN the rotation, and THEN the translation
                // THEN
                // TransformedVector = TranslationMatrix * RotationMatrix * ScaleMatrix * OriginalVector;
                
                if(count>0)
                {
                    if(stimuli[count]!=stimuli[count-1])
                    {
                        viewMatrix = glm::rotate(viewMatrix, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // z-axis
                        //std::cout<<"stimulate ... "<<count<<" "<<c<<std::endl;
                    }
                }
                
                //
                if(b_debug)
                {
                    if(glfwGetKey(window, GLFW_KEY_N))
                    {
                        count++;
                        viewMatrix = glm::translate(viewMatrix, glm::vec3(xtranslate, 0.0f, 0.0f));
                        
                        std::cout<<"count "<<count<<std::endl;
                    }
                }
                else
                {
                    count++;
                    viewMatrix = glm::translate(viewMatrix, glm::vec3(xtranslate, 0.0f, 0.0f));
                }
                
                viewMatrixColor[c] = viewMatrix;
            }
            
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "Proj"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "View"), 1, GL_FALSE, glm::value_ptr(viewMatrixColor[c]));
            
            //
            if(b_DataChanged)
            {
                initFramebuffer(w, h, c, 1, &textures[c], NULL);
            }
            
            glBindFramebuffer (GL_FRAMEBUFFER, fb[c]);
            glViewport(0, 0, windowWidth, windowHeight);
            
            //
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            glBindVertexArray (vao);
            glDrawArrays (GL_TRIANGLES, 0, 6);
        }
        
        //std::cout<<count<<std::endl;
        
        // 2nd pass: render to screen
        if(b_input)
        {
            glBindFramebuffer (GL_FRAMEBUFFER, 0);
            glViewport(0, 0, windowWidth, windowHeight);
            glClearColor (0.0, 0.0, 0.0, 1.0);
            glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            //
            glUseProgram(postsp);
            
            //
            for(int c=0; c<3; c++)
            {
                glActiveTexture(GL_TEXTURE0 + c);
                glBindTexture(GL_TEXTURE_2D, textures[c]);
            }
            
            glBindVertexArray (g_ss_quad_vao);
            glDrawArrays (GL_TRIANGLES, 0, 6);
            
//            glGetTexImage (GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, &pix[0]);
//            //glTexSubImage2D (GL_TEXTURE_2D,0,0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,&pix[0]);
//            
//            FILE *fp=NULL;
//            
//            string fn = "testFrame" + std::to_string(count) + ".raw";
//            
//            std::cout<<fn<<std::endl;
//            
//            fp = fopen(fn.c_str(), "w");
//            
//            if (fp == NULL) {
//                std::cout<<"Can't open output file %s!\n";
//                return -1;
//            }
//            
//            for(size_t i=0; i<size; i++)
//                fprintf(fp, "%d", pix[i]);
//            
//            fclose(fp);
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
            for(int c=0; c<4; c++)
            {
                glActiveTexture(GL_TEXTURE0+c);
                glBindTexture(GL_TEXTURE_2D, textures[c]);
                glUniform1i(locTex[c], c);
            }
            
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