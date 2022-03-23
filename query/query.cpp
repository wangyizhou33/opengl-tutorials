// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
using namespace glm;

#include <common/shader.hpp>

#include <iostream>

int main(void)
{
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(800, 800, "Query", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr,
                "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("../query/SimpleVertexShader.vertexshader",
                                   "../query/SimpleFragmentShader.fragmentshader");


    static const GLfloat g_vertex_buffer_data[] =
    {
        -0.5f, -0.5f, 0.0f,     // first object
        -0.5f, 0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f, 0.5f, 0.0f,
        -0.25f + 0.5f, -0.25f, 0.0f,   // second object
        -0.25f + 0.5f, 0.25f, 0.0f,
         0.25f + 0.5f, -0.25f, 0.0f,
         0.25f + 0.5f, 0.25f, 0.0f,
    };
    
    static const GLfloat g_color_buffer_data[] =
    {
        0.0f, 1.0f, 0.0f,   // first object
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,   // second object
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };

    GLuint vertexbuffer0;
    glGenBuffers(1, &vertexbuffer0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data) / 2, g_vertex_buffer_data, GL_STATIC_DRAW);

    GLuint colorbuffer0;
    glGenBuffers(1, &colorbuffer0);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data) / 2, g_color_buffer_data, GL_STATIC_DRAW);

    GLuint vertexbuffer1;
    glGenBuffers(1, &vertexbuffer1);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data) / 2, &g_vertex_buffer_data[4 * 3], GL_STATIC_DRAW);

    GLuint colorbuffer1;
    glGenBuffers(1, &colorbuffer1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data) / 2, &g_color_buffer_data[4 * 3], GL_STATIC_DRAW);


    GLuint queryTotal;
    glGenQueries(1, &queryTotal);
    GLuint totalAreaPixelCount = 0;


    GLuint queryIntersection;
    glGenQueries(1, &queryIntersection);
    GLuint intersectionPixelCount = 0;

    do
    {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glEnable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // only visual effect
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glDepthMask(GL_FALSE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // draw the first object
        {
            // prepare test 
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

            // 1rst attribute buffer : vertices
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer0);
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
            glBindBuffer(GL_ARRAY_BUFFER, colorbuffer0);
            glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                3,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
            );

            glBeginQuery(GL_SAMPLES_PASSED, queryTotal);
            // Draw the triangle !
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // 3 indices starting at 0 -> 1 triangle

            glEndQuery(GL_SAMPLES_PASSED);

            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
        }
        // draw the second object
        {
            glStencilFunc(GL_EQUAL, 1, 0xFF);
            glStencilMask(0x00);

            // 1rst attribute buffer : vertices
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer1);
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
            glBindBuffer(GL_ARRAY_BUFFER, colorbuffer1);
            glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                3,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
            );

            glBeginQuery(GL_SAMPLES_PASSED, queryIntersection);
            // Draw the triangle !
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // 3 indices starting at 0 -> 1 triangle

            glEndQuery(GL_SAMPLES_PASSED);

            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
        }

        GLboolean isValidQuery = glIsQuery(queryTotal) && glIsQuery(queryIntersection);
        if (isValidQuery)
        {
            glGetQueryObjectuiv(queryTotal,
                                GL_QUERY_RESULT,
                                &totalAreaPixelCount);
            glGetQueryObjectuiv(queryIntersection,
                                GL_QUERY_RESULT,
                                &intersectionPixelCount);
            std::cout << "area pixel count " << totalAreaPixelCount
                      << ", intersection pixel count " << intersectionPixelCount << std::endl;
        }

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
            glfwWindowShouldClose(window) == 0);

    // Cleanup VBO
    glDeleteBuffers(1, &vertexbuffer0);
    glDeleteBuffers(1, &colorbuffer0);
    glDeleteBuffers(1, &vertexbuffer1);
    glDeleteBuffers(1, &colorbuffer1);

    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);

    glDeleteQueries(1, &queryTotal);
    glDeleteQueries(1, &queryIntersection);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

