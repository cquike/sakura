#include <iostream>
#include <string>
#include <sys/time.h>

#include <libsakura/sakura.h>
#include <libsakura/localdef.h>
#include "loginit.h"
#include "aligned_memory.h"
#include "gtest/gtest.h"

/* the number of elements in input/output array to test */
#define NUM_IN 8 // length of base data. DO NOT MODIFY!
#define NUM_IN_LONG (1 << 18) //2**18
#define UNALIGN_OFFSET 1 // should be != ALIGNMENT
using namespace std;

/*
 * A super class to test various bit operation of an value and array
 * INPUTS:
 * - bit_mask = 0...010
 * - in = [ 0...000, 0...001, 0...010, 0...011, 0...000, 0...001, 0...010, 0...011 ]
 * - edit_mask = [F, F, F, F, T, T, T, T]
 */
template<typename DataType>
class BitOperation: public ::testing::Test {
protected:

	BitOperation() :
			verbose(false) {
	}

	typedef LIBSAKURA_SYMBOL(Status) (*function_ptr_t)(DataType, size_t,
			DataType const*, bool const*, DataType*);

	// Types of operation
	enum operation_type {
		And = 0,
		NonImplication,
		ConverseNonImplication,
		Nor,
		Implication,
		Nand,
		Or,
		ConverseImplication,
		Xor,
		Xnor,
		//Not, /// Not has different interface
		NUM_OPERATION
	};

	struct test_kit {
		string name;               // name of operation
		function_ptr_t function;   // pointer to function
		DataType answer[NUM_IN];   // answer array
		bool invert_mask;          // is operation needs inverting mask
	};

