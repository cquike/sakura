#include <cassert>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "libsakura/sakura.h"
#include "libsakura/optimized_implementation_factory_impl.h"
#include "libsakura/localdef.h"

namespace {

void OperateLogicalAnd(size_t num_in, bool const *in1,
		bool const *in2, bool *out) {
	STATIC_ASSERT(sizeof(*in1) == sizeof(uint8_t));
	STATIC_ASSERT(sizeof(*in2) == sizeof(uint8_t));
	STATIC_ASSERT(true == 1);
	STATIC_ASSERT(false == 0);

	auto src1 = reinterpret_cast<uint8_t const *>(in1);
	auto src2 = reinterpret_cast<uint8_t const *>(in2);
	for (size_t i = 0; i < num_in; ++i) {
		out[i] = static_cast<bool>(src1[i] & src2[i]);
	}

	/* old logic
	for (size_t i = 0; i < num_in; ++i) {
		out[i] = in1[i] && in2[i];
	}
	*/
}

} /* anonymous namespace */

namespace LIBSAKURA_PREFIX {
void ADDSUFFIX(LogicalOperation, ARCH_SUFFIX)::OperateLogicalAnd(
		size_t num_in, bool const in1[/*num_in*/],
		bool const in2[/*num_in*/], bool out[/*num_in*/]) const {
	::OperateLogicalAnd(num_in, in1, in2, out);
}

}
