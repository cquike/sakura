#include <iostream>
#include <memory>
#include <cmath>
#include <string>
#include <stdarg.h>

#include <libsakura/sakura.h>

#include "gtest/gtest.h"

#include "aligned_memory.h"
//#include "CubicSplineInterpolator1D.h"

void InitializeDoubleArray(size_t num_array, double array[], ...) {
	va_list arguments_list;
	va_start(arguments_list, array);
	for (size_t i = 0; i < num_array; ++i) {
		array[i] = va_arg(arguments_list, double);
	}
}

void InitializeFloatArray(size_t num_array, float array[], ...) {
	va_list arguments_list;
	va_start(arguments_list, array);
	for (size_t i = 0; i < num_array; ++i) {
		array[i] = static_cast<float>(va_arg(arguments_list, double));
	}
}

class Interpolate1dFloatTest: public ::testing::Test {
protected:
	virtual void SetUp() {
		initialize_result_ = sakura_Initialize(nullptr, nullptr);
		polynomial_order_ = 0;
		sakura_alignment_ = sakura_GetAlignment();
	}
	virtual void TearDown() {
		sakura_CleanUp();
	}
	virtual void AllocateMemory(size_t num_base, size_t num_interpolated) {
		size_t num_arena_base = num_base + sakura_alignment_ - 1;
		size_t num_arena_interpolated = num_interpolated + sakura_alignment_
				- 1;
		storage_for_x_base_.reset(new double[num_arena_base]);
		x_base_ = sakura_AlignDouble(num_arena_base, storage_for_x_base_.get(),
				num_base);
		storage_for_x_interpolated_.reset(new double[num_arena_interpolated]);
		x_interpolated_ = sakura_AlignDouble(num_arena_interpolated,
				storage_for_x_interpolated_.get(), num_interpolated);
		storage_for_y_base_.reset(new float[num_arena_base]);
		y_base_ = sakura_AlignFloat(num_arena_base, storage_for_y_base_.get(),
				num_base);
		storage_for_y_interpolated_.reset(new float[num_arena_interpolated]);
		y_interpolated_ = sakura_AlignFloat(num_arena_interpolated,
				storage_for_y_interpolated_.get(), num_interpolated);
		storage_for_y_expected_.reset(new float[num_arena_interpolated]);
		y_expected_ = sakura_AlignFloat(num_arena_interpolated,
				storage_for_y_expected_.get(), num_interpolated);

		// check alignment
		ASSERT_TRUE(x_base_ != nullptr)<< "x_base_ is null";
		ASSERT_TRUE(sakura_IsAligned(x_base_))<< "x_base_ is not aligned";
		ASSERT_TRUE(sakura_IsAligned(y_base_))<< "y_base_ is not aligned";
		ASSERT_TRUE(sakura_IsAligned(x_interpolated_))<< "x_interpolated_ is not aligned";
		ASSERT_TRUE(sakura_IsAligned(y_interpolated_))<< "y_interpolated_ is not aligned";
	}

	virtual void RunInterpolate1d(sakura_InterpolationMethod interpolation_method,
			size_t num_base, size_t num_interpolated, sakura_Status expected_status,
			bool check_result) {
		// sakura must be properly initialized
		ASSERT_EQ(sakura_Status_kOK, initialize_result_)
				<< "sakura must be properly initialized!";

		// execute interpolation
		sakura_Status result = sakura_Interpolate1dFloat(
				interpolation_method, polynomial_order_, num_base,
				x_base_, y_base_, num_interpolated, x_interpolated_,
				y_interpolated_);

		// Should return InvalidArgument status
		std::string message = (expected_status == sakura_Status_kOK) ?
		"Interpolate1dFloat had any problems during execution." :
		"Interpolate1dFloat should fail!";
		EXPECT_EQ(expected_status, result) << message;

		if (check_result && (expected_status == result)) {
			// Value check
			for (size_t index = 0; index < num_interpolated; ++index) {
				std::cout << "Expected value at index " << index << ": "
				<< y_expected_[index] << std::endl;
				EXPECT_FLOAT_EQ(y_expected_[index], y_interpolated_[index])
				<< "interpolated value differs from expected value at " << index
				<< ": " << y_expected_[index] << ", " << y_interpolated_[index];
			}
		}
	}

