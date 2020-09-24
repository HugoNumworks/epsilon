#include "calculation_store.h"
#include "../shared/poincare_helpers.h"
#include <poincare/rational.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>
#include "../exam_mode_configuration.h"
#include <assert.h>
#include <quiz.h>


using namespace Poincare;
using namespace Shared;

namespace Calculation {

CalculationStore::CalculationStore() :
  m_bufferEnd(m_buffer),
  m_numberOfCalculations(0),
  m_slidedBuffer(false),
  m_indexOfFirstMemoizedCalculationPointer(0)
{
  resetMemoizedModelsAfterCalculationIndex(-1);
}

ExpiringPointer<Calculation> CalculationStore::calculationAtIndex(int i) {
  assert(!m_slidedBuffer);
  assert(i >= 0 && i < m_numberOfCalculations);
  assert(m_indexOfFirstMemoizedCalculationPointer >= 0);
  if (i >= m_indexOfFirstMemoizedCalculationPointer && i < m_indexOfFirstMemoizedCalculationPointer + k_numberOfMemoizedCalculationPointers) {
    // The calculation is within the range of memoized calculations
    Calculation * c = m_memoizedCalculationPointers[i-m_indexOfFirstMemoizedCalculationPointer];
    if (c != nullptr) {
      // The pointer was memoized
      return ExpiringPointer<Calculation>(c);
    }
    c = bufferCalculationAtIndex(i);
    m_memoizedCalculationPointers[i-m_indexOfFirstMemoizedCalculationPointer] = c;
    return c;
  }
  // Slide the memoization buffer
  if (i >= m_indexOfFirstMemoizedCalculationPointer) {
    // Slide the memoization buffer to the left
    memmove(m_memoizedCalculationPointers, m_memoizedCalculationPointers+1, (k_numberOfMemoizedCalculationPointers - 1) * sizeof(Calculation *));
    m_memoizedCalculationPointers[k_numberOfMemoizedCalculationPointers - 1] = nullptr;
    m_indexOfFirstMemoizedCalculationPointer++;
  } else {
    // Slide the memoization buffer to the right
    memmove(m_memoizedCalculationPointers+1, m_memoizedCalculationPointers, (k_numberOfMemoizedCalculationPointers - 1) * sizeof(Calculation *));
    m_memoizedCalculationPointers[0] = nullptr;
    m_indexOfFirstMemoizedCalculationPointer--;
  }
  return calculationAtIndex(i);
}

ExpiringPointer<Calculation> CalculationStore::push(const char * text, Context * context, HeightComputer heightComputer) {
  /* Compute ans now, before the buffer is slided and before the calculation
   * might be deleted */
  quiz_print("#e\n");
  Expression ans = ansExpression(context);
  quiz_print("#a\n");
  // Prepare the buffer for the new calculation
  int minSize = Calculation::MinimalSize();
  assert(k_bufferSize > minSize);
  while (remainingBufferSize() < minSize || m_numberOfCalculations > k_maxNumberOfCalculations) {
    deleteLastCalculation();
  }
  quiz_print("#u\n");
  char * newCalculationsLocation = slideCalculationsToEndOfBuffer();
  char * nextSerializationLocation = m_buffer;
  quiz_print("#i\n");
  // Add the beginning of the calculation
  {
    quiz_print("#h\n");
    /* Copy the begining of the calculation. The calculation minimal size is
     * available, so this memmove will not overide anything. */
    Calculation newCalc = Calculation();
    size_t calcSize = sizeof(newCalc);
    quiz_print("# \n");
    memmove(nextSerializationLocation, &newCalc, calcSize);
    nextSerializationLocation += calcSize;
  }
  quiz_print("#w\n");
  /* Add the input expression.
   * We do not store directly the text entered by the user because we do not
   * want to keep Ans symbol in the calculation store. */
  const char * inputSerialization = nextSerializationLocation;
  {
    quiz_print("#E\n");
    Expression input = Expression::Parse(text, context).replaceSymbolWithExpression(Symbol::Ans(), ans);
    if (!pushSerializeExpression(input, nextSerializationLocation, &newCalculationsLocation)) {
      /* If the input does not fit in the store (event if the current
       * calculation is the only calculation), just replace the calculation with
       * undef. */
      quiz_print("#/\n");
      return emptyStoreAndPushUndef(context, heightComputer);
    }
    quiz_print("#r\n");
    nextSerializationLocation += strlen(nextSerializationLocation) + 1;
  }
  quiz_print("#z\n");
  // Compute and serialize the outputs
  /* The serialized outputs are:
   * - the exact ouput
   * - the approximate output with the maximal number of significant digits
   * - the approximate output with the displayed number of significant digits */
  {
    quiz_print("#n\n");
    // Outputs hold exact output, approximate output and its duplicate
    constexpr static int numberOfOutputs = Calculation::k_numberOfExpressions - 1;
    Expression outputs[numberOfOutputs] = {Expression(), Expression(), Expression()};
    quiz_print("#s\n");
    PoincareHelpers::ParseAndSimplifyAndApproximate(inputSerialization, &(outputs[0]), &(outputs[1]), context, Poincare::ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
    quiz_print("#P\n");
    if (ExamModeConfiguration::exactExpressionsAreForbidden(GlobalPreferences::sharedGlobalPreferences()->examMode()) && outputs[1].hasUnit()) {
      // Hide results with units on units if required by the exam mode configuration
      quiz_print("#y\n");
      outputs[1] = Undefined::Builder();
    }
    outputs[2] = outputs[1];
    quiz_print("#;\n");
    int numberOfSignificantDigits = Poincare::PrintFloat::k_numberOfStoredSignificantDigits;
    quiz_print("#f\n");
    for (int i = 0; i < numberOfOutputs; i++) {
      if (i == numberOfOutputs - 1) {
        numberOfSignificantDigits = Poincare::Preferences::sharedPreferences()->numberOfSignificantDigits();
      }
      if (!pushSerializeExpression(outputs[i], nextSerializationLocation, &newCalculationsLocation, numberOfSignificantDigits)) {
        /* If the exat/approximate output does not fit in the store (event if the
         * current calculation is the only calculation), replace the output with
         * undef if it fits, else replace the whole calcualtion with undef. */
        Expression undef = Undefined::Builder();
        if (!pushSerializeExpression(undef, nextSerializationLocation, &newCalculationsLocation)) {
          quiz_print("#x\n");
          return emptyStoreAndPushUndef(context, heightComputer);
        }
      }
      nextSerializationLocation += strlen(nextSerializationLocation) + 1;
    }
  }
  quiz_print("#-\n");

  // Restore the other calculations
  size_t slideSize = m_buffer + k_bufferSize - newCalculationsLocation;
  memcpy(nextSerializationLocation, newCalculationsLocation, slideSize);
  quiz_print("#=\n");
  m_slidedBuffer = false;
  m_numberOfCalculations++;
  m_bufferEnd+= nextSerializationLocation - m_buffer;
  quiz_print("#t\n");

  // Clean the memoization
  resetMemoizedModelsAfterCalculationIndex(-1);

  ExpiringPointer<Calculation> calculation = ExpiringPointer<Calculation>(reinterpret_cast<Calculation *>(m_buffer));
  /* Heights are computed now to make sure that the display output is decided
   * accordingly to the remaining size in the Poincare pool. Once it is, it
   * can't change anymore: the calculation heights are fixed which ensures that
   * scrolling computation is right. */
  quiz_print("#§\n");
  calculation->setHeights(
      heightComputer(calculation.pointer(), false),
      heightComputer(calculation.pointer(), true));
  return calculation;
}

void CalculationStore::deleteCalculationAtIndex(int i) {
  assert(i >= 0 && i < m_numberOfCalculations);
  assert(!m_slidedBuffer);
  ExpiringPointer<Calculation> calcI = calculationAtIndex(i);
  char * nextCalc = reinterpret_cast<char *>(calcI->next());
  assert(m_bufferEnd >= nextCalc);
  size_t slidingSize = m_bufferEnd - nextCalc;
  memmove((char *)(calcI.pointer()), nextCalc, slidingSize);
  m_bufferEnd -= (nextCalc - (char *)(calcI.pointer()));
  m_numberOfCalculations--;
  resetMemoizedModelsAfterCalculationIndex(i);
}

void CalculationStore::deleteAll() {
  /* We might call deleteAll because the app closed due to a pool allocation
   * failure, so we cannot assert that m_slidedBuffer is false. */
  m_slidedBuffer = false;
  m_bufferEnd = m_buffer;
  m_numberOfCalculations = 0;
  resetMemoizedModelsAfterCalculationIndex(-1);
}

void CalculationStore::tidy() {
  if (m_slidedBuffer) {
    deleteAll();
    return;
  }
  resetMemoizedModelsAfterCalculationIndex(-1);
}

Expression CalculationStore::ansExpression(Context * context) {
  if (numberOfCalculations() == 0) {
    return Rational::Builder(0);
  }
  ExpiringPointer<Calculation> mostRecentCalculation = calculationAtIndex(0);
  /* Special case: the exact output is a Store/Equal expression.
   * Store/Equal expression can only be at the root of an expression.
   * To avoid turning 'ans->A' in '2->A->A' or '2=A->A' (which cannot be
   * parsed), ans is replaced by the approximation output when any Store or
   * Equal expression appears. */
  Expression e = mostRecentCalculation->exactOutput();
  bool exactOuptutInvolvesStoreEqual = e.type() == ExpressionNode::Type::Store || e.type() == ExpressionNode::Type::Equal;
  if (mostRecentCalculation->input().recursivelyMatches(Expression::IsApproximate, context) || exactOuptutInvolvesStoreEqual) {
    return mostRecentCalculation->approximateOutput(context, Calculation::NumberOfSignificantDigits::Maximal);
  }
  return mostRecentCalculation->exactOutput();
}

Calculation * CalculationStore::bufferCalculationAtIndex(int i) {
  int currentIndex = 0;
  for (Calculation * c : *this) {
    if (currentIndex == i) {
      return c;
    }
    currentIndex++;
  }
  assert(false);
  return nullptr;
}

bool CalculationStore::pushSerializeExpression(Expression e, char * location, char * * newCalculationsLocation, int numberOfSignificantDigits) {
  assert(m_slidedBuffer);
  assert(*newCalculationsLocation <= m_buffer + k_bufferSize);
  bool expressionIsPushed = false;
  while (true) {
    size_t locationSize = *newCalculationsLocation - location;
    expressionIsPushed = (PoincareHelpers::Serialize(e, location, locationSize, numberOfSignificantDigits) < (int)locationSize-1);
    if (expressionIsPushed || *newCalculationsLocation >= m_buffer + k_bufferSize) {
      break;
    }
    *newCalculationsLocation = *newCalculationsLocation + deleteLastCalculation();
    assert(*newCalculationsLocation <= m_buffer + k_bufferSize);
  }
  return expressionIsPushed;
}

char * CalculationStore::slideCalculationsToEndOfBuffer() {
  int calculationsSize = m_bufferEnd - m_buffer;
  char * calculationsNewPosition = m_buffer + k_bufferSize - calculationsSize;
  memmove(calculationsNewPosition, m_buffer, calculationsSize);
  m_slidedBuffer = true;
  return calculationsNewPosition;
}

size_t CalculationStore::deleteLastCalculation(const char * calculationsStart) {
  assert(m_numberOfCalculations > 0);
  size_t result;
  if (!m_slidedBuffer) {
    assert(calculationsStart == nullptr);
    const char * previousBufferEnd = m_bufferEnd;
    m_bufferEnd = lastCalculationPosition(m_buffer);
    assert(previousBufferEnd > m_bufferEnd);
    result = previousBufferEnd - m_bufferEnd;
  } else {
    assert(calculationsStart != nullptr);
    const char * lastCalc = lastCalculationPosition(calculationsStart);
    assert(*lastCalc == 0);
    result = m_buffer + k_bufferSize - lastCalc;
    memmove(const_cast<char *>(calculationsStart + result), calculationsStart, m_buffer + k_bufferSize - calculationsStart - result);
  }
  m_numberOfCalculations--;
  resetMemoizedModelsAfterCalculationIndex(-1);
  return result;
}

const char * CalculationStore::lastCalculationPosition(const char * calculationsStart) const {
  assert(calculationsStart >= m_buffer && calculationsStart < m_buffer + k_bufferSize);
  Calculation * c = reinterpret_cast<Calculation *>(const_cast<char *>(calculationsStart));
  int calculationIndex = 0;
  while (calculationIndex < m_numberOfCalculations - 1) {
    c = c->next();
    calculationIndex++;
  }
  return reinterpret_cast<const char *>(c);
}

Shared::ExpiringPointer<Calculation> CalculationStore::emptyStoreAndPushUndef(Context * context, HeightComputer heightComputer) {
  /* We end up here as a result of a failed calculation push. The store
   * attributes are not necessarily clean, so we need to reset them. */
  m_slidedBuffer = false;
  deleteAll();
  return push(Undefined::Name(), context, heightComputer);
}

void CalculationStore::resetMemoizedModelsAfterCalculationIndex(int index) {
  if (index < m_indexOfFirstMemoizedCalculationPointer) {
    memset(&m_memoizedCalculationPointers, 0, k_numberOfMemoizedCalculationPointers * sizeof(Calculation *));
    return;
  }
  if (index >= m_indexOfFirstMemoizedCalculationPointer + k_numberOfMemoizedCalculationPointers) {
    return;
  }
  for (int i = index - m_indexOfFirstMemoizedCalculationPointer; i < k_numberOfMemoizedCalculationPointers; i++) {
    m_memoizedCalculationPointers[i] = nullptr;
  }
}

}
