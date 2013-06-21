#include <sys/time.h>
#include <cstddef>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <complex>

#include <libsakura/sakura.h>
#include "gtest/gtest.h"

#define NO_TRANSFORM 0
#define NO_VERIFY 0
#define DO_FORTRAN (1 || !NO_VERIFY)

#define AVX __attribute__((aligned (32)))
#define elementsof(x) (sizeof(x) / sizeof((x)[0]))

using namespace std;

namespace {
double currenttime() {
	struct timeval tv;
	int result = gettimeofday(&tv, NULL);
	return tv.tv_sec + ((double) tv.tv_usec) / 1000000.;
}

template<typename T>
T square(T n) {
	return n * n;
}

typedef int32_t integer;

inline integer at2(integer B, integer a, integer b) {
	return a * B + b;
}
inline integer at3(integer B, integer C, integer a, integer b, integer c) {
	return a * (B * C) + at2(C, b, c);
}
inline integer at4(integer B, integer C, integer D, integer a, integer b,
		integer c, integer d) {
	return a * (B * C * D) + at3(C, D, b, c, d);
}

template<size_t ALIGN>
class AlignedMemory {
	char buf[ALIGN - 1];
	AlignedMemory() {
	}
public:
	static void *operator new(size_t size, size_t realSize) {
		void *p = new char[size + realSize];
		cerr << "alloc: " << p << endl;
		return p;
	}
	static void operator delete(void *p) {
		cerr << "free: " << p << endl;
		delete[] reinterpret_cast<char *>(p);
	}

