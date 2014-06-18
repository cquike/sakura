#include <cassert>
#include <cstddef>
#include <cmath>
#include <algorithm>

#include "libsakura/sakura.h"
#include "libsakura/optimized_implementation_factory_impl.h"
#include "libsakura/localdef.h"

#define FORCE_EIGEN 0

#if defined(__AVX__) && !defined(ARCH_SCALAR) && (! FORCE_EIGEN)
#include <immintrin.h>
#include <cstdint>

namespace {

union m256 {
	__m256 m256;
	__m256i m256i;
	__m256d m256d;
	float floatv[8];
	double doublev[4];
	int32_t intv[8];
	char charv[32];
};

inline float AddHorizontally(__m256 packed_values) {
	packed_values = _mm256_hadd_ps(packed_values, packed_values);
	packed_values = _mm256_hadd_ps(packed_values, packed_values);
	__m128 sum2 = _mm256_extractf128_ps(packed_values, 1);
	return _mm_cvtss_f32(
			_mm_add_ss(_mm256_castps256_ps128(packed_values), sum2));
}

inline int32_t AddHorizontally(__m256i packed_values) {
#if defined(__AVX2__)
	packed_values = _mm256_hadd_epi32(packed_values, packed_values);
	packed_values = _mm256_hadd_epi32(packed_values, packed_values);
	__m128i sum2 = _mm256_extractf128_si256(packed_values, 1);
	return _mm_cvtsi128_si32(
			_mm_add_epi32(_mm256_castsi256_si128(packed_values), sum2));
#else
	__m128i count2 = _mm256_castsi256_si128(
	_mm256_permute2f128_si256(packed_values, packed_values, 1));
	__m128i count4w = _mm_hadd_epi32(_mm256_castsi256_si128(packed_values),
			count2);
	count4w = _mm_hadd_epi32(count4w, count4w);
	count4w = _mm_hadd_epi32(count4w, count4w);
	int32_t total = _mm_extract_epi32(count4w, 0);
	return total;
#endif
}

inline int32_t AddHorizontally128(__m128i packed_values) {
	packed_values = _mm_hadd_epi32(packed_values, packed_values);
	packed_values = _mm_hadd_epi32(packed_values, packed_values);
	int32_t total = _mm_extract_epi32(packed_values, 0);
	return total;
}

#if 0
inline void ComputeStatisticsSimple(float const data_arg[], bool const is_valid_arg[],
		size_t elements, LIBSAKURA_SYMBOL(StatisticsResult) *result) {
	auto data = AssumeAligned(data_arg);
	auto is_valid = AssumeAligned(is_valid_arg);
	float sum = 0;
	float square_sum = 0;
	size_t count = 0;
	float min = NAN, max = NAN;
	int32_t index_of_min = -1;
	int32_t index_of_max = -1;

	for (int32_t i = 0; i < static_cast<int32_t>(elements); ++i) {
		if (is_valid[i]) {
			++count;
			auto value = data[i];
			sum += value;
			if (isnanf(min)
					|| (!isnanf(value) && value < min)) {
				min = value;
				index_of_min = i;
			}
			if (isnanf(max)
					|| (!isnanf(value) && value > max)) {
				max = value;
				index_of_max = i;
			}
			square_sum += value * value;
		}
	}
	result->sum = sum;
	result->count = count;
	float float_count = static_cast<float>(count);
	result->max = max;
	result->index_of_max = index_of_max;
	result->min = min;
	result->index_of_min = index_of_min;
	float rms2 = NAN;
	if (count == 0) {
		result->mean = NAN;
	} else {
		result->mean = sum / float_count;
		rms2 = square_sum / float_count;
	}

	result->rms = std::sqrt(rms2);
	result->stddev = std::sqrt(rms2 - result->mean * result->mean);
}
#endif

void ComputeStatisticsSimd(float const data[], bool const is_valid[],
		size_t elements, LIBSAKURA_SYMBOL(StatisticsResult) *result_) {
	LIBSAKURA_SYMBOL(StatisticsResult) &result = *result_;
	STATIC_ASSERT(sizeof(m256) == sizeof(__m256 ));
	__m256 sum = _mm256_setzero_ps();
	__m256 const zero = _mm256_setzero_ps();
	__m256 const nan = _mm256_set1_ps(NAN);
	__m256 min = nan;
	__m256 max = nan;
	__m256i index_of_min = _mm256_set1_epi32(-1);
	__m256i index_of_max = _mm256_set1_epi32(-1);
	__m256 square_sum = zero;
#if defined(__AVX2__)
	__m256i zero256i = _mm256_setzero_si256();
	__m256i count = zero256i;
	double const *mask_ = AssumeAligned(reinterpret_cast<double const *>(is_valid));
#else
	__m128i const zero128i = _mm_setzero_si128();
	__m128i count = zero128i;
	float const *mask_ = AssumeAligned(reinterpret_cast<float const *>(is_valid));
#endif
	__m256 const *data_ = AssumeAligned(reinterpret_cast<__m256 const *>(data));
	for (size_t i = 0; i < elements / (sizeof(__m256 ) / sizeof(float)); ++i) {
#if defined(__AVX2__)
		__m256i mask8 = _mm256_cvtepi8_epi32(
				_mm_castpd_si128(_mm_load1_pd(&mask_[i])));
		count = _mm256_add_epi32(count, mask8);
		mask8 = _mm256_cmpeq_epi32(mask8, zero256i);
#else
		__m128i mask0 = _mm_castps_si128(_mm_load_ss(&mask_[i * 2]));
		__m128i mask1 = _mm_castps_si128(_mm_load_ss(&mask_[i * 2 + 1]));
		mask0 = _mm_cvtepu8_epi32(mask0);
		mask1 = _mm_cvtepu8_epi32(mask1);
		count = _mm_add_epi32(_mm_add_epi32(count, mask0), mask1);

		mask0 = _mm_cmpeq_epi32(mask0, zero128i);
		mask1 = _mm_cmpeq_epi32(mask1, zero128i);
		__m256i mask8 = _mm256_insertf128_si256(_mm256_castsi128_si256(mask0),
				mask1, 1);
#endif
		__m256 maskf = _mm256_cvtepi32_ps(mask8);
		/* maskf: 0xffffffff means invalid data, 0 means valid data. */

		__m256 value = data_[i];
		sum += _mm256_blendv_ps(value, zero, maskf);
		square_sum += _mm256_blendv_ps(_mm256_mul_ps(value, value), zero,
				maskf);

		min = _mm256_min_ps(min, _mm256_blendv_ps(value, min, maskf));
		max = _mm256_max_ps(max, _mm256_blendv_ps(value, max, maskf));
		value = _mm256_blendv_ps(value, nan, maskf);
		__m256i const index = _mm256_set1_epi32(i);
		index_of_min = _mm256_castps_si256(
				_mm256_blendv_ps(_mm256_castsi256_ps(index),
						_mm256_castsi256_ps(index_of_min),
						_mm256_cmp_ps(min, value, _CMP_NEQ_UQ)));
		index_of_max = _mm256_castps_si256(
				_mm256_blendv_ps(_mm256_castsi256_ps(index),
						_mm256_castsi256_ps(index_of_max),
						_mm256_cmp_ps(max, value, _CMP_NEQ_UQ)));
	}
	{
#if defined(__AVX2__)
		size_t numTotal = static_cast<size_t>(AddHorizontally(count));
#else
		size_t numTotal = static_cast<size_t>(AddHorizontally128(count));
#endif
		float total = AddHorizontally(sum);
		result.sum = total;
		result.count = numTotal;
	}

	{
		m256 tmp;
		tmp.m256 = min;
		m256 tmp_index;
		tmp_index.m256i = index_of_min;

		float r = tmp.floatv[0];
		int result_index = tmp_index.intv[0] * ELEMENTSOF(tmp.intv);
		for (unsigned i = 1; i < ELEMENTSOF(tmp.floatv); ++i) {
			if (isnanf(r) || (!isnanf(tmp.floatv[i]) && tmp.floatv[i] < r)) {
				r = tmp.floatv[i];
				result_index = tmp_index.intv[i] * ELEMENTSOF(tmp.intv) + i;
			}
		}
		result.min = r;
		result_index = std::max(-1, result_index);
		result.index_of_min = result_index;
	}

	{
		m256 tmp;
		tmp.m256 = max;
		m256 tmp_index;
		tmp_index.m256i = index_of_max;

		float r = tmp.floatv[0];
		int result_index = tmp_index.intv[0] * ELEMENTSOF(tmp.intv);
		for (unsigned i = 1; i < ELEMENTSOF(tmp.floatv); ++i) {
			if (isnanf(r) || (!isnanf(tmp.floatv[i]) && tmp.floatv[i] > r)) {
				r = tmp.floatv[i];
				result_index = tmp_index.intv[i] * ELEMENTSOF(tmp.intv) + i;
			}
		}
		result.max = r;
		result_index = std::max(-1, result_index);
		result.index_of_max = result_index;
	}
	float total = AddHorizontally(square_sum);
	int start = (elements / (sizeof(__m256 ) / sizeof(float)))
			* (sizeof(__m256 ) / sizeof(float));
	assert(elements <= INT32_MAX);
	for (int32_t i = start; i < static_cast<int32_t>(elements); ++i) {
		if (is_valid[i]) {
			++result.count;
			result.sum += data[i];
			if (isnanf(result.min)
					|| (!isnanf(data[i]) && data[i] < result.min)) {
				result.min = data[i];
				result.index_of_min = i;
			}
			if (isnanf(result.max)
					|| (!isnanf(data[i]) && data[i] > result.max)) {
				result.max = data[i];
				result.index_of_max = i;
			}
			total += data[i] * data[i];
		}
	}
	float float_count = static_cast<float>(result.count);
	float rms2 = NAN;
	if (result.count == 0) {
		result.mean = NAN;
	} else {
		result.mean = result.sum / float_count;
		rms2 = total / float_count;
	}

	result.rms = std::sqrt(rms2);
	result.stddev = std::sqrt(rms2 - result.mean * result.mean);
}

} /* anonymous namespace */

