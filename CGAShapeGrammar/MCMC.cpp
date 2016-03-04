#include "MCMC.h"
#include "CGA.h"
#include "Rectangle.h"
#include "GLUtils.h"
#include "GLWidget3D.h"
#include <QDir>
#include <time.h>
#include <QTextStream>

namespace mcmc {
	const float SIMILARITY_METRICS_ALPHA = 1000.0f;
	const float SIMILARITY_METRICS_BETA = 1000.0f;

	Chain::Chain(const cga::Grammar& grammar, float T) {
		this->grammar = grammar;
		this->T = T;

		// initialize the paramter values
		for (auto it = this->grammar.attrs.begin(); it != this->grammar.attrs.end(); ++it) {
			float range = it->second.range_end - it->second.range_start;
			it->second.value = std::to_string(range / 9.0f * (rand() % 10) + it->second.range_start);
		}
	}

	void Chain::generateProposal() {
		next_grammar = grammar;

		// next_grammarのパラメータ値を変更する
		for (auto it = next_grammar.attrs.begin(); it != next_grammar.attrs.end(); ++it) {
			float range = it->second.range_end - it->second.range_start;
			float unit = range / 9.0f;
			int index = (std::stof(it->second.value) - it->second.range_start) / unit;
			int r = rand() % 3;

			if (r == 0) {
				index = std::max(0, index - 1);
			}
			else if (r == 2) {
				index = std::min(9, index + 1);
			}

			it->second.value = std::to_string(unit * index + it->second.range_start);
		}
	}

	void Chain::update() {
		// update the best grammar
		if (next_E < best_E) {
			best_E = next_E;
			best_grammar = next_grammar;
		}

		// Metropolis
		float acceptance_ratio = std::min(1.0f, exp(-next_E/T) / exp(-E/T));
		if ((rand() % 100) / 100.0f < acceptance_ratio) {
			grammar = next_grammar;
			E = next_E;
		}

		// decrease T
		T = T / 1.0001f;
	}


	MCMC::MCMC(const cv::Mat& target, GLWidget3D* glWidget, cga::Grammar grammar) {
		this->target = target;
		this->glWidget = glWidget;
		this->orig_grammar = grammar;

		// compute a distance map
		cv::Mat grayImage;
		cv::cvtColor(target, grayImage, CV_RGB2GRAY);
		cv::distanceTransform(grayImage, targetDistMap, CV_DIST_L2, 3);

		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
		//cv::imwrite("results/targetDistMap.png", targetDistMap);
		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////

		targetDistMap.convertTo(targetDistMap, CV_32F);
	}

