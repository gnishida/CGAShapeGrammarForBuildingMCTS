#include "MCTS.h"
#include "GLWidget3D.h"
#include "GLUtils.h"
#include <QDir>
#include <QTextStream>
#include <time.h>
#include "Rectangle.h"

namespace mcts {
	const double PARAM_EXPLORATION = 1.0;
	const double PARAM_EXPLORATION_VARIANCE = 0.1;
	const float M_PI = 3.141592653f;
	const float INITIAL_SEGMENT_LENGTH = 0.5f;
	const float INITIAL_SEGMENT_WIDTH = 0.3f;
	const int MAX_LEVEL = 3;
	const int MAX_DIST = 20;
	const float SIMILARITY_METRICS_ALPHA = 1000.0f;
	const float SIMILARITY_METRICS_BETA = 1000.0f;
	const int BASE_PART = 3;
	const int SIMULATION_DEPTH = 100;

	float time_select = 0.0f;
	float time_expand = 0.0f;
	float time_simulate = 0.0f;
	float time_backpropagate = 0.0f;

	Nonterminal::Nonterminal(const boost::shared_ptr<cga::Shape>& shape, bool visited) {
		this->shape = shape;
		this->visited = visited;
	}

	boost::shared_ptr<Nonterminal> Nonterminal::clone() {
		boost::shared_ptr<Nonterminal> newNonterminal = boost::shared_ptr<Nonterminal>(new Nonterminal(shape->clone(shape->_name), visited));
		for (int i = 0; i < children.size(); ++i) {
			newNonterminal->children.push_back(children[i]->clone());
		}
		return newNonterminal;
	}

	DerivationTree::DerivationTree() {
		root = NULL;
	}

	DerivationTree::DerivationTree(const boost::shared_ptr<Nonterminal>& root) {
		this->root = root;
	}

	DerivationTree DerivationTree::clone() const {
		if (root != NULL) {
			return DerivationTree(root->clone());
		}
		else {
			return DerivationTree();
		}
	}

	State::State() {
	}

	State::State(const boost::shared_ptr<Nonterminal>& root, const cga::Grammar& grammar) {
		derivationTree = DerivationTree(root);
		this->queue.push_back(root);
		this->grammar = grammar;
	}

	State State::clone() const {
		// derivationTreeは、コピーコンストラクタにより、クローンを生成
		State newState;
		newState.derivationTree = derivationTree.clone();
		newState.grammar = grammar;

		std::list<boost::shared_ptr<Nonterminal> > queue;
		queue.push_back(newState.derivationTree.root);
		while (!queue.empty()) {
			boost::shared_ptr<Nonterminal> node = queue.front();
			queue.pop_front();

			if (!node->visited) {
				newState.queue.push_back(node);
			}

			for (int i = 0; i < node->children.size(); ++i) {
				queue.push_back(node->children[i]);
			}
		}

		return newState;
	}

	bool State::applyAction(int action) {
		if (queue.empty()) return false;

		boost::shared_ptr<Nonterminal> node = queue.front();
		queue.pop_front();

		if (node->visited) return false;

		applyRule(node, action, grammar, queue);
		node->visited = true;

		return true;
	}

	MCTSTreeNode::MCTSTreeNode(const State& state) {
		visits = 0;
		bestValue = 0;
		meanValue = 0;
		valueFixed = false;
		varianceValues = 0;
		this->state = state;
		parent = NULL;

		if (!state.queue.empty()) {
			// queueが空でない場合、先頭のnon-terminalに基づいて、unexpandedActionsを設定する
			this->unexpandedActions = actions(state.queue.front(), this->state.grammar);
		}
	}

	boost::shared_ptr<MCTSTreeNode> MCTSTreeNode::UCTSelectChild() {
		double max_uct = -std::numeric_limits<double>::max();
		boost::shared_ptr<MCTSTreeNode> bestChild = NULL;

		for (int i = 0; i < children.size(); ++i) {
			// スコアが確定済みの子ノードは探索対象外とする
			if (children[i]->valueFixed) continue;

			double uct;
			if (children[i]->visits == 0) {
				uct = 10000 + rand() % 1000;
			}
			else {
				uct = children[i]->bestValue
					+ PARAM_EXPLORATION * sqrt(2 * log((double)visits) / (double)children[i]->visits);
					//+ PARAM_EXPLORATION_VARIANCE * sqrt(children[i]->varianceValues + 0.0 / (double)children[i]->visits);
			}

			if (uct > max_uct) {
				max_uct = uct;
				bestChild = children[i];
			}
		}

		return bestChild;
	}

