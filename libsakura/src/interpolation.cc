#include <cassert>
#include <iostream>
#include <memory>
#include <cstdalign>

#include <libsakura/sakura.h>
#include <libsakura/optimized_implementation_factory_impl.h>
#include <libsakura/localdef.h>

//#include <Eigen/Core>
//
//using ::Eigen::Map;
//using ::Eigen::Array;
//using ::Eigen::Dynamic;
//using ::Eigen::Aligned;

namespace {

using namespace LIBSAKURA_PREFIX;

// Polynomial interpolation using Neville's algorithm
template<typename DataType>
int DoNevillePolynomial(double const x_base[], DataType const y_base[],
		unsigned int left_index, int num_elements, double x_interpolated,
		DataType &y_interpolated) {

	// working pointers
	double const *x_ptr = &x_base[left_index];
	DataType const *y_ptr = &y_base[left_index];

	// storage for C and D in Neville's algorithm
	size_t sakura_alignment = LIBSAKURA_SYMBOL(GetAlignment)();
	size_t elements_in_arena = num_elements + sakura_alignment - 1;
	std::unique_ptr<DataType[]> storage_for_c(new DataType[elements_in_arena]);
	std::unique_ptr<DataType[]> storage_for_d(new DataType[elements_in_arena]);
	DataType *c = reinterpret_cast<DataType*>(LIBSAKURA_SYMBOL(AlignAny)(
			sizeof(DataType) * elements_in_arena, storage_for_c.get(),
			sizeof(DataType) * num_elements));
	DataType *d = reinterpret_cast<DataType*>(LIBSAKURA_SYMBOL(AlignAny)(
			sizeof(DataType) * elements_in_arena, storage_for_d.get(),
			sizeof(DataType) * num_elements));

	for (int i = 0; i < num_elements; ++i) {
		c[i] = y_ptr[i];
		d[i] = y_ptr[i];
	}

	// Neville's algorithm
	y_interpolated = c[0];
	for (int m = 1; m < num_elements; ++m) {
		// Evaluate Cm1, Cm2, Cm3, ... Cm[n-m] and Dm1, Dm2, Dm3, ... Dm[n-m].
		// Those are stored to c[0], c[1], ..., c[n-m-1] and d[0], d[1], ...,
		// d[n-m-1].
		for (int i = 0; i < num_elements - m; ++i) {
			DataType cd = c[i + 1] - d[i];
			double dx = x_ptr[i] - x_ptr[i + m];
			try {
				cd /= static_cast<DataType>(dx);
			} catch (...) {
				std::cerr << "x_base has duplicate elements" << std::endl;
				return 1;
			}
			c[i] = (x_ptr[i] - x_interpolated) * cd;
			d[i] = (x_ptr[i + m] - x_interpolated) * cd;
		}

		// In each step, c[0] holds Cm1 which is a correction between
		// P12...m and P12...[m+1]. Thus, the following repeated update
		// corresponds to the route P1 -> P12 -> P123 ->...-> P123...n.
		y_interpolated += c[0];
	}

	return 0;
}

void DeriveSplineCorrectionTerm(bool is_descending, size_t num_base,
		double const x_base[], float const y_base[],
		float y_base_2nd_derivative[]) {
	size_t sakura_alignment = LIBSAKURA_SYMBOL(GetAlignment)();
	size_t elements_in_arena = num_base + sakura_alignment - 2;
	std::unique_ptr<float[]> storage_for_u(new float[elements_in_arena]);
	float *upper_triangular = const_cast<float *>(LIBSAKURA_SYMBOL(AlignFloat)(
			elements_in_arena, storage_for_u.get(), num_base - 1));

	// This is a condition of natural cubic spline
	y_base_2nd_derivative[0] = 0.0;
	y_base_2nd_derivative[num_base - 1] = 0.0;
	upper_triangular[0] = 0.0;

	// Solve tridiagonal system.
	// Here tridiagonal matrix is decomposed to upper triangular matrix.
	// upper_tridiangular stores upper triangular elements, while
	// y_base_2nd_derivative stores right-hand-side vector. The diagonal
	// elements are normalized to 1.
	if (is_descending) {
		// x_base is descending order
		double a1 = x_base[num_base - 2] - x_base[num_base - 1];
		double a2, b1;
		for (size_t i = 1; i < num_base - 1; ++i) {
			a2 = x_base[num_base - i - 2] - x_base[num_base - i - 1];
			b1 = 1.0 / (x_base[num_base - i - 2] - x_base[num_base - i]);
			y_base_2nd_derivative[i] = 3.0 * b1
					* ((y_base[num_base - i - 2] - y_base[num_base - i - 1])
							/ a2
							- (y_base[num_base - i - 1] - y_base[num_base - i])
									/ a1
							- y_base_2nd_derivative[i - 1] * 0.5 * a1);
			a1 = 1.0 / (1.0 - upper_triangular[i - 1] * 0.5 * a1 * b1);
			y_base_2nd_derivative[i] *= a1;
			upper_triangular[i] = 0.5 * a2 * b1 * a1;
			a1 = a2;
		}
	} else {
		// x_base is ascending order
		double a1 = x_base[1] - x_base[0];
		double a2, b1;
		for (size_t i = 1; i < num_base - 1; ++i) {
			a2 = x_base[i + 1] - x_base[i];
			b1 = 1.0 / (x_base[i + 1] - x_base[i - 1]);
			y_base_2nd_derivative[i] = 3.0 * b1
					* ((y_base[i + 1] - y_base[i]) / a2
							- (y_base[i] - y_base[i - 1]) / a1
							- y_base_2nd_derivative[i - 1] * 0.5 * a1);
			a1 = 1.0 / (1.0 - upper_triangular[i - 1] * 0.5 * a1 * b1);
			y_base_2nd_derivative[i] *= a1;
			upper_triangular[i] = 0.5 * a2 * b1 * a1;
			a1 = a2;
		}
	}

	// Solve the system by backsubstitution and store solution to
	// y_base_2nd_derivative
	for (size_t k = num_base - 2; k >= 1; --k)
		y_base_2nd_derivative[k] -= upper_triangular[k]
				* y_base_2nd_derivative[k + 1];
}

float DoSplineInterpolation(int lower_index, int upper_index,
		int lower_index_correct, int upper_index_correct, double x_interpolated,
		double const x_base[], float const y_base[],
		float const y_base_2nd_derivative[]) {
	float dx = x_base[lower_index] - x_base[upper_index];
	float a = (x_base[upper_index] - x_interpolated) / dx;
	float b = (x_interpolated - x_base[lower_index]) / dx;
	return a * y_base[lower_index] + b * y_base[upper_index]
			+ ((a * a * a - a) * y_base_2nd_derivative[lower_index_correct]
					+ (b * b * b - b)
							* y_base_2nd_derivative[upper_index_correct])
					* (dx * dx) / 6.0;
}

float PerformSplineInterpolation(int location, double x_interpolated,
		size_t num_base, double const x_base[], float const y_base[],
		float const y_base_2nd_derivative[]) {
	return DoSplineInterpolation(location - 1, location, location - 1, location,
			x_interpolated, x_base, y_base, y_base_2nd_derivative);
}

float PerformSplineInterpolationDescending(int location, double x_interpolated,
		size_t num_base, double const x_base[], float const y_base[],
		float const y_base_2nd_derivative[]) {
	return DoSplineInterpolation(location, location - 1,
			num_base - location - 1, num_base - location - 2, x_interpolated,
			x_base, y_base, y_base_2nd_derivative);
}

}

