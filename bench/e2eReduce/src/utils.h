#ifndef _SAKURA_E2E_UTILS_H_
#define _SAKURA_E2E_UTILS_H_

#include <memory>

#include <casacore/casa/Exceptions/Error.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Vector.h>

#include <casacore/tables/Tables/Table.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>
#include <casacore/tables/Tables/TableParse.h>

#include <libsakura/sakura.h>

namespace {
/**
 * Aligned array utility
 */
struct Deleter {
	inline void operator()(void *ptr) const {
		free(ptr);
	}
};

class AlignedArrayGenerator {
public:
	AlignedArrayGenerator() :
			alignment_(sakura_GetAlignment()), index_(0), pointer_holder_(128) {
	}
	template<class T> inline T *GetAlignedArray(size_t num_elements) {
		size_t num_arena = (num_elements + alignment_ - 1) * sizeof(T);
		if (index_ >= pointer_holder_.size()) {
			pointer_holder_.resize(pointer_holder_.size() + 128);
		}
		void *ptr = malloc(num_arena);
		pointer_holder_[index_++].reset(ptr);
		return reinterpret_cast<T *>(sakura_AlignAny(num_arena, ptr,
				num_elements * sizeof(T)));
	}
private:
	size_t alignment_;
	size_t index_;
	std::vector<std::unique_ptr<void, Deleter> > pointer_holder_;
};

/**
 * CASA related utility functions
 */
std::string GetTaqlString(std::string table_name, unsigned int ifno) {
	std::ostringstream oss;
	oss << "SELECT FROM \"" << table_name << "\" WHERE IFNO == " << ifno
			<< " ORDER BY TIME";
	return oss.str();
}

casa::Table GetSelectedTable(std::string table_name, unsigned int ifno) {
	casa::String taql(GetTaqlString(table_name, ifno));
	return tableCommand(taql);
}

template<class T>
void GetArrayCell(T *array, size_t row_index,
		casa::ROArrayColumn<T> const column, casa::IPosition const cell_shape) {
	casa::Array < T > casa_array(cell_shape, array, casa::SHARE);
	column.get(row_index, casa_array);
}

template<class T>
void GetArrayColumn(T *array, casa::ROArrayColumn<T> const column,
		casa::IPosition const column_shape) {
	casa::Array < T > casa_array(column_shape, array, casa::SHARE);
	column.getColumn(casa_array);
}

template<class T>
void GetScalarColumn(T *array, casa::ROScalarColumn<T> const column,
		size_t num_rows) {
	casa::Vector < T
			> casa_array(casa::IPosition(1, num_rows), array, casa::SHARE);
	column.getColumn(casa_array);
}

void GetFromCalTable(std::string const table_name, unsigned int ifno,
		std::string const data_name, AlignedArrayGenerator *array_generator,
		float **data, double **timestamp, size_t *num_data, size_t *num_row) {
	casa::Table table = GetSelectedTable(table_name, ifno);
	*num_row = table.nrow();
	if (*num_row == 0) {
		throw casa::AipsError("Selected sky table is empty.");
	}
	casa::ROArrayColumn<float> caldata_column(table, data_name);
	casa::ROScalarColumn<double> time_column(table, "TIME");
	*num_data = caldata_column(0).nelements();
	*data = array_generator->GetAlignedArray<float>((*num_data) * (*num_row));
	*timestamp = array_generator->GetAlignedArray<double>(*num_row);
	GetArrayColumn(*data, caldata_column,
			casa::IPosition(2, *num_data, *num_row));
	GetScalarColumn(*timestamp, time_column, *num_row);
}
}

#endif /* _SAKURA_E2E_UTILS_H_ */