	boost::shared_ptr<MCTSTreeNode> MCTSTreeNode::bestChild() {
		double bestValue = -std::numeric_limits<double>::max();
		boost::shared_ptr<MCTSTreeNode> bestChild = NULL;

		for (int i = 0; i < children.size(); ++i) {
			if (children[i]->bestValue > bestValue) {
				bestValue = children[i]->bestValue;
				bestChild = children[i];
			}
		}

		return bestChild;
	}

	void MCTSTreeNode::addValue(float value) {
		values.push_back(value);
		if (value > bestValue) {
			bestValue = value;
		}

		// compute the variance
		float total_val = 0.0f;
		float total_val2 = 0.0f;
		for (int i = 0; i < values.size(); ++i) {
			total_val = values[i];
			total_val2 = values[i] * values[i];
		}

		meanValue = total_val / values.size();
		varianceValues = total_val2 / values.size() - meanValue * meanValue;
	}

	int MCTSTreeNode::randomlySelectAction() {
		int index = rand() % unexpandedActions.size();
		int action = unexpandedActions[index];
		unexpandedActions.erase(unexpandedActions.begin() + index);
		return action;
	}

	MCTS::MCTS(const cv::Mat& target, GLWidget3D* glWidget, cga::Grammar grammar) {
		this->target = target;
		this->glWidget = glWidget;
		this->grammar = grammar;

		// compute a distance map
		cv::Mat grayImage;
		cv::cvtColor(target, grayImage, CV_RGB2GRAY);
		cv::distanceTransform(grayImage, targetDistMap, CV_DIST_L2, 3);

		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
		//cv::imwrite("results/targetDistMap.png", targetDistMap);
		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////

		targetDistMap.convertTo(targetDistMap, CV_32F);
	}

	State MCTS::inverse(int maxDerivationSteps, int maxMCTSIterations) {
		// initialize computation time
		time_select = 0.0f;
		time_expand = 0.0f;
		time_simulate = 0.0f;
		time_backpropagate = 0.0f;

		if (!QDir("results").exists()) {
			QDir().mkpath("results");
		}

		boost::shared_ptr<cga::Shape> start = boost::shared_ptr<cga::Shape>(new cga::Rectangle("Start", "", glm::translate(glm::rotate(glm::mat4(), -3.141592f * 0.5f, glm::vec3(1, 0, 0)), glm::vec3(-0.5, -0.5, 0)), glm::mat4(), 1, 1, glm::vec3(1, 1, 1)));
		State state(boost::shared_ptr<Nonterminal>(new Nonterminal(start, false)), grammar);
		boost::shared_ptr<MCTSTreeNode> node = boost::shared_ptr<MCTSTreeNode>(new MCTSTreeNode(state));

		for (int iter = 0; iter < maxDerivationSteps; ++iter) {
			std::cout << "iter: " << (iter + 1) << std::endl;

			int best_action = mcts(node, maxMCTSIterations);

			// 次のルートノードを作成
			State child_state = node->state.clone();
			child_state.applyAction(best_action);
			node = boost::shared_ptr<MCTSTreeNode>(new MCTSTreeNode(child_state));
			
			////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
			QString filename = QString("results/result_%1.png").arg(iter);
			QImage image;
			render(node->state.derivationTree, image);
			cv::Mat backMat = target.clone();
			QImage background(backMat.data, backMat.cols, backMat.rows, backMat.step, QImage::Format_RGB888);
			QPainter painter(&background);
			painter.setOpacity(0.8);
			painter.drawImage(0, 0, image);
			background.save(filename);
			////////////////////////////////////////////// DEBUG //////////////////////////////////////////////

			// これ以上derivationできない場合は、終了
			if (node->state.queue.empty()) break;
			if (node->unexpandedActions.size() == 0) break;
		}

		// show compuattion time
		std::cout << "Select: " << time_select << std::endl;
		std::cout << "Expand: " << time_expand << std::endl;
		std::cout << "Simulate: " << time_simulate << std::endl;
		std::cout << "Back: " << time_backpropagate << std::endl;

		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
		for (auto it = node->state.grammar.attrs.begin(); it != node->state.grammar.attrs.end(); ++it) {
			std::cout << "param: " << it->first << " = " << it->second.value << std::endl;
		}
		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////


		return node->state;
	}

#if 0
	void MCTS::randomGeneration(RenderManager* renderManager) {
		State state(boost::shared_ptr<cga::Shape>(new cga::Rectangle("X", 0, 0, INITIAL_SEGMENT_LENGTH)));
		randomDerivation(state.derivationTree, state.queue);

		glWidget->renderManager.removeObjects();
		std::vector<Vertex> vertices;
		generateGeometry(renderManager, glm::mat4(), state.derivationTree.root, vertices);
		glWidget->renderManager.addObject("tree", "", vertices, true);
	}
#endif

