/*
 * UNICADO - UNIversity Conceptual Aircraft Design and Optimization
 *
 * Copyright (C) 2025 UNICADO consortium
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Description:
 * This file is part of UNICADO.
 */

#include <standardFiles/functions.h>
#include <gtest/gtest.h>
#include <algorithm>



class RegressionCalculationFixture : public::testing::Test {
 protected:
  void SetUp() override {
    coefficients_ord_1 = std::vector<double>({0.0,1.0});
    coefficients_ord_2 = std::vector<double>({1.5,2.3,3.2});
    coefficients_ord_3 = std::vector<double>({1.5,2.3,3.2,5.2});
    for (int i = 0; i < 10; ++i) {
      x.push_back( 3.2*i - 5.0 );
    }
    y_ord_1.resize(x.size());
    y_ord_2.resize(x.size());
    y_ord_3.resize(x.size());
    std::transform(x.begin(),x.end(), y_ord_1.begin(), [this](double val) { return coefficients_ord_1.at(0) + coefficients_ord_1.at(1)*val;});
    std::transform(x.begin(),x.end(), y_ord_2.begin(), [this](double val) { return coefficients_ord_2.at(0) + coefficients_ord_2.at(1)*val + coefficients_ord_2.at(2)*val*val;});
    std::transform(x.begin(),x.end(), y_ord_3.begin(), [this](double val) { return coefficients_ord_3.at(0) + coefficients_ord_3.at(1)*val + coefficients_ord_3.at(2)*val*val + coefficients_ord_3.at(3)*val*val*val;});
  }
 public:
  std::vector<double> x;
  std::vector<double> y_ord_1;
  std::vector<double> y_ord_2;
  std::vector<double> y_ord_3;
  std::vector<double> coefficients_ord_1;
  std::vector<double> coefficients_ord_2;
  std::vector<double> coefficients_ord_3;
};

TEST_F(RegressionCalculationFixture, EigenExecutionOrderOne) {
  int regressionOrder = 1;
  std::vector<double> output = calcRegressionCoefficientsUsingQRdecomp(x, y_ord_1, regressionOrder);
  for (int i = 0; i < regressionOrder+1; ++i) {
    std::cout << output.at(i) << std::endl;
    EXPECT_NEAR(coefficients_ord_1.at(i),output.at(i),ACCURACY_HIGH);
  }
}

TEST_F(RegressionCalculationFixture, EigenExecutionOrderTwo) {
  int regressionOrder = 2;
  std::vector<double> output = calcRegressionCoefficientsUsingQRdecomp(x, y_ord_2, regressionOrder);
  for (int i = 0; i < regressionOrder+1; ++i) {
    std::cout << output.at(i) << std::endl;
    EXPECT_NEAR(coefficients_ord_2.at(i),output.at(i),ACCURACY_HIGH);
  }
}

TEST_F(RegressionCalculationFixture, EigenExecutionOrderThree) {
  int regressionOrder = 3;
  std::vector<double> output = calcRegressionCoefficientsUsingQRdecomp(x, y_ord_3, regressionOrder);
  for (int i = 0; i < regressionOrder+1; ++i) {
    std::cout << output.at(i) << std::endl;
    EXPECT_NEAR(coefficients_ord_3.at(i),output.at(i),ACCURACY_HIGH);
  }
}