	void SetTestKit(operation_type const operation, test_kit *kit) {
		// Answers for each operation
		DataType const base_pattern1 = (~static_cast<DataType>(0)); // 11...111
		DataType const base_pattern00 = (~static_cast<DataType>(0) << 2); // 11...100

		kit->function = nullptr;

		switch (operation) {
		case And: {
			DataType answer[] = { 0, 1, 2, 3, 0, 0, 2, 2 };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "AND";
			kit->invert_mask = false;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case NonImplication: {
			DataType answer[] = { 0, 1, 2, 3, 0, 1, 0, 1 };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "Material Nonimplication";
			kit->invert_mask = true;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case ConverseNonImplication: {
			DataType answer[] = { 0, 1, 2, 3, 2, 2, 0, 0 };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "Converse Nonimplication";
			kit->invert_mask = false;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case Nor: {
			DataType answer[] = { 0, 1, 2, 3,
					static_cast<DataType>(base_pattern00 + 1), base_pattern00,
					static_cast<DataType>(base_pattern00 + 1), base_pattern00 };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "NOR";
			kit->invert_mask = true;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case Implication: {
			DataType answer[] = { 0, 1, 2, 3, base_pattern1,
					static_cast<DataType>(base_pattern1 << 1), base_pattern1,
					static_cast<DataType>(base_pattern1 << 1) };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "Material Implication";
			kit->invert_mask = false;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case Nand: {
			DataType answer[] = { 0, 1, 2, 3, base_pattern1, base_pattern1,
					static_cast<DataType>(base_pattern1 - 2),
					static_cast<DataType>(base_pattern1 - 2) };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "NAND";
			kit->invert_mask = true;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case Or: {
			DataType answer[] = { 0, 1, 2, 3, 2, 3, 2, 3 };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "OR";
			kit->invert_mask = false;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case ConverseImplication: {
			DataType answer[] = { 0, 1, 2, 3,
					static_cast<DataType>(base_pattern1 - 2),
					static_cast<DataType>(base_pattern1 - 2), base_pattern1,
					base_pattern1 };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "Converse Implication";
			kit->invert_mask = true;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case Xor: {
			DataType answer[] = { 0, 1, 2, 3, 2, 3, 0, 1 };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "XOR";
			kit->invert_mask = false;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
		case Xnor: {
			DataType answer[] = { 0, 1, 2, 3,
					static_cast<DataType>(base_pattern1 - 2),
					static_cast<DataType>(base_pattern1 << 2), base_pattern1,
					static_cast<DataType>(base_pattern1 << 1) };
			STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
			kit->name = "XNOR";
			kit->invert_mask = true;
			for (size_t i = 0; i < NUM_IN; ++i)
				kit->answer[i] = answer[i];
			break;
		}
//		case Not: {
//			DataType answer[] = { 0, 1, 2, 3,
//					static_cast<DataType>(base_pattern00 + 3),
//					static_cast<DataType>(base_pattern00 + 2),
//					static_cast<DataType>(base_pattern00 + 1), base_pattern00 };
//		STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);
//			kit->name = "XOR";
//			kit->invert_mask = false;
//			for (size_t i = 0; i < NUM_IN; ++i)
//				kit->answer[i] = answer[i];
//			break;
//		}
		default:
			FAIL()<< "Invalid operation type.";
		}
	}

	virtual void SetUp() {
		size_t const ntype(4);
		bit_mask_ = 2; /* bit pattern of 0...010 */
		for (size_t i = 0; i < NUM_IN; ++i) {
			data_[i] = i % ntype; /* repeat bit pattern of *00, *01, *10, *11,... */
			edit_mask_[i] = (i / ntype % 2 == 1); /*{F, F, F, F, T, T, T, T, (repeated)};*/
		}

		// Initialize sakura
		LIBSAKURA_SYMBOL(Status) status = LIBSAKURA_SYMBOL(Initialize)(nullptr,
				nullptr);
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	}

	virtual void TearDown() {
		// Clean-up sakura
		LIBSAKURA_SYMBOL(CleanUp)();
	}

	// Create arbitrary length of input data and edit mask by repeating in_[] and edit_mask_[]
	void GetInputDataInLength(size_t num_data, DataType *data, bool *mask) {
		size_t const num_in(NUM_IN);
		for (size_t i = 0; i < num_data; ++i) {
			data[i] = data_[i % num_in];
			mask[i] = edit_mask_[i % num_in];
		}
	}

	/* Converts an input value to a bit pattern.*/
//	char* BToS(DataType in_value) {
	string BToS(DataType value) {
		char buff[bit_size + 1];
		buff[bit_size] = '\0';
		for (size_t i = 0; i < bit_size; ++i) {
			if ((value >> i) % 2 == 0)
			buff[bit_size - 1 - i] = '0';
			else
			buff[bit_size - 1 - i] = '1';
		}
		return string(buff);
	}

	/* Converts an bit pattern (char) to a value of DataType.*/
	DataType SToB(char* bit_pattern) {
		DataType result(0);
		size_t i = 0;
		while (bit_pattern[i] != '\0') {
			//result = result * 2 + ((uint8_t) in_string[i]);
			result <<= 1;
			if (bit_pattern[i] == '1')
			result += 1;
			++i;
		}
		return result;
	}

	void PrintInputs() {
		cout << "bit_mask = " << BToS(bit_mask_);
		cout << endl;
		PrintArray("data", NUM_IN, data_);
	}

	void PrintArray(char const *name, size_t num_data, DataType *data_array) {
		cout << name << " = [";
		for (size_t i = 0; i < num_data - 1; ++i)
		cout << BToS(data_array[i]) << ", ";
		cout << BToS(data_array[num_data - 1]);
		cout << " ]" << endl;
	}

	void PrintArray(char const *name, size_t num_data, bool const *data_array) {
		cout << name << " = [ ";
		for (size_t i = 0; i < num_data - 1; ++i)
		cout << (data_array[i] ? "T" : "F") << ", ";
		cout << (data_array[num_data - 1] ? "T" : "F");
		cout << " ]" << endl;
	}

	/*
	 * Compare data with reference array, and assert values of corresponding
	 * elements are the exact match.
	 * If num_data > num_reference, elements of reference_array
	 * are repeated from the beginning as many times as necessary.
	 */
	void ExactCompare(size_t num_data, DataType const *data_array, size_t num_reference,
			DataType const *reference_array) {
		for (size_t i = 0; i < num_data; ++i) {
			ASSERT_EQ(reference_array[i % num_reference], data_array[i]);
		}
	}
	DataType data_[NUM_IN];

	bool edit_mask_[NUM_IN];

	bool verbose;
	DataType bit_mask_;
	static size_t const bit_size = sizeof(DataType) * 8;

	operation_type const num_operation = NUM_OPERATION;
	// The standard test list
	static size_t const num_standard_test = 5;
	operation_type test_list_standard[num_standard_test] = {And , ConverseNonImplication, Implication, Or, Xor};
};

/*
 * Tests various bit operations (except for NOT) of an uint32_t value and array
 * INPUTS:
 * - bit_mask = 0...010
 * - in = [ 0...000, 0...001, 0...010, 0...011, 0...000, 0...001, 0...010, 0...011, ...repeated... ]
 * - edit_mask = [F, F, F, F, T, T, T, T]
 *
 * RESULTS:
 * - And:
 *   [ 0...000, 0...001, 0...010, 0...011, 0...000, 0...000, 0...010, 0...010, ...repeated... ]
 * - Nonimplication:
 *   [ 0...000, 0...001, 0...010, 0...011, 0...000, 0...001, 0...000, 0...001, ...repeated... ]
 * - ConverseNonImplication:
 *   [ 0...000, 0...001, 0...010, 0...011, 0...010, 0...010, 0...000, 0...000, ...repeated... ]
 * - Nor:
 *   [ 0...000, 0...001, 0...010, 0...011, 1...101, 1...100, 1...101, 1...100, ...repeated... ]
 * - Implication:
 *   [ 0...000, 0...001, 0...010, 0...011, 1...111, 1...110, 1...111, 1...110, ...repeated... ]
 * - Nand:
 *   [ 0...000, 0...001, 0...010, 0...011, 1...111, 1...111, 1...101, 1...101, ...repeated... ]
 * - Or:
 *   [ 0...000, 0...001, 0...010, 0...011, 0...010, 0...011, 0...010, 0...011, ...repeated... ]
 * - ConverseImplication:
 *   [ 0...000, 0...001, 0...010, 0...011, 1...101, 1...101, 1...111, 1...111, ...repeated... ]
 * - Xor:
 *   [ 0...000, 0...001, 0...010, 0...011, 0...010, 0...011, 0...000, 0...001, ...repeated... ]
 * - Xnor:
 *   [ 0...000, 0...001, 0...010, 0...011, 1...101, 1...100, 1...111, 1...110, ...repeated... ]
 * - Not:
 *   [ 0...000, 0...001, 0...010, 0...011, 1...111, 1...110, 1...101, 1...100, ...repeated... ]
 *
 */
class BitOperation32: public BitOperation<uint32_t> {

protected:
	test_kit GetTestKit(operation_type const operation) {
		test_kit ret_kit;
		SetTestKit(operation, &ret_kit);
		switch (operation) {
		case And:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint32And);
			break;
		case NonImplication:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint32And);
			break;
		case ConverseNonImplication:
			ret_kit.function =
			LIBSAKURA_SYMBOL(OperateBitsUint32ConverseNonImplication);
			break;
		case Nor:
			ret_kit.function =
			LIBSAKURA_SYMBOL(OperateBitsUint32ConverseNonImplication);
			break;
		case Implication:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint32Implication);
			break;
		case Nand:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint32Implication);
			break;
		case Or:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint32Or);
			break;
		case ConverseImplication:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint32Or);
			break;
		case Xor:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint32Xor);
			break;
		case Xnor:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint32Xor);
			break;
		default:
			//this should have already failed at SetTestKit.
			ADD_FAILURE()<< "Invalid operation type.";
		}
		return ret_kit;
	}

};

/*
 * Tests various bit operations of an uint8_t value and array
 * INPUTS:
 * - bit_mask = 00000010
 * - in = [ 00000000, 00000001, 00000010, 00000011, 00000000, 00000001, 00000010, 00000011, ...repeated... ]
 * - edit_mask = [F, F, F, F, T, T, T, T]
 *
 * RESULTS:
 * - And:
 *   [ 00000000, 00000001, 00000010, 00000011, 00000000, 00000000, 00000010, 00000010, ...repeated... ]
 * - Nonimplication:
 *   [ 00000000, 00000001, 00000010, 00000011, 00000000, 00000001, 00000000, 00000001, ...repeated... ]
 * - ConverseNonImplication:
 *   [ 00000000, 00000001, 00000010, 00000011, 00000010, 00000010, 00000000, 00000000, ...repeated... ]
 * - Nor:
 *   [ 00000000, 00000001, 00000010, 00000011, 11111101, 11111100, 11111101, 11111100, ...repeated... ]
 * - Implication:
 *   [ 00000000, 00000001, 00000010, 00000011, 11111111, 11111110, 11111111, 11111110, ...repeated... ]
 * - Nand:
 *   [ 00000000, 00000001, 00000010, 00000011, 11111111, 11111111, 11111101, 11111101, ...repeated... ]
 * - Or:
 *   [ 00000000, 00000001, 00000010, 00000011, 00000010, 00000011, 00000010, 00000011, ...repeated... ]
 * - ConverseImplication:
 *   [ 00000000, 00000001, 00000010, 00000011, 11111101, 11111101, 11111111, 11111111, ...repeated... ]
 * - Xor:
 *   [ 00000000, 00000001, 00000010, 00000011, 00000010, 00000011, 00000000, 00000001, ...repeated... ]
 * - Xnor:
 *   [ 00000000, 00000001, 00000010, 00000011, 11111101, 11111100, 11111111, 11111110, ...repeated... ]
 * - Not:
 *   [ 00000000, 00000001, 00000010, 00000011, 11111111, 11111110, 11111101, 11111100, ...repeated... ]
 */
class BitOperation8: public BitOperation<uint8_t> {

protected:
	test_kit GetTestKit(operation_type const operation) {
		test_kit ret_kit;
		SetTestKit(operation, &ret_kit);
		switch (operation) {
		case And:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint8And);
			break;
		case NonImplication:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint8And);
			break;
		case ConverseNonImplication:
			ret_kit.function =
			LIBSAKURA_SYMBOL(OperateBitsUint8ConverseNonImplication);
			break;
		case Nor:
			ret_kit.function =
			LIBSAKURA_SYMBOL(OperateBitsUint8ConverseNonImplication);
			break;
		case Implication:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint8Implication);
			break;
		case Nand:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint8Implication);
			break;
		case Or:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint8Or);
			break;
		case ConverseImplication:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint8Or);
			break;
		case Xor:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint8Xor);
			break;
		case Xnor:
			ret_kit.function = LIBSAKURA_SYMBOL(OperateBitsUint8Xor);
			break;
		default:
			//this should have already failed at SetTestKit.
			ADD_FAILURE()<< "Invalid operation type.";
		}
		return ret_kit;
	}

};