#else /* defined(__AVX__) && !defined(ARCH_SCALAR) && (! FORCE_EIGEN) */

#define EIGEN_DENSEBASE_PLUGIN "eigen_binary_visitor_plugin.h"
#include <Eigen/Core>

using ::Eigen::Map;
using ::Eigen::Array;
using ::Eigen::Dynamic;
using ::Eigen::Aligned;

namespace {

template<typename Scalar, typename ScararOther, typename Index>
class StatVisitor {
public:
	StatVisitor() :
			count(0), sum(0), min(NAN), max(NAN), square_sum(0), index_of_min(
					-1), index_of_max(-1) {
	}
// called for the first coefficient
	inline bool Init(const Scalar& value, const ScararOther &is_valid, int i,
			int j) {
		assert(j == 0); //  support only 1 dimension array
		if (!is_valid) {
			return false;
		}
		count = 1;
		sum = value;
		min = max = value;
		square_sum = value * value;
		index_of_min = index_of_max = i;
		return true;
	}
// called for all other coefficients
	inline void operator()(const Scalar& value, const ScararOther & is_valid,
			int i, int j) {
		assert(j == 0); //  support only 1 dimension array
		if (is_valid) {
			++count;
			assert(!isnanf(value));
			sum += value;
			square_sum += value * value;
			if (value < min) {
				min = value;
				index_of_min = i;
			}
			if (value > max) {
				max = value;
				index_of_max = i;
			}
		}
	}