	void MCMC::run(int maxIterations) {
		if (!QDir("results_mcmc").exists()) {
			QDir().mkpath("results_mcmc");
		}

		time_t start = clock();

		cga::Grammar current_grammar = orig_grammar;
		float T = 1.0f;

		QFile file("results_mcmc/result.txt");
		file.open(QIODevice::WriteOnly);
		QTextStream out(&file);
		
		// initialize chain
		Chain chain1(orig_grammar, 1.0f);
		chain1.E = evaluate(render(chain1.grammar));
		chain1.best_grammar = chain1.grammar;
		chain1.best_E = chain1.E;
		
		Chain chain2(orig_grammar, 10.0f);
		chain2.E = evaluate(render(chain2.grammar));
		chain2.best_grammar = chain2.grammar;
		chain2.best_E = chain2.E;


		for (int iter = 0; iter < maxIterations; ++iter) {
			cga::Grammar next_grammar = current_grammar;

			if (rand() % 10 == 0) {
				// swap
				float p = std::min(1.0f, expf(-chain2.E / chain1.T - chain1.E / chain2.T) / expf(-chain1.E / chain1.T - chain2.E / chain2.T));
				if ((rand() % 100) / 100.0f < p) {
					std::swap(chain1.T, chain2.T);
				}
			}
			else {
				// next_grammarのパラメータ値を変更する
				chain1.generateProposal();
				chain2.generateProposal();

				chain1.next_E = evaluate(render(chain1.next_grammar));
				chain2.next_E = evaluate(render(chain2.next_grammar));

				chain1.update();
				chain2.update();
			}


			if ((iter + 1) % 100 == 0) {
				float best_E;
				QImage best_image = render(chain1.best_grammar);
				if (chain1.best_E < chain2.best_E) {
					best_E = chain1.best_E;
					best_image = render(chain1.best_grammar);
				}
				else {
					best_E = chain2.best_E;
					best_image = render(chain2.best_grammar);
				}

				QString filename = QString("results_mcmc/result_%1.png").arg(iter + 1);
				best_image.save(filename);

				std::cout << "--------------------------------------------------------" << std::endl;
				std::cout << "Iter: " << (iter + 1) << ", Best value: " << best_E << std::endl;
				time_t end = clock();
				std::cout << "Time elapsed: " << (double)(end - start) / CLOCKS_PER_SEC << "sec" << std::endl;

				out << (iter + 1) << "," << best_E << "\n";
			}

			////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
			/*
			QString filename = QString("results_mcmc/result_%1.png").arg(iter);
			cv::Mat backMat = target.clone();
			QImage background(backMat.data, backMat.cols, backMat.rows, backMat.step, QImage::Format_RGB888);
			QPainter painter(&background);
			painter.setOpacity(0.8);
			painter.drawImage(0, 0, next_image);
			background.save(filename);
			*/
			////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
		}

		file.close();

		time_t end = clock();
		std::cout << "Time elapsed: " << (double)(end - start) / CLOCKS_PER_SEC << "sec" << std::endl;

		float best_E;
		QImage best_image = render(chain1.best_grammar);
		if (chain1.best_E < chain2.best_E) {
			best_E = chain1.best_E;
			render(chain1.best_grammar);
		}
		else {
			best_E = chain2.best_E;
			render(chain2.best_grammar);
		}
		
		std::cout << "Best value: " << best_E << std::endl;
	}

	QImage MCMC::render(cga::Grammar& grammar) {
		// run the derivation
		cga::CGA cga;
		boost::shared_ptr<cga::Shape> axiom = boost::shared_ptr<cga::Shape>(new cga::Rectangle("Start", "", glm::translate(glm::rotate(glm::mat4(), -3.141592f * 0.5f, glm::vec3(1, 0, 0)), glm::vec3(-0.5, -0.5, 0)), glm::mat4(), 1, 1, glm::vec3(1, 1, 1)));
		cga.stack.push_back(axiom);
		cga.derive(grammar, true);

		// render the image
		std::vector<boost::shared_ptr<glutils::Face> > faces;
		cga.generateGeometry(faces);
		glWidget->renderManager.removeObjects();
		glWidget->renderManager.addFaces(faces);
		glWidget->renderManager.renderingMode = RenderManager::RENDERING_MODE_LINE;
		glWidget->render();
		return glWidget->grabFrameBuffer();
	}

	float MCMC::evaluate(QImage& image) {
		// convert to gray scale image
		cv::Mat sourceImage(image.height(), image.width(), CV_8UC4, image.bits(), image.bytesPerLine());
		cv::Mat grayImage;
		cv::cvtColor(sourceImage, grayImage, CV_RGB2GRAY);

		// compute a distance map
		cv::Mat distMap;
		cv::distanceTransform(grayImage, distMap, CV_DIST_L2, 3);

		return distance(distMap, targetDistMap, SIMILARITY_METRICS_ALPHA, SIMILARITY_METRICS_BETA);
	}

	float distance(const cv::Mat& distMap, const cv::Mat& targetDistMap, float alpha, float beta) {
		float dist1 = 0.0f;
		float dist2 = 0.0f;

		for (int r = 0; r < distMap.rows; ++r) {
			for (int c = 0; c < distMap.cols; ++c) {
				if (targetDistMap.at<float>(r, c) == 0) {
					dist1 += distMap.at<float>(r, c);
				}
				if (distMap.at<float>(r, c) == 0) {
					dist2 += targetDistMap.at<float>(r, c);
				}
			}
		}

		// 画像サイズに基づいて、normalizeする
		float Z = distMap.rows * distMap.cols * (distMap.rows + distMap.cols) * 0.5;
		dist1 /= Z;
		dist2 /= Z;

		float dist = alpha * dist1 + beta * dist2;

		return dist;
		//return expf(-dist / T);

	}
}