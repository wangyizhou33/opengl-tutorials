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
    resetGLSettings();

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    std::cerr << "render background" << std::endl;
    renderBackground();
    std::cerr << "render foreground" << std::endl;

    GLuint queryForeground;
    glGenQueries(1, &queryForeground);
    glBeginQuery(GL_SAMPLES_PASSED, queryForeground);
    renderForeground();
    glEndQuery(GL_SAMPLES_PASSED);
    GLboolean isValidQuery = glIsQuery(queryForeground);
    if (isValidQuery)
    {
        GLuint pixelCount;
        glGetQueryObjectuiv(queryForeground,
                            GL_QUERY_RESULT,
                            &pixelCount);
        std::cout << "foreground pixel count: " << pixelCount << std::endl;
    }

    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteQueries(1, &queryForeground);
}

void ObjectInPathAnalyzer::process(const std::vector<LaneData>& lanes, const std::vector<ObstacleData>& obstacles)
{
    outputData_.clear();

    // Clear the screen
    resetGLSettings();

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    for (const ObstacleData& obstacle : obstacles)
    {
        outputData_.push_back({});
        LaneAssignmentData& last = outputData_.back();
        last.obstacleId = obstacle.id;
        last.obstacleVertexData = trivialObstacleTriangulation(obstacle);

        // gray obstacle rendering without stencil testing
        std::vector<GLfloat> obsVertexData = trivialObstacleTriangulation(obstacle);
        std::vector<GLfloat> obsColorData{};
        for (size_t i = 0u; i < 4u; ++i) // each obstacle has 4 vertices
        {
            obsColorData.push_back(0.0f); // r
            obsColorData.push_back(0.0f); // g
            obsColorData.push_back(0.0f); // b
            obsColorData.push_back(0.5f); // a
        }

        uint32_t area = renderObstacle(obsVertexData, obsColorData); // obstacle total area
        last.obstacleTotalPixelCount = area;
    }

    // lane background with stencil buffer filling
    for (const LaneData& lane: lanes)
    {
        uint32_t laneId = lane.id;

        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);
        std::vector<GLfloat> laneVertexData = trivialLaneTriangulation(lane);
        std::vector<GLfloat> laneColorData{};

        RGBColor color = getNextColor();

        for (size_t i = 0u; i < laneVertexData.size() / 3; ++i) // laneVertexData.size() is number of vertices
        {
            laneColorData.push_back(color.r); // r
            laneColorData.push_back(color.g); // g
            laneColorData.push_back(color.b);  // b
            laneColorData.push_back(0.3f);    // a
        }
        renderLane(laneVertexData, laneColorData);

        // enable stencil testing
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);
        // render all obstacles with stencil test/
        for (size_t i = 0u; i < obstacles.size(); ++i)
        {
            const std::vector<GLfloat> obsVertexData = outputData_.at(i).obstacleVertexData;

            // use an opaque color for the obstacle
            std::vector<GLfloat> obsColorData{};
            for (size_t i = 0u; i < obsVertexData.size() / 3; ++i)
            {
                obsColorData.push_back(color.r); // r
                obsColorData.push_back(color.g); // g
                obsColorData.push_back(color.b);  // b
                obsColorData.push_back(0.9f);    // a
            }

            uint32_t intersectionArea = renderObstacle(obsVertexData, obsColorData);

            // push back (non-trivial) result to the output container
            if (intersectionArea > 0)
            {
                // obstacle #i intersects with lane #laneId for "intersectionArea" pixel
                outputData_.at(i).laneIds.push_back(laneId);
                outputData_.at(i).intersectionPixelCounts.push_back(intersectionArea);
            }
        }
    }

    glDeleteVertexArrays(1, &VertexArrayID);

    // summarizing the result
    for (const LaneAssignmentData& elem : outputData_)
    {
        for (size_t i = 0u; i < elem.laneIds.size(); ++i)
        {
            float32_t ratio = static_cast<float32_t>(elem.intersectionPixelCounts.at(i)) / static_cast<float32_t>(elem.obstacleTotalPixelCount);
            std::cout << "Obstacle #" << elem.obstacleId << " is lane assigned to lane #" << elem.laneIds.at(i) 
                      << " for ratio " << ratio << std::endl;
        }
    }
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