	static AlignedMemory *newAlignedMemory(size_t size) {
		AlignedMemory *p = new (size) AlignedMemory();
		return p;
	}
	template<typename T>
	T *memory() {
		uint64_t addr = reinterpret_cast<uint64_t>(&buf[ALIGN - 1]);
		addr &= ~(ALIGN - 1);
		return reinterpret_cast<T *>(addr);
	}
};

typedef AlignedMemory<32> AlignedMem;

#define NROW (size_t(1024)/2)
#define ROW_FACTOR (size_t(1)) //NROW * ROW_FACTOR分の行を処理する
#define NVISCHAN (size_t(1024))
#define NVISPOL (size_t(4))
#define NCHAN (NVISCHAN)
#define NPOL (NVISPOL)
#define SAMPLING (size_t(100))
#define SUPPORT (size_t(10))
#define NX (size_t(200))
#define NY (size_t(180))
#define CONV_TABLE_SIZE (size_t(ceil(sqrt(2.)*((SUPPORT+1) * SAMPLING))))

float convTab[CONV_TABLE_SIZE] AVX;
uint32_t chanmap[NVISCHAN] AVX;
uint32_t polmap[NVISPOL] AVX;

integer flag[NROW][NVISCHAN][NVISPOL] AVX;
bool flag_[NROW][NVISPOL][NVISCHAN] AVX;

integer rflag[NROW] AVX;
bool rflag_[NROW] AVX;

float weight[NROW][NVISCHAN] AVX;
double sumwt[NCHAN][NPOL] AVX;
double sumwt2[NPOL][NCHAN] AVX;

double fortran_time;

void initTabs() {
	for (size_t i = 0; i < elementsof(convTab); i++) {
		convTab[i] = exp(-square(2.0 * i / elementsof(convTab)));
	}
	for (size_t i = 0; i < elementsof(chanmap); i++) {
		chanmap[i] = i % NCHAN;
	}
	for (size_t i = 0; i < elementsof(polmap); i++) {
		polmap[i] =  i % NPOL;
	}
	for (size_t i = 0; i < elementsof(flag); i++) {
		for (size_t j = 0; j < elementsof(flag[0]); j++) {
			for (size_t k = 0; k < elementsof(flag[0][0]); k++) {
				flag[i][j][k] = 0;
				flag_[i][k][j] = true;
				//flag[i][j][k] = j == k;
				//flag_[i][k][j] = j != k;
			}
		}
	}
	for (size_t i = 0; i < elementsof(rflag); i++) {
		rflag[i] = 0;
		rflag_[i] = true;
	}
	for (size_t i = 0; i < elementsof(weight); i++) {
		for (size_t j = 0; j < elementsof(weight[0]); j++) {
			weight[i][j] = drand48(); // - 0.1;
		}
	}
}

void clearTabs(complex<float> (*grid)[NCHAN][NPOL][NY][NX],
		float (*wgrid)[NCHAN][NPOL][NY][NX], double (*sumwt)[NCHAN][NPOL]) {
	memset(*grid, 0, sizeof(*grid));
	memset(*wgrid, 0, sizeof(*wgrid));
	memset(*sumwt, 0, sizeof(*sumwt));
}

#if 0
void clearTabs(float (*grid)[NCHAN][NPOL][NY][NX],
		float (*wgrid)[NCHAN][NPOL][NY][NX], double (*sumwt)[NCHAN][NPOL]) {
	memset(*grid, 0, sizeof(*grid));
	memset(*wgrid, 0, sizeof(*wgrid));
	memset(*sumwt, 0, sizeof(*sumwt));
}
#endif

template<typename A, typename B>
void clearTabs(A *grid,
		A *wgrid, B *sumwt) {
	memset(*grid, 0, sizeof(*grid));
	memset(*wgrid, 0, sizeof(*wgrid));
	memset(*sumwt, 0, sizeof(*sumwt));
}

template<typename T>
inline bool NearlyEqual(T aa, T bb) {
	T threshold = 0.00007;
	//T threshold = 0.0000003;
	return aa == bb || (abs(aa-bb) / (abs(aa) + abs(bb))) < threshold;
}

bool cmpgrid(complex<float> (*a)[NCHAN][NPOL][NY][NX],
		float (*b)[NY][NX][NPOL][NCHAN]) {
	cout << "comparing grid\n";
	size_t count = 0;
	size_t count2 = 0;
	bool differ = false;
	for (size_t i = 0; i < elementsof(*a); i++) {
		for (size_t j = 0; j < elementsof((*a)[0]); j++) {
			for (size_t k = 0; k < elementsof((*a)[0][0]); k++) {
				for (size_t l = 0; l < elementsof((*a)[0][0][0]); l++) {
					float aa = (*a)[i][j][k][l].real();
					float bb = (*b)[k][l][j][i];
					if (NearlyEqual<float>(aa, bb)) {
						if (differ) {
							cout << "... just before [" << i << "][" << j
									<< "][" << k << "][" << l << "]\n";
						}
						count = 0;
						differ = false;
					} else {
						if (count < 20) {
							cout << "[" << i << "][" << j << "][" << k << "]["
									<< l << "]: " << setprecision(10)
									<< aa << " <> "
									<< bb << endl;
							differ = true;
						}
						count++;
						count2++;
						if (count2 > 200) {
							goto exit;
						}
					}
				}
			}
		}
	}
	exit: if (differ) {
		cout << "... END\n";
	}
	return count2 > 0 ? false : true;
}

bool cmpwgrid(float (*a)[NCHAN][NPOL][NY][NX],
		float (*b)[NY][NX][NPOL][NCHAN]) {
	cout << "comparing wgrid\n";
	size_t count = 0;
	size_t count2 = 0;
	bool differ = false;
	for (size_t i = 0; i < elementsof(*a); i++) {
		for (size_t j = 0; j < elementsof((*a)[0]); j++) {
			for (size_t k = 0; k < elementsof((*a)[0][0]); k++) {
				for (size_t l = 0; l < elementsof((*a)[0][0][0]); l++) {
					float aa = (*a)[i][j][k][l];
					float bb = (*b)[k][l][j][i];
					if (NearlyEqual<float>(aa, bb)) {
						//if (bb != 0.) cout << aa << ", " << bb << endl;
						if (differ) {
							cout << "... just before [" << i << "][" << j
									<< "][" << k << "][" << l << "]\n";
						}
						count = 0;
						differ = false;
					} else {
						if (count < 20) {
							cout << "[" << i << "][" << j << "][" << k << "]["
									<< l << "]: " << setprecision(10)
									<< aa << " <> "
									<< bb << endl;
							differ = true;
						}
						count++;
						count2++;
						if (count2 > 200) {
							goto exit;
						}
					}
				}
			}
		}
	}
	exit: if (differ) {
		cout << "... END\n";
	}
	return count2 > 0 ? false : true;
}

bool cmpsumwt(double (*a)[NCHAN][NPOL], double (*b)[NPOL][NCHAN]) {
	cout << "comparing sumwt\n";
	size_t count = 0;
	size_t count2 = 0;
	bool differ = false;
	for (size_t i = 0; i < elementsof(*a); i++) {
		for (size_t j = 0; j < elementsof((*a)[0]); j++) {
			double aa = (*a)[i][j];
			double bb = (*b)[j][i];
			if (NearlyEqual<double>(aa, bb)) {
				if (differ) {
					cout << "... just before [" << i << "][" << j << "]\n";
				}
				count = 0;
				differ = false;
			} else {
				if (count < 20) {
					cout << "[" << i << "][" << j << "]: " << setprecision(10)
							<< aa << " <> " << bb << endl;
					differ = true;
				}
				count++;
				count2++;
			}
		}
	}
	if (differ) {
		cout << "... END\n";
	}
	return count2 > 0 ? false : true;
}
}

