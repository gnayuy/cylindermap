# cylindermap
cylindrical mapping with glsl

## compile

Install [GLFW][] by following the instructions on their [web site][GLFW].

Install the latest version of [GLM][].

Install the [glbinding][] to let your Apple Macbook pro support the latest OpenGL.

On Mac, just run "g++ -std=c++11 -o cylindermap cylindermap.cpp -framework OpenGL -lGLEW -lglfw -lglbinding"

## usage

Check the input drawing by:

./cylindermap input

Check the project frames by:

./cylindermap


[GLM]: http://glm.g-truc.net
[GLFW]: http://glfw.org
[glbinding]: https://github.com/cginternals/glbinding
