/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    LCNLPSolver.cpp
 * @author  Duy-Nguyen Ta
 * @author  Krunal Chande
 * @author  Luca Carlone
 * @date    Dec 15, 2014
 */

#include <gtsam_unstable/nonlinear/LCNLPSolver.h>
#include <gtsam_unstable/linear/QPSolver.h>

namespace gtsam {


/* ************************************************************************* */
bool LCNLPSolver::isStationary(const VectorValues& delta) const {
  return delta.vector().lpNorm<Eigen::Infinity>() < errorTol;
}

/* ************************************************************************* */
bool LCNLPSolver::isPrimalFeasible(const LCNLPState& state) const {
  return lcnlp_.linearEqualities.checkFeasibility(state.values, errorTol);
}

/* ************************************************************************* */
bool LCNLPSolver::isDualFeasible(const VectorValues& duals) const {
  BOOST_FOREACH(const NonlinearFactor::shared_ptr& factor, lcnlp_.linearInequalities) {
    NonlinearConstraint::shared_ptr inequality = boost::dynamic_pointer_cast<NonlinearConstraint>(factor);
    Key dualKey = inequality->dualKey();
    if (!duals.exists(dualKey)) continue; // should be inactive constraint!
    double dual = duals.at(dualKey)[0];   // because we only support single-valued inequalities
    if (dual > 0.0) {   // See the explanation in QPSolver::identifyLeavingConstraint, we want dual < 0 ?
      return false;
    }
  }
  return true;
}

/* ************************************************************************* */
bool LCNLPSolver::isComplementary(const LCNLPState& state) const {
  return lcnlp_.linearInequalities.checkFeasibilityAndComplimentary(state.values, state.duals, errorTol);
}

/* ************************************************************************* */
bool LCNLPSolver::checkConvergence(const LCNLPState& state, const VectorValues& delta) const {
  return isStationary(delta) && isPrimalFeasible(state) && isDualFeasible(state.duals) && isComplementary(state);
}

/* ************************************************************************* */
VectorValues LCNLPSolver::initializeDuals() const {
  VectorValues duals;
  BOOST_FOREACH(const NonlinearFactor::shared_ptr& factor, lcnlp_.linearEqualities) {
    NonlinearConstraint::shared_ptr constraint = boost::dynamic_pointer_cast<NonlinearConstraint>(factor);
    duals.insert(constraint->dualKey(), zero(factor->dim()));
  }

  return duals;
}

/* ************************************************************************* */
std::pair<Values, VectorValues> LCNLPSolver::optimize(const Values& initialValues) const {
  LCNLPState state(initialValues);
  state.duals = initializeDuals();
  while (!state.converged && state.iterations < 100) {
    state = iterate(state);
  }
  return std::make_pair(state.values, state.duals);
}

/* ************************************************************************* */
LCNLPState LCNLPSolver::iterate(const LCNLPState& state) const {
  static const bool debug = false;

  // construct the qp subproblem
  QP qp;
  qp.cost = *lcnlp_.cost.linearize(state.values);
  qp.equalities.add(*lcnlp_.linearEqualities.linearize(state.values));
  qp.inequalities.add(*lcnlp_.linearInequalities.linearize(state.values));

  if (debug)
    qp.print("QP subproblem:");

  // solve the QP subproblem
  VectorValues delta, duals;
  QPSolver qpSolver(qp);
  boost::tie(delta, duals) = qpSolver.optimize();

  if (debug)
    delta.print("delta = ");

  if (debug)
    duals.print("duals = ");

  // update new state
  LCNLPState newState;
  newState.values = state.values.retract(delta);
  newState.duals = duals;
  newState.converged = checkConvergence(newState, delta);
  newState.iterations = state.iterations + 1;

  return newState;
}
}



