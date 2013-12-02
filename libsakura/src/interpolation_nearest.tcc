#ifndef _LIBSAKURA_INTERPOLATION_NEAREST_TCC_
#define _LIBSAKURA_INTERPOLATION_NEAREST_TCC_

namespace {

// TODO: documentation needs to be improved.
/**
 * Nearest interpolation engine classes
 *
 * nearest condition
 * - interpolated_position[i] == middle_point
 *     ---> nearest is y_left (left side)
 * - interpolated_position[i] < middle_point and ascending order
 *     ---> nearest is y_left (left side)
 * - interpolated_position[i] > middle_point and ascending order
 *     ---> nearest is y_right (right side)
 * - interpolated_position[i] < middle_point and descending order
 *     ---> nearest is y_right (right side)
 * - interpolated_position[i] > middle_point and descending order
 *     ---> nearest is y_left (left side)
 */

template<class XDataType, class YDataType>
class NearestXInterpolatorImpl {
public:
	void PrepareForInterpolation(uint8_t polynomial_order, size_t num_base,
			size_t num_array, XDataType const base_position[],
			YDataType const base_data[]) {
	}
	void Interpolate1D(size_t num_base, XDataType const base_position[],
			size_t num_array, YDataType const base_data[],
			size_t num_interpolated, XDataType const interpolated_position[],
			YDataType interpolated_data[], size_t num_location,
			size_t const location[]) {
		for (size_t k = 0; k < num_location - 1; ++k) {
			XDataType middle_point = 0.5
					* (base_position[k + 1] + base_position[k]);
			for (size_t j = 0; j < num_array; ++j) {
				YDataType left_value = base_data[j * num_base + k];
				YDataType right_value = base_data[j * num_base + k + 1];
				YDataType *work = &interpolated_data[j * num_interpolated];
				for (size_t i = location[k]; i < location[k + 1]; ++i) {
					if ((interpolated_position[i] != middle_point)
							& ((interpolated_position[i] > middle_point))) {
						work[i] = right_value;
					} else {
						work[i] = left_value;
					}
				}
			}
		}
	}
};

template<class XDataType, class YDataType>
class NearestYInterpolatorImpl {
public:
	void PrepareForInterpolation(uint8_t polynomial_order, size_t num_base,
			size_t num_array, XDataType const base_position[],
			YDataType const base_data[]) {
	}
	void Interpolate1D(size_t num_base, XDataType const base_position[],
			size_t num_array, YDataType const base_data[],
			size_t num_interpolated, XDataType const interpolated_position[],
			YDataType interpolated_data[], size_t num_location,
			size_t const location[]) {
		for (size_t k = 0; k < num_location - 1; ++k) {
			XDataType middle_point = 0.5
					* (base_position[k + 1] + base_position[k]);
			for (size_t i = location[k]; i < location[k + 1]; ++i) {
				size_t offset_index = 0;
				if ((interpolated_position[i] != middle_point)
						& ((interpolated_position[i] > middle_point))) {
					offset_index = 1;
				}
				YDataType *work = &interpolated_data[num_array * i];
				YDataType const *nearest = &base_data[num_array
						* (k + offset_index)];
				for (size_t j = 0; j < num_array; ++j) {
					work[j] = nearest[j];
				}
			}
		}
	}
};

}

#endif /* _LIBSAKURA_INTERPOLATION_NEAREST_TCC_ */