extern "C" {
void ggridsd_(double const xy[][2], const complex<float>*, integer*, integer*,
		integer*, const integer*, const integer*, const float*, integer*,
		integer*, complex<float>*, float*, integer*, integer*, integer *,
		integer *, integer*, integer*, float*, integer*, integer*, double*);
}

void trySpeed(bool dowt, float (*values_)[NROW][NVISPOL][NVISCHAN],
		double (*x)[NROW], double (*y)[NROW],
		complex<float> (*grid)[NCHAN][NPOL][NY][NX],
		float (*wgrid)[NCHAN][NPOL][NY][NX],
		AlignedMem *grid2Mem, float (*grid2)[NY][NX][NPOL][NCHAN],
		AlignedMem *wgrid2Mem, float (*wgrid2)[NY][NX][NPOL][NCHAN]) {
	{
		cout << "Zeroing values... " << flush;
		double start = currenttime();
		clearTabs(grid2, wgrid2, &sumwt2);
		double end = currenttime();
		cout << "done. " << end - start << " sec\n";
	}

	{
		cout << "Gridding by C++(Speed) ... " << flush;
		double start = currenttime();
		for (size_t i = 0; i < ROW_FACTOR; ++i) {
			sakura_Status result = sakura_GridConvolving(NROW, 0, NROW,
					rflag_, *x, *y,
					SUPPORT, SAMPLING,
					NVISPOL, (uint32_t*)polmap,
					NVISCHAN, (uint32_t*)chanmap,
					flag_[0][0],
					(*values_)[0][0],
					weight[0],
					dowt,
					elementsof(convTab),
					convTab,
					NPOL, NCHAN,
					NX, NY,
					sumwt2[0],
					(*wgrid2)[0][0][0],
					(*grid2)[0][0][0]);
			assert(result == sakura_Status_kOK);
			EXPECT_EQ(result, sakura_Status_kOK);
		}
		double end = currenttime();
		double new_time = end - start;
		cout << "done. " << new_time << " sec\n";
		cout << "ratio: " << (new_time / fortran_time) << ", "
				<< (fortran_time / new_time) << "times faster" << endl;
	}
#if !NO_VERIFY
	EXPECT_TRUE(cmpgrid(grid, grid2));
	EXPECT_TRUE(cmpwgrid(wgrid, wgrid2));
#endif
	EXPECT_TRUE(cmpsumwt(&sumwt, &sumwt2));
}

