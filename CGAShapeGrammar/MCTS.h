#pragma once

#include <opencv2/opencv.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <list>
#include <map>
#include "Vertex.h"
#include <QImage>
#include "Shape.h"
#include "Grammar.h"

class GLWidget3D;
class RenderManager;

namespace mcts {

	class Nonterminal {
	public:
		boost::shared_ptr<cga::Shape> shape;
		bool visited;
		std::vector<boost::shared_ptr<Nonterminal> > children;

	public:
		Nonterminal(const boost::shared_ptr<cga::Shape>& shape, bool visited);
		boost::shared_ptr<Nonterminal> clone();
	};

	class DerivationTree {
	public:
		boost::shared_ptr<Nonterminal> root;

	public:
		DerivationTree();
		DerivationTree(const boost::shared_ptr<Nonterminal>& root);
		DerivationTree clone() const;
	};

	class State {
	public:
		DerivationTree derivationTree;
		std::list<boost::shared_ptr<Nonterminal> > queue;
		cga::Grammar grammar;

	public:
		State();
		State(const boost::shared_ptr<Nonterminal>& nonterminal, const cga::Grammar& grammar);
		State clone() const;
		bool applyAction(int action);
	};

	class MCTSTreeNode {
	public:
		int visits;
		float bestValue;
		bool valueFixed;
		State state;
		boost::shared_ptr<MCTSTreeNode> parent;
		std::vector<boost::shared_ptr<MCTSTreeNode> > children;
		std::vector<int> unexpandedActions;
		int selectedAction;

	public:
		MCTSTreeNode(const State& state);
		boost::shared_ptr<MCTSTreeNode> UCTSelectChild();
		int randomlySelectAction();
		boost::shared_ptr<MCTSTreeNode> bestChild();
		void addValue(float value);
	};

	class MCTS {
	private:
		cv::Mat target;
		cv::Mat targetDistMap;
		GLWidget3D* glWidget;
		cga::Grammar grammar;

	public:
		MCTS(const cv::Mat& target, GLWidget3D* glWidget, cga::Grammar grammar);

		State inverse(int maxDerivationSteps, int maxMCTSIterations);
		//void randomGeneration(RenderManager* renderManager);
		boost::shared_ptr<MCTSTreeNode> mcts(const boost::shared_ptr<MCTSTreeNode>& rootNode, int maxMCTSIterations);
		boost::shared_ptr<MCTSTreeNode> select(const boost::shared_ptr<MCTSTreeNode>& rootNode);
		boost::shared_ptr<MCTSTreeNode> expand(const boost::shared_ptr<MCTSTreeNode>& leafNode);
		float simulate(const boost::shared_ptr<MCTSTreeNode>& childNode);
		void backpropage(const boost::shared_ptr<MCTSTreeNode>& childNode, float energy);
		float evaluate(const DerivationTree& derivationTree);
		void render(const DerivationTree& derivationTree, QImage& image);
		void generateGeometry(RenderManager* renderManager, const glm::mat4& modelMat, const boost::shared_ptr<Nonterminal>& node, std::vector<boost::shared_ptr<glutils::Face> >& faces);
	};

	std::vector<int> actions(const boost::shared_ptr<Nonterminal>& nonterminal, const cga::Grammar& grammar);
	void randomDerivation(DerivationTree& derivationTree, cga::Grammar& grammar, std::list<boost::shared_ptr<Nonterminal> >& queue);
	void applyRule(boost::shared_ptr<Nonterminal>& nonterminal, int action, cga::Grammar& grammar, std::list<boost::shared_ptr<Nonterminal> >& queue);
	float distance(const cv::Mat& distMap, const cv::Mat& targetDistMap, float alpha, float beta);

}