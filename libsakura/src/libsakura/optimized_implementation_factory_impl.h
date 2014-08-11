/*
 * @SAKURA_LICENSE_HEADER_START@
 * @SAKURA_LICENSE_HEADER_END@
 */
#ifndef LIBSAKURA_LIBSAKURA_OPTIMIZED_IMPLEMENTATION_FACTORY_IMPL_H_
#define LIBSAKURA_LIBSAKURA_OPTIMIZED_IMPLEMENTATION_FACTORY_IMPL_H_

// Author: Kohji Nakamura <k.nakamura@nao.ac.jp>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution

#include <libsakura/optimized_implementation_factory.h>

namespace LIBSAKURA_PREFIX {

template<class DataType>
class ApplyCalibrationDefault: public ApplyCalibration<DataType> {
public:
	ApplyCalibrationDefault() {}
	virtual ~ApplyCalibrationDefault() {
	}
	virtual void ApplyPositionSwitchCalibration(size_t num_scaling_factor,
			DataType const scaling_factor[/*num_scaling_factor*/],
			size_t num_data, DataType const target[/*num_data*/],
			DataType const reference[/*num_data*/],
			DataType result[/*num_data*/]) const;
};

template<class DataType>
class ApplyCalibrationAfterSandyBridge: public ApplyCalibration<DataType> {
public:
	ApplyCalibrationAfterSandyBridge() {}
	virtual ~ApplyCalibrationAfterSandyBridge() {
	}
	virtual void ApplyPositionSwitchCalibration(size_t num_scaling_factor,
			DataType const scaling_factor[/*num_scaling_factor*/],
			size_t num_data, DataType const target[/*num_data*/],
			DataType const reference[/*num_data*/],
			DataType result[/*num_data*/]) const;
};

template<class DataType>
class ApplyCalibrationAfterHaswell: public ApplyCalibration<DataType> {
public:
	ApplyCalibrationAfterHaswell() {}
	virtual ~ApplyCalibrationAfterHaswell() {
	}
	virtual void ApplyPositionSwitchCalibration(size_t num_scaling_factor,
			DataType const scaling_factor[/*num_scaling_factor*/],
			size_t num_data, DataType const target[/*num_data*/],
			DataType const reference[/*num_data*/],
			DataType result[/*num_data*/]) const;
};

class BaselineDefault: public Baseline {
public:
	BaselineDefault() {}
	virtual ~BaselineDefault() {
	}
	virtual void CreateBaselineContext(
	LIBSAKURA_SYMBOL(BaselineType) const baseline_type, uint16_t const order,
			size_t const num_data,
			LIBSAKURA_SYMBOL(BaselineContext) **context) const;
	virtual void DestroyBaselineContext(
	LIBSAKURA_SYMBOL(BaselineContext) *context) const;
	virtual void GetBestFitBaseline(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineContext) const *context,
			float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
	virtual void SubtractBaseline(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineContext) const *context,
			float clip_threshold_sigma, uint16_t num_fitting_max,
			bool get_residual,
			bool final_mask[/*num_data*/], float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
	virtual void SubtractBaselinePolynomial(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/], uint16_t order,
			float clip_threshold_sigma, uint16_t num_fitting_max,
			bool get_residual,
			bool final_mask[/*num_data*/], float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
};

class BaselineAfterSandyBridge: public Baseline {
public:
	BaselineAfterSandyBridge() {}
	virtual ~BaselineAfterSandyBridge() {
	}
	virtual void CreateBaselineContext(
	LIBSAKURA_SYMBOL(BaselineType) const baseline_type, uint16_t const order,
			size_t const num_data,
			LIBSAKURA_SYMBOL(BaselineContext) **context) const;
	virtual void DestroyBaselineContext(
	LIBSAKURA_SYMBOL(BaselineContext) *context) const;
	virtual void GetBestFitBaseline(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineContext) const *context,
			float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
	virtual void SubtractBaseline(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineContext) const *context,
			float clip_threshold_sigma, uint16_t num_fitting_max,
			bool get_residual,
			bool final_mask[/*num_data*/], float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
	virtual void SubtractBaselinePolynomial(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/], uint16_t order,
			float clip_threshold_sigma, uint16_t num_fitting_max,
			bool get_residual,
			bool final_mask[/*num_data*/], float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
};

class BaselineAfterHaswell: public Baseline {
public:
	BaselineAfterHaswell() {}
	virtual ~BaselineAfterHaswell() {
	}
	virtual void CreateBaselineContext(
	LIBSAKURA_SYMBOL(BaselineType) const baseline_type, uint16_t const order,
			size_t const num_data,
			LIBSAKURA_SYMBOL(BaselineContext) **context) const;
	virtual void DestroyBaselineContext(
	LIBSAKURA_SYMBOL(BaselineContext) *context) const;
	virtual void GetBestFitBaseline(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineContext) const *context,
			float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
	virtual void SubtractBaseline(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineContext) const *context,
			float clip_threshold_sigma, uint16_t num_fitting_max,
			bool get_residual,
			bool final_mask[/*num_data*/], float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
	virtual void SubtractBaselinePolynomial(size_t num_data,
			float const data[/*num_data*/],
			bool const mask[/*num_data*/], uint16_t order,
			float clip_threshold_sigma, uint16_t num_fitting_max,
			bool get_residual,
			bool final_mask[/*num_data*/], float out[/*num_data*/],
			LIBSAKURA_SYMBOL(BaselineStatus) *baseline_status) const;
};

template<typename DataType>
class BitOperationDefault: public BitOperation<DataType> {
public:
	BitOperationDefault() {}
	virtual ~BitOperationDefault() {
	}
	virtual void OperateBitsAnd(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsConverseNonImplication(DataType bit_mask,
			size_t num_data, DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsImplication(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsNot(size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsOr(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsXor(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
};

template<typename DataType>
class BitOperationAfterSandyBridge: public BitOperation<DataType> {
public:
	BitOperationAfterSandyBridge() {}
	virtual ~BitOperationAfterSandyBridge() {
	}
	virtual void OperateBitsAnd(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsConverseNonImplication(DataType bit_mask,
			size_t num_data, DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsImplication(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsNot(size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsOr(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsXor(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
};

template<typename DataType>
class BitOperationAfterHaswell: public BitOperation<DataType> {
public:
	BitOperationAfterHaswell() {}
	virtual ~BitOperationAfterHaswell() {
	}
	virtual void OperateBitsAnd(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsConverseNonImplication(DataType bit_mask,
			size_t num_data, DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsImplication(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsNot(size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsOr(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
	virtual void OperateBitsXor(DataType bit_mask, size_t num_data,
			DataType const data[/*num_data*/],
			bool const edit_mask[/*num_data*/],
			DataType result[/*num_data*/]) const;
};

template<typename DataType>
class BoolFilterCollectionDefault: public BoolFilterCollection<DataType> {
public:
	BoolFilterCollectionDefault() {}
	virtual ~BoolFilterCollectionDefault() {
	}
	virtual void SetTrueInRangesInclusive(size_t num_data,
			DataType const data[/*num_data*/], size_t num_condition,
			DataType const lower_bounds[/*num_condition*/],
			DataType const upper_bounds[/*num_condition*/],
			bool result[/*num_data*/]) const;
	virtual void SetTrueInRangesExclusive(size_t num_data,
			DataType const data[/*num_data*/], size_t num_condition,
			DataType const lower_bounds[/*num_condition*/],
			DataType const upper_bounds[/*num_condition*/],
			bool result[/*num_data*/]) const;
	virtual void SetTrueGreaterThan(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueGreaterThanOrEquals(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueLessThan(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueLessThanOrEquals(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetFalseIfNanOrInf(size_t num_data,
			DataType const data[/*num_data*/], bool result[/*num_data*/]) const;
	virtual void ToBool(size_t num_data, DataType const data[/*num_data*/],
	bool result[/*num_data*/]) const;
	virtual void InvertBool(size_t num_data, bool const data[/*num_data*/],
	bool result[/*num_data*/]) const;
};

template<typename DataType>
class BoolFilterCollectionAfterSandyBridge: public BoolFilterCollection<DataType> {
public:
	BoolFilterCollectionAfterSandyBridge() {}
	virtual ~BoolFilterCollectionAfterSandyBridge() {
	}
	virtual void SetTrueInRangesInclusive(size_t num_data,
			DataType const data[/*num_data*/], size_t num_condition,
			DataType const lower_bounds[/*num_condition*/],
			DataType const upper_bounds[/*num_condition*/],
			bool result[/*num_data*/]) const;
	virtual void SetTrueInRangesExclusive(size_t num_data,
			DataType const data[/*num_data*/], size_t num_condition,
			DataType const lower_bounds[/*num_condition*/],
			DataType const upper_bounds[/*num_condition*/],
			bool result[/*num_data*/]) const;
	virtual void SetTrueGreaterThan(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueGreaterThanOrEquals(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueLessThan(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueLessThanOrEquals(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetFalseIfNanOrInf(size_t num_data,
			DataType const data[/*num_data*/], bool result[/*num_data*/]) const;
	virtual void ToBool(size_t num_data, DataType const data[/*num_data*/],
	bool result[/*num_data*/]) const;
	virtual void InvertBool(size_t num_data, bool const data[/*num_data*/],
	bool result[/*num_data*/]) const;
};

template<typename DataType>
class BoolFilterCollectionAfterHaswell: public BoolFilterCollection<DataType> {
public:
	BoolFilterCollectionAfterHaswell() {}
	virtual ~BoolFilterCollectionAfterHaswell() {
	}
	virtual void SetTrueInRangesInclusive(size_t num_data,
			DataType const data[/*num_data*/], size_t num_condition,
			DataType const lower_bounds[/*num_condition*/],
			DataType const upper_bounds[/*num_condition*/],
			bool result[/*num_data*/]) const;
	virtual void SetTrueInRangesExclusive(size_t num_data,
			DataType const data[/*num_data*/], size_t num_condition,
			DataType const lower_bounds[/*num_condition*/],
			DataType const upper_bounds[/*num_condition*/],
			bool result[/*num_data*/]) const;
	virtual void SetTrueGreaterThan(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueGreaterThanOrEquals(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueLessThan(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetTrueLessThanOrEquals(size_t num_data,
			DataType const data[/*num_data*/], DataType threshold,
			bool result[/*num_data*/]) const;
	virtual void SetFalseIfNanOrInf(size_t num_data,
			DataType const data[/*num_data*/], bool result[/*num_data*/]) const;
	virtual void ToBool(size_t num_data, DataType const data[/*num_data*/],
	bool result[/*num_data*/]) const;
	virtual void InvertBool(size_t num_data, bool const data[/*num_data*/],
	bool result[/*num_data*/]) const;
};

class ConvolutionDefault: public Convolution {
public:
	ConvolutionDefault() {}
	virtual ~ConvolutionDefault() {
	}
	virtual void CreateConvolve1DContext(size_t num_data,
	LIBSAKURA_SYMBOL(Convolve1DKernelType) kernel_type, size_t kernel_width,
	bool use_fft, LIBSAKURA_SYMBOL(Convolve1DContext) **context) const;
	virtual void Convolve1D(LIBSAKURA_SYMBOL(Convolve1DContext) const *context,
			size_t num_data, float const input_data[/*num_data*/],
			float output_data[/*num_data*/]) const;
	virtual void DestroyConvolve1DContext(
	LIBSAKURA_SYMBOL(Convolve1DContext) *context) const;
};

class ConvolutionAfterSandyBridge: public Convolution {
public:
	ConvolutionAfterSandyBridge() {}
	virtual ~ConvolutionAfterSandyBridge() {
	}
	virtual void CreateConvolve1DContext(size_t num_data,
	LIBSAKURA_SYMBOL(Convolve1DKernelType) kernel_type, size_t kernel_width,
	bool use_fft, LIBSAKURA_SYMBOL(Convolve1DContext) **context) const;
	virtual void Convolve1D(LIBSAKURA_SYMBOL(Convolve1DContext) const *context,
			size_t num_data, float const input_data[/*num_data*/],
			float output_data[/*num_data*/]) const;
	virtual void DestroyConvolve1DContext(
	LIBSAKURA_SYMBOL(Convolve1DContext) *context) const;
};

class ConvolutionAfterHaswell: public Convolution {
public:
	ConvolutionAfterHaswell() {}
	virtual ~ConvolutionAfterHaswell() {
	}
	virtual void CreateConvolve1DContext(size_t num_data,
	LIBSAKURA_SYMBOL(Convolve1DKernelType) kernel_type, size_t kernel_width,
	bool use_fft, LIBSAKURA_SYMBOL(Convolve1DContext) **context) const;
	virtual void Convolve1D(LIBSAKURA_SYMBOL(Convolve1DContext) const *context,
			size_t num_data, float const input_data[/*num_data*/],
			float output_data[/*num_data*/]) const;
	virtual void DestroyConvolve1DContext(
	LIBSAKURA_SYMBOL(Convolve1DContext) *context) const;
};

class GriddingDefault: public Gridding {
public:
	GriddingDefault() {}
	virtual void GridConvolving(size_t num_spectra, size_t start_spectrum,
			size_t end_spectrum,
			bool const spectrum_mask[/*num_spectra*/],
			double const x[/*num_spectra*/], double const y[/*num_spectra*/],
			size_t support, size_t sampling, size_t num_polarization,
			uint32_t const polarization_map[/*num_polarization*/],
			size_t num_channels, uint32_t const channel_map[/*num_channels*/],
			bool const mask/*[num_spectra][num_polarization]*/[/*num_channels*/],
			float const value/*[num_spectra][num_polarization]*/[/*num_channels*/],
			float const weight/*[num_spectra]*/[/*num_channels*/],
			bool do_weight,
			size_t num_convolution_table/*= ceil(sqrt(2.)*(support+1)*sampling)*/,
			float const convolution_table[/*num_convolution_table*/],
			size_t num_polarization_for_grid, size_t num_channels_for_grid,
			size_t width, size_t height,
			double weight_sum/*[num_polarization_for_grid]*/[/*num_channels_for_grid*/],
			float weight_of_grid/*[height][width][num_polarization_for_grid]*/[/*num_channels_for_grid*/],
			float grid/*[height][width][num_polarization_for_grid]*/[/*num_channels_for_grid*/]) const;
};

class GriddingAfterSandyBridge: public Gridding {
public:
	GriddingAfterSandyBridge() {}
	virtual void GridConvolving(size_t num_spectra, size_t start_spectrum,
			size_t end_spectrum,
			bool const spectrum_mask[/*num_spectra*/],
			double const x[/*num_spectra*/], double const y[/*num_spectra*/],
			size_t support, size_t sampling, size_t num_polarization,
			uint32_t const polarization_map[/*num_polarization*/],
			size_t num_channels, uint32_t const channel_map[/*num_channels*/],
			bool const mask/*[num_spectra][num_polarization]*/[/*num_channels*/],
			float const value/*[num_spectra][num_polarization]*/[/*num_channels*/],
			float const weight/*[num_spectra]*/[/*num_channels*/],
			bool do_weight,
			size_t num_convolution_table/*= ceil(sqrt(2.)*(support+1)*sampling)*/,
			float const convolution_table[/*num_convolution_table*/],
			size_t num_polarization_for_grid, size_t num_channels_for_grid,
			size_t width, size_t height,
			double weight_sum/*[num_polarization_for_grid]*/[/*num_channels_for_grid*/],
			float weight_of_grid/*[height][width][num_polarization_for_grid]*/[/*num_channels_for_grid*/],
			float grid/*[height][width][num_polarization_for_grid]*/[/*num_channels_for_grid*/]) const;
};

class GriddingAfterHaswell: public Gridding {
public:
	GriddingAfterHaswell() {}
	virtual void GridConvolving(size_t num_spectra, size_t start_spectrum,
			size_t end_spectrum,
			bool const spectrum_mask[/*num_spectra*/],
			double const x[/*num_spectra*/], double const y[/*num_spectra*/],
			size_t support, size_t sampling, size_t num_polarization,
			uint32_t const polarization_map[/*num_polarization*/],
			size_t num_channels, uint32_t const channel_map[/*num_channels*/],
			bool const mask/*[num_spectra][num_polarization]*/[/*num_channels*/],
			float const value/*[num_spectra][num_polarization]*/[/*num_channels*/],
			float const weight/*[num_spectra]*/[/*num_channels*/],
			bool do_weight,
			size_t num_convolution_table/*= ceil(sqrt(2.)*(support+1)*sampling)*/,
			float const convolution_table[/*num_convolution_table*/],
			size_t num_polarization_for_grid, size_t num_channels_for_grid,
			size_t width, size_t height,
			double weight_sum/*[num_polarization_for_grid]*/[/*num_channels_for_grid*/],
			float weight_of_grid/*[height][width][num_polarization_for_grid]*/[/*num_channels_for_grid*/],
			float grid/*[height][width][num_polarization_for_grid]*/[/*num_channels_for_grid*/]) const;
};

template<class XDataType, class YDataType>
class InterpolationDefault: public Interpolation<XDataType, YDataType> {
public:
	InterpolationDefault() {}
	virtual ~InterpolationDefault() {
	}
	virtual void InterpolateXAxis(
	LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method,
			uint8_t polynomial_order, size_t num_x_base,
			XDataType const x_base[/*num_x_base*/], size_t num_y,
			YDataType const data_base[/*num_x_base*num_y*/],
			size_t num_x_interpolated,
			XDataType const x_interpolated[/*num_x_interpolated*/],
			YDataType data_interpolated[/*num_x_interpolated*num_y*/]) const;
	virtual void InterpolateYAxis(
	LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method,
			uint8_t polynomial_order, size_t num_y_base,
			XDataType const y_base[/*num_y_base*/], size_t num_x,
			YDataType const data_base[/*num_y_base*num_x*/],
			size_t num_y_interpolated,
			XDataType const y_interpolated[/*num_y_interpolated*/],
			YDataType data_interpolated[/*num_y_interpolated*num_x*/]) const;
};

template<class XDataType, class YDataType>
class InterpolationAfterSandyBridge: public Interpolation<XDataType, YDataType> {
public:
	InterpolationAfterSandyBridge() {}
	virtual ~InterpolationAfterSandyBridge() {
	}
	virtual void InterpolateXAxis(
	LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method,
			uint8_t polynomial_order, size_t num_x_base,
			XDataType const x_base[/*num_x_base*/], size_t num_y,
			YDataType const data_base[/*num_x_base*num_y*/],
			size_t num_x_interpolated,
			XDataType const x_interpolated[/*num_x_interpolated*/],
			YDataType data_interpolated[/*num_x_interpolated*num_y*/]) const;
	virtual void InterpolateYAxis(
	LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method,
			uint8_t polynomial_order, size_t num_y_base,
			XDataType const x_base[/*num_y_base*/], size_t num_x,
			YDataType const data_base[/*num_y_base*num_x*/],
			size_t num_y_interpolated,
			XDataType const y_interpolated[/*num_y_interpolated*/],
			YDataType data_interpolated[/*num_y_interpolated*num_x*/]) const;
};

template<class XDataType, class YDataType>
class InterpolationAfterHaswell: public Interpolation<XDataType, YDataType> {
public:
	InterpolationAfterHaswell() {}
	virtual ~InterpolationAfterHaswell() {
	}
	virtual void InterpolateXAxis(
	LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method,
			uint8_t polynomial_order, size_t num_x_base,
			XDataType const x_base[/*num_x_base*/], size_t num_y,
			YDataType const data_base[/*num_x_base*num_y*/],
			size_t num_x_interpolated,
			XDataType const x_interpolated[/*num_x_interpolated*/],
			YDataType data_interpolated[/*num_x_interpolated*num_y*/]) const;
	virtual void InterpolateYAxis(
	LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method,
			uint8_t polynomial_order, size_t num_y_base,
			XDataType const x_base[/*num_y_base*/], size_t num_x,
			YDataType const data_base[/*num_y_base*num_x*/],
			size_t num_y_interpolated,
			XDataType const y_interpolated[/*num_y_interpolated*/],
			YDataType data_interpolated[/*num_y_interpolated*num_x*/]) const;
};

class NumericOperationDefault: public NumericOperation {
public:
	NumericOperationDefault() {}
	virtual ~NumericOperationDefault() {
	}
	virtual void GetLeastSquareFittingCoefficients(size_t const num_data,
			float const data[/*num_data*/], bool const mask[/*num_data*/],
			size_t const num_model_bases,
			double const basis_data[/*num_model_bases*num_data*/],
			double lsq_matrix[/*num_model_bases*num_model_bases*/],
			double lsq_vector[/*num_model_bases*/]) const;
	virtual void UpdateLeastSquareFittingCoefficients(size_t const num_data,
			float const data[/*num_data*/], size_t const num_exclude_indices,
			size_t const exclude_indices[/*num_data*/],
			size_t const num_model_bases,
			double const basis_data[/*num_model_bases*num_data*/],
			double lsq_matrix[/*num_model_bases*num_model_bases*/],
			double lsq_vector[/*num_model_bases*/]) const;
	virtual void SolveSimultaneousEquationsByLU(size_t num_equations,
			double const in_matrix[/*num_equations*num_equations*/],
			double const in_vector[/*num_equations*/],
			double out[/*num_equations*/]) const;
};

class NumericOperationAfterSandyBridge: public NumericOperation {
public:
	NumericOperationAfterSandyBridge() {}
	virtual ~NumericOperationAfterSandyBridge() {
	}
	virtual void GetLeastSquareFittingCoefficients(size_t const num_data,
			float const data[/*num_data*/], bool const mask[/*num_data*/],
			size_t const num_model_bases,
			double const basis_data[/*num_model_bases*num_data*/],
			double lsq_matrix[/*num_model_bases*num_model_bases*/],
			double lsq_vector[/*num_model_bases*/]) const;
	virtual void UpdateLeastSquareFittingCoefficients(size_t const num_data,
			float const data[/*num_data*/], size_t const num_exclude_indices,
			size_t const exclude_indices[/*num_data*/],
			size_t const num_model_bases,
			double const basis_data[/*num_model_bases*num_data*/],
			double lsq_matrix[/*num_model_bases*num_model_bases*/],
			double lsq_vector[/*num_model_bases*/]) const;
	virtual void SolveSimultaneousEquationsByLU(size_t num_equations,
			double const in_matrix[/*num_equations*num_equations*/],
			double const in_vector[/*num_equations*/],
			double out[/*num_equations*/]) const;
};

class NumericOperationAfterHaswell: public NumericOperation {
public:
	NumericOperationAfterHaswell() {}
	virtual ~NumericOperationAfterHaswell() {
	}
	virtual void GetLeastSquareFittingCoefficients(size_t const num_data,
			float const data[/*num_data*/], bool const mask[/*num_data*/],
			size_t const num_model_bases,
			double const basis_data[/*num_model_bases*num_data*/],
			double lsq_matrix[/*num_model_bases*num_model_bases*/],
			double lsq_vector[/*num_model_bases*/]) const;
	virtual void UpdateLeastSquareFittingCoefficients(size_t const num_data,
			float const data[/*num_data*/], size_t const num_exclude_indices,
			size_t const exclude_indices[/*num_data*/],
			size_t const num_model_bases,
			double const basis_data[/*num_model_bases*num_data*/],
			double lsq_matrix[/*num_model_bases*num_model_bases*/],
			double lsq_vector[/*num_model_bases*/]) const;
	virtual void SolveSimultaneousEquationsByLU(size_t num_equations,
			double const in_matrix[/*num_equations*num_equations*/],
			double const in_vector[/*num_equations*/],
			double out[/*num_equations*/]) const;
};

class StatisticsDefault: public Statistics {
public:
	StatisticsDefault() {}
	virtual void ComputeStatistics(float const data[], bool const mask[],
			size_t elements,
			LIBSAKURA_SYMBOL(StatisticsResult) *result) const;
};

class StatisticsAfterSandyBridge: public Statistics {
public:
	StatisticsAfterSandyBridge() {}
	virtual void ComputeStatistics(float const data[], bool const mask[],
			size_t elements,
			LIBSAKURA_SYMBOL(StatisticsResult) *result) const;
};

class StatisticsAfterHaswell: public Statistics {
public:
	StatisticsAfterHaswell() {}
	virtual void ComputeStatistics(float const data[], bool const mask[],
			size_t elements,
			LIBSAKURA_SYMBOL(StatisticsResult) *result) const;
};

} // namespace LIBSAKURA_PREFIX

#endif /* LIBSAKURA_LIBSAKURA_OPTIMIZED_IMPLEMENTATION_FACTORY_IMPL_H_ */
