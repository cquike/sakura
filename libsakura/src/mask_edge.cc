/*
 * @SAKURA_LICENSE_HEADER_START@
 * @SAKURA_LICENSE_HEADER_END@
 */
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <memory>

#include <libsakura/config.h>
#include <libsakura/localdef.h>
#include <libsakura/logger.h>
#include <libsakura/sakura.h>
#include <libsakura/memory_manager.h>

namespace {
// a logger for this module
auto logger = LIBSAKURA_PREFIX::Logger::GetLogger("mask_edge");

/**
 * @brief Convert location of data points to the one in pixel coordinate.
 *
 * @param[in] pixel_scale scaling factor for pixel size as a fraction of median
 * separation between two neighboring data points. 0.5 is recommended.
 * @param[in] num_data number of data points
 * @param[in] x x-axis coordinate value of data points. Its length must be @a num_data.
 * must-be-aligned
 * @param[in] y y-axis coordinate value of data points. Its length must be @a num_data.
 * must-be-aligned
 * @param[out] pixel_x x-axis coordinate value of data points in pixel coordinate.
 * Its length must be @a num_data. must-be-aligned.
 * @param[out] pixel_y y-axis coordinate value of data points in pixel coordinate.
 * Its length must be @a num_data. must-be-aligned.
 * @param[out] center_x x-axis coordinate value of central (reference) point for
 * conversion to pixel coordinate.
 * @param[out] center_y y-axis coordinate value of central (reference) point for
 * conversion to pixel coordinate.
 * @param[out] num_horizontal number of pixels along x-axis.
 * @param[out] num_vertical number of pixels along y-axis.
 * @param[out] pixel_width resulting width of the pixel. Pixels are always square.
 */
template<typename DataType>
inline LIBSAKURA_SYMBOL(Status) ConvertToPixel(DataType pixel_scale,
		size_t num_data, DataType const x[], DataType const y[],
		DataType pixel_x[], DataType pixel_y[], DataType *center_x,
		DataType *center_y, size_t *num_horizontal, size_t *num_vertical,
		DataType *pixel_width) {
	// To derive median separation between two neighboring data points
	// use pixel_x and pixel_y as a working storage
	STATIC_ASSERT(sizeof(DataType) >= sizeof(bool));
	bool *mask = reinterpret_cast<bool *>(pixel_y);
	for (size_t i = 0; i < num_data - 1; ++i) {
		DataType separation_x = x[i + 1] - x[i];
		DataType separation_y = y[i + 1] - y[i];
		pixel_x[i] = separation_x * separation_x + separation_y * separation_y;
		mask[i] = true;
	}
	// sort data to evaluate median
	std::qsort(pixel_x, num_data - 1, sizeof(DataType),
			[](const void *a, const void *b) {
				DataType aa = *static_cast<DataType const *>(a);
				DataType bb = *static_cast<DataType const *>(b);
				if (aa < bb) {
					return -1;
				}
				else if (aa > bb) {
					return 1;
				}
				else {
					return 0;
				}
			});
	DataType median_separation = std::sqrt(pixel_x[(num_data - 1) / 2]);

	// pixel width
	*pixel_width = median_separation * pixel_scale;

	// minimul and maximum position
	DataType blc_x = x[0];
	DataType blc_y = y[0];
	DataType trc_x = x[0];
	DataType trc_y = y[0];
	for (size_t i = 0; i < num_data - 1; ++i) {
		DataType local_x = x[i];
		DataType local_y = y[i];
		blc_x = std::min(blc_x, local_x);
		trc_x = std::max(trc_x, local_x);
		blc_y = std::min(blc_y, local_y);
		trc_y = std::max(trc_y, local_y);
	}

	// number of pixels
	constexpr DataType kMargin = DataType(1.1);
	DataType const wx = (trc_x - blc_x) * kMargin;
	DataType const wy = (trc_y - blc_y) * kMargin;
	*num_horizontal = static_cast<size_t>(std::ceil(wx / *pixel_width));
	*num_vertical = static_cast<size_t>(std::ceil(wy / *pixel_width));

	// center coordinate
	constexpr DataType kTwo = DataType(2);
	*center_x = (blc_x + trc_x) / kTwo;
	*center_y = (blc_y + trc_y) / kTwo;

	// compute pixel_x and pixel_y
	DataType pixel_center_x = static_cast<DataType>(*num_horizontal - 1) / kTwo;
	DataType pixel_center_y = static_cast<DataType>(*num_vertical - 1) / kTwo;
	for (size_t i = 0; i < num_data; ++i) {
		pixel_x[i] = pixel_center_x + (x[i] - *center_x) / *pixel_width;
		pixel_y[i] = pixel_center_y + (y[i] - *center_y) / *pixel_width;
	}

	return LIBSAKURA_SYMBOL(Status_kOK);
}

/**
 * @brief Count up number of data in each pixel
 *
 * @param[in] num_data number of data points
 * @param[in] pixel_x x-axis coordinate value of data points in pixel coordinate.
 * Its length must be @a num_data. must-be-aligned.
 * @param[in] pixel_y y-axis coordinate value of data points in pixel coordinate.
 * Its length must be @a num_data. must-be-aligned.
 * @param[in] num_horizontal number of pixels along x-axis.
 * @param[in] num_vertical number of pixels along y-axis.
 * @param[out] counts data counts in each pixel. This is a flattened one-dimensional
 * array of two-dimensional array with shape of [@a num_vertical][@a num_horizontal].
 * must-be-aligned
 */
template<typename DataType>
inline LIBSAKURA_SYMBOL(Status) CountUp(size_t num_data,
		DataType const pixel_x[], DataType const pixel_y[],
		size_t num_horizontal, size_t num_vertical, size_t counts[]) {

	// initialization
	for (size_t i = 0; i < num_horizontal * num_vertical; ++i) {
		counts[i] = 0;
	}

	// data counts for each pixel
	for (size_t i = 0; i < num_data; ++i) {
		size_t ix = static_cast<size_t>(std::round(pixel_x[i]));
		size_t iy = static_cast<size_t>(std::round(pixel_y[i]));
		assert(ix < num_horizontal);
		assert(iy < num_vertical);
		size_t index = ix * num_vertical + iy;
		assert(index < num_horizontal * num_vertical);
		counts[index] += 1;
	}

	return LIBSAKURA_SYMBOL(Status_kOK);
}

/**
 * @brief Binarize data counts
 *
 * @param[in] num_horizontal number of pixels along x-axis.
 * @param[in] num_vertical number of pixels along y-axis.
 * @param[in,out] counts data counts in each pixel. This is a flattened one-dimensional
 * array of two-dimensional array with shape of [@a num_vertical][@a num_horizontal].
 * On output, counts is binarized so that non-zero vaule is converted to 1.
 * must-be-aligned
 */
inline LIBSAKURA_SYMBOL(Status) Binarize(size_t num_horizontal,
		size_t num_vertical, size_t counts[]) {
	for (size_t i = 0; i < num_horizontal * num_vertical; ++i) {
		counts[i] = (counts[i] > 0) ? 1 : 0;
	}

	return LIBSAKURA_SYMBOL(Status_kOK);
}

template<typename DataType>
inline size_t SearchForward(size_t start, size_t increment, size_t n,
		DataType const mask[]) {
	for (size_t i = 0; i < n; ++i) {
		size_t index = start + i * increment;
		if (mask[index] > 0) {
			return index;
		}
	}
	return start + (n - 1) * increment;
}

template<typename DataType>
inline size_t SearchBackward(size_t start, size_t increment, size_t n,
		DataType const mask[]) {
	for (ssize_t i = 0; i < n; ++i) {
		assert(i * increment <= start);
		size_t index = start - i * increment;
		if (mask[index] > 0) {
			return index;
		}
	}
	return 0;
}

/**
 * @brief Fill closed or approximately closed region
 *
 * Fill (approximately) closed region. To overcome incomplete closure, it iteratively
 * fill the region until number of filled pixels converges.
 *
 * @param[in] num_horizontal number of pixels along x-axis.
 * @param[in] num_vertical number of pixels along y-axis.
 * @param[in,out] pixel_mask binarized data counts. On input, pixels that contains any number
 * of data points have the value 1. Otherwise, the value is 0. On output, iterative fill process
 * is applied. must-be-aligned
 */
inline LIBSAKURA_SYMBOL(Status) Fill(size_t num_max_iteration,
		size_t num_horizontal, size_t num_vertical, size_t pixel_mask[]) {

	size_t num_masked = 1;
	size_t num_iteration = 0;

	do {
		num_masked = 0;
		for (size_t i = 0; i < num_horizontal; ++i) {
			size_t search_start = num_vertical * i;
			size_t search_end = search_start + num_vertical;
			size_t increment = 1;
			size_t start = SearchForward(search_start, increment, num_vertical,
					pixel_mask);
			search_start += num_vertical - 1;
			search_end = start;
			size_t end = SearchBackward(search_start, increment, num_vertical,
					pixel_mask);
			for (size_t j = start; j <= end; ++j) {
				if (pixel_mask[j] == 0) {
					pixel_mask[j] = 1;
					++num_masked;
				}
			}
		}
		for (size_t i = 0; i < num_vertical; ++i) {
			size_t search_start = i;
			size_t increment = num_vertical;
			size_t search_end = search_start + increment * num_horizontal;
			size_t start = SearchForward(search_start, increment,
					num_horizontal, pixel_mask);
			search_start += (num_horizontal - 1) * increment;
			search_end = start;
			size_t end = SearchBackward(search_start, increment, num_horizontal,
					pixel_mask);
			for (size_t j = start; j <= end; j += increment) {
				if (pixel_mask[j] == 0) {
					pixel_mask[j] = 1;
					++num_masked;
				}
			}
		}
		++num_iteration;
	} while (0 < num_masked && num_iteration < num_max_iteration);

	return LIBSAKURA_SYMBOL(Status_kOK);
}

inline void DetectEdge(size_t num_horizontal, size_t num_vertical,
		size_t const pixel_mask[], size_t edge[]) {
	for (size_t i = 0; i < num_vertical; ++i) {
		edge[i] = pixel_mask[i];
	}
	size_t offset = num_vertical * (num_horizontal - 1);
	for (size_t i = 0; i < num_vertical; ++i) {
		edge[offset + i] = pixel_mask[offset + i];
	}
	for (size_t i = 1; i < num_horizontal - 1; ++i) {
		edge[i * num_vertical] = pixel_mask[i * num_vertical];
		edge[(i + 1) * num_vertical - 1] =
				pixel_mask[(i + 1) * num_vertical - 1];
	}
	for (size_t i = 1; i < num_horizontal - 1; ++i) {
		for (size_t j = 1; j < num_vertical - 1; ++j) {
			size_t index = num_vertical * i + j;
			if (pixel_mask[index] == 1) {
				size_t index_below = index - num_vertical;
				size_t index_above = index + num_vertical;
				size_t surroundings = pixel_mask[index - 1]
						* pixel_mask[index + 1] * pixel_mask[index_below - 1]
						* pixel_mask[index_below] * pixel_mask[index_below + 1]
						* pixel_mask[index_above - 1] * pixel_mask[index_above]
						* pixel_mask[index_above - 1];
				edge[index] = (surroundings == 0) ? 1 : 0;
			}
		}
	}
}

inline void TrimEdge(size_t num_horizontal, size_t num_vertical,
		size_t const edge[], size_t pixel_mask[]) {
	for (size_t i = 0; i < num_horizontal * num_vertical; ++i) {
		if (edge[i] > 0) {
			pixel_mask[i] = 0;
		}
	}
}

/**
 * @brief Find data near edge using reference pixel image
 *
 * @param[in] fraction
 * @param[in] num_horizontal
 * @param[in] num_vertical
 * @param[in] pixel_mask pixel mask information. The value is 1 if any number of data
 * points are included in the pixel, or 0 otherwise. It will be destroyed in the function.
 * must-be-aligned
 * @param[in] num_data
 * @param[in] pixel_x
 * @param[in] pixel_y
 * @param[out] mask
 */
template<typename DataType>
inline LIBSAKURA_SYMBOL(Status) DetectDataNearEdge(float fraction,
		size_t num_horizontal, size_t num_vertical, size_t pixel_mask[],
		size_t num_data, DataType const pixel_x[], DataType const pixel_y[],
		bool mask[], size_t *num_masked) {

	assert(fraction < 1.0f);
	size_t threshold = static_cast<size_t>(static_cast<float>(num_data)
			* fraction);
	size_t num_masked_local = 0;

	// working array to store edge information
	size_t *edge = nullptr;
	size_t num_pixel = num_horizontal * num_vertical;
	std::unique_ptr<void, LIBSAKURA_PREFIX::Memory> storage(
			LIBSAKURA_PREFIX::Memory::AlignedAllocateOrException(
					num_pixel * sizeof(size_t), &edge));

	// iteration loop for detection of data points and edge trimming
	do {
		// detection
		DetectEdge(num_horizontal, num_vertical, pixel_mask, edge);

		// marking
		for (size_t i = 0; i < num_data; ++i) {
			size_t ix = static_cast<size_t>(std::round(pixel_x[i]));
			size_t iy = static_cast<size_t>(std::round(pixel_y[i]));
			assert(ix < num_horizontal);
			assert(iy < num_vertical);
			size_t index = ix * num_vertical + iy;
			assert(index < num_pixel);
			if (mask[i] == false && edge[index] == 1) {
				mask[i] = true;
				++num_masked_local;
			}
		}

		// trimming
		TrimEdge(num_horizontal, num_vertical, edge, pixel_mask);
	} while (num_masked_local <= threshold);

	*num_masked = num_masked_local;

	return LIBSAKURA_SYMBOL(Status_kOK);
}

/**
 * @brief Mask isolated unmasked points
 *
 * @param[in] num_mask number of mask
 * @param[in,out] mask boolean mask
 */
inline LIBSAKURA_SYMBOL(Status) ImproveDetection(size_t num_mask,
		bool mask[]) {
	constexpr size_t kIsolationThreshold = 3;
	size_t isolation_count = 0;
	for (size_t i = 0; i < num_mask; ++i) {
		if (mask[i]) {
			if (0 < isolation_count && isolation_count < kIsolationThreshold) {
				size_t start = i - isolation_count;
				size_t end = i;
				for (size_t j = start; j < end; ++j) {
					mask[j] = true;
				}
			}
			isolation_count = 0;
		} else {
			++isolation_count;
		}
	}

	return LIBSAKURA_SYMBOL(Status_kOK);
}

/**
 * Edge detection algorithm consists of the following steps:
 *
 *    ConvertToPixel
 *        set up pixel coordinate based on median separation
 *        between two neighboring points and user-specified
 *        scaling factor, pixel_size, and convert data list
 *        to pixel coordinate.
 *    CountUp
 *        count up number of points contained in each pixel.
 *    Binarize
 *        binarization of countup result such that pixel value
 *        is 1 if there is a data point in this pixel, otherwise
 *        the value is 0.
 *    Fill
 *        set value 1 to pixels that is located between two pixels
 *        with value of 1.
 *    DetectDatanearEdge
 *        find map edge and mask points that are located in edge pixels.
 *        edge finding and masking process is repeated until the fraction
 *        of number of masked points exceeds user-specified fraction.
 *    ImproveDetection
 *        isolated unmasked points are masked.
 */
template<typename DataType>
inline LIBSAKURA_SYMBOL(Status) MaskDataNearEdge(float fraction,
		DataType pixel_scale, size_t num_data, DataType const x[],
		DataType const y[], bool mask[]) {

	// ConvertToPixel
	// allocate storage for two aligned arrays by one allocation call
	STATIC_ASSERT(sizeof(DataType) < LIBSAKURA_ALIGNMENT);
	constexpr size_t kStride = LIBSAKURA_ALIGNMENT / sizeof(DataType);
	size_t const reminder = num_data % kStride;
	size_t const margin = (reminder == 0) ? 0 : (kStride - reminder);
	size_t const size_of_storage = (2 * num_data + margin) * sizeof(DataType);
	DataType *pixel_x = nullptr;
	std::unique_ptr<void, LIBSAKURA_PREFIX::Memory> storage(
			LIBSAKURA_PREFIX::Memory::AlignedAllocateOrException(
					size_of_storage, &pixel_x));
	DataType *pixel_y = pixel_x + (num_data + margin);
	DataType center_x = 0.0;
	DataType center_y = 0.0;
	DataType pixel_width = 0.0;
	size_t num_horizontal = 0;
	size_t num_vertical = 0;

	LIBSAKURA_SYMBOL(Status) status = ConvertToPixel(pixel_scale, num_data, x,
			y, pixel_x, pixel_y, &center_x, &center_y, &num_horizontal,
			&num_vertical, &pixel_width);

	// CountUp
	size_t *counts = nullptr;
	size_t num_pixel = num_horizontal * num_vertical;
	std::unique_ptr<void, LIBSAKURA_PREFIX::Memory> storage2(
			LIBSAKURA_PREFIX::Memory::AlignedAllocateOrException(
					num_pixel * sizeof(size_t), &counts));
	status = CountUp(num_data, pixel_x, pixel_y, num_horizontal, num_vertical,
			counts);

	if (status != LIBSAKURA_SYMBOL(Status_kOK)) {
		return status;
	}

	// Binarize
	status = Binarize(num_horizontal, num_vertical, counts);

	if (status != LIBSAKURA_SYMBOL(Status_kOK)) {
		return status;
	}

	// Fill
	constexpr size_t kMaxIteration = 10;
	status = Fill(kMaxIteration, num_horizontal, num_vertical, counts);

	if (status != LIBSAKURA_SYMBOL(Status_kOK)) {
		return status;
	}

	// Detect
	size_t num_masked = 0;
	status = DetectDataNearEdge(fraction, num_horizontal, num_vertical, counts,
			num_data, pixel_x, pixel_y, mask, &num_masked);

	if (status != LIBSAKURA_SYMBOL(Status_kOK)) {
		return status;
	}

	// ImproveDetection
	status = ImproveDetection(num_data, mask);

	return status;
}

} // anonymous namespace