void ObjectInPathAnalyzer::resetGLSettings()
{
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

std::vector<GLfloat> ObjectInPathAnalyzer::trivialObstacleTriangulation(const ObstacleData& obs)
{
    std::vector<GLfloat> ret{};

    for (size_t i = 0u; i < 4u; ++i)
    {
        ret.push_back(obs.boundaryPoints.at(i).x);
        ret.push_back(obs.boundaryPoints.at(i).y);
        ret.push_back(obs.boundaryPoints.at(i).z);
    }

    return ret;
}

uint32_t ObjectInPathAnalyzer::renderObstacle(const std::vector<GLfloat>& vertexData, const std::vector<GLfloat>& colorData)
{
    GLuint query;
    glGenQueries(1, &query);
    glBeginQuery(GL_SAMPLES_PASSED, query);

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), &vertexData[0], GL_STATIC_DRAW);

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, colorData.size() * sizeof(GLfloat), &colorData[0], GL_STATIC_DRAW);

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

    glEndQuery(GL_SAMPLES_PASSED);

    GLuint pixelCount{0u};
    GLboolean isValidQuery = glIsQuery(query);
    if (isValidQuery)
    {
        glGetQueryObjectuiv(query,
                            GL_QUERY_RESULT,
                            &pixelCount);
    }
    glDeleteQueries(1, &query);

    return static_cast<uint32_t>(pixelCount);
}


std::vector<GLfloat> ObjectInPathAnalyzer::trivialLaneTriangulation(const LaneData& lane)
{
    // check lane data matches the expectation
    if (lane.leftDiv.size() != lane.rightDiv.size())
    {
        throw std::runtime_error("left and right divider sizes do not match.\n");
    }

    std::vector<GLfloat> ret{};

    for (size_t i = 0u; i + 1 < lane.leftDiv.size(); ++i)
    {
        ret.push_back(lane.leftDiv[i].x);
        ret.push_back(lane.leftDiv[i].y);
        ret.push_back(lane.leftDiv[i].z);

        ret.push_back(lane.leftDiv[i + 1].x);
        ret.push_back(lane.leftDiv[i + 1].y);
        ret.push_back(lane.leftDiv[i + 1].z);

        ret.push_back(lane.rightDiv[i].x);
        ret.push_back(lane.rightDiv[i].y);
        ret.push_back(lane.rightDiv[i].z);

        ret.push_back(lane.leftDiv[i + 1].x);
        ret.push_back(lane.leftDiv[i + 1].y);
        ret.push_back(lane.leftDiv[i + 1].z);

        ret.push_back(lane.rightDiv[i].x);
        ret.push_back(lane.rightDiv[i].y);
        ret.push_back(lane.rightDiv[i].z);

        ret.push_back(lane.rightDiv[i + 1].x);
        ret.push_back(lane.rightDiv[i + 1].y);
        ret.push_back(lane.rightDiv[i + 1].z);
    }

    return ret;
}

void ObjectInPathAnalyzer::renderLane(const std::vector<GLfloat>& vertexData, const std::vector<GLfloat>& colorData)
{
    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), &vertexData[0], GL_STATIC_DRAW);

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, colorData.size() * sizeof(GLfloat), &colorData[0], GL_STATIC_DRAW);

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

    glDrawArrays(GL_TRIANGLES, 0, vertexData.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
}

RGBColor ObjectInPathAnalyzer::getNextColor()
{
    RGBColor ret = COLOR_SET[currColor];

    currColor++;
    currColor %= 6u;

    return ret;
}