namespace LIBSAKURA_PREFIX {

template<typename DataType>
void ADDSUFFIX(Interpolation, ARCH_SUFFIX)<DataType>::Interpolate1dNearest(
		size_t num_base, double const x_base[/* num_base */],
		DataType const y_base[/* num_base */], size_t num_interpolated,
		double const x_interpolated[/* num_interpolated */],
		DataType y_interpolated[/*num_interpolated*/]) const {
	assert(num_base > 0);
	assert(num_interpolated > 0);
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_interpolated));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_interpolated));

	if (num_base == 1) {
//		Map<Array<DataType, Dynamic, 1>, Aligned> y_interpolated_vector(
//				y_interpolated, num_interpolated);
//		y_interpolated_vector.setConstant(y_base[0]);
		for (size_t index = 0; index < num_interpolated; ++index) {
			y_interpolated[index] = y_base[0];
		}
	} else {
		// TODO: change the following code as follows:
		// 1. locate x_base[0] against x_interpolated
		// 2. locate x_base[num_base-1] against x_interpolated
		// 3. set y_base[0] for elements locating left side of x_base[0]
		// 4. set y_base[num_base-1] for elements locating right side of x_base[num_base-1]
		// 5. do interpolation for elements between x_base[0] and x_base[num_base-1]
		int location = 0;
		int end_position = static_cast<int>(num_base) - 1;
		for (size_t index = 0; index < num_interpolated; ++index) {
			location = this->Locate(location, end_position, num_base, x_base,
					x_interpolated[index]);
			if (location == 0) {
				y_interpolated[index] = y_base[0];
			} else if (location == static_cast<int>(num_base)) {
				y_interpolated[index] = y_base[location - 1];
			} else {
				double dx_left = fabs(
						x_interpolated[index] - x_base[location - 1]);
				double dx_right = fabs(
						x_interpolated[index] - x_base[location]);
				if (dx_left <= dx_right) {
					y_interpolated[index] = y_base[location - 1];
				} else {
					y_interpolated[index] = y_base[location];
				}
			}
		}
	}
}