/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test various bit operations (except for NOT) with an Uint8 array of length 8
 */

TEST_F(BitOperation8, MiscOp) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_data, data, edit_mask);

	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}
	// Loop over operation types (Full tests)
	for (size_t iop = 0; iop < num_operation; ++iop) {
		test_kit kit = GetTestKit((operation_type) iop);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, result);

		if (verbose)
			PrintArray("result", num_data, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_data, result, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Test bit operation NOT by sakura_OperateBitsUint8Not
 * RESULT:
 * out = [00000000, 00000001, 00000010, 00000011, 11111111, 11111110, 11111101, 11111100 ]
 */
TEST_F(BitOperation8, Not) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	uint8_t const base_pattern = (~static_cast<uint8_t>(0) << 2); // 11111100
	uint8_t answer[] = { 0, 1, 2, 3, static_cast<uint8_t>(base_pattern + 3),
			static_cast<uint8_t>(base_pattern + 2),
			static_cast<uint8_t>(base_pattern + 1), base_pattern };
	STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);

	GetInputDataInLength(num_data, data, edit_mask);
	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data,
			edit_mask, result);

	if (verbose)
		PrintArray("result", num_data, result);

// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	ExactCompare(num_data, result, ELEMENTSOF(answer), answer);
}

/*
 * Test various bit operations (except for NOT) with an Uint32 array of length 8
 */