	sakura_Status initialize_result_;
	size_t sakura_alignment_;
	int polynomial_order_;

	std::unique_ptr<double[]> storage_for_x_base_;
	std::unique_ptr<float[]> storage_for_y_base_;
	std::unique_ptr<double[]> storage_for_x_interpolated_;
	std::unique_ptr<float[]> storage_for_y_interpolated_;
	std::unique_ptr<float[]> storage_for_y_expected_;
	double *x_base_;
	double *x_interpolated_;
	float *y_base_;
	float *y_interpolated_;
	float *y_expected_;
};

TEST_F(Interpolate1dFloatTest, InvalidType) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 5;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.7, 0.5);

// execute interpolation
// Should return InvalidArgument status
	RunInterpolate1d(sakura_InterpolationMethod_kNumMethod, num_base,
			num_interpolated, sakura_Status_kInvalidArgument, false);
}

TEST_F(Interpolate1dFloatTest, ZeroLengthBaseArray) {
	// initial setup
	size_t const num_base = 0;
	size_t const num_interpolated = 5;

	// execute interpolation
	// Should return InvalidArgument status
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kInvalidArgument, false);
}

TEST_F(Interpolate1dFloatTest, NegativePolynomialOrder) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 5;
	polynomial_order_ = -1;

	// execute interpolation
	// Should return InvalidArgument status
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kInvalidArgument, false);
}

TEST_F(Interpolate1dFloatTest, NegativePolynomialOrderButNearest) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	polynomial_order_ = -1;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 1.0, 1.0,
			-1.0, -1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, InputArrayNotAligned) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 5;
	AllocateMemory(num_base, num_interpolated);

	// execute interpolation
	x_base_ = &(x_base_[1]);
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, 1,
			num_interpolated, sakura_Status_kInvalidArgument, false);
}

