#pragma once

#include <opencv2/opencv.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Shape.h"
#include "Grammar.h"
#include <QImage>

class GLWidget3D;
class RenderManager;

namespace mcmc {
	class MCMC {
	private:
		cv::Mat target;
		cv::Mat targetDistMap;
		GLWidget3D* glWidget;
		cga::Grammar orig_grammar;

	public:
		MCMC(const cv::Mat& target, GLWidget3D* glWidget, cga::Grammar grammar);

		void run(int maxIterations);
		QImage render(cga::Grammar& gramamr);
		float evaluate(QImage& image, float T);
	};

	float similarity(const cv::Mat& distMap, const cv::Mat& targetDistMap, float alpha, float beta, float T);
}