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

#include "ObjectInPathAnalyzer.hpp"

constexpr uint32_t WIN_W = 800;
constexpr uint32_t WIN_H = 800;

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
    window = glfwCreateWindow(WIN_W, WIN_H, "Query", NULL, NULL);
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

    LaneData lane0{}; // lane #0
    LaneData lane1{}; // lane #1
    LaneData lane2{}; // lane #1
    lane0.id = 0u;
    lane1.id = 1u; 
    lane2.id = 2u; 

    for (float32_t x = -10.0f; x < 100.0f; x += 5.0f)
    {
        lane0.leftDiv.push_back(Point3f{.x = x, .y = 3.0f, .z = 0.0f});
        lane0.rightDiv.push_back(Point3f{.x = x, .y = 1.0f, .z = 0.0f});
        lane1.leftDiv.push_back(Point3f{.x = x, .y = -1.0f, .z = 0.0f});
        lane1.rightDiv.push_back(Point3f{.x = x, .y = 1.0f, .z = 0.0f});
        lane2.leftDiv.push_back(Point3f{.x = x, .y = -1.0f, .z = 0.0f});
        lane2.rightDiv.push_back(Point3f{.x = x, .y = -3.0f, .z = 0.0f});

    }

    ObstacleData obs0{}; // obstacle#0
    ObstacleData obs1{}; // obstacle#1
    ObstacleData obs2{}; // obstacle#1
    obs0.id = 0u;
    obs1.id = 1u;
    obs2.id = 2u;

    // obs0 fully in lane1
    obs0.boundaryPoints.push_back(Point3f{.x = 20.0f, .y = -0.75f, .z = 0.0f});
    obs0.boundaryPoints.push_back(Point3f{.x = 20.0f, .y = 0.75f, .z = 0.0f});
    obs0.boundaryPoints.push_back(Point3f{.x = 25.0f, .y = -0.75f, .z = 0.0f});
    obs0.boundaryPoints.push_back(Point3f{.x = 25.0f, .y = 0.75f, .z = 0.0f});

    // obs1 partially in lane0
    obs1.boundaryPoints.push_back(Point3f{.x = 20.0f, .y = 2.0f, .z = 0.0f});
    obs1.boundaryPoints.push_back(Point3f{.x = 20.0f, .y = 4.0f, .z = 0.0f});
    obs1.boundaryPoints.push_back(Point3f{.x = 25.0f, .y = 2.0f, .z = 0.0f});
    obs1.boundaryPoints.push_back(Point3f{.x = 25.0f, .y = 4.0f, .z = 0.0f});

    // obs2 not assigned
    obs2.boundaryPoints.push_back(Point3f{.x = 10.0f, .y = 4.0f, .z = 0.0f});
    obs2.boundaryPoints.push_back(Point3f{.x = 10.0f, .y = 5.5f, .z = 0.0f});
    obs2.boundaryPoints.push_back(Point3f{.x = 15.0f, .y = 4.0f, .z = 0.0f});
    obs2.boundaryPoints.push_back(Point3f{.x = 15.0f, .y = 5.5f, .z = 0.0f});


    ObjectInPathAnalyzer oipa{};
    // oipa.process();
    oipa.process(std::vector<LaneData>({lane0, lane1, lane2}), 
                 std::vector<ObstacleData>({obs0, obs1, obs2}));

    glBindFramebuffer(GL_READ_FRAMEBUFFER, oipa.getFramebuffer()); // redundant. fbo is already the read framebuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);   // set screen as the draw framebuffer

    do
    {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_STENCIL_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        glBlitFramebuffer(0, 0, WIN_W, WIN_H,
                          0, 0, WIN_W, WIN_H,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
            glfwWindowShouldClose(window) == 0);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

