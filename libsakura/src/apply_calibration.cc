/*
 * @SAKURA_LICENSE_HEADER_START@
 * Copyright (C) 2013-2014
 * National Astronomical Observatory of Japan
 * 2-21-1, Osawa, Mitaka, Tokyo, 181-8588, Japan.
 * 
 * This file is part of Sakura.
 * 
 * Sakura is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation, either version 3 of the License, or (at your 
 * option) any later version.
 * 
 * Sakura is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with Sakura.  If not, see <http://www.gnu.org/licenses/>.
 * @SAKURA_LICENSE_HEADER_END@
 */
#include <cassert>
#include <cstddef>
#include <cmath>
#include <climits>
#include <algorithm>
#include <iostream>

#include <Eigen/Core>

#include "libsakura/sakura.h"
#include "libsakura/localdef.h"
#include "libsakura/logger.h"
#include "libsakura/packed_type.h"
namespace {
#include "libsakura/packed_operation.h"
}

using ::Eigen::Map;
using ::Eigen::Array;
using ::Eigen::Dynamic;
using ::Eigen::Aligned;

namespace {

// a logger for this module
auto logger = LIBSAKURA_PREFIX::Logger::GetLogger("apply_calibration");

#if defined(__AVX__) && !defined(ARCH_SCALAR)
#include <immintrin.h>

template<typename Arch, typename DataType, typename Context>
struct PacketAction {
	static inline void Prologue(Context *context) {
	}
	static inline void Action(size_t idx,
			typename Arch::PacketType const *scaling_factor,
			typename Arch::PacketType const *target,
			typename Arch::PacketType const *reference,
			typename Arch::PacketType *result, Context *context) {
//		*result = LIBSAKURA_SYMBOL(SimdMath)<Arch, DataType>::Mul(
//		LIBSAKURA_SYMBOL(SimdMath)<Arch, DataType>::Mul(*scaling_factor,
//		LIBSAKURA_SYMBOL(SimdMath)<Arch, DataType>::Sub(*target, *reference)),
//		LIBSAKURA_SYMBOL(SimdMath)<Arch, DataType>::Reciprocal(*reference));
		*result = LIBSAKURA_SYMBOL(SimdMath)<Arch, DataType>::Div(
		LIBSAKURA_SYMBOL(SimdMath)<Arch, DataType>::Mul(*scaling_factor,
		LIBSAKURA_SYMBOL(SimdMath)<Arch, DataType>::Sub(*target, *reference)),
				*reference);
	}
	static inline void Epilogue(Context *context) {
	}
};

template<typename ScalarType, typename Context>
struct ScalarAction {
	static inline void Prologue(Context *context) {
	}
	static inline void Action(size_t idx, ScalarType const*scaling_factor,
			ScalarType const*target, ScalarType const*reference,
			ScalarType *result, Context *context) {
		*result = *scaling_factor * (*target - *reference) / *reference;
	}
	static inline void Epilogue(Context *context) {
	}
};

template<class DataType>
inline void ApplyPositionSwitchCalibrationSimd(size_t num_scaling_factor,
		DataType const scaling_factor[/*num_scaling_factor*/], size_t num_data,
		DataType const target[/*num_data*/],
		DataType const reference[/*num_data*/], DataType result[/*num_data*/]) {
	if (num_scaling_factor == 1) {
		DataType const constant_scaling_factor = scaling_factor[0];
		for (size_t i = 0; i < num_data; ++i) {
			result[i] = constant_scaling_factor * (target[i] - reference[i])
					/ reference[i];
		}
	} else {
		typedef LIBSAKURA_SYMBOL(SimdArchNative) Arch;
		LIBSAKURA_SYMBOL(SimdIterate)<Arch, DataType const, Arch,
				DataType const, Arch, DataType const, Arch, DataType,
				PacketAction<Arch, DataType, void>,
				ScalarAction<DataType, void>, void>(num_data, scaling_factor,
				target, reference, result, (void*) nullptr);
	}
}

template<>
inline void ApplyPositionSwitchCalibrationSimd<float>(size_t num_scaling_factor,
		float const scaling_factor[/*num_scaling_factor*/], size_t num_data,
		float const target[/*num_data*/], float const reference[/*num_data*/],
		float result[/*num_data*/]) {
	constexpr size_t kNumFloat = LIBSAKURA_SYMBOL(SimdPacketAVX)::kNumFloat;
	size_t num_packed_operation = num_data / kNumFloat;
	size_t num_data_packed = num_packed_operation * kNumFloat;
	if (num_scaling_factor == 1) {
		__m256 s = _mm256_broadcast_ss(scaling_factor);
//		__m256 s = _mm256_set1_ps(scaling_factor[0]);
		__m256  const *tar_m256 = reinterpret_cast<__m256  const *>(target);
		__m256  const *ref_m256 = reinterpret_cast<__m256  const *>(reference);
		__m256 *res_m256 = reinterpret_cast<__m256 *>(result);
		for (size_t i = 0; i < num_packed_operation; ++i) {
			res_m256[i] = _mm256_div_ps(
					_mm256_mul_ps(s, _mm256_sub_ps(tar_m256[i], ref_m256[i])),
					ref_m256[i]);
		}
		for (size_t i = num_data_packed; i < num_data; ++i) {
			result[i] = scaling_factor[0] * (target[i] - reference[i])
					/ reference[i];
		}
	} else {
		__m256  const *sca_m256 =
				reinterpret_cast<__m256  const *>(scaling_factor);
		__m256  const *tar_m256 = reinterpret_cast<__m256  const *>(target);
		__m256  const *ref_m256 = reinterpret_cast<__m256  const *>(reference);
		__m256 *res_m256 = reinterpret_cast<__m256 *>(result);
		for (size_t i = 0; i < num_packed_operation; ++i) {
			// Here, we don't use _mm256_rcp_ps with _mm256_mul_ps instead of
			// _mm256_div_ps since the former loses accuracy (worse than
			// documented).
			res_m256[i] = _mm256_div_ps(
					_mm256_mul_ps(sca_m256[i],
							_mm256_sub_ps(tar_m256[i], ref_m256[i])),
					ref_m256[i]);
		}
		for (size_t i = num_data_packed; i < num_data; ++i) {
			result[i] = scaling_factor[i] * (target[i] - reference[i])
					/ reference[i];
		}
	}
}
#endif

template<class DataType>
inline void ApplyPositionSwitchCalibrationEigen(size_t num_scaling_factor,
		DataType const scaling_factor[/*num_scaling_factor*/], size_t num_data,
		DataType const target[/*num_data*/],
		DataType const reference[/*num_data*/], DataType result[/*num_data*/]) {
	Map<Array<DataType, Dynamic, 1>, Aligned> eigen_result(result, num_data);
	Map<Array<DataType, Dynamic, 1>, Aligned> eigen_target(
			const_cast<DataType *>(target), num_data);
	Map<Array<DataType, Dynamic, 1>, Aligned> eigen_reference(
			const_cast<DataType *>(reference), num_data);
	if (num_scaling_factor == 1) {
		DataType const constant_scaling_factor = scaling_factor[0];
		eigen_result = constant_scaling_factor
				* (eigen_target - eigen_reference) / eigen_reference;
	} else {
		Map<Array<DataType, Dynamic, 1>, Aligned> eigen_scaling_factor(
				const_cast<DataType *>(scaling_factor), num_data);
		eigen_result = eigen_scaling_factor * (eigen_target - eigen_reference)
				/ eigen_reference;
	}
}

template<class DataType>
inline void ApplyPositionSwitchCalibrationDefault(size_t num_scaling_factor,
		DataType const *scaling_factor_arg, size_t num_data,
		DataType const *target_arg,
		DataType const *reference_arg,
		DataType *result_arg) {
	DataType const *__restrict scaling_factor = AssumeAligned(
			scaling_factor_arg);
	DataType const *__restrict target = AssumeAligned(target_arg);
	DataType const *__restrict reference = AssumeAligned(reference_arg);
	DataType *__restrict result = AssumeAligned(result_arg);
	if (num_scaling_factor == 1) {
		DataType const constant_scaling_factor = scaling_factor[0];
		for (size_t i = 0; i < num_data; ++i) {
			result[i] = constant_scaling_factor * (target[i] - reference[i])
					/ reference[i];
		}
	} else {
		for (size_t i = 0; i < num_data; ++i) {
			result[i] = scaling_factor[i] * (target[i] - reference[i])
					/ reference[i];
		}
	}
}

template<class DataType>
void ApplyPositionSwitchCalibration(
		size_t num_scaling_factor,
		DataType const scaling_factor[/*num_scaling_factor*/], size_t num_data,
		DataType const target[/*num_data*/],
		DataType const reference[/*num_data*/],
		DataType result[/*num_data*/]) {
	assert(num_scaling_factor > 0);
	assert(num_scaling_factor == 1 || num_scaling_factor >= num_data);
	assert(scaling_factor != nullptr);
	assert(target != nullptr);
	assert(reference != nullptr);
	assert(result != nullptr);
	assert(LIBSAKURA_SYMBOL(IsAligned)(scaling_factor));
	assert(LIBSAKURA_SYMBOL(IsAligned)(target));
	assert(LIBSAKURA_SYMBOL(IsAligned)(reference));
	assert(LIBSAKURA_SYMBOL(IsAligned)(result));
	if (target == result) {
#if defined(__AVX__) && !defined(ARCH_SCALAR)
		ApplyPositionSwitchCalibrationSimd(num_scaling_factor, scaling_factor,
				num_data, target, reference, result);
#else
		ApplyPositionSwitchCalibrationEigen(num_scaling_factor, scaling_factor,
				num_data, target, reference, result);
#endif
	} else {
		ApplyPositionSwitchCalibrationDefault(num_scaling_factor,
				scaling_factor, num_data, target, reference, result);
	}
}

} /* anonymous namespace */

