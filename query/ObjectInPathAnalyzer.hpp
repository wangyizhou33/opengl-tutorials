#ifndef OBJECT_IN_PATH_ANALYZER_HPP
#define OBJECT_IN_PATH_ANALYZER_HPP

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <GL/glew.h>

class ObjectInPathAnalyzer
{
public: 
    ObjectInPathAnalyzer();
    ~ObjectInPathAnalyzer();

    // [optional] get the framebuffer to be renderred in a glfw window, for debugging purpose
    unsigned int getFramebuffer() const {return fbo_;};

    // main function
    void process(); 

private:
    /**** framebuffer ****/

    // create a framebuffer
    void createFramebuffer();

    // release a framebuffer
    void releaseFramebuffer();

    unsigned int fbo_;
    unsigned int colorTexture_; // attached to fbo_
    unsigned int depthTexture_; // attached to fbo_

    // framebuffer dimension
    static constexpr uint32_t WIN_W = 800;
    static constexpr uint32_t WIN_H = 800;

    /**** shader ****/
    GLuint programID_;
    void loadShaders();

    /**** background data ****/
    // background is lane data for driving
    //               freespace minus obstacle for parking
    GLfloat bgData_[100];
    size_t bgSize_;     // number of vertices
                        // number of triangles = bgSize_ - 2 because the layout is GL_TRIANGLE_STRIP

    void renderBackground();

    /**** foreground data ****/
    // foreground is obstacle for driving
    //               ego pose for parking
    GLfloat fgData_[100];
    size_t fgSize_;     // number of vertices

    void renderForeground();

    // for demonstration purpose
    void mockData();

}; // class ObjectInPathAnalyzer


#endif // OBJECT_IN_PATH_ANALYZER_HPP