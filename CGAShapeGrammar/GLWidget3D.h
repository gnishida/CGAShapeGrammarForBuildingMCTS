#pragma once

#include <glew.h>
#include "Shader.h"
#include "Vertex.h"
#include <QGLWidget>
#include <QMouseEvent>
#include <QTimer>
#include "Camera.h"
#include "ShadowMapping.h"
#include "RenderManager.h"
#include "CGA.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class MainWindow;

class GLWidget3D : public QGLWidget {
public:
	GLWidget3D(MainWindow *parent = 0);

	void drawScene();
	void render();
	void loadCGA(char* filename);
	void generateBuildingImages(int image_width, int image_height, bool grayscale);
	void runMCTS();
	void EDLine(const cv::Mat& source, cv::Mat& result, bool grayscale);
	void draw2DPolyline(cv::Mat& img, const glm::vec2& p0, const glm::vec2& p1, int polyline_index);
	bool isImageValid(const cv::Mat& image);

	void keyPressEvent(QKeyEvent* e);
	void keyReleaseEvent(QKeyEvent* e);

protected:
	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();    
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent* e);

public:
	static enum { RENDERING_MODE_REGULAR = 0, RENDERING_MODE_LINE };

public:
	MainWindow* mainWin;
	Camera camera;
	glm::vec3 light_dir;
	glm::mat4 light_mvpMatrix;
	bool shiftPressed;

	RenderManager renderManager;

	cga::CGA system;
	std::vector<std::vector<glm::vec2> > style_polylines;
	std::vector<boost::shared_ptr<glutils::Face> > faces;

};

