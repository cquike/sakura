#include <cassert>
#include <iostream>
#include <memory>
#include <cstdalign>
#include <utility>

#include <libsakura/sakura.h>
#include <libsakura/optimized_implementation_factory_impl.h>
#include <libsakura/localdef.h>

namespace {

using namespace LIBSAKURA_PREFIX;

// get aligned array
// allocated memory is managed by unique_ptr that is given as an argument
template<class DataType>
DataType const *GetAlignedArray(size_t num_array,
		std::unique_ptr<DataType[]> *storage) {
	size_t sakura_alignment = LIBSAKURA_SYMBOL(GetAlignment)();
	size_t elements_in_arena = num_array + sakura_alignment - 1;
	storage->reset(new DataType[elements_in_arena]);
	return reinterpret_cast<DataType const *>(LIBSAKURA_SYMBOL(AlignAny)(
			sizeof(DataType) * elements_in_arena, storage->get(),
			sizeof(DataType) * num_array));
}

template<class XDataType, class YDataType>
void DeriveSplineCorrectionTerm(bool is_descending, size_t num_base,
		XDataType const x_base[], YDataType const y_base[],
		YDataType y_base_2nd_derivative[]) {
	std::unique_ptr<YDataType[]> storage_for_u;
	YDataType *upper_triangular = const_cast<YDataType *>(GetAlignedArray<
			YDataType>(num_base, &storage_for_u));

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
		XDataType a1 = x_base[num_base - 2] - x_base[num_base - 1];
		for (size_t i = 1; i < num_base - 1; ++i) {
			XDataType a2 = x_base[num_base - i - 2] - x_base[num_base - i - 1];
			XDataType b1 = 1.0
					/ (x_base[num_base - i - 2] - x_base[num_base - i]);
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
		XDataType a1 = x_base[1] - x_base[0];
		for (size_t i = 1; i < num_base - 1; ++i) {
			XDataType a2 = x_base[i + 1] - x_base[i];
			XDataType b1 = 1.0 / (x_base[i + 1] - x_base[i - 1]);
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

template<class XDataType, class YDataType>
class InterpolationWorker {
public:
	InterpolationWorker(size_t num_base, XDataType const x_base[],
			YDataType const y_base[], int polynomial_order = -1) :
			num_base_(num_base), x_base_(x_base), y_base_(y_base), polynomial_order_(
					polynomial_order) {
	}
	virtual ~InterpolationWorker() {
	}
	virtual void PrepareForInterpolation() {
	}
	virtual void Interpolate(size_t left_index, size_t right_index,
			size_t location, XDataType const x_interpolated[],
			YDataType y_interpolated[]) = 0;
protected:
	size_t num_base_;
	XDataType const *x_base_;
	YDataType const *y_base_;
	int const polynomial_order_;
};

template<class XDataType, class YDataType>
class NearestInterpolationWorker: public InterpolationWorker<XDataType,
		YDataType> {
public:
	NearestInterpolationWorker(size_t num_base, XDataType const x_base[],
			YDataType const y_base[]) :
			InterpolationWorker<XDataType, YDataType>(num_base, x_base, y_base) {
	}
	virtual ~NearestInterpolationWorker() {
	}
	virtual void Interpolate(size_t left_index, size_t right_index,
			size_t location, XDataType const x_interpolated[],
			YDataType y_interpolated[]) {
		XDataType half_width = 0.5
				* fabs(this->x_base_[location] - this->x_base_[location - 1]);
		for (size_t i = left_index; i < right_index; ++i) {
			XDataType dx_right = fabs(
					x_interpolated[i] - this->x_base_[location]);
			// nearest condition
			// if dx_right / dx > 0.5, index will be (location - 1) which means nearest
			// is left side. Otherwise, index will be (location), right side.
			if (dx_right >= half_width) {
				y_interpolated[i] = this->y_base_[location - 1];
			} else {
				y_interpolated[i] = this->y_base_[location];
			}
		}
	}
};

template<class XDataType, class YDataType>
class LinearInterpolationWorker: public InterpolationWorker<XDataType, YDataType> {
public:
	LinearInterpolationWorker(size_t num_base, XDataType const x_base[],
			YDataType const y_base[]) :
			InterpolationWorker<XDataType, YDataType>(num_base, x_base, y_base) {
	}
	virtual ~LinearInterpolationWorker() {
	}
	virtual void Interpolate(size_t left_index, size_t right_index,
			size_t location, XDataType const x_interpolated[],
			YDataType y_interpolated[]) {
		XDataType dydx = static_cast<XDataType>(this->y_base_[location]
				- this->y_base_[location - 1])
				/ (this->x_base_[location] - this->x_base_[location - 1]);
		YDataType base_term = this->y_base_[location - 1];
		for (size_t i = left_index; i < right_index; ++i) {
			y_interpolated[i] =
					base_term
							+ static_cast<YDataType>(dydx
									* (x_interpolated[i]
											- this->x_base_[location - 1]));
		}
	}
};

template<class XDataType, class YDataType>
class PolynomialInterpolationWorker: public InterpolationWorker<XDataType,
		YDataType> {
public:
	PolynomialInterpolationWorker(size_t num_base, XDataType const x_base[],
			YDataType const y_base[], int polynomial_order) :
			InterpolationWorker<XDataType, YDataType>(num_base, x_base, y_base,
					polynomial_order), kNumElements_(
					this->polynomial_order_ + 1), storage_for_c_(), storage_for_d_(), c_(
					nullptr), d_(nullptr) {
	}
	virtual ~PolynomialInterpolationWorker() {
	}
	virtual void PrepareForInterpolation() {
		c_ = const_cast<XDataType *>(GetAlignedArray<XDataType>(kNumElements_,
				&storage_for_c_));
		d_ = const_cast<XDataType *>(GetAlignedArray<XDataType>(kNumElements_,
				&storage_for_d_));
	}
	virtual void Interpolate(size_t left_index, size_t right_index,
			size_t location, XDataType const x_interpolated[],
			YDataType y_interpolated[]) {
		int j = location - 1 - this->polynomial_order_ / 2;
		size_t m = this->num_base_ - 1
				- static_cast<size_t>(this->polynomial_order_);
		size_t k = static_cast<size_t>((j > 0) ? j : 0);
		k = (k > m) ? m : k;
		for (size_t i = left_index; i < right_index; ++i) {
			PerformNevilleAlgorithm(k, x_interpolated[i], &y_interpolated[i]);
		}
	}
private:
	void PerformNevilleAlgorithm(size_t left_index, XDataType x_interpolated,
			YDataType *y_interpolated) {

		// working pointers
		XDataType const *x_ptr = &(this->x_base_[left_index]);
		YDataType const *y_ptr = &(this->y_base_[left_index]);

		for (size_t i = 0; i < kNumElements_; ++i) {
			c_[i] = static_cast<XDataType>(y_ptr[i]);
			d_[i] = c_[i];
		}

		// Neville's algorithm
		XDataType y_interpolated_work = c_[0];
		for (size_t m = 1; m < kNumElements_; ++m) {
			// Evaluate Cm1, Cm2, Cm3, ... Cm[n-m] and Dm1, Dm2, Dm3, ... Dm[n-m].
			// Those are stored to c[0], c[1], ..., c[n-m-1] and d[0], d[1], ...,
			// d[n-m-1].
			for (size_t i = 0; i < kNumElements_ - m; ++i) {
				XDataType cd = c_[i + 1] - d_[i];
				XDataType dx = x_ptr[i] - x_ptr[i + m];
				assert(dx != 0);
				cd /= dx;
				c_[i] = (x_ptr[i] - x_interpolated) * cd;
				d_[i] = (x_ptr[i + m] - x_interpolated) * cd;
			}

			// In each step, c[0] holds Cm1 which is a correction between
			// P12...m and P12...[m+1]. Thus, the following repeated update
			// corresponds to the route P1 -> P12 -> P123 ->...-> P123...n.
			y_interpolated_work += c_[0];
		}

		*y_interpolated = static_cast<YDataType>(y_interpolated_work);
	}

	size_t const kNumElements_;
	std::unique_ptr<XDataType[]> storage_for_c_;
	std::unique_ptr<XDataType[]> storage_for_d_;
	XDataType *c_;
	XDataType *d_;
};

template<class XDataType, class YDataType>
class SplineInterpolationWorker: public InterpolationWorker<XDataType, YDataType> {
public:
	SplineInterpolationWorker(size_t num_base, XDataType const x_base[],
			YDataType const y_base[]) :
			InterpolationWorker<XDataType, YDataType>(num_base, x_base, y_base), storage_for_y_(
					nullptr), y_base_2nd_derivative_(nullptr) {
	}
	virtual ~SplineInterpolationWorker() {
	}
	virtual void PrepareForInterpolation() {
		// Derive second derivative at x_base
		const bool kIsDescending = (this->x_base_[0]
				> this->x_base_[this->num_base_ - 1]);
		y_base_2nd_derivative_ = const_cast<YDataType *>(GetAlignedArray<
				YDataType>(this->num_base_, &storage_for_y_));
		DeriveSplineCorrectionTerm<XDataType, YDataType>(kIsDescending,
				this->num_base_, this->x_base_, this->y_base_,
				y_base_2nd_derivative_);
	}
protected:
	std::unique_ptr<YDataType[]> storage_for_y_;
	YDataType *y_base_2nd_derivative_;
};

template<class XDataType, class YDataType>
class SplineInterpolationWorkerDescending: public SplineInterpolationWorker<
		XDataType, YDataType> {
public:
	SplineInterpolationWorkerDescending(size_t num_base,
			XDataType const x_base[], YDataType const y_base[]) :
			SplineInterpolationWorker<XDataType, YDataType>(num_base, x_base,
					y_base) {
	}
	virtual ~SplineInterpolationWorkerDescending() {
	}
	virtual void Interpolate(size_t left_index, size_t right_index,
			size_t location, XDataType const x_interpolated[],
			YDataType y_interpolated[]) {
		assert(this->y_base_2nd_derivative_ != nullptr);
		XDataType dx = this->x_base_[location] - this->x_base_[location - 1];
		for (size_t i = left_index; i < right_index; ++i) {
			XDataType a = (this->x_base_[location] - x_interpolated[i]) / dx;
			XDataType b = (x_interpolated[i] - this->x_base_[location - 1])
					/ dx;
			y_interpolated[i] =
					static_cast<YDataType>(a * this->y_base_[location - 1]
							+ b * this->y_base_[location]
							+ ((a * a * a - a)
									* this->y_base_2nd_derivative_[this->num_base_
											- location - 2]
									+ (b * b * b - b)
											* this->y_base_2nd_derivative_[this->num_base_
													- location - 1]) * (dx * dx)
									/ 6.0);

		}
	}
};

template<class XDataType, class YDataType>
class SplineInterpolationWorkerAscending: public SplineInterpolationWorker<
		XDataType, YDataType> {
public:
	SplineInterpolationWorkerAscending(size_t num_base,
			XDataType const x_base[], YDataType const y_base[]) :
			SplineInterpolationWorker<XDataType, YDataType>(num_base, x_base,
					y_base) {
	}
	virtual ~SplineInterpolationWorkerAscending() {
	}
	virtual void Interpolate(size_t left_index, size_t right_index,
			size_t location, XDataType const x_interpolated[],
			YDataType y_interpolated[]) {
		assert(this->y_base_2nd_derivative_ != nullptr);
		XDataType dx = this->x_base_[location] - this->x_base_[location - 1];
		for (size_t i = left_index; i < right_index; ++i) {
			XDataType a = (this->x_base_[location] - x_interpolated[i]) / dx;
			XDataType b = (x_interpolated[i] - this->x_base_[location - 1])
					/ dx;
			y_interpolated[i] = static_cast<YDataType>(a
					* this->y_base_[location - 1] + b * this->y_base_[location]
					+ ((a * a * a - a)
							* this->y_base_2nd_derivative_[location - 1]
							+ (b * b * b - b)
									* this->y_base_2nd_derivative_[location])
							* (dx * dx) / 6.0);

		}
	}
};

template<class XDataType, class YDataType>
YDataType const *GetAscendingArray(size_t num_array, XDataType const x_base[],
		YDataType const base_array[], std::unique_ptr<YDataType[]> *storage) {
	if (x_base[0] < x_base[num_array - 1]) {
		return base_array;
	} else {
		YDataType const *output_array = GetAlignedArray<YDataType>(num_array,
				storage);
		YDataType *work_array = const_cast<YDataType *>(output_array);
		for (size_t i = 0; i < num_array; ++i) {
			work_array[i] = base_array[num_array - 1 - i];
		}
		return output_array;
	}
}

template<class XDataType, class YDataType>
InterpolationWorker<XDataType, YDataType> *CreateInterpolationWorker(
LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method, size_t num_base,
		XDataType const x_base[], YDataType const y_base[],
		int polynomial_order) {
	switch (interpolation_method) {
	case LIBSAKURA_SYMBOL(InterpolationMethod_kNearest):
		return new NearestInterpolationWorker<XDataType, YDataType>(num_base,
				x_base, y_base);
	case LIBSAKURA_SYMBOL(InterpolationMethod_kLinear):
		return new LinearInterpolationWorker<XDataType, YDataType>(num_base,
				x_base, y_base);
	case LIBSAKURA_SYMBOL(InterpolationMethod_kPolynomial):
		if (polynomial_order < 0) {
			// invalid polynomial order
			return nullptr;
		} else if (polynomial_order == 0) {
			// This is special case: 0-th polynomial interpolation
			// acts like nearest interpolation
			return new NearestInterpolationWorker<XDataType, YDataType>(
					num_base, x_base, y_base);
		} else if (static_cast<size_t>(polynomial_order + 1) >= num_base) {
			// use full region for interpolation
			return new PolynomialInterpolationWorker<XDataType, YDataType>(
					num_base, x_base, y_base, num_base - 1);
		} else {
			// use sub-region around the nearest points
			return new PolynomialInterpolationWorker<XDataType, YDataType>(
					num_base, x_base, y_base, polynomial_order);
		}
	case LIBSAKURA_SYMBOL(InterpolationMethod_kSpline):
		return new SplineInterpolationWorkerAscending<XDataType, YDataType>(
				num_base, x_base, y_base);
	default:
		// invalid interpolation method type
		return nullptr;
	}
}

} /* anonymous namespace */

namespace LIBSAKURA_PREFIX {

template<class XDataType, class YDataType>
LIBSAKURA_SYMBOL(Status) ADDSUFFIX(Interpolation, ARCH_SUFFIX)<XDataType,
		YDataType>::Interpolate1d(
LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method,
		int polynomial_order, size_t num_base,
		XDataType const x_base[/* num_base */],
		YDataType const y_base[/* num_base */], size_t num_interpolated,
		XDataType const x_interpolated[/* num_interpolated */],
		YDataType y_interpolated[/*num_interpolated*/]) const {
	assert(num_base > 0);
	assert(num_interpolated > 0);
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_base));
	assert(LIBSAKURA_SYMBOL(IsAligned)(x_interpolated));
	assert(LIBSAKURA_SYMBOL(IsAligned)(y_interpolated));

	// input arrays are not aligned
	if (!LIBSAKURA_SYMBOL(IsAligned)(x_base)
			|| !LIBSAKURA_SYMBOL(IsAligned)(y_base)
			|| !LIBSAKURA_SYMBOL(IsAligned)(x_interpolated)
			|| !LIBSAKURA_SYMBOL(IsAligned)(y_interpolated)) {
		std::cerr << "ERROR: input arrays are not aligned" << std::endl;
		return LIBSAKURA_SYMBOL(Status_kInvalidArgument);
	}

	if (num_base == 1) {
		// No need to interpolate, just substitute y_base[0]
		// to all elements in y_interpolated
		for (size_t index = 0; index < num_interpolated; ++index) {
			y_interpolated[index] = y_base[0];
		}
		return LIBSAKURA_SYMBOL(Status_kOK);
	}

	// Perform 1-dimensional interpolation

	// make input arrays ascending order
	std::unique_ptr<XDataType[]> storage_for_x_base_work;
	std::unique_ptr<YDataType[]> storage_for_y_base_work;
	std::unique_ptr<XDataType[]> storage_for_x_interpolated_work;
	XDataType const *x_base_work = GetAscendingArray<XDataType, XDataType>(
			num_base, x_base, x_base, &storage_for_x_base_work);
	YDataType const *y_base_work = GetAscendingArray<XDataType, YDataType>(
			num_base, x_base, y_base, &storage_for_y_base_work);
	XDataType const *x_interpolated_work = GetAscendingArray<XDataType,
			XDataType>(num_interpolated, x_interpolated, x_interpolated,
			&storage_for_x_interpolated_work);

	// Generate worker class
	std::unique_ptr<InterpolationWorker<XDataType, YDataType> > interpolator(
			CreateInterpolationWorker<XDataType, YDataType>(
					interpolation_method, num_base, x_base_work, y_base_work,
					polynomial_order));
	if (interpolator.get() == nullptr) {
		// failed to create interpolation worker object
		std::cerr
				<< "ERROR: Invalid interpolation method type or Negative polynomial order for polynomial interpolation"
				<< std::endl;
		return LIBSAKURA_SYMBOL(Status_kInvalidArgument);
	}

	// Any preparation for interpolation should be done here
	interpolator->PrepareForInterpolation();

	// Locate each element in x_base against x_interpolated
	std::unique_ptr<size_t[]> storage_for_location_base;
	size_t *location_base = const_cast<size_t *>(GetAlignedArray<size_t>(
			num_base, &storage_for_location_base));
	size_t start_position = 0;
	size_t end_position = num_interpolated - 1;
	for (size_t i = 0; i < num_base; ++i) {
		location_base[i] = this->Locate(start_position, end_position,
				num_interpolated, x_interpolated_work, x_base_work[i]);
		start_position = location_base[i];
	}

	// Outside of x_base[0]
	for (size_t i = 0; i < location_base[0]; ++i) {
		y_interpolated[i] = y_base_work[0];
	}

	// Between x_base[0] and x_base[num_base-1]
	for (size_t i = 0; i < num_base - 1; ++i) {
		interpolator->Interpolate(location_base[i], location_base[i + 1], i + 1,
				x_interpolated_work, y_interpolated);
	}

	// Outside of x_base[num_base-1]
	for (size_t i = location_base[num_base - 1]; i < num_interpolated; ++i) {
		y_interpolated[i] = y_base_work[num_base - 1];
	}

	// swap output array
	if (x_interpolated[0] >= x_interpolated[num_interpolated - 1]) {
		size_t num_interpolated_half = num_interpolated / 2;
		for (size_t i = 0; i < num_interpolated_half; ++i) {
			std::swap<YDataType>(y_interpolated[i],
					y_interpolated[num_interpolated - 1 - i]);
		}
	}
	return LIBSAKURA_SYMBOL(Status_kOK);
}

template class ADDSUFFIX(Interpolation, ARCH_SUFFIX)<double, float> ;
} /* namespace LIBSAKURA_PREFIX */
