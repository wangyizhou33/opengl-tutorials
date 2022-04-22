#include "ObjectInPathAnalyzer.hpp"
#include <iostream>
#include <common/shader.hpp> // LoadShaders
#include <algorithm> // std::copy

ObjectInPathAnalyzer::ObjectInPathAnalyzer()
{
    createFramebuffer();
    loadShaders();
}

ObjectInPathAnalyzer::~ObjectInPathAnalyzer()
{
    releaseFramebuffer();

    glDeleteProgram(programID_);

}

void ObjectInPathAnalyzer::process()
{
    mockData();

    // Clear the screen
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_STENCIL_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // only visual effect
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    std::cerr << "render background" << std::endl;
    renderBackground();
    std::cerr << "render foreground" << std::endl;
    renderForeground();

    glDeleteVertexArrays(1, &VertexArrayID);
}

void ObjectInPathAnalyzer::createFramebuffer()
{
    // bind a framebuffer
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);  

    // color texture attachment
    glGenTextures(1, &colorTexture_);
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIN_W, WIN_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);  

    // depth texture attachment
    glGenTextures(1, &depthTexture_);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, WIN_W, WIN_H, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture_, 0);  

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer is completed." << std::endl;
    }
    else
    {
        throw std::runtime_error("incomplete framebuffer\n");
    }
}

void ObjectInPathAnalyzer::releaseFramebuffer()
{
    glDeleteTextures(1, &colorTexture_);
    glDeleteTextures(1, &depthTexture_);
    glDeleteFramebuffers(1, &fbo_);
}

void ObjectInPathAnalyzer::loadShaders()
{
    programID_ = LoadShaders("../query/SimpleVertexShader.vertexshader",
                             "../query/SimpleFragmentShader.fragmentshader");

    // Use our shader
    glUseProgram(programID_);
}


void ObjectInPathAnalyzer::renderBackground()
{
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, bgSize_ * 3 * sizeof(GLfloat), bgData_, GL_STATIC_DRAW);

    static const GLfloat colorBufferData[] =
    {
        0.0f, 1.0f, 0.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 0.5f,
    };

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colorBufferData), colorBufferData, GL_STATIC_DRAW);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );

    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(
        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        4,                                // size
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
    );

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // 3 indices starting at 0 -> 1 triangle

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
}

void ObjectInPathAnalyzer::renderForeground()
{
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0x00);


    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, fgSize_ * 3 * sizeof(GLfloat), fgData_, GL_STATIC_DRAW);

    static const GLfloat colorBufferData[] =
    {
        0.0f, 0.0f, 1.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 0.5f,
    };

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colorBufferData), colorBufferData, GL_STATIC_DRAW);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );

    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(
        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        4,                                // size
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
    );

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // 3 indices starting at 0 -> 1 triangle

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
}

void ObjectInPathAnalyzer::mockData()
{
    static const GLfloat bg[] = 
    {
        -5.0f, -5.0f, 0.0f,
        -5.0f, 5.0f, 0.0f,
        5.0f, -5.0f, 0.0f,
        5.0f, 5.0f, 0.0f,
    };

    std::copy(&bg[0], &bg[12], &bgData_[0]);
    bgSize_ = 4u;

    static const GLfloat fg[] = 
    {
        -2.5f + 5.0f, -2.5f, 0.0f,
        -2.5f + 5.0f, 2.5f, 0.0f,
         2.5f + 5.0f, -2.5f, 0.0f,
         2.5f + 5.0f, 2.5f, 0.0f,
    };

    std::copy(&fg[0], &fg[12], &fgData_[0]);
    fgSize_ = 4u;
}