#define CHECK_ARGS(x) do { \
	if (!(x)) { \
		return LIBSAKURA_SYMBOL(Status_kInvalidArgument); \
	} \
} while (false)

extern "C" LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(MaskDataNearEdge)(
		float fraction, double pixel_scale, size_t num_data, double const x[],
		double const y[], bool mask[]) {
	CHECK_ARGS(0.0f < pixel_scale);
	CHECK_ARGS(fraction < 1.0f);
	CHECK_ARGS(LIBSAKURA_SYMBOL(IsAligned)(x));
	CHECK_ARGS(LIBSAKURA_SYMBOL(IsAligned)(y));
	CHECK_ARGS(LIBSAKURA_SYMBOL(IsAligned)(mask));

	try {
		return MaskDataNearEdge(fraction, pixel_scale, num_data, x, y, mask);
	} catch (const std::bad_alloc &e) {
		// failed to allocate memory
		LOG4CXX_ERROR(logger, "Memory allocation failed.");
		return LIBSAKURA_SYMBOL(Status_kNoMemory);
	} catch (...) {
		// any exception is thrown during interpolation
		assert(false);
		LOG4CXX_ERROR(logger, "Aborted due to unknown error");
		return LIBSAKURA_SYMBOL(Status_kUnknownError);
	}

}