	/**
	 * 現在のstateをルートノードとしてsearch treeを生成し、
	 * MCTSアルゴリズムによりbest actionを探し、
	 * 現在のstateに反映して新しいstateを返却する。
	 *
	 * @param state					現在のstate
	 * @param maxMCTSIterations		MCTSアルゴリズムを何回走らせるか
	 * @return						新しいstate
	 */
	int MCTS::mcts(const boost::shared_ptr<MCTSTreeNode>& rootNode, int maxMCTSIterations) {
		//boost::shared_ptr<MCTSTreeNode> rootNode = boost::shared_ptr<MCTSTreeNode>(new MCTSTreeNode(state));
		for (int iter = 0; iter < maxMCTSIterations; ++iter) {
			// MCTS selection
			time_t start = clock();
			boost::shared_ptr<MCTSTreeNode> LeafNode = select(rootNode);
			time_t end = clock();
			time_select += (double)(end - start) / CLOCKS_PER_SEC;

			// MCTS expansion
			start = clock();
			boost::shared_ptr<MCTSTreeNode> childNode = expand(LeafNode);
			end = clock();
			time_expand += (double)(end - start) / CLOCKS_PER_SEC;

			// MCTS simulation
			start = clock();
			float value = simulate(childNode);
			end = clock();
			time_simulate += (double)(end - start) / CLOCKS_PER_SEC;

			// MCTS backpropagation
			start = clock();
			backpropage(childNode, value);
			end = clock();
			time_backpropagate += (double)(end - start) / CLOCKS_PER_SEC;

			// 子ノードが1個なら、終了
			if (rootNode->unexpandedActions.size() == 0 && rootNode->children.size() <= 1) break;
		}

		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
		if (!QDir("results/").exists())	QDir().mkpath("results/");
		QFile file("results/visits.txt");
		file.open(QIODevice::Append);
		QTextStream out(&file);
		for (int i = 0; i < rootNode->children.size(); ++i) {
			if (i > 0) out << ",";
			out << rootNode->children[i]->selectedAction << "(#visits: " << rootNode->children[i]->visits << ", #val: " << rootNode->children[i]->bestValue << ")";
		}
		out << "\n";
		file.close();
		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////

		return rootNode->bestChild()->selectedAction;
	}

	boost::shared_ptr<MCTSTreeNode> MCTS::select(const boost::shared_ptr<MCTSTreeNode>& rootNode) {
		boost::shared_ptr<MCTSTreeNode> node = rootNode;

		// 探索木のリーフノードまで探索
		while (node->unexpandedActions.size() == 0 && node->children.size() > 0) {
			boost::shared_ptr<MCTSTreeNode> childNode = node->UCTSelectChild();
			if (childNode == NULL) break;
			node = childNode;
		}

		return node;
	}

	boost::shared_ptr<MCTSTreeNode> MCTS::expand(const boost::shared_ptr<MCTSTreeNode>& leafNode) {
		boost::shared_ptr<MCTSTreeNode> node = leafNode;

		// expandできない場合、つまり、本当のleafNodeなら、そのノードをそのまま返す
		if (node->unexpandedActions.size() == 0) {
			return node;
		}
		// 子ノードがまだ全てexpandされていない時は、1つランダムにexpand
		else {
			int action = node->randomlySelectAction();
			
			State child_state = node->state.clone();
			child_state.applyAction(action);

			boost::shared_ptr<MCTSTreeNode> child_node = boost::shared_ptr<MCTSTreeNode>(new MCTSTreeNode(child_state));
			child_node->selectedAction = action;
			node->children.push_back(child_node);
			child_node->parent = node;

			return child_node;
		}
	}
	