TEST_F(BitOperation32, MiscOp) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_data, data, edit_mask);

	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}
	// Loop over operation types (Full tests)
	for (size_t iop = 0; iop < num_operation; ++iop) {
		test_kit kit = GetTestKit((operation_type) iop);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, result);

		if (verbose)
			PrintArray("result", num_data, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_data, result, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Test bit operation NOT by sakura_OperateBitsUint32Not
 * RESULT:
 * out = [0...000, 0...001, 0...010, 0...011, 1...111, 1...110, 1...101, 1...100 ]
 */
TEST_F(BitOperation32, Not) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	uint32_t const base_pattern = (~static_cast<uint32_t>(0) << 2); // 1...100
	uint32_t answer[] = { 0, 1, 2, 3, static_cast<uint32_t>(base_pattern + 3),
			static_cast<uint32_t>(base_pattern + 2),
			static_cast<uint32_t>(base_pattern + 1), base_pattern };
	STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);

	GetInputDataInLength(num_data, data, edit_mask);
	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data, edit_mask, result);

	if (verbose)
		PrintArray("result", num_data, result);

// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	ExactCompare(num_data, result, ELEMENTSOF(answer), answer);
}

/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test various in-place (&out == &in) bit operations (except for NOT) with an Uint8 array of length 10
 */
TEST_F(BitOperation8, MiscOpInPlace) {
	size_t const num_data(10);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];

	// Loop over operation types  (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		GetInputDataInLength(num_data, data, edit_mask);
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		if (verbose) {
			PrintArray("in (before)", num_data, data);
			PrintArray("mask", num_data, edit_mask);
		}
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, data);

		if (verbose)
			PrintArray("in (after)", num_data, data);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_data, data, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Test bit operation NOT by sakura_OperateBitsUint8Not in-place operation (&out == &in)
 * RESULT:
 * out = [00000000, 00000001, 00000010, 00000011, 11111111, 11111110, 11111101, 11111100 ]
 */
TEST_F(BitOperation8, NotInPlace) {
	size_t const num_data(12);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	uint8_t const base_pattern = (~static_cast<uint8_t>(0) << 2); // 11111100
	uint8_t answer[] = { 0, 1, 2, 3, static_cast<uint8_t>(base_pattern + 3),
			static_cast<uint8_t>(base_pattern + 2),
			static_cast<uint8_t>(base_pattern + 1), base_pattern };
	STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);

	GetInputDataInLength(num_data, data, edit_mask);
	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data,
			edit_mask, data);

	if (verbose)
		PrintArray("in (after)", num_data, data);

// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	ExactCompare(num_data, data, ELEMENTSOF(answer), answer);
}

/*
 * Test various in-place (&out == &in) bit operations (except for NOT) with an Uint32 array of length 10
 */
TEST_F(BitOperation32, MiscOpInPlace) {
	size_t const num_data(10);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];

	// Loop over operation types  (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		GetInputDataInLength(num_data, data, edit_mask);
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		if (verbose) {
			PrintArray("in (before)", num_data, data);
			PrintArray("mask", num_data, edit_mask);
		}
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, data);

		if (verbose)
			PrintArray("in (after)", num_data, data);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_data, data, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Test bit operation NOT by sakura_OperateBitsUint32Not in-place operation (&out == &in)
 * RESULT:
 * out = [0...000, 0...001, 0...010, 0...011, 1...111, 1...110, 1...101, 1...100 ]
 */
TEST_F(BitOperation32, NotInPlace) {
	size_t const num_data(12);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	uint32_t const base_pattern = (~static_cast<uint32_t>(0) << 2); // 1...100
	uint32_t answer[] = { 0, 1, 2, 3, static_cast<uint32_t>(base_pattern + 3),
			static_cast<uint32_t>(base_pattern + 2),
			static_cast<uint32_t>(base_pattern + 1), base_pattern };
	STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);

	GetInputDataInLength(num_data, data, edit_mask);
	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data, edit_mask, data);

	if (verbose)
		PrintArray("in (after)", num_data, data);

// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	ExactCompare(num_data, data, ELEMENTSOF(answer), answer);
}

/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test various bit operations (except for NOT) with an Uint8 array of length 11
 */

TEST_F(BitOperation8, MiscOpLengthEleven) {
	size_t const num_data(11);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_data, data, edit_mask);

	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}
	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, result);

		if (verbose)
			PrintArray("result", num_data, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_data, result, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Test bit operation NOT by sakura_OperateBitsUint8Not with an array of length 11
 * RESULT:
 * out = [00000000, 00000001, 00000010, 00000011, 11111111, 11111110, 11111101, 11111100,
 *        00000000, 00000001, 00000010 ]
 */
TEST_F(BitOperation8, NotLengthEleven) {
	size_t const num_data(11);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	uint8_t const base_pattern = (~static_cast<uint8_t>(0) << 2); // 11111100
	uint8_t answer[] = { 0, 1, 2, 3, static_cast<uint8_t>(base_pattern + 3),
			static_cast<uint8_t>(base_pattern + 2),
			static_cast<uint8_t>(base_pattern + 1), base_pattern };
	STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);

	GetInputDataInLength(num_data, data, edit_mask);
	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data,
			edit_mask, result);

	if (verbose)
		PrintArray("result", num_data, result);

// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	ExactCompare(num_data, result, ELEMENTSOF(answer), answer);
}

/*
 * Test various bit operations (except for NOT) with an Uint32 array of length 11
 */

TEST_F(BitOperation32, MiscOpLengthEleven) {
	size_t const num_data(11);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_data, data, edit_mask);

	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}
	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, result);

		if (verbose)
			PrintArray("result", num_data, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_data, result, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Test bit operation NOT by sakura_OperateBitsUint32Not with an array of length 11
 * RESULT:
 * out = [ 0...000, 0...001, 0...010, 0...011, 1...111, 1...110, 1...101, 1...100,
 *         0...000, 0...001, 0...010 ]
 */
TEST_F(BitOperation32, NotLengthEleven) {
	size_t const num_data(11);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	uint32_t const base_pattern = (~static_cast<uint32_t>(0) << 2); // 1...100
	uint32_t answer[] = { 0, 1, 2, 3, static_cast<uint32_t>(base_pattern + 3),
			static_cast<uint32_t>(base_pattern + 2),
			static_cast<uint32_t>(base_pattern + 1), base_pattern };
	STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);

	GetInputDataInLength(num_data, data, edit_mask);
	if (verbose) {
		PrintArray("in (before)", num_data, data);
		PrintArray("mask", num_data, edit_mask);
	}

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data, edit_mask, result);

	if (verbose)
		PrintArray("result", num_data, result);

// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	ExactCompare(num_data, result, ELEMENTSOF(answer), answer);
}

/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test various bit operations (except for NOT) with an Uint8 array of length zero
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kOK)
 */