TEST_F(Interpolate1dFloatTest, Nearest) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 1.0, 1.0,
			-1.0, -1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, NearestDescending) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 1.0, 0.0);
	InitializeFloatArray(num_base, y_base_, -1.0, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 1.0, 1.0,
			-1.0, -1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, NearestOpposite) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 1.0, 0.0);
	InitializeFloatArray(num_base, y_base_, -1.0, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, 1.5, 0.7, 0.5, 0.1,
			0.0, -1.0);
	InitializeFloatArray(num_interpolated, y_expected_, -1.0, -1.0, 1.0, 1.0,
			1.0, 1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, SingleBase) {
	// initial setup
	size_t const num_base = 1;
	size_t const num_interpolated = 3;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0);
	InitializeFloatArray(num_base, y_base_, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, Linear) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 0.8, 0.0,
			-0.4, -1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kLinear, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, LinearDescending) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 1.0, 0.0);
	InitializeFloatArray(num_base, y_base_, -1.0, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 0.8, 0.0,
			-0.4, -1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kLinear, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, LinearOpposite) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 1.0, 0.0);
	InitializeFloatArray(num_base, y_base_, -1.0, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, 1.5, 0.7, 0.5, 0.1,
			0.0, -1.0);
	InitializeFloatArray(num_interpolated, y_expected_, -1.0, -0.4, 0.0, 0.8,
			1.0, 1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kLinear, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, PolynomialOrder0) {
	// initial setup
	polynomial_order_ = 0;
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 1.0, 1.0,
			-1.0, -1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, PolynomialOrder1) {
	// initial setup
	polynomial_order_ = 1;
	size_t const num_base = 2;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 0.8, 0.0,
			-0.4, -1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, PolynomialOrder2Full) {
	// initial setup
	polynomial_order_ = 2;
	size_t const num_base = 3;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0, 2.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0, 0.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);

	// expected value can be calculated by y = 1.5 x^2 - 3.5 x + 1.0
	y_expected_[0] = 1.0; // out of range
	for (size_t i = 1; i < num_interpolated; ++i) {
		y_expected_[i] = (1.5 * x_interpolated_[i] - 3.5) * x_interpolated_[i]
				+ 1.0;
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, PolynomialOrder2FullDescending) {
	// initial setup
	polynomial_order_ = 2;
	size_t const num_base = 3;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 2.0, 1.0, 0.0);
	InitializeFloatArray(num_base, y_base_, 0.0, -1.0, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);

	// expected value can be calculated by y = 1.5 x^2 - 3.5 x + 1.0
	y_expected_[0] = 1.0; // out of range
	for (size_t i = 1; i < num_interpolated; ++i) {
		y_expected_[i] = (1.5 * x_interpolated_[i] - 3.5) * x_interpolated_[i]
				+ 1.0;
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, PolynomialOrder2FullOpposite) {
	// initial setup
	polynomial_order_ = 2;
	size_t const num_base = 3;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 2.0, 1.0, 0.0);
	InitializeFloatArray(num_base, y_base_, 0.0, -1.0, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, 1.5, 0.7, 0.5, 0.1,
			0.0, -1.0);

	// expected value can be calculated by y = 1.5 x^2 - 3.5 x + 1.0
	y_expected_[5] = 1.0; // out of range
	for (size_t i = 0; i < num_interpolated - 1; ++i) {
		y_expected_[i] = (1.5 * x_interpolated_[i] - 3.5) * x_interpolated_[i]
				+ 1.0;
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, PolynomialOrder1Sub) {
	// initial setup
	polynomial_order_ = 1;
	size_t const num_base = 3;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0, 2.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0, 0.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 0.8, 0.0,
			-0.4, -0.5);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, Spline) {
	// initial setup
	size_t const num_base = 3;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 1.0, 2.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0, 0.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 0.72575,
			-0.28125, -0.66775, -0.78125);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kSpline, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, SplineDescending) {
	// initial setup
	size_t const num_base = 3;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 2.0, 1.0, 0.0);
	InitializeFloatArray(num_base, y_base_, 0.0, -1.0, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, -1.0, 0.0, 0.1,
			0.5, 0.7, 1.5);
	InitializeFloatArray(num_interpolated, y_expected_, 1.0, 1.0, 0.72575,
			-0.28125, -0.66775, -0.78125);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kSpline, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, SplineOpposite) {
	// initial setup
	size_t const num_base = 3;
	size_t const num_interpolated = 6;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 2.0, 1.0, 0.0);
	InitializeFloatArray(num_base, y_base_, 0.0, -1.0, 1.0);
	InitializeDoubleArray(num_interpolated, x_interpolated_, 1.5, 0.7, 0.5, 0.1,
			0.0, -1.0);
	InitializeFloatArray(num_interpolated, y_expected_, -0.78125, -0.66775,
			-0.28125, 0.72575, 1.0, 1.0);

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kSpline, num_base,
			num_interpolated, sakura_Status_kOK, true);
}

TEST_F(Interpolate1dFloatTest, SingleBasePerformance) {
	// initial setup
	size_t const num_base = 1;
	size_t const num_interpolated = 200000000;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0);
	InitializeFloatArray(num_base, y_base_, 1.0);
	double dx = 1.0 / static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = -0.5 + dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

TEST_F(Interpolate1dFloatTest, NearestPerformance) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 200000000;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 100.0);
	InitializeFloatArray(num_base, y_base_, 1.0, 0.0);
	double dx = fabs(x_base_[1] - x_base_[0])
			/ static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = x_base_[0] + dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

TEST_F(Interpolate1dFloatTest, NearestOppositePerformance) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 200000000;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 100.0, 0.0);
	InitializeFloatArray(num_base, y_base_, 0.0, 1.0);
	double dx = fabs(x_base_[1] - x_base_[0])
			/ static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = x_base_[0] - dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kNearest, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

TEST_F(Interpolate1dFloatTest, LinearPerformance) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 200000000;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 100.0);
	InitializeFloatArray(num_base, y_base_, 1.0, 0.0);
	double dx = fabs(x_base_[1] - x_base_[0])
			/ static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = x_base_[0] + dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kLinear, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

TEST_F(Interpolate1dFloatTest, LinearOppositePerformance) {
	// initial setup
	size_t const num_base = 2;
	size_t const num_interpolated = 200000000;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 100.0, 0.0);
	InitializeFloatArray(num_base, y_base_, 0.0, 1.0);
	double dx = fabs(x_base_[1] - x_base_[0])
			/ static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = x_base_[0] - dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kLinear, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