	size_t count;
	Scalar sum;
	Scalar min, max;
	Scalar square_sum;
	int index_of_min, index_of_max;
};

template<typename DataType>
inline void ComputeStatisticsEigen(DataType const *data, bool const *is_valid,
		size_t elements,
		LIBSAKURA_SYMBOL(StatisticsResult) *result_) {
	LIBSAKURA_SYMBOL(StatisticsResult) &result = *result_;

	assert(LIBSAKURA_SYMBOL(IsAligned)(data));
	Map<Array<DataType, Dynamic, 1>, Aligned> data_(const_cast<float *>(data),
			elements);

	assert(LIBSAKURA_SYMBOL(IsAligned)(is_valid));
	Map<Array<bool, Dynamic, 1>, Aligned> is_valid_(
			const_cast<bool *>(is_valid), elements);

	StatVisitor<DataType, bool,
			typename Map<Array<DataType, Dynamic, 1>, Aligned>::Index> visitor;
	data_.VisitWith(is_valid_, visitor);
	result.count = visitor.count;
	result.sum = visitor.sum;
	result.min = visitor.min;
	result.index_of_min = visitor.index_of_min;
	result.max = visitor.max;
	result.index_of_max = visitor.index_of_max;
	float rms2 = NAN;
	if (visitor.count == 0) {
		result.mean = NAN;
	} else {
		result.mean = result.sum / result.count;
		rms2 = visitor.square_sum / result.count;
	}
	result.rms = std::sqrt(rms2);
	result.stddev = std::sqrt(rms2 - result.mean * result.mean);
}

} /* anonymous namespace */

#endif /* defined(__AVX__) && !defined(ARCH_SCALAR) && (! FORCE_EIGEN) */

namespace LIBSAKURA_PREFIX {
void ADDSUFFIX(Statistics, ARCH_SUFFIX)::ComputeStatistics(float const data[],
bool const is_valid[], size_t elements,
LIBSAKURA_SYMBOL(StatisticsResult) *result) const {
#if defined( __AVX__) && !defined(ARCH_SCALAR) && (! FORCE_EIGEN)
	ComputeStatisticsSimd(data, is_valid, elements, result);
#else
	ComputeStatisticsEigen(data, is_valid, elements, result);
#endif
}
}