	float MCTS::simulate(const boost::shared_ptr<MCTSTreeNode>& childNode) {
		State state = childNode->state.clone();
		randomDerivation(state.derivationTree, state.grammar, state.queue);
		return evaluate(state.derivationTree);
	}

	void MCTS::backpropage(const boost::shared_ptr<MCTSTreeNode>& childNode, float value) {
		boost::shared_ptr<MCTSTreeNode> node = childNode;

		// リーフノードなら、スコアを確定する
		if (node->unexpandedActions.size() == 0 && node->children.size() == 0) {
			node->valueFixed = true;
		}

		while (node != NULL) {
			node->visits++;
			node->addValue(value);

			// 子ノードが全て展開済みで、且つ、スコア確定済みなら、このノードのスコアも確定とする
			if (node->unexpandedActions.size() == 0) {
				bool fixed = true;
				for (int c = 0; c < node->children.size(); ++c) {
					if (!node->children[c]->valueFixed) {
						fixed = false;
						break;
					}
				}
				node->valueFixed = fixed;
			}

			node = node->parent;
		}
	}

	float MCTS::evaluate(const DerivationTree& derivationTree) {
		QImage image;
		render(derivationTree.root, image);
		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
		//image.save("results/output.png");
		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////

		cv::Mat sourceImage(image.height(), image.width(), CV_8UC4, image.bits(), image.bytesPerLine());
		cv::Mat grayImage;
		cv::cvtColor(sourceImage, grayImage, CV_RGB2GRAY);


		// compute a distance map
		cv::Mat distMap;
		cv::distanceTransform(grayImage, distMap, CV_DIST_L2, 3);
		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
		//cv::imwrite("results/distMap.png", distMap);
		////////////////////////////////////////////// DEBUG //////////////////////////////////////////////
		distMap.convertTo(distMap, CV_32F);

		// compute the squared difference
		return similarity(distMap, targetDistMap, SIMILARITY_METRICS_ALPHA, SIMILARITY_METRICS_BETA);
	}

	void MCTS::render(const DerivationTree& derivationTree, QImage& image) {
		glWidget->renderManager.removeObjects();
		std::vector<boost::shared_ptr<glutils::Face> > faces;
		generateGeometry(&glWidget->renderManager, glm::mat4(), derivationTree.root, faces);
		glWidget->renderManager.addFaces(faces);
		glWidget->renderManager.renderingMode = RenderManager::RENDERING_MODE_LINE;
		glWidget->render();
		
		image = glWidget->grabFrameBuffer();
	}

	void MCTS::generateGeometry(RenderManager* renderManager, const glm::mat4& modelMat, const boost::shared_ptr<Nonterminal>& node, std::vector<boost::shared_ptr<glutils::Face> >& faces) {
		glm::mat4 mat;

		if (node->children.size() == 0) {
			node->shape->generateGeometry(faces, 1.0f);
		}
		else {
			for (int i = 0; i < node->children.size(); ++i) {
				generateGeometry(renderManager, mat, node->children[i], faces);
			}
		}
	}

	std::vector<int> actions(const boost::shared_ptr<Nonterminal>& nonterminal, const cga::Grammar& grammar) {
		std::vector<int> ret;
		
		if (grammar.rules.find(nonterminal->shape->_name) == grammar.rules.end()) {
			return ret;
		}

		int num_options = 1;
		for (int i = 0; i < grammar.rules.at(nonterminal->shape->_name).operators.size(); ++i) {
			for (int j = 0; j < grammar.rules.at(nonterminal->shape->_name).operators[i]->params.size(); ++j) {
				if (grammar.attrs.find(grammar.rules.at(nonterminal->shape->_name).operators[i]->params[j]) != grammar.attrs.end()) {
					if (!grammar.attrs.at(grammar.rules.at(nonterminal->shape->_name).operators[i]->params[j]).fixed) {
						num_options *= 10;
					}
				}
			}
		}
				
		for (int i = 0; i < num_options; ++i) {
			ret.push_back(i);
		}
		
		return ret;
	}

