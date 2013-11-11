#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>

#include <libsakura/localdef.h>
#include <libsakura/memory_manager.h>
#include <libsakura/optimized_implementation_factory_impl.h>
#include <libsakura/sakura.h>

#define EIGEN_DENSEBASE_PLUGIN "eigen_binary_visitor_plugin.h"
#include <Eigen/Core>
#include <Eigen/LU>

using ::Eigen::Map;
using ::Eigen::Array;
using ::Eigen::Dynamic;
using ::Eigen::Aligned;

namespace {
inline void OperateFloatSubtraction(size_t num_in, float const *in1,
		float const *in2, float *out) {
	assert(LIBSAKURA_SYMBOL(IsAligned)(in1));
	assert(LIBSAKURA_SYMBOL(IsAligned)(in2));
	assert(LIBSAKURA_SYMBOL(IsAligned)(out));
	STATIC_ASSERT(sizeof(in1) == sizeof(in2));
	STATIC_ASSERT(true == 1);
	STATIC_ASSERT(false == 0);

	for (size_t i = 0; i < num_in; ++i) {
		out[i] = in1[i] - in2[i];
	}
}

inline void GetCoefficientsForLeastSquareFitting(size_t num_data,
		float const *data, bool const *mask,
		size_t num_model_bases, double const *model,
		double *out_matrix, double *out_vector) {

	assert(LIBSAKURA_SYMBOL(IsAligned)(data));
	assert(LIBSAKURA_SYMBOL(IsAligned)(mask));
	assert(LIBSAKURA_SYMBOL(IsAligned)(model));
	assert(LIBSAKURA_SYMBOL(IsAligned)(out_matrix));
	assert(LIBSAKURA_SYMBOL(IsAligned)(out_vector));

	for (size_t i = 0; i < num_model_bases; ++i) {
		for (size_t j = 0; j < num_model_bases; ++j) {
			size_t idx = num_model_bases * i + j;
			out_matrix[idx] = 0.0;

			for (size_t k = 0; k < num_data ; ++k){
				if (!mask[k]) continue;

				size_t idx_i = num_data * i + k;
				size_t idx_j = num_data * j + k;
				out_matrix[idx] += model[idx_i] * model[idx_j];
			}
		}

		out_vector[i] = 0.0;
		for (size_t k = 0; k < num_data; ++k) {
			if (!mask[k]) continue;

			size_t idx = num_data * i + k;
			out_vector[i] += model[idx] * data[k];
		}
	}
}

inline void SolveSimultaneousEquationsByLU(size_t num_equations,
		double const *lsq_matrix0, double const *lsq_vector0, double *out) {
	assert(LIBSAKURA_SYMBOL(IsAligned)(lsq_matrix0));
	assert(LIBSAKURA_SYMBOL(IsAligned)(lsq_vector0));
	assert(LIBSAKURA_SYMBOL(IsAligned)(out));
	Map<Array<double, Dynamic, 1>, Aligned> lsq_matrix0_array(const_cast<double *>(lsq_matrix0),
			num_equations*num_equations);
	Map<Array<double, Dynamic, 1>, Aligned> lsq_vector0_array(const_cast<double *>(lsq_vector0),
			num_equations);
	Map<Array<double, Dynamic, 1>, Aligned> out_array(const_cast<double *>(out),
			num_equations);

	::Eigen::MatrixXd lsq_matrix(num_equations, num_equations);
	::Eigen::VectorXd lsq_vector(num_equations);
	for (size_t i = 0; i < num_equations; ++i) {
		for (size_t j = 0; j < num_equations; ++j) {
			lsq_matrix(i, j) = lsq_matrix0_array[num_equations*i+j];
		}
		lsq_vector(i) = lsq_vector0_array[i];
	}

	::Eigen::VectorXd lu_result = lsq_matrix.fullPivLu().solve(lsq_vector);
	for (size_t i = 0; i < num_equations; ++i) {
		out_array[i] = lu_result(i);
	}
}

/*
inline void DoGetBestFitBaselineModel(size_t num_data, size_t num_equations,
inline void GetBestFitBaselineModel(size_t num_data,
*/
} /* anonymous namespace */

namespace LIBSAKURA_PREFIX {
void ADDSUFFIX(NumericOperation, ARCH_SUFFIX)::OperateFloatSubtraction(
		size_t num_in,
		float const in1[/*num_in*/],
		float const in2[/*num_in*/],
		float out[/*num_in*/]) const {
	::OperateFloatSubtraction(num_in, in1, in2, out);
}

void ADDSUFFIX(NumericOperation, ARCH_SUFFIX)::GetCoefficientsForLeastSquareFitting(
		size_t num_data, float const data[/*num_data*/], bool const mask[/*num_data*/],
		size_t num_model_bases, double const model[/*num_model_bases*num_data*/],
		double out_matrix[/*num_model_bases*num_model_bases*/],
		double out_vector[/*num_model_bases*/]) const {
	::GetCoefficientsForLeastSquareFitting(
			num_data, data, mask, num_model_bases, model, out_matrix, out_vector);
}

void ADDSUFFIX(NumericOperation, ARCH_SUFFIX)::SolveSimultaneousEquationsByLU(
		size_t num_equations,
		double const lsq_matrix0[/*num_equations*num_equations*/],
		double const lsq_vector0[/*num_equations*/],
		double out[/*num_equations*/]) const {
	::SolveSimultaneousEquationsByLU(num_equations, lsq_matrix0, lsq_vector0, out);
}

}
