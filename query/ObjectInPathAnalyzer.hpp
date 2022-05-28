#ifndef OBJECT_IN_PATH_ANALYZER_HPP
#define OBJECT_IN_PATH_ANALYZER_HPP

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <GL/glew.h>
#include <vector>
#include <utility> // pair

using float32_t = float;
using float64_t = double;
template<typename T>
struct Point3
{
    T x{0.0};
    T y{0.0};
    T z{0.0};
};

struct RGBColor
{
    float32_t r;
    float32_t g;
    float32_t b;
};

struct RGBAColor
{
    float32_t r;
    float32_t g;
    float32_t b;
    float32_t a;

    RGBAColor() : r(0.f), g(0.f), b(0.f), a(0.f){};
    RGBAColor(const RGBColor& rgb, float32_t _a) : r(rgb.r), g(rgb.g), b(rgb.b), a(_a){};
};

constexpr RGBColor GREEN = {.r = 0.0f, .g = 1.0f, .b = 0.0f};
constexpr RGBColor RED = {.r = 1.0f, .g = 0.0f, .b = 0.0f};
constexpr RGBColor BLUE = {.r = 0.0f, .g = 0.0f, .b = 1.0f};
constexpr RGBColor YELLOW = {.r = 1.0f, .g = 1.0f, .b = 0.0f};
constexpr RGBColor PURPLE = {.r = 1.0f, .g = 0.0f, .b = 1.0f};
constexpr RGBColor CYAN = {.r = 0.0f, .g = 1.0f, .b = 1.0f};

static const RGBColor COLOR_SET[] = 
{
    GREEN,
    RED,
    BLUE,
    YELLOW,
    PURPLE,
    CYAN
};

using Point3f = Point3<float32_t>;
using Point3d = Point3<float64_t>;

struct LaneData
{
    std::vector<Point3f> leftDiv;
    std::vector<Point3f> rightDiv;  // assume left and right dividers have the same size
    uint32_t id;    // assume each lane has a unique id
};

struct ObstacleData
{
    std::vector<Point3f> boundaryPoints{};  // assume 4 vertices. 0-1-2 and 1-2-3 form 2 triangles which cover the total area
    uint32_t id;    // assume each obstacle has a unique id
};

struct FreespaceData
{
    std::vector<std::pair<float32_t, float32_t>> data;
};

// output data type
struct LaneAssignmentData
{
    uint32_t obstacleId;
    std::vector<uint32_t> laneIds;
    std::vector<uint32_t> intersectionPixelCounts;
    uint32_t obstacleTotalPixelCount;
    std::vector<GLfloat> obstacleVertexData{};
};

class ObjectInPathAnalyzer
{
public: 
    ObjectInPathAnalyzer();
    ~ObjectInPathAnalyzer();

    // [optional] get the framebuffer to be renderred in a glfw window, for debugging purpose
    unsigned int getFramebuffer() const {return fbo_;};

    // illustration
    void process(); 

    // process lane assignment
    void process(const std::vector<LaneData>& lanes, const std::vector<ObstacleData>& obstacles);

    void process(const FreespaceData& freespace,
                 const std::vector<ObstacleData>& obstacles,
                 const std::vector<ObstacleData>& querys);

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

    // clear all GL settings
    // call at the begining of each process function call
    void resetGLSettings();

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

    GLuint vertexbuffer_;
    /**** result ****/
    std::vector<LaneAssignmentData> outputData_{};

    /**** render functions ****/
    // assume that obs has 4 vertices. 0-1-2 and 1-2-3 form 2 triangles which cover the total area
    std::vector<GLfloat> trivialObstacleTriangulation(const ObstacleData& obs);

    uint32_t renderObstacle(const std::vector<GLfloat>& vertexData, RGBAColor color);

    // assume left and right dividers have the same size
    std::vector<GLfloat> trivialLaneTriangulation(const LaneData& lane);

    void renderLane(const std::vector<GLfloat>& vertexData, RGBAColor color);

    std::vector<GLfloat> trivialFreespaceTriangulation(const FreespaceData& fs);

    void renderFreespace(const std::vector<GLfloat>& vertexData, RGBAColor colorData);

    uint32_t currColor = 0;
    RGBColor getNextColor();

}; // class ObjectInPathAnalyzer


#endif // OBJECT_IN_PATH_ANALYZER_HPP