extern "C" LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(ApplyPositionSwitchCalibrationFloat)(
		size_t num_scaling_factor,
		float const scaling_factor[/*num_scaling_factor*/], size_t num_data,
		float const target[/*num_data*/], float const reference[/*num_data*/],
		float result[/*num_data*/]) noexcept {
	if (num_data == 0) {
		// Nothing to do
		return LIBSAKURA_SYMBOL(Status_kOK);
	}

	if (num_scaling_factor == 0) {
		// scaling factor must be given
		LOG4CXX_ERROR(logger, "Empty scaling factor (num_scaling_factor == 0)");
		return LIBSAKURA_SYMBOL(Status_kInvalidArgument);
	}

	if (num_scaling_factor != 1 && num_scaling_factor != num_data) {
		// scaling factor must be given
		LOG4CXX_ERROR(logger,
				"Invalid number of scaling factor. num_scaling_factor must be 1 or >= num_data");
		return LIBSAKURA_SYMBOL(Status_kInvalidArgument);
	}

	if (scaling_factor == nullptr || target == nullptr || reference == nullptr
			|| result == nullptr) {
		// null pointer
		LOG4CXX_ERROR(logger, "Input pointers are null");
		return LIBSAKURA_SYMBOL(Status_kInvalidArgument);
	}

	if (!LIBSAKURA_SYMBOL(IsAligned)(scaling_factor)
			|| !LIBSAKURA_SYMBOL(IsAligned)(target)
			|| !LIBSAKURA_SYMBOL(IsAligned)(reference)
			|| !LIBSAKURA_SYMBOL(IsAligned)(result)) {
		// array is not aligned
		LOG4CXX_ERROR(logger, "Arrays are not aligned");
		return LIBSAKURA_SYMBOL(Status_kInvalidArgument);
	}

	try {
		ApplyPositionSwitchCalibration(num_scaling_factor,
				scaling_factor, num_data, target, reference, result);
		return LIBSAKURA_SYMBOL(Status_kOK);
	} catch (...) {
		// any exception is thrown during interpolation
		assert(false);
		LOG4CXX_ERROR(logger, "Aborted due to unknown error");
		return LIBSAKURA_SYMBOL(Status_kUnknownError);
	}
}