template<typename DataType>
void ADDSUFFIX(Interpolation, ARCH_SUFFIX)<DataType>::Interpolate1dLinear(
		size_t num_base, double const x_base[/*num_base*/],
		DataType const y_base[/*num_base*/], size_t num_interpolated,
		double const x_interpolated[/*num_interpolated*/],
		DataType y_interpolated[/*num_interpolated*/]) const {
	assert(num_base > 0);
	assert(num_interpolated > 0);
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_interpolated));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_interpolated));

	if (num_base == 1) {
//		Map<Array<DataType, Dynamic, 1>, Aligned> y_interpolated_vector(
//				y_interpolated, num_interpolated);
//		y_interpolated_vector.setConstant(y_base[0]);
		for (size_t index = 0; index < num_interpolated; ++index) {
			y_interpolated[index] = y_base[0];
		}
	} else {
		int location = 0;
		int end_position = static_cast<int>(num_base) - 1;
		for (size_t index = 0; index < num_interpolated; ++index) {
			location = this->Locate(location, end_position, num_base, x_base,
					x_interpolated[index]);
			if (location == 0) {
				y_interpolated[index] = y_base[0];
			} else if (location == static_cast<int>(num_base)) {
				y_interpolated[index] = y_base[location - 1];
			} else {
				y_interpolated[index] = y_base[location - 1]
						+ (y_base[location] - y_base[location - 1])
								* (x_interpolated[index] - x_base[location - 1])
								/ (x_base[location] - x_base[location - 1]);
			}
		}
	}
}