TEST_F(BitOperation8, MiscOpLengthZero) {
	size_t const num_data(0);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_data, result, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Test bit operation NOT by sakura_OperateBitsUint8Not with an array of length zero
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kOK)
 */
TEST_F(BitOperation8, NotLengthZero) {
	size_t const num_data(0);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data,
			edit_mask, result);

// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
}

/*
 * Test various bit operations (except for NOT) with an Uint32 array of length zero
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kOK)
 */

TEST_F(BitOperation32, MiscOpLengthZero) {
	size_t const num_data(0);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_data, result, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Test bit operation NOT by sakura_OperateBitsUint32Not with an array of length zero
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kOK)
 */
TEST_F(BitOperation32, NotLengthZero) {
	size_t const num_data(0);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data, edit_mask, result);

	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
}

/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test failure cases of various bit operations (except for NOT): NULL data array
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kInvalidArgument)
 */
TEST_F(BitOperation8, MiscOpFailNullData) {
	size_t const num_data(NUM_IN);
	uint8_t dummy[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(dummy)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(dummy)];

	uint8_t *data_null = nullptr;

	GetInputDataInLength(num_data, dummy, edit_mask);

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data_null,
				edit_mask, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

/*
 * Test failure cases of various bit operations (except for NOT): NULL data array
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kInvalidArgument)
 */
TEST_F(BitOperation32, MiscOpFailNullData) {
	size_t const num_data(NUM_IN);
	uint32_t dummy[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(dummy)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(dummy)];

	uint32_t *data_null = nullptr;

	GetInputDataInLength(num_data, dummy, edit_mask);

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data_null,
				edit_mask, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation8, NotFailNullData) {
	size_t const num_data(NUM_IN);
	uint8_t dummy[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(dummy)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(dummy)];

	uint8_t *data_null = nullptr;

	GetInputDataInLength(num_data, dummy, edit_mask);

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data_null,
			edit_mask, result);
// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

TEST_F(BitOperation32, NotFailNullData) {
	size_t const num_data(NUM_IN);
	uint32_t dummy[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(dummy)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(dummy)];

	uint32_t *data_null = nullptr;

	GetInputDataInLength(num_data, dummy, edit_mask);

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data_null, edit_mask, result);
// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test failure cases of various bit operations (except for NOT): NULL mask array
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kInvalidArgument)
 */
TEST_F(BitOperation8, MiscOpFailNullMask) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint8_t data[num_data];

	bool dummy[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t out[ELEMENTSOF(data)];

	bool *mask_null = nullptr;

	GetInputDataInLength(num_data, data, dummy);

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				mask_null, out);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation32, MiscOpFailNullMask) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint32_t data[num_data];

	bool dummy[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t out[ELEMENTSOF(data)];

	bool *mask_null = nullptr;

	GetInputDataInLength(num_data, data, dummy);

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				mask_null, out);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation8, NotFailNullMask) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint8_t data[num_data];

	bool dummy[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t out[ELEMENTSOF(data)];

	bool *mask_null = nullptr;

	GetInputDataInLength(num_data, data, dummy);

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data,
			mask_null, out);
// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

TEST_F(BitOperation32, NotFailNullMask) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint32_t data[num_data];

	bool dummy[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t out[ELEMENTSOF(data)];

	bool *mask_null = nullptr;

	GetInputDataInLength(num_data, data, dummy);

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data, mask_null, out);
// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test failure cases of various bit operations (except for NOT): NULL result array
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kInvalidArgument)
 */
TEST_F(BitOperation8, MiscOpFailNullResult) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];

	uint8_t *result_null = nullptr;

	GetInputDataInLength(num_data, data, edit_mask);

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, result_null);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation32, MiscOpFailNullResult) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];

	uint32_t *result_null = nullptr;

	GetInputDataInLength(num_data, data, edit_mask);

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data,
				edit_mask, result_null);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation8, NotFailNullResult) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint8_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];

	uint8_t *result_null = nullptr;

	GetInputDataInLength(num_data, data, edit_mask);

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data,
			edit_mask, result_null);
// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}


TEST_F(BitOperation32, NotFailNullResult) {
	size_t const num_data(NUM_IN);
	SIMD_ALIGN
	uint32_t data[num_data];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];

	uint32_t *result_null = nullptr;

	GetInputDataInLength(num_data, data, edit_mask);

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data, edit_mask, result_null);
// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test failure cases of various bit operations (except for NOT): Unaligned data array
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kInvalidArgument)
 */
