#include <quiz.h>
#include <apps/shared/global_context.h>
#include <poincare/test/helper.h>
#include <string.h>
#include <assert.h>
#include "../calculation_store.h"

using namespace Poincare;
using namespace Calculation;

void assert_store_is(CalculationStore * store, const char * * result) {
  for (int i = 0; i < store->numberOfCalculations(); i++) {
    quiz_assert(strcmp(store->calculationAtIndex(i)->inputText(), result[i]) == 0);
  }
}

KDCoordinate dummyHeight(::Calculation::Calculation * c, bool expanded) { return 0; }

void assertCalculationIs(const char * input, ::Calculation::Calculation::DisplayOutput display, ::Calculation::Calculation::EqualSign sign, const char * exactOutput, const char * displayedApproximateOutput, const char * storedApproximateOutput, Context * context, CalculationStore * store) {
  quiz_print("ExactOutput :\n");
  quiz_print(exactOutput);
  quiz_print("Pushing to store this input :\n");
  quiz_print(input);
  store->push(input, context, dummyHeight);
  quiz_print("Push successful\n");
  Shared::ExpiringPointer<::Calculation::Calculation> lastCalculation = store->calculationAtIndex(0);
  quiz_assert(lastCalculation->displayOutput(context) == display);
  if (sign != ::Calculation::Calculation::EqualSign::Unknown) {
    quiz_assert(lastCalculation->exactAndApproximateDisplayedOutputsAreEqual(context) == sign);
  }
  if (exactOutput) {
    quiz_print("exactOutputText :\n");
    quiz_print(lastCalculation->exactOutputText());
    quiz_assert_print_if_failure(strcmp(exactOutput, lastCalculation->exactOutputText()) == 0, input);
  }
  quiz_print("exactOutput Assert successful\n");
  if (displayedApproximateOutput) {
    quiz_assert_print_if_failure(strcmp(lastCalculation->approximateOutputText(::Calculation::Calculation::NumberOfSignificantDigits::UserDefined), displayedApproximateOutput) == 0, input);
  }
  if (storedApproximateOutput) {
    quiz_assert_print_if_failure(strcmp(lastCalculation->approximateOutputText(::Calculation::Calculation::NumberOfSignificantDigits::Maximal), storedApproximateOutput) == 0, input);
  }
  store->deleteAll();
}


QUIZ_CASE(calculation_complex_format) {
  Shared::GlobalContext globalContext;
  CalculationStore store;

  Poincare::Preferences::sharedPreferences()->setComplexFormat(Poincare::Preferences::ComplexFormat::Real);

  Poincare::Preferences::sharedPreferences()->setComplexFormat(Poincare::Preferences::ComplexFormat::Cartesian);
  assertCalculationIs("(-2)^(1/4)", ::Calculation::Calculation::DisplayOutput::ExactAndApproximate, ::Calculation::Calculation::EqualSign::Approximation, "root(8,4)/2+root(8,4)/2×𝐢", nullptr, nullptr, &globalContext, &store);

  Poincare::Preferences::sharedPreferences()->setComplexFormat(Poincare::Preferences::ComplexFormat::Polar);
  assertCalculationIs("(-2)^(1/4)", ::Calculation::Calculation::DisplayOutput::ExactAndApproximate, ::Calculation::Calculation::EqualSign::Approximation, "root(2,4)×ℯ^\u0012π/4×𝐢\u0013", nullptr, nullptr, &globalContext, &store);

  Poincare::Preferences::sharedPreferences()->setComplexFormat(Poincare::Preferences::ComplexFormat::Cartesian);
}