TEST(Gridding, Basic) {
	sakura_Status result = sakura_Initialize();
	EXPECT_EQ(result, sakura_Status_kOK);

	srand48(0);
	initTabs();
	AlignedMem *xy = AlignedMem::newAlignedMemory(sizeof(double[NROW][2]));
	double (*xy_)[NROW][2] = xy->memory<double[NROW][2]>();

	AlignedMem *x = AlignedMem::newAlignedMemory(sizeof(double[NROW]));
	double (*x_)[NROW] = x->memory<double[NROW]>();
	AlignedMem *y = AlignedMem::newAlignedMemory(sizeof(double[NROW]));
	double (*y_)[NROW] = y->memory<double[NROW]>();
	cout << "Initializing xy... " << flush;
	{
		double start = currenttime();
		for (size_t i = 0; i < elementsof(*xy_); i++) {
			(*xy_)[i][0] = (*x_)[i] = NX * drand48();
			(*xy_)[i][1] = (*y_)[i] = NY * drand48();
		}
		double end = currenttime();
		double new_time = end - start;
		cout << "done. " << new_time << " sec\n";
	}

	AlignedMem *valueMem = AlignedMem::newAlignedMemory(
			sizeof(complex<float> [NROW][NVISCHAN][NVISPOL]));
	complex<float> (*values)[NROW][NVISCHAN][NVISPOL] = valueMem->memory<
			complex<float> [NROW][NVISCHAN][NVISPOL]>();

	AlignedMem *valueMem2 = AlignedMem::newAlignedMemory(
			sizeof(float[NROW][NVISPOL][NVISCHAN]));
	float (*values2)[NROW][NVISPOL][NVISCHAN] = valueMem2->memory<
			float[NROW][NVISPOL][NVISCHAN]>();
	cout << sizeof(*values) << endl;
	cout << "Initializing values... " << flush;
	{
		double start = currenttime();
		for (size_t i = 0; i < elementsof(*values); i++) {
			for (size_t j = 0; j < elementsof((*values)[0]); j++) {
				for (size_t k = 0; k < elementsof((*values)[0][0]); k++) {
					(*values)[i][j][k] = (*values2)[i][k][j] = drand48();
				}
			}
		}
		double end = currenttime();
		cout << "done. " << end - start << " sec\n";
	}

	AlignedMem *gridMem = AlignedMem::newAlignedMemory(
			sizeof(complex<float> [NCHAN][NPOL][NY][NX]));
	complex<float> (*grid)[NCHAN][NPOL][NY][NX] = gridMem->memory<
			complex<float> [NCHAN][NPOL][NY][NX]>();

	AlignedMem *wgridMem = AlignedMem::newAlignedMemory(
			sizeof(float[NCHAN][NPOL][NY][NX]));
	float (*wgrid)[NCHAN][NPOL][NY][NX] = wgridMem->memory<
			float[NCHAN][NPOL][NY][NX]>();
	cout << "grid, wgrid size: " << sizeof(*grid) << ", " << sizeof(*wgrid)
			<< endl;
	{
		cout << "Zeroing values... " << flush;
		double start = currenttime();
		clearTabs(grid, wgrid, &sumwt);
		double end = currenttime();
		cout << "done. " << end - start << " sec\n";
	}

	assert(sizeof(int) == sizeof(uint32_t));
	bool dowt = true;
	{
		cout << "Gridding by Fortran ... " << flush;
		double start = currenttime();
		integer dowt_ = integer(dowt);
		integer nvispol = NVISPOL;
		integer nvischan = NVISCHAN;
		integer nrow = NROW;
		integer nx = NX, ny = NY;
		integer npol = NPOL, nchan = NCHAN;
		integer support = SUPPORT, sampling = SAMPLING;
		for (size_t i = 0; i < ROW_FACTOR * DO_FORTRAN; ++i) {
			integer irow = -1;
#if 1
			ggridsd_((*xy_), (*values)[0][0], &nvispol, &nvischan, &dowt_,
					flag[0][0], rflag, weight[0], &nrow, &irow,
					(*grid)[0][0][0], (*wgrid)[0][0][0], &nx, &ny, &npol,
					&nchan, &support, &sampling, convTab, (int*)chanmap, (int*)polmap,
					sumwt[0]);
#endif
		}
		double end = currenttime();
		fortran_time = end - start;
		cout << "done. " << fortran_time << " sec\n";
	}
#if NO_VERIFY
	delete gridMem;
	delete wgridMem;
	gridMem = 0;
	wgridMem = 0;
	grid = 0;
	wgrid = 0;
#endif

	delete valueMem;

	AlignedMem *grid2Mem = AlignedMem::newAlignedMemory(
			sizeof(float[NY][NX][NPOL][NCHAN]));
	float (*grid2)[NY][NX][NPOL][NCHAN] = grid2Mem->memory<
			float[NY][NX][NPOL][NCHAN]>();
	AlignedMem *wgrid2Mem = AlignedMem::newAlignedMemory(
			sizeof(float[NY][NX][NPOL][NCHAN]));
	float (*wgrid2)[NY][NX][NPOL][NCHAN] = wgrid2Mem->memory<
			float[NY][NX][NPOL][NCHAN]>();

	trySpeed(dowt, values2, x_, y_, grid, wgrid, grid2Mem, grid2, wgrid2Mem,
			wgrid2);

	sakura_CleanUp();
}