TEST_F(BitOperation8, MiscOpFailNotAlignedData) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint8_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	uint8_t *data_shift = &data[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(data_shift));

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data_shift, edit_mask, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation32, MiscOpFailNotAlignedData) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint32_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	uint32_t *data_shift = &data[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(data_shift));

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data_shift, edit_mask, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation8, NotFailNotAlignedData) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint8_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	uint8_t *data_shift = &data[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(data_shift));

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data,
			data_shift, edit_mask, result);
	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

TEST_F(BitOperation32, NotFailNotAlignedData) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint32_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	uint32_t *data_shift = &data[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(data_shift));

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data_shift, edit_mask, result);
	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}


/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test failure cases of various bit operations (except for NOT): Unaligned mask array
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kInvalidArgument)
 */
TEST_F(BitOperation8, MiscOpFailNotAlignedMask) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint8_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	bool *mask_shift = &edit_mask[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(mask_shift));

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data, mask_shift, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation32, MiscOpFailNotAlignedMask) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint32_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	bool *mask_shift = &edit_mask[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(mask_shift));

	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data, mask_shift, result);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation8, NotFailNotAlignedMask) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint8_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	bool *mask_shift = &edit_mask[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(mask_shift));

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data,
			mask_shift, result);
	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

TEST_F(BitOperation32, NotFailNotAlignedMask) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint32_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	bool *mask_shift = &edit_mask[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(mask_shift));

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data, mask_shift, result);
	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}


/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Test failure cases of various bit operations (except for NOT): Unaligned result array
 * RESULT:
 *   LIBSAKURA_SYMBOL(Status_kInvalidArgument)
 */
TEST_F(BitOperation8, MiscOpFailNotAlignedResult) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint8_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	uint8_t *result_shift = &result[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(result_shift));
	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data, edit_mask, result_shift);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation32, MiscOpFailNotAlignedResult) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint32_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	uint32_t *result_shift = &result[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(result_shift));
	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		LIBSAKURA_SYMBOL(Status) status = (*kit.function)(
				(kit.invert_mask ? ~bit_mask_ : bit_mask_), num_data, data, edit_mask, result_shift);

		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
	}
}

TEST_F(BitOperation8, NotFailNotAlignedResult) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint8_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	uint8_t *result_shift = &result[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(result_shift));

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint8Not(num_data, data,
			edit_mask, result_shift);
	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

TEST_F(BitOperation32, NotFailNotAlignedResult) {
	size_t offset(UNALIGN_OFFSET);
	size_t const num_data(NUM_IN);
	size_t const num_elements(num_data + offset);
	SIMD_ALIGN
	uint32_t data[num_elements];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	GetInputDataInLength(num_elements, data, edit_mask);

	// Define unaligned array
	uint32_t *result_shift = &result[offset];
	assert(! LIBSAKURA_SYMBOL(IsAligned)(result_shift));

	LIBSAKURA_SYMBOL(Status) status = sakura_OperateBitsUint32Not(num_data,
			data, edit_mask, result_shift);
	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kInvalidArgument), status);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/*
 * Long test of various bit operations (except for NOT) with with a large Uint8 array
 */

