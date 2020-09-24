#include <poincare/nth_root.h>
#include <poincare/addition.h>
#include <poincare/constant.h>
#include <poincare/division.h>
#include <poincare/naperian_logarithm.h>
#include <poincare/power.h>
#include <poincare/undefined.h>
#include <poincare/nth_root_layout.h>
#include <poincare/subtraction.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>
#include <assert.h>
#include <cmath>
#include <utility>
#include <quiz.h>


namespace Poincare {

constexpr Expression::FunctionHelper NthRoot::s_functionHelper;

int NthRootNode::numberOfChildren() const { return NthRoot::s_functionHelper.numberOfChildren(); }

Layout NthRootNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return NthRootLayout::Builder(
      childAtIndex(0)->createLayout(floatDisplayMode, numberOfSignificantDigits),
      childAtIndex(1)->createLayout(floatDisplayMode, numberOfSignificantDigits));
}

int NthRootNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, NthRoot::s_functionHelper.name());
}

Expression NthRootNode::shallowReduce(ReductionContext reductionContext) {
  return NthRoot(this).shallowReduce(reductionContext);
}

template<typename T>
Evaluation<T> NthRootNode::templatedApproximate(Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit) const {
  quiz_print("#nthroot#templatedApproximate-start\n");
  Evaluation<T> base = childAtIndex(0)->approximate(T(), context, complexFormat, angleUnit);
  quiz_print("#nthroot#templatedApproximate-x\n");
  Evaluation<T> index = childAtIndex(1)->approximate(T(), context, complexFormat, angleUnit);
  quiz_print("#nthroot#templatedApproximate-a\n");
  Complex<T> result = Complex<T>::Undefined();
  quiz_print("#nthroot#templatedApproximate-C\n");
  if (base.type() == EvaluationNode<T>::Type::Complex
      && index.type() == EvaluationNode<T>::Type::Complex)
  {
    quiz_print("#nthroot#templatedApproximate-T\n");
    std::complex<T> basec = static_cast<Complex<T> &>(base).stdComplex();
    std::complex<T> indexc = static_cast<Complex<T> &>(index).stdComplex();
    /* If the complexFormat is Real, we look for nthroot of form root(x,q) with
     * x real and q integer because they might have a real form which does not
     * correspond to the principale angle. */
    if (complexFormat == Preferences::ComplexFormat::Real && indexc.imag() == 0.0 && std::round(indexc.real()) == indexc.real()) {
      // root(x, q) with q integer and x real
      quiz_print("#nthroot#templatedApproximate-n\n");
      Complex<T> result = PowerNode::computeNotPrincipalRealRootOfRationalPow(basec, (T)1.0, indexc.real());
       if (!result.isUndefined()) {
         quiz_print("#nthroot#templatedApproximate-end2\n");
         return std::move(result);
       }
    }
    quiz_print("#nthroot#templatedApproximate-l\n");
    result = PowerNode::compute(basec, std::complex<T>(1.0)/(indexc), complexFormat);
  }
  quiz_print("#nthroot#templatedApproximate-end1\n");
  return std::move(result);
}


Expression NthRoot::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  quiz_print("#nthroot#START\n");
  {
    Expression e = Expression::defaultShallowReduce();
    quiz_print("#nthroot# \n");
    if (e.isUndefined()) {
      quiz_print("#nthroot#;\n");
      return e;
    }
  }
  quiz_print("#nthroot#d\n");
  if (childAtIndex(0).deepIsMatrix(reductionContext.context()) || childAtIndex(1).deepIsMatrix(reductionContext.context())) {
    quiz_print("#nthroot#f\n");
    return replaceWithUndefinedInPlace();
  }
  quiz_print("#nthroot#=\n");
  Expression invIndex = Power::Builder(childAtIndex(1), Rational::Builder(-1));
  quiz_print("#nthroot#l\n");
  Power p = Power::Builder(childAtIndex(0), invIndex);
  quiz_print("#nthroot#c\n");
  invIndex.shallowReduce(reductionContext);
  quiz_print("#nthroot#)\n");
  replaceWithInPlace(p);
  quiz_print("#nthroot#END\n");
  return p.shallowReduce(reductionContext);
}

}
