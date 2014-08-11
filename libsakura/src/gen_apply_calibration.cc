/*
 * @SAKURA_LICENSE_HEADER_START@
 * @SAKURA_LICENSE_HEADER_END@
 */
#include <sstream>
#include <cassert>
#include <cstdlib>
#include <climits>
#include <memory>

#include "libsakura/sakura.h"
#include "libsakura/localdef.h"
#include "libsakura/optimized_implementation_factory_impl.h"
#include "libsakura/logger.h"

namespace {
// a logger for this module
auto logger = LIBSAKURA_PREFIX::Logger::GetLogger("apply_calibration");

}

extern "C" LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(ApplyPositionSwitchCalibration)(
		size_t num_scaling_factor,
		float const scaling_factor[/*num_scaling_factor*/], size_t num_data,
		float const target[/*num_data*/], float const reference[/*num_data*/],
		float result[/*num_data*/]) {
	if (num_data == 0) {
		// Nothing to do
		return LIBSAKURA_SYMBOL(Status_kOK);
	}

	if (num_scaling_factor == 0) {
		// scaling factor must be given
		LOG4CXX_ERROR(logger, "Empty scaling factor (num_scaling_factor == 0)");
		return LIBSAKURA_SYMBOL(Status_kInvalidArgument);
	}

	if (num_scaling_factor > 1 && num_scaling_factor < num_data) {
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

	// get object optimized to run-time environment
	auto object =
			::LIBSAKURA_PREFIX::OptimizedImplementationFactory::GetFactory()->GetApplyCalibrationImpl();

	try {
		object->ApplyPositionSwitchCalibration(num_scaling_factor,
				scaling_factor, num_data, target, reference, result);
		return LIBSAKURA_SYMBOL(Status_kOK);
	} catch (...) {
		// any exception is thrown during interpolation
		assert(false);
		LOG4CXX_ERROR(logger, "Aborted due to unknown error");
		return LIBSAKURA_SYMBOL(Status_kUnknownError);
	}
}
