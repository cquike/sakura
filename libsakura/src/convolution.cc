#include <cassert>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fftw3.h>
#include <memory>
#include <climits>

#include <libsakura/sakura.h>
#include <libsakura/optimized_implementation_factory_impl.h>
#include <libsakura/localdef.h>
#include <libsakura/logger.h>
#include <libsakura/memory_manager.h>

extern "C" {
struct LIBSAKURA_SYMBOL(Convolve1DContext) {
	bool use_fft;
	size_t num_data;
	size_t kernel_width;
	fftwf_plan plan_real_to_complex_float;
	fftwf_plan plan_complex_to_real_float;
	fftwf_complex *fft_applied_complex_kernel;
	float *real_array;
	float *real_array_kernel;
	void *work_real_array;
	void *work_real_array_kernel;
};
}

namespace {

inline fftwf_complex* AllocateFFTArray(size_t num_data) {
	return (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * num_data);
}

inline void FreeFFTArray(fftwf_complex *ptr) {
	if (ptr != nullptr) {
		fftwf_free(ptr);
	}
}

inline void DestroyFFTPlan(fftwf_plan ptr) {
	if (ptr != nullptr) {
		fftwf_destroy_plan(ptr);
	}
}

inline void ApplyMaskByZero(size_t num_data, float const *input_data,
bool const *mask, float *output_data) {
	for (size_t i = 0; i < num_data; ++i) {
		if (mask[i]) {
			output_data[i] = input_data[i];
		} else {
			output_data[i] = 0.0;
		}
	}
}

inline bool FindFalseToGetStart(size_t num_data, float const input_data,
bool const mask, size_t* X1, float* Y1, size_t count) {
	if (!mask) {
		X1[count] = num_data;
		Y1[count] = input_data;
		return true;
	}
	return false;
}
inline bool FindTrueToGetEnd(size_t num_data, float const input_data,
bool const mask, size_t* X2, float* Y2, size_t count) {
	if (mask) {
		X2[count] = num_data;
		Y2[count] = input_data;
		return true;
	}
	return false;
}

inline void ApplyMaskByLinearInterpolation(size_t num_data,
		float const *input_data,
		bool const *mask, float *output_data) {
	size_t X1[num_data], X2[num_data];
	float Y1[num_data], Y2[num_data];
	bool find_zero = true;
	if (!mask[0]) {
		find_zero = false;
	}
	size_t count = 0;
	for (size_t i = 1; i < num_data; ++i) { // finding pair (x1,y1),(x2,y2)
		if (find_zero) {
			if (FindFalseToGetStart(i - 1, input_data[i - 1], mask[i], X1, Y1,
					count)) {
				find_zero = false;
			}
		} else {
			if (FindTrueToGetEnd(i, input_data[i], mask[i], X2, Y2, count)) {
				find_zero = true;
				count++;
			} else if (i == (num_data - 1)) { // reached final ch
				FindTrueToGetEnd(i, Y1[count], true, X2, Y2, count);
			}
		}
	}
	const size_t count_max = count;
	size_t n = 0;
	bool found = false;
	for (size_t i = 0; i < num_data; ++i) {
		if (!found) {
			output_data[i] = input_data[i];
			if (i == X1[n]) {
				found = true;
			}
		} else if (found && i < X2[n]) {
			output_data[i] = i * (Y2[n] - Y1[n]) / (X2[n] - X1[n])
					+ (X2[n] * Y1[n] - X1[n] * Y2[n]) / (X2[n] - X1[n]);
		} else if (found && i == X2[n]) {
			found = false;
			if (n < count_max)
				n++;
			output_data[i] = input_data[i];
		}
	}
}

inline void Create1DGaussianKernelForWithoutFFT(size_t num_data,
		size_t kernel_width, float* output_data) {
	float const reciprocal_of_denominator = 1.66510922231539551270632928979040
			/ kernel_width; // sqrt(log(16))/ kernel_width
	float const height = .939437278699651333772340328410 / kernel_width; // sqrt(8*log(2)/(2*M_PI)) / kernel_width
	float center = (num_data) / 2.f;
	if (num_data % 2 != 0) {
		center = (num_data - 1) / 2.f;
	}
	size_t middle = (num_data) / 2;
	size_t loop_max = middle;
	size_t plus_one_for_odd = 1;
	if (num_data % 2 == 0) {
		plus_one_for_odd = 0;
		float value = (-center) * reciprocal_of_denominator;
		output_data[0] = height * exp(-(value * value));
	}
	output_data[middle] = height;
	for (size_t j = 1; j < loop_max; ++j) {
		float value = (j - center) * reciprocal_of_denominator;
		output_data[num_data - j + plus_one_for_odd] = output_data[j
				- plus_one_for_odd] = height * exp(-(value * value));
	}
}

inline void Create1DGaussianKernel(size_t num_data, size_t kernel_width,
		float* output_data) {
	float const reciprocal_of_denominator = 1.66510922231539551270632928979040
			/ kernel_width; // sqrt(log(16))/ kernel_width
	float const height = .939437278699651333772340328410 / kernel_width; // sqrt(8*log(2)/(2*M_PI)) / kernel_width
	float center = (num_data) / 2.f;
	if (num_data % 2 != 0) {
		center = (num_data - 1) / 2.f;
	}
	size_t middle = (num_data) / 2;
	size_t loop_max = middle;
	output_data[0] = height;
	output_data[middle] = height
			* exp(
					-(center * reciprocal_of_denominator)
							* (center * reciprocal_of_denominator));
	size_t plus_one_for_odd = 0;
	if (num_data % 2 != 0) {
		plus_one_for_odd = 1;
		output_data[middle + 1] = output_data[middle];
	}
	for (size_t j = 1; j < loop_max; ++j) {
		float value = (j - center) * reciprocal_of_denominator;
		output_data[middle + j + plus_one_for_odd] = output_data[middle - j] =
				height * exp(-(value * value));
	}
}

inline void Create1DKernel(size_t num_data,
LIBSAKURA_SYMBOL(Convolve1DKernelType) kernel_type, size_t kernel_width,
		float* output_data) {
	switch (kernel_type) {
	case LIBSAKURA_SYMBOL(Convolve1DKernelType_kGaussian):
		Create1DGaussianKernel(num_data, kernel_width, output_data);
		break;
	case LIBSAKURA_SYMBOL(Convolve1DKernelType_kBoxcar):
		break;
	case LIBSAKURA_SYMBOL(Convolve1DKernelType_kHanning):
		break;
	case LIBSAKURA_SYMBOL(Convolve1DKernelType_kHamming):
		break;
	default:
		assert(false);
		break;
	}
}

inline void ConvolutionWithoutFFT(size_t num_data, float const *input_data_arg,
		size_t kernel_width, float const *input_kernel,
		float *output_data_arg) {
	assert(!(LIBSAKURA_SYMBOL(IsAligned)(input_data_arg)));
	assert(!(LIBSAKURA_SYMBOL(IsAligned)(output_data_arg)));
	auto input_data = AssumeAligned(input_data_arg);
	auto output_data = AssumeAligned(output_data_arg);
	for (size_t j = 0; j < num_data; ++j) {
		float center = input_data[j] * input_kernel[0];
		float right = 0.0, left = 0.0;
		//for (size_t i = 0; (i < num_data / 2 - 1); ++i) {
		for (size_t i = 0;
				i < (6 * kernel_width / 2.35482004503094938202313865291938) // 6 sigma
				&& (kernel_width < num_data); ++i) {
			if (j + 1 + i < num_data) {
				left += input_data[j + 1 + i] * input_kernel[num_data - 1 - i];
			} else {
				left += input_data[j + 1 + i - num_data]
						* input_kernel[num_data - 1 - i];
			}
		}
		//for (size_t k = 1; k < num_data / 2 + 1; ++k) {
		for (size_t k = 1;
				k < (6 * kernel_width / 2.35482004503094938202313865291938) // 6 sigma
				&& (kernel_width < num_data); ++k) {
			if (j < k) {
				right += input_data[num_data + j - k] * input_kernel[k];
			} else {
				right += input_data[j - k] * input_kernel[k];
			}
		}
		output_data[j] = left + center + right;
	}
}

inline void ConvolutionWithoutFFTRemovePollution(size_t num_data,
		float const *input_data_arg, size_t kernel_width,
		float const *input_kernel, float *output_data_arg) {
	assert(!(LIBSAKURA_SYMBOL(IsAligned)(input_data_arg)));
	assert(!(LIBSAKURA_SYMBOL(IsAligned)(output_data_arg)));
	auto input_data = AssumeAligned(input_data_arg);
	auto output_data = AssumeAligned(output_data_arg);
	for (size_t j = 0; j < num_data; ++j) {
		float center = input_data[j] * input_kernel[0];
		float right = 0.0, left = 0.0;
		for (size_t i = 0; ((j + 1 + i < num_data) && (i < num_data / 2 - 1));
				++i) {
			left += input_data[j + 1 + i] * input_kernel[num_data - 1 - i];
		}
		for (size_t k = 0; k < j && (k < num_data / 2); ++k) {
			right += input_data[j - 1 - k] * input_kernel[k + 1];
		}
		output_data[j] = left + center + right;
	}
}

inline void CreateConvolve1DContext(size_t num_data,
LIBSAKURA_SYMBOL(Convolve1DKernelType) kernel_type, size_t kernel_width,
bool use_fft, LIBSAKURA_SYMBOL(Convolve1DContext)** context) {
	assert(*context == nullptr);
	if (use_fft) {
		float *real_array_kernel = nullptr;
		std::unique_ptr<void, LIBSAKURA_PREFIX::Memory> work_real_array_kernel(
				LIBSAKURA_PREFIX::Memory::AlignedAllocateOrException(
						sizeof(float) * num_data, &real_array_kernel));
		if (work_real_array_kernel == nullptr) {
			throw std::bad_alloc();
		}
		if (!(LIBSAKURA_SYMBOL(IsAligned)(real_array_kernel))) {
			throw std::runtime_error("real_array_kernel is not aligned");
		}
		float *real_array = nullptr;
		std::unique_ptr<void, LIBSAKURA_PREFIX::Memory> work_real_array(
				LIBSAKURA_PREFIX::Memory::AlignedAllocateOrException(
						sizeof(float) * num_data, &real_array));
		if (work_real_array == nullptr) {
			throw std::bad_alloc();
		}
		if (!(LIBSAKURA_SYMBOL(IsAligned)(real_array))) {
			throw std::runtime_error("real_array is not aligned");
		}
		std::unique_ptr<fftwf_complex[], decltype(&FreeFFTArray)> fft_applied_complex_kernel(
				AllocateFFTArray(num_data / 2 + 1), FreeFFTArray);
		if (fft_applied_complex_kernel == nullptr) {
			throw std::bad_alloc();
		}
		std::unique_ptr<fftwf_complex[], decltype(&FreeFFTArray)> fft_applied_complex_input_data(
				AllocateFFTArray(num_data / 2 + 1), FreeFFTArray);
		if (fft_applied_complex_input_data == nullptr) {
			throw std::bad_alloc();
		}
		std::unique_ptr<fftwf_complex[], decltype(&FreeFFTArray)> multiplied_complex_data(
				AllocateFFTArray(num_data / 2 + 1), FreeFFTArray);
		if (multiplied_complex_data == nullptr) {
			throw std::bad_alloc();
		}
		fftwf_plan plan_real_to_complex_float_kernel = fftwf_plan_dft_r2c_1d(
				num_data, real_array_kernel, fft_applied_complex_kernel.get(),
				FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
		ScopeGuard guard_for_fft_plan_kernel([&]() {
			DestroyFFTPlan(plan_real_to_complex_float_kernel);
		});
		if (plan_real_to_complex_float_kernel == nullptr) {
			guard_for_fft_plan_kernel.Disable();
			throw std::bad_alloc();
		}
		fftwf_plan plan_real_to_complex_float = fftwf_plan_dft_r2c_1d(num_data,
				real_array, fft_applied_complex_input_data.get(),
				FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
		ScopeGuard guard_for_fft_plan([&]() {
			DestroyFFTPlan(plan_real_to_complex_float);
		});
		if (plan_real_to_complex_float == nullptr) {
			guard_for_fft_plan.Disable();
			throw std::bad_alloc();
		}
		fftwf_plan plan_complex_to_real_float = fftwf_plan_dft_c2r_1d(num_data,
				multiplied_complex_data.get(), real_array,
				FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
		ScopeGuard guard_for_ifft_plan([&]() {
			DestroyFFTPlan(plan_complex_to_real_float);
		});
		if (plan_complex_to_real_float == nullptr) {
			guard_for_ifft_plan.Disable();
			throw std::bad_alloc();
		}
		Create1DKernel(num_data, kernel_type, kernel_width, real_array_kernel);
		fftwf_execute(plan_real_to_complex_float_kernel);
		guard_for_fft_plan_kernel.CleanUpNow();
		std::unique_ptr<LIBSAKURA_SYMBOL(Convolve1DContext),
		LIBSAKURA_PREFIX::Memory> work_context(
				static_cast<LIBSAKURA_SYMBOL(Convolve1DContext)*>(LIBSAKURA_PREFIX::Memory::Allocate(
						sizeof(LIBSAKURA_SYMBOL(Convolve1DContext)))),
				LIBSAKURA_PREFIX::Memory());
		if (work_context == nullptr) {
			throw std::bad_alloc();
		}
		work_context->fft_applied_complex_kernel =
				fft_applied_complex_kernel.release();
		work_context->real_array = real_array;
		work_context->real_array_kernel = nullptr;
		work_context->work_real_array = work_real_array.release();
		work_context->work_real_array_kernel = work_real_array_kernel.release();
		work_context->plan_real_to_complex_float = plan_real_to_complex_float;
		guard_for_fft_plan.Disable();
		work_context->plan_complex_to_real_float = plan_complex_to_real_float;
		guard_for_ifft_plan.Disable();
		work_context->num_data = num_data;
		work_context->kernel_width = kernel_width;
		work_context->use_fft = use_fft;
		*context = work_context.release();
	} else {
		float *real_array_kernel = nullptr;
		std::unique_ptr<void, LIBSAKURA_PREFIX::Memory> work_real_array_kernel(
				LIBSAKURA_PREFIX::Memory::AlignedAllocateOrException(
						sizeof(float) * num_data, &real_array_kernel));
		if (work_real_array_kernel == nullptr) {
			throw std::bad_alloc();
		}
		if (!(LIBSAKURA_SYMBOL(IsAligned)(real_array_kernel))) {
			throw std::runtime_error("real_array_kernel is not aligned");
		}
		Create1DKernel(num_data, kernel_type, kernel_width, real_array_kernel);
		std::unique_ptr<LIBSAKURA_SYMBOL(Convolve1DContext),
		LIBSAKURA_PREFIX::Memory> work_context(
				static_cast<LIBSAKURA_SYMBOL(Convolve1DContext)*>(LIBSAKURA_PREFIX::Memory::Allocate(
						sizeof(LIBSAKURA_SYMBOL(Convolve1DContext)))),
				LIBSAKURA_PREFIX::Memory());
		if (work_context == nullptr) {
			throw std::bad_alloc();
		}
		work_context->fft_applied_complex_kernel = nullptr;
		work_context->plan_complex_to_real_float = nullptr;
		work_context->plan_real_to_complex_float = nullptr;
		work_context->real_array = nullptr;
		work_context->real_array_kernel = real_array_kernel;
		work_context->work_real_array = nullptr;
		work_context->work_real_array_kernel = work_real_array_kernel.release();
		work_context->num_data = num_data;
		work_context->kernel_width = kernel_width;
		work_context->use_fft = use_fft;
		*context = work_context.release();
	}
}

inline void Convolve1D(
LIBSAKURA_SYMBOL(Convolve1DContext) const *context, size_t num_data,
		float const input_data_arg[/*num_data*/],
		bool const mask_arg[/*num_data*/],
		float output_data_arg[/*num_data*/]) {
	if (!(context->num_data == num_data)) {
		throw std::invalid_argument("num_data must equal to context->num_data");
	}
	assert(!(LIBSAKURA_SYMBOL(IsAligned)(input_data_arg)));
	assert(!(LIBSAKURA_SYMBOL(IsAligned)(output_data_arg)));
	auto input_data = AssumeAligned(input_data_arg);
	auto output_data = AssumeAligned(output_data_arg);
	if (context->use_fft) {
		std::unique_ptr<fftwf_complex[], decltype(&FreeFFTArray)> fft_applied_complex_input_data(
				AllocateFFTArray(num_data / 2 + 1), FreeFFTArray);
		if (fft_applied_complex_input_data == nullptr) {
			throw std::bad_alloc();
		}
		std::unique_ptr<fftwf_complex[], decltype(&FreeFFTArray)> multiplied_complex_data(
				AllocateFFTArray(num_data / 2 + 1), FreeFFTArray);
		if (multiplied_complex_data == nullptr) {
			throw std::bad_alloc();
		}
		fftwf_execute_dft_r2c(context->plan_real_to_complex_float,
				const_cast<float*>(input_data),
				fft_applied_complex_input_data.get());
		float scale = 1.0 / num_data;
		for (size_t i = 0; i < num_data / 2 + 1; ++i) {
			multiplied_complex_data[i][0] =
					(context->fft_applied_complex_kernel[i][0]
							* fft_applied_complex_input_data[i][0]
							- context->fft_applied_complex_kernel[i][1]
									* fft_applied_complex_input_data[i][1])
							* scale;
			multiplied_complex_data[i][1] =
					(context->fft_applied_complex_kernel[i][0]
							* fft_applied_complex_input_data[i][1]
							+ context->fft_applied_complex_kernel[i][1]
									* fft_applied_complex_input_data[i][0])
							* scale;
		}
		fftwf_execute_dft_c2r(context->plan_complex_to_real_float,
				multiplied_complex_data.get(), output_data);
	} else {
		ConvolutionWithoutFFT(num_data, const_cast<float*>(input_data),
				context->kernel_width, context->real_array_kernel, output_data);
	}
}

inline void DestroyConvolve1DContext(
LIBSAKURA_SYMBOL(Convolve1DContext)* context) {
	if (context != nullptr) {
		if (context->plan_real_to_complex_float != nullptr) {
			DestroyFFTPlan(context->plan_real_to_complex_float);
		}
		if (context->plan_complex_to_real_float != nullptr) {
			DestroyFFTPlan(context->plan_complex_to_real_float);
		}
		if (context->fft_applied_complex_kernel != nullptr) {
			FreeFFTArray(context->fft_applied_complex_kernel);
		}
		if (context->work_real_array_kernel != nullptr) {
			LIBSAKURA_PREFIX::Memory::Free(context->work_real_array_kernel);
		}
		if (context->work_real_array != nullptr) {
			LIBSAKURA_PREFIX::Memory::Free(context->work_real_array);
		}
		LIBSAKURA_PREFIX::Memory::Free(context);
	}
}

} /* anonymous namespace */

namespace LIBSAKURA_PREFIX {
void ADDSUFFIX(Convolution, ARCH_SUFFIX)::CreateConvolve1DContext(
		size_t num_data, LIBSAKURA_SYMBOL(Convolve1DKernelType) kernel_type,
		size_t kernel_width, bool use_fft,
		LIBSAKURA_SYMBOL(Convolve1DContext) **context) const {
	::CreateConvolve1DContext(num_data, kernel_type, kernel_width, use_fft,
			context);
}

void ADDSUFFIX(Convolution, ARCH_SUFFIX)::Convolve1D(
LIBSAKURA_SYMBOL(Convolve1DContext) const *context, size_t num_data,
		float const input_data[/*num_data*/],
		bool const mask[/*num_data*/], float output_data[/*num_data*/]) const {
	::Convolve1D(context, num_data, input_data, mask, output_data);

}

void ADDSUFFIX(Convolution, ARCH_SUFFIX)::DestroyConvolve1DContext(
LIBSAKURA_SYMBOL(Convolve1DContext) *context) const {
	::DestroyConvolve1DContext(context);
}

}