template<typename DataType>
void ADDSUFFIX(Interpolation, ARCH_SUFFIX)<DataType>::Interpolate1dPolynomial(
		int polynomial_order, size_t num_base,
		double const x_base[/*num_base*/], DataType const y_base[/*num_base*/],
		size_t num_interpolated,
		double const x_interpolated[/*num_interpolated*/],
		DataType y_interpolated[/*num_interpolated*/]) const {
	assert(num_base > 0);
	assert(num_interpolated > 0);
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_interpolated));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_interpolated));

	if (num_base == 1) {
//		Map<Array<DataType, Dynamic, 1>, Aligned> y_interpolated_vector(
//				y_interpolated, num_interpolated);
//		y_interpolated_vector.setConstant(y_base[0]);
		for (size_t index = 0; index < num_interpolated; ++index) {
			y_interpolated[index] = y_base[0];
		}
	} else if (polynomial_order == 0) {
		// This is special case: 0-th polynomial interpolation acts like nearest interpolation
		Interpolate1dNearest(num_base, x_base, y_base, num_interpolated,
				x_interpolated, y_interpolated);
	} else if (polynomial_order + 1 >= static_cast<int>(num_base)) {
		// use full region for interpolation
		// call polynomial interpolation
		int location = 0;
		int end_position = static_cast<int>(num_base) - 1;
		for (size_t index = 0; index < num_interpolated; ++index) {
			location = this->Locate(location, end_position, num_base, x_base,
					x_interpolated[index]);
			if (location == 0) {
				y_interpolated[index] = y_base[0];
			} else if (location == static_cast<int>(num_base)) {
				y_interpolated[index] = y_base[location - 1];
			} else {
				// call polynomial interpolation
				int status = DoNevillePolynomial<DataType>(x_base, y_base, 0,
						num_base, x_interpolated[index], y_interpolated[index]);
			}
		}
	} else {
		// use sub-region around the nearest points
		int location = 0;
		int end_position = static_cast<int>(num_base) - 1;
		for (size_t index = 0; index < num_interpolated; ++index) {
			location = this->Locate(location, end_position, num_base, x_base,
					x_interpolated[index]);
			if (location == 0) {
				y_interpolated[index] = y_base[0];
			} else if (location == static_cast<int>(num_base)) {
				y_interpolated[index] = y_base[location - 1];
			} else {
				int j = location - 1 - polynomial_order / 2;
				unsigned int m = static_cast<unsigned int>(num_base) - 1
						- static_cast<unsigned int>(polynomial_order);
				unsigned int k = static_cast<unsigned int>((j > 0) ? j : 0);
				// call polynomial interpolation
				int status = DoNevillePolynomial<DataType>(x_base, y_base, k,
						polynomial_order + 1, x_interpolated[index],
						y_interpolated[index]);
			}
		}
	}
}

template<typename DataType>
void ADDSUFFIX(Interpolation, ARCH_SUFFIX)<DataType>::Interpolate1dSpline(
		size_t num_base, double const x_base[/*num_base*/],
		DataType const y_base[/*num_base*/], size_t num_interpolated,
		double const x_interpolated[/*num_interpolated*/],
		DataType y_interpolated[/*num_interpolated*/]) const {
	assert(num_base > 0);
	assert(num_interpolated > 0);
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_interpolated));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_interpolated));

	if (num_base == 1) {
//		Map<Array<DataType, Dynamic, 1>, Aligned> y_interpolated_vector(
//				y_interpolated, num_interpolated);
//		y_interpolated_vector.setConstant(y_base[0]);
		for (size_t index = 0; index < num_interpolated; ++index) {
			y_interpolated[index] = y_base[0];
		}
	} else {
// Derive second derivative at x_base
		const bool kIsDescending = (x_base[0] > x_base[num_base - 1]);
		size_t sakura_alignment = LIBSAKURA_SYMBOL(GetAlignment)();
		size_t elements_in_arena = num_base + sakura_alignment - 1;
		std::unique_ptr<float[]> storage_for_y(new float[elements_in_arena]);
		float *y_base_2nd_derivative =
				const_cast<float *>(LIBSAKURA_SYMBOL(AlignFloat)(
						elements_in_arena, storage_for_y.get(), num_base));
		DeriveSplineCorrectionTerm(kIsDescending, num_base, x_base, y_base,
				y_base_2nd_derivative);

// spline interpolation with correction by y_base_2nd_derivative
		int location = 0;
		int end_position = static_cast<int>(num_base) - 1;
		for (size_t index = 0; index < num_interpolated; ++index) {
			location = this->Locate(location, end_position, num_base, x_base,
					x_interpolated[index]);
			if (location == 0) {
				y_interpolated[index] = y_base[0];
			} else if (location == static_cast<int>(num_base)) {
				y_interpolated[index] = y_base[location - 1];
			} else {
				// call polynomial interpolation
				if (kIsDescending) {
					y_interpolated[index] =
							PerformSplineInterpolationDescending(location,
									x_interpolated[index], num_base, x_base,
									y_base, y_base_2nd_derivative);
				} else {
					y_interpolated[index] = PerformSplineInterpolation(location,
							x_interpolated[index], num_base, x_base, y_base,
							y_base_2nd_derivative);
				}
			}
		}
	}
}

template class ADDSUFFIX(Interpolation, ARCH_SUFFIX)<float> ;
} /* namespace LIBSAKURA_PREFIX */