TEST_F(Interpolate1dFloatTest, PolynomialOrder2FullPerformance) {
	// initial setup
	polynomial_order_ = 2;
	size_t const num_base = 3;
	size_t const num_interpolated = 100000000; // 1/2 of Nearest and Linear
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 100.0, 200.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0, 0.0);
	double dx = fabs(x_base_[2] - x_base_[0])
			/ static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = x_base_[0] + dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

TEST_F(Interpolate1dFloatTest, PolynomialOrder2FullOppositePerformance) {
	// initial setup
	polynomial_order_ = 2;
	size_t const num_base = 3;
	size_t const num_interpolated = 100000000; // 1/2 of Nearest and Linear
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 200.0, 100.0, 0.0);
	InitializeFloatArray(num_base, y_base_, 0.0, -1.0, 1.0);
	double dx = fabs(x_base_[2] - x_base_[0])
			/ static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = x_base_[0] - dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kPolynomial, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

TEST_F(Interpolate1dFloatTest, SplinePerformance) {
	// initial setup
	size_t const num_base = 3;
	size_t const num_interpolated = 200000000;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 0.0, 100.0, 200.0);
	InitializeFloatArray(num_base, y_base_, 1.0, -1.0, 0.0);
	double dx = fabs(x_base_[2] - x_base_[0])
			/ static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = x_base_[0] + dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kSpline, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

TEST_F(Interpolate1dFloatTest, SplineOppositePerformance) {
	// initial setup
	size_t const num_base = 3;
	size_t const num_interpolated = 200000000;
	AllocateMemory(num_base, num_interpolated);
	InitializeDoubleArray(num_base, x_base_, 200.0, 100.0, 0.0);
	InitializeFloatArray(num_base, y_base_, 0.0, -1.0, 1.0);
	double dx = fabs(x_base_[2] - x_base_[0])
			/ static_cast<double>(num_interpolated - 1);
	for (size_t i = 0; i < num_interpolated; ++i) {
		x_interpolated_[i] = x_base_[0] - dx * static_cast<double>(i);
	}

	// execute interpolation
	RunInterpolate1d(sakura_InterpolationMethod_kSpline, num_base,
			num_interpolated, sakura_Status_kOK, false);
}

//
// AasapSplinePerformance
// Test performance of asap spline interpolator.
// To enable this test, you have to follow the procedure below:
//
// 1. Uncomment the line at the top of this file that includes
//    "CubicSplineInterpolator1D.h".
//
// 2. Copy the following files to test directory from asap/src:
//        - Interpolator1D.h, Interpolator1D.tcc
//        - CubicSplineInterpolator1D.h, CubicSplineInterpolator1D.tcc
//        - Locator.h, Locator.tcc
//        - BisectionLocator.h, BisectionLocator.tcc
//    These files should be located at test directory.
//
// 3. Comment out all casa related lines from these files.
//
// 4. Change Interpolator1D::createLocator() so that it always uses
//    BisectionLocator.
//
//TEST_F(Interpolate1dFloatTest, AsapSplinePerformance) {
//	EXPECT_EQ(sakura_Status_kOK, initialize_result_);
//
//	size_t const num_base = 3;
//	size_t const num_interpolated = 200000000;
//
//	// initial setup
//	AllocateMemory(num_base, num_interpolated);
//	x_base_[0] = 0.0;
//	x_base_[1] = 100.0;
//	x_base_[2] = 200.0;
//	y_base_[0] = 1.0;
//	y_base_[1] = -1.0;
//	y_base_[2] = 0.0;
//	double dx = fabs(x_base_[2] - x_base_[0])
//			/ static_cast<double>(num_interpolated - 1);
//	for (size_t i = 0; i < num_interpolated; ++i) {
//		x_interpolated_[i] = x_base_[0] + dx * static_cast<double>(i);
//	}
//
//	// execute interpolation
//	std::unique_ptr<asap::CubicSplineInterpolator1D<double, float> > interpolator(
//			new asap::CubicSplineInterpolator1D<double, float>());
//	interpolator->setX(x_base_, num_base);
//	interpolator->setY(y_base_, num_base);
//	for (size_t i = 0; i < num_interpolated; ++i) {
//		y_interpolated_[i] = interpolator->interpolate(x_interpolated_[i]);
//	}
//}