TEST_F(BitOperation8, MiscOpLong) {
	SIMD_ALIGN
	uint8_t data[NUM_IN_LONG];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint8_t result[ELEMENTSOF(data)];

	size_t const num_large(ELEMENTSOF(data));

	double start, end;
	size_t const num_repeat = 20000;
	LIBSAKURA_SYMBOL(Status) status;

	// Create long input data by repeating in_data and edit_mask_
	GetInputDataInLength(num_large, data, edit_mask);

	cout << "Iterating " << num_repeat << " loops for each operation. The length of arrays is "
			<< num_large << endl;
	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		start = sakura_GetCurrentTime();
		for (size_t i = 0; i < num_repeat; ++i) {
			status = (*kit.function)( (kit.invert_mask ? ~bit_mask_ : bit_mask_), num_large, data,	edit_mask, result);
		}
		end = sakura_GetCurrentTime();
		cout << "Elapse time of operation: " << end - start << " sec"
				<< endl;
		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_large, result, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Long test of various bit operations (except for NOT) with with a large Uint32 array
 */

TEST_F(BitOperation32, MiscOpLong) {
	SIMD_ALIGN
	uint32_t data[NUM_IN_LONG];
	SIMD_ALIGN
	bool edit_mask[ELEMENTSOF(data)];
	SIMD_ALIGN
	uint32_t result[ELEMENTSOF(data)];

	size_t const num_large(ELEMENTSOF(data));

	double start, end;
	size_t const num_repeat = 20000;
	LIBSAKURA_SYMBOL(Status) status;

	// Create long input data by repeating in_data and edit_mask_
	GetInputDataInLength(num_large, data, edit_mask);

	cout << "Iterating " << num_repeat << " loops for each operation. The length of arrays is "
			<< num_large << endl;
	// Loop over operation types (Standard tests)
	for (size_t iop = 0; iop < num_standard_test; ++iop) {
		operation_type operation = test_list_standard[iop];
		test_kit kit = GetTestKit(operation);
		cout << "Testing bit operation: " << kit.name << endl;
		start = sakura_GetCurrentTime();
		for (size_t i = 0; i < num_repeat; ++i) {
			status = (*kit.function)( (kit.invert_mask ? ~bit_mask_ : bit_mask_), num_large, data,	edit_mask, result);
		}
		end = sakura_GetCurrentTime();
		cout << "Elapse time of operation: " << end - start << " sec"
				<< endl;
		// Verification
		EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
		ExactCompare(num_large, result, ELEMENTSOF(kit.answer), kit.answer);
	}
}

/*
 * Long test of bit operation NOT by sakura_OperateBitsUint8Not with a large array
 * RESULT:
 * out = [00000000, 00000001, 00000010, 00000011, 11111111, 11111110, 11111101, 11111100, .... repeated... ]
 */
TEST_F(BitOperation8, NotLong) {
	SIMD_ALIGN
	uint8_t data_long[NUM_IN_LONG];
	SIMD_ALIGN
	bool edit_mask_long[ELEMENTSOF(data_long)];
	SIMD_ALIGN
	uint8_t result_long[ELEMENTSOF(data_long)];

	uint8_t const base_pattern = (~static_cast<uint8_t>(0) << 2); // 11111100
	uint8_t answer[] = { 0, 1, 2, 3, static_cast<uint8_t>(base_pattern + 3),
			static_cast<uint8_t>(base_pattern + 2),
			static_cast<uint8_t>(base_pattern + 1), base_pattern };
	STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);

	size_t const num_large(ELEMENTSOF(data_long));

	double start, end;
	size_t const num_repeat = 20000;
	LIBSAKURA_SYMBOL(Status) status;

	// Create long input data by repeating in_data and edit_mask_
	GetInputDataInLength(num_large, data_long, edit_mask_long);

	cout << "Iterating " << num_repeat << " loops. The length of arrays is "
			<< num_large << endl;
	start = sakura_GetCurrentTime();
	for (size_t i = 0; i < num_repeat; ++i) {
		status = sakura_OperateBitsUint8Not(num_large, data_long,
				edit_mask_long, result_long);
	}
	end = sakura_GetCurrentTime();
	cout << "Elapse time of actual operation: " << end - start << " sec"
			<< endl;

	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	for (size_t i = 0; i < num_large; ++i) {
		ASSERT_EQ(answer[i % ELEMENTSOF(answer)], result_long[i]);
	}
}

/*
 * Long test of bit operation NOT by sakura_OperateBitsUint32Not with a large array
 * RESULT:
 * out = [0...000, 0...001, 0...010, 0...011, 1...111, 1...110, 1...101, 1...100, .... repeated... ]
 */
TEST_F(BitOperation32, NotLong) {
	SIMD_ALIGN
	uint32_t data_long[NUM_IN_LONG];
	SIMD_ALIGN
	bool edit_mask_long[ELEMENTSOF(data_long)];
	SIMD_ALIGN
	uint32_t result_long[ELEMENTSOF(data_long)];

	uint32_t const base_pattern = (~static_cast<uint32_t>(0) << 2); // 1...100
	uint32_t answer[] = { 0, 1, 2, 3, static_cast<uint32_t>(base_pattern + 3),
			static_cast<uint32_t>(base_pattern + 2),
			static_cast<uint32_t>(base_pattern + 1), base_pattern };
	STATIC_ASSERT(ELEMENTSOF(answer) == NUM_IN);

	size_t const num_large(ELEMENTSOF(data_long));

	double start, end;
	size_t const num_repeat = 20000;
	LIBSAKURA_SYMBOL(Status) status;

	// Create long input data by repeating in_data and edit_mask_
	GetInputDataInLength(num_large, data_long, edit_mask_long);

	cout << "Iterating " << num_repeat << " loops. The length of arrays is "
			<< num_large << endl;
	start = sakura_GetCurrentTime();
	for (size_t i = 0; i < num_repeat; ++i) {
		status = sakura_OperateBitsUint32Not(num_large, data_long,
				edit_mask_long, result_long);
	}
	end = sakura_GetCurrentTime();
	cout << "Elapse time of actual operation: " << end - start << " sec"
			<< endl;

	// Verification
	EXPECT_EQ(LIBSAKURA_SYMBOL(Status_kOK), status);
	for (size_t i = 0; i < num_large; ++i) {
		ASSERT_EQ(answer[i % ELEMENTSOF(answer)], result_long[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
