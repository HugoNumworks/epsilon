#include <poincare/parenthesis.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>
#include <quiz.h>

namespace Poincare {

int ParenthesisNode::polynomialDegree(Context * context, const char * symbolName) const {
  return childAtIndex(0)->polynomialDegree(context, symbolName);
}

Layout ParenthesisNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::Parentheses(childAtIndex(0)->createLayout(floatDisplayMode, numberOfSignificantDigits), false);
}

int ParenthesisNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, "");
}

Expression ParenthesisNode::shallowReduce(ReductionContext reductionContext) {
  return Parenthesis(this).shallowReduce();
}

template<typename T>
Evaluation<T> ParenthesisNode::templatedApproximate(Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit) const {
  return childAtIndex(0)->approximate(T(), context, complexFormat, angleUnit);
}

Expression Parenthesis::shallowReduce() {
  quiz_print("###ParenthesisReduce##x\n");
  Expression e = Expression::defaultShallowReduce();
  quiz_print("###ParenthesisReduce##f\n");
  if (e.isUndefined()) {
    quiz_print("###ParenthesisReduce##e\n");
    return e;
  }
  quiz_print("###ParenthesisReduce##g\n");
  Expression c = childAtIndex(0);
  quiz_print("###ParenthesisReduce##h\n");
  replaceWithInPlace(c);
  quiz_print("###ParenthesisReduce##j\n");
  return c;
}

}