	/**
	 * 指定されたderivation treeに対して、derivationを最後まで実行する。
	 * grammarおよびqueueは変更される。
	 * derivationTreeは使わないが、間接的に木構造は更新される。
	 *
	 * @param derivationTree	derivation tree
	 * @param grammar			grammar
	 * @param queue				queue for derivation
	 */
	void randomDerivation(DerivationTree& derivationTree, cga::Grammar& grammar, std::list<boost::shared_ptr<Nonterminal> >& queue) {
		// 未決定のパラメータについて、適当に数値を割り当てる
		for (auto it = grammar.attrs.begin(); it != grammar.attrs.end(); ++it) {
			if (it->second.fixed) continue;
			if (!it->second.hasRange) continue;

			it->second.value = std::to_string((it->second.range_end - it->second.range_start) / 9 * (rand() % 10) + it->second.range_start);
		}

		for (int iter = 0; iter < SIMULATION_DEPTH && !queue.empty(); ++iter) {
		//while (!queue.empty()) {
			boost::shared_ptr<Nonterminal> nonterminal = queue.front();
			queue.pop_front();

			if (grammar.rules.find(nonterminal->shape->_name) == grammar.rules.end()) continue;

			boost::shared_ptr<cga::Shape> shape = nonterminal->shape;
			std::list<boost::shared_ptr<cga::Shape> > stack;

			for (int i = 0; i < grammar.rules[nonterminal->shape->_name].operators.size(); ++i) {
				shape = grammar.rules[nonterminal->shape->_name].operators[i]->apply(shape, grammar, stack);

				while (!stack.empty()) {
					boost::shared_ptr<cga::Shape> child = stack.front();
					stack.pop_front();

					boost::shared_ptr<Nonterminal> child_nt = boost::shared_ptr<Nonterminal>(new Nonterminal(child, false));
					queue.push_back(child_nt);
					nonterminal->children.push_back(child_nt);
				}

				if (shape == NULL) break;
			}
		}
	}

	/**
	 * 指定されたnon-terminalに、指定されたactionを適用する。
	 * 適用後に生成されるnon-terminalを、元のnon-terminalの子ノードとして登録するとともに、
	 * キューにも格納する。
	 */
	void applyRule(boost::shared_ptr<Nonterminal>& nonterminal, int action, cga::Grammar& grammar, std::list<boost::shared_ptr<Nonterminal> >& queue) {
		boost::shared_ptr<Nonterminal> orig_nonterminal = nonterminal;
		nonterminal->visited = true;

		boost::shared_ptr<cga::Shape> shape = nonterminal->shape;

		if (grammar.rules.find(shape->_name) == grammar.rules.end()) {
			return;
		}

		for (int i = 0; i < grammar.rules[shape->_name].operators.size(); ++i) {
			std::list<boost::shared_ptr<cga::Shape> > children;

			if (grammar.rules[shape->_name].operators[i]->params.size() == 0) {
				shape = grammar.rules[shape->_name].operators[i]->apply(shape, grammar, children);
			}
			else {
				for (int j = 0; j < grammar.rules[shape->_name].operators[i]->params.size(); ++j) {
					std::string param_name = grammar.rules[shape->_name].operators[i]->params[j];
					if (!grammar.attrs[param_name].fixed) {
						int a = action % 10;
						action = (action - a) / 10;
						float range = grammar.attrs[param_name].range_end - grammar.attrs[param_name].range_start;
						grammar.attrs[param_name].value = std::to_string(range / 9 * a + grammar.attrs[param_name].range_start);
						grammar.attrs[param_name].fixed = true;
					}
				}

				shape = grammar.rules[shape->_name].operators[i]->apply(shape, grammar, children);
			}

			// 子ノードを追加する
			while (!children.empty()) {
				boost::shared_ptr<cga::Shape> child = children.front();
				children.pop_front();

				boost::shared_ptr<Nonterminal> child_nt = boost::shared_ptr<Nonterminal>(new Nonterminal(child, false));

				// Terminalなら、訪問済みマークをつけることで、clone()の際にキューに入らないようにする。
				if (actions(child_nt, grammar).size() == 0) {
					child_nt->visited = true;
				}
				// Non-terminalなら、キューに追加する
				else {
					queue.push_back(child_nt);
				}
				orig_nonterminal->children.push_back(child_nt);
			}

			if (shape == NULL) break;
		}
	}

	float similarity(const cv::Mat& distMap, const cv::Mat& targetDistMap, float alpha, float beta) {
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

		return expf(-dist);

	}

}