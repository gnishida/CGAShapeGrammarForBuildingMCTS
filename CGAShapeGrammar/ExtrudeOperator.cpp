#include "ExtrudeOperator.h"
#include "CGA.h"
#include "Shape.h"

namespace cga {

ExtrudeOperator::ExtrudeOperator(const std::string& height) {
	this->name = "extrude";
	this->params.push_back(height);
	//this->height = height;
}

boost::shared_ptr<Shape> ExtrudeOperator::apply(boost::shared_ptr<Shape>& shape, const Grammar& grammar, std::list<boost::shared_ptr<Shape> >& stack) {
	float actual_height = grammar.evalFloat(params[0], shape);

	return shape->extrude(shape->_name, actual_height);
}

}
