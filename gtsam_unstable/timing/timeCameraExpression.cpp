/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    timeCameraExpression.cpp
 * @brief   time CalibratedCamera derivatives
 * @author  Frank Dellaert
 * @date    October 3, 2014
 */

#include <gtsam_unstable/slam/expressions.h>
#include <gtsam_unstable/nonlinear/BADFactor.h>
#include <gtsam/slam/ProjectionFactor.h>
#include <gtsam/slam/GeneralSFMFactor.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/geometry/Cal3_S2.h>

#include <time.h>
#include <iostream>

using namespace std;
using namespace gtsam;

static const int n = 100000;

void time(const NonlinearFactor& f, const Values& values) {
  long timeLog = clock();
  GaussianFactor::shared_ptr gf;
  for (int i = 0; i < n; i++)
    gf = f.linearize(values);
  long timeLog2 = clock();
  double seconds = (double) (timeLog2 - timeLog) / CLOCKS_PER_SEC;
  // cout << ((double) n / seconds) << " calls/second" << endl;
  cout << ((double) seconds * 1000000 / n) << " musecs/call" << endl;
}

int main() {

  // Create leaves
  Pose3_ x(1);
  Point3_ p(2);
  Cal3_S2_ K(3);

  // Some parameters needed
  Point2 z(-17, 30);
  SharedNoiseModel model = noiseModel::Unit::Create(2);

  // Create values
  Values values;
  values.insert(1, Pose3());
  values.insert(2, Point3(0, 0, 1));
  values.insert(3, Cal3_S2());

  // UNCALIBRATED

  // Dedicated factor
  // Oct 3, 2014, Macbook Air
  // 4.44887 musecs/call
  GeneralSFMFactor2<Cal3_S2> oldFactor2(z, model, 1, 2, 3);
  time(oldFactor2, values);

  // BADFactor
  // Oct 3, 2014, Macbook Air
  // 20.7554 musecs/call
  BADFactor<Point2> newFactor2(model, z,
      uncalibrate(K, project(transform_to(x, p))));
  time(newFactor2, values);

  // CALIBRATED

  boost::shared_ptr<Cal3_S2> fixedK(new Cal3_S2());

  // Dedicated factor
  // Oct 3, 2014, Macbook Air
  // 3.69707 musecs/call
  GenericProjectionFactor<Pose3, Point3> oldFactor1(z, model, 1, 2, fixedK);
  time(oldFactor1, values);

  // BADFactor
  // Oct 3, 2014, Macbook Air
  // 17.092 musecs/call
  BADFactor<Point2> newFactor1(model, z,
      uncalibrate(Cal3_S2_(*fixedK), project(transform_to(x, p))));
  time(newFactor1, values);

  return 0;
}
