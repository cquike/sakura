/**
 * @file
 *  Sakura main header file.
 *
 * sakura.h
 *
 *  Created on: 2013/02/20
 *      Author: kohji
 */

#ifndef LIBSAKURA_LIBSAKURA_SAKURA_H_
#define LIBSAKURA_LIBSAKURA_SAKURA_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <libsakura/config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @~japanese
 * @brief 関数の呼び出し結果を表す。
 *
 */
typedef enum {
	/**
	 * @~japanese
	 * @brief 成功または正常
	 */LIBSAKURA_SYMBOL(Status_kOK) = 0, /**
	 * @~japanese
	 * @brief 失敗または異常
	 */LIBSAKURA_SYMBOL(Status_kNG) = 1, /**
	 * @~japanese
	 * @brief 引数が不正だった。
	 */LIBSAKURA_SYMBOL(Status_kInvalidArgument) = 2, /**
	 * @~japanese
	 * @brief メモリーが足りない。
	 */LIBSAKURA_SYMBOL(Status_kNoMemory) = 3, /**
	 * @~japanese
	 * @brief 原因不明のエラー。
	 */LIBSAKURA_SYMBOL(Status_kUnknownError) = 99
}LIBSAKURA_SYMBOL(Status);
/**
 * @~english
 * @brief Initializes Sakura Library
 * @return Only when sakura_Status_kOK is returned, you can use Sakura Library.
 * @~japanese
 * @brief Sakuraライブラリを初期化する。
 *
 * 他の全てのSakuraライブラリAPIの呼び出しに先立って、呼び出すこと。
 * マルチスレッドセーフではないので、単一のスレッドから呼び出すこと。
 * @ref sakura_CleanUp() の呼び出しを挟まず、複数回この関数を呼び出してはならない。
 * @~
 * MT-unsafe
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(Initialize)();

/**
 * @~english
 * @brief Cleans up Sakura Library
 * @~japanese
 * @brief Sakuraライブラリをクリーンアップする。
 *
 * マルチスレッドセーフではないので、単一のスレッドから呼び出すこと。
 * また、この関数を呼び出す時点で、他のSakuraライブラリの呼び出しは全て完了しており、以後の呼び出しも発生しないこと。
 * @~
 * MT-unsafe
 */
void LIBSAKURA_SYMBOL(CleanUp)();

/**
 * @~english
 * @brief Returns the current time.
 * @~japanese
 * @brief 現在時刻(単位は秒)を返す。
 *
 * 精度はgettimeofday(2)依存。
 * @~
 * MT-safe
 */
double LIBSAKURA_SYMBOL(GetCurrentTime)();

/*
 * memory alignment(for SIMD)
 */
/**
 * @~japanese
 * @brief Sakuraライブラリが想定するアライメントに、@a ptr が合っているか調べる
 *
 * @param ptr アラインされているか調べたいアドレス。NULL も受け付ける。
 * @return アラインされているなら true , そうでないなら false
 * @~
 * MT-safe
 */

bool LIBSAKURA_SYMBOL(IsAligned)(void const *ptr);

/**
 * @~japanese
 * @brief Sakuraライブラリがベクトル演算を行う配列に期待するアライメントを返す
 *
 * @return Sakuraライブラリは、戻り値の倍数アドレスにアラインされることを期待する
 * @~
 * MT-safe
 */
size_t LIBSAKURA_SYMBOL (GetAlignment)();

/**
 * @~japanese
 * @brief @a arena がアラインされていないならば、
 * アドレスを必要最小限増加させ、アラインされたアドレスを返す。
 * @a arena がアラインされていれば@a arena を返す。
 *
 * @param arena 確保されている領域の先頭アドレス
 * @param size_of_arena 確保されている領域のサイズ
 * @param size_required アライン後も利用可能でなければならないサイズ
 * @return アラインされたアドレス。もし、 @a size_required を格納するのに
 * 十分な大きさの@a size_of_arena が無いならば、 nullptr を返す。
 * @~
 * MT-safe
 */
void *LIBSAKURA_SYMBOL(AlignAny)(size_t size_of_arena, void *arena,
		size_t size_required);
float *LIBSAKURA_SYMBOL(AlignFloat)(size_t elements_in_arena, float *arena,
		size_t elements_required);
double *LIBSAKURA_SYMBOL(AlignDouble)(size_t elements_in_arena, double *arena,
		size_t elements_required);

/**
 * @~japanese
 * Sakuraライブラリが動的にメモリーを確保するときに呼び出す関数の型。
 * 関数はリエントラントな実装でなければならない。
 * sizeが0の場合も、(メモリーが確保できるなら)長さ0の領域のポインタを返すこと(NULLを返さないこと)。
 * @param size
 * @~
 * MT-safe
 */
typedef void *(*LIBSAKURA_SYMBOL(UserAllocator))(size_t size);

/**
 * @~japanese
 * Sakuraライブラリが動的に確保したメモリーを開放するときに呼び出す関数の型。
 * 関数はリエントラントな実装でなければならない。
 * @a pointer にNULLが渡された場合、何も行わないこと。
 * @param pointer	@ref sakura_UserAllocator が返したアドレスまたはNULL。
 * @~
 * MT-safe
 */
typedef void (*LIBSAKURA_SYMBOL(UserDeallocator))(void *pointer);

/**
 * @~japanese
 * @ref sakura_ComputeStatistics の結果を格納する構造体。
 */
typedef struct {
	size_t count; /**< 個数 */
	float sum; /**< 合計 */
	float mean; /**< 平均 */
	float rms; /**< 二乗平均平方根 */
	float stddev; /**< 分散 */
	float min; /**< 最小値 */
	float max; /**< 最大値 */
	int index_of_min; /**< 最小値のインデックス(有効な値がなければ-1) */
	int index_of_max; /**< 最大値のインデックス(有効な値がなければ-1) */
}LIBSAKURA_SYMBOL(StatisticsResult);
/**
 * @~japanese
 * @brief 統計値を計算する。どのような統計値を算出するかは
 * @ref sakura_StatisticsResult を参照。
 *
 * @param num_data @a data 及び@a is_valid の要素の数
 * @param data 対象となるデータ。対応する@a is_valid がtrueの場合、Inf/NaNであってはならない。
 * <br/>must-be-aligned
 * @param is_valid データのマスク。この値が false だと、
 * 対応する@a data の要素が無視される。
 * <br/>must-be-aligned
 * @param result 結果の格納先。計算不可能な場合は、構造体内のメンバーの値にNaNが設定される。
 * 同じ値が複数あった場合、どの値のインデックスが@a index_of_min, @a index_of_maxに格納されるかは不定である。
 * @return 終了ステータス
 * @~
 * MT-safe
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(ComputeStatistics)(size_t num_data,
		float const data[], bool const is_valid[],
		LIBSAKURA_SYMBOL(StatisticsResult) *result);
/**
 * @~japanese
 * @brief validな値のみを先頭に詰めて昇順にソートする。
 *
 * @param is_valid データのマスク。この値が false だと、
 * 対応する@a data の要素が無視される
 * @param num_data @a data 及び@a is_valid の要素の数
 * @param data ソート対象のデータ。In placeでソートするので、この配列内の順序は変更される。
 * 対応する@a is_valid がtrueの場合、NaNであってはならない。
 * @param new_num_data (validでないデータを除いた)ソートされた要素数( <= @a num_data) の格納先
 * @return 終了ステータス
 * @~
 * MT-safe
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(SortValidValuesDensely)(
		size_t num_data, bool const is_valid[], float data[],
		size_t *new_num_data);
/**
 * @~japanese
 * @brief 畳み込みしながらグリッドする。
 * 各浮動小数点の数値はNaN/+-Infであってはならない。
 * @param num_spectra 次の関係でなければならない。 0 <= start_spectrum <= end_spectrum <= num_spectra
 * @param start_spectrum 開始spectrumの添字
 * @param end_spectrum 終了spectrumの添字+1
 * @param spectrum_mask	要素数はnum_spectra。falseのスペクトルは無視される。<br/>must-be-aligned
 * @param x 要素数は@a num_spectra 。2次元平面に投射済みのx座標。<br/>must-be-aligned
 * @param y 要素数は@a num_spectra 。2次元平面に投射済みのy座標。<br/>must-be-aligned
 * @param support @a width x @a height 平面における畳み込みカーネルの広がり(中心か らのpixel数)。範囲は、0 < support <= (INT32_MAX - 1) / 2<br/>
 * ただし、@a support * @a sampling に比例するサイズの領域をスタック上に確保するので、@a support * @a sampling が大きな値となる場合、スタックオーバーフローを起こす。
 * @param sampling 畳み込みカーネルの精度(/pixel)。範囲は、0 < sampling <= INT32_MAX
 * @param num_polarizations 範囲は、0 < num_polarization <= INT32_MAX
 * @param polarization_map	要素数は、num_polarization。各要素の値は、[0,num_polarization_for_grid)でなければならない。<br/>must-be-aligned
 * @param num_channels 範囲は、0 < num_channels <= INT32_MAX
 * @param channel_map	要素数は、num_channels。各要素の値は、[0,num_channels_for_grid)でなければならない。<br/>must-be-aligned
 * @param mask	要素のレイアウトは、[num_spectra][num_polarization][num_channels]。falseの場合は、該当するスペクトル、偏波、チ ャネルの組み合わせのデータは無視される。<br/>must-be-aligned
 * @param value	要素のレイアウトは、[num_spectra][num_polarization][num_channels]。グリッディングする値。<br/>must-be-aligned
 * @param weight 要素のレイアウトは、[num_spectra][num_channels]。重み。<br/>must-be-aligned
 * @param weight_only @a value に重みを掛けたものではなく、重み自体をグリッディングする場合は、true。
 * @param num_convolution_table @ convolution_table の要素数。 範囲は、ceil(sqrt(2.)*(support+1)*sampling) <= convolution_table <= INT32_MAX / 32
 * @param convolution_table	要素数は、@a num_convolution_table 。畳み込みに使用する重みカーブ。各要素の値は、NaN,Infであってはならない。要素0が中心を表す。<br/>must-be-aligned
 * @param num_polarization_for_grid 範囲は、0 < num_polarization_for_grid <= INT32_MAX
 * @param num_channels_for_grid 範囲は、0 < num_channels_for_grid <= INT32_MAX
 * @param width 範囲は、0 < width <= INT32_MAX
 * @param height 範囲は、0 < height <= INT32_MAX
 * @param weight_sum	要素のレイアウトは、[num_polarization_for_grid][num_channels_for_grid]。重みの合計。<br/>must-be-aligned
 * @param weight_of_grid	要素のレイアウトは、[height][width][num_polarization_for_grid][num_channels_for_grid]。グリッドの重み。<br/>must-be-aligned
 * @param grid	要素のレイアウトは、[height][width][num_polarization_for_grid][num_channels_for_grid]。グリッディング結果。<br/>must-be-aligned
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(GridConvolving)(size_t num_spectra,
		size_t start_spectrum, size_t end_spectrum,
		bool const spectrum_mask[/*num_spectra*/],
		double const x[/*num_spectra*/], double const y[/*num_spectra*/],
		size_t support, size_t sampling, size_t num_polarization,
		uint32_t const polarization_map[/*num_polarization*/],
		size_t num_channels, uint32_t const channel_map[/*num_channels*/],
		bool const mask/*[num_spectra][num_polarization]*/[/*num_channels*/],
		float const value/*[num_spectra][num_polarization]*/[/*num_channels*/],
		float const weight/*[num_spectra]*/[/*num_channels*/], bool weight_only,
		size_t num_convolution_table/*= ceil(sqrt(2.)*(support+1)*sampling)*/,
		float const convolution_table[/*num_convolution_table*/],
		size_t num_polarization_for_grid, size_t num_channels_for_grid,
		size_t width, size_t height,
		double weight_sum/*[num_polarization_for_grid]*/[/*num_channels_for_grid*/],
		float weight_of_grid/*[height][width][num_polarization_for_grid]*/[/*num_channels_for_grid*/],
		float grid/*[height][width][num_polarization_for_grid]*/[/*num_channels_for_grid*/]);
/**
 * @~english
 * @brief Invoke bit operation AND between an uint8_t value and array.
 * @details Invokes the following bit operation to @a i- th element of @a out : @n
 * @a out [i] = ( @a edit_mask [i] ? (@a bit_maks & @a in [i]) : @a in [i] )
 *
 * @param bit_mask An uint8_t value. The bit operation is invoked
 * between this value and the array, @a in.
 * @param num_in The number of elements in the arrays, @a in,
 * @a edit_mask, and @a out.
 * @param in An input array (uint8_t) of size, @a num_in. The bit operation
 * is invoked between this array and @a bit_mask.
 * <br/>must-be-aligned
 * @param edit_mask A boolean mask array of size, @a num_in. The bit operation
 * is skipped for the elements with the value, false.
 * <br/>must-be-aligned
 * @param out The output array (uint8_t) of size, @a num_in. It stores the result
 * of the bit operation between @a bit_mask and @a in. The bit operation is skipped
 * and the value in array, @a in, is adopted for the elements where corresponding
 * elements in @a edit_mask is false.
 * <br/>must-be-aligned
 * @return @a sakura_Status
 * @~japanese
 * @brief ビットマスク（uint8_t型）と一次元配列（uint8_t型）のビット積を取る。
 * @details 配列の@a i- 番目の要素に対して次の算を行い、出力@a out を返す: @n
 * @a out [i] = ( @a edit_mask[i] ? (@a bit_maks & @a in [i]) : @a in [i] )
 *
 * @param bit_mask ビットマスク（uint8_t型）
 * @param num_in @a in, @a edit_mask 及び@a out の要素の数。
 * @param in 入力配列（uint8_t型）。要素数は@a num_in でなければならない。
 * <br/>must-be-aligned
 * @param edit_mask データのマスク。要素数は@a num_in でなければならない。
 * この値が true だと、対応する入力配列@a in とビットマスク@a bit_maks のビット積を計算する。
 * この値が false だと、その要素のビット演算は行われず、対応する入力配列@a in の要素がそのまま出力となる。
 * <br/>must-be-aligned
 * @param out 結果の格納先。要素数は@a num_in でなければならない。
 * <br/>must-be-aligned
 * @return @a sakura_Status
 *@~
 * MT-safe
 *
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(OperateBitsUint8And)(
		uint8_t bit_mask, size_t num_in, uint8_t const in[/*num_in*/],
		bool const edit_mask[/*num_in*/], uint8_t out[/*num_in*/]);
/**
 * @~english
 * @brief Invoke bit operation AND between an utint32_t value and array.
 * @details Invokes the following bit operation to @a i- th element of @a out : @n
 * @a out [i] = ( @a edit_mask [i] ? (@a bit_maks & @a in [i]) : @a in [i] )
 *
 * @param bit_mask An uint32_t value. The bit operation is invoked
 * between this value and the array, @a in.
 * @param num_in The number of elements in the arrays, @a in,
 * @a edit_mask, and @a out.
 * @param in An input array (uint32_t) of size, @a num_in. The bit operation
 * is invoked between this array and @a bit_mask.
 * <br/>must-be-aligned
 * @param edit_mask A boolean mask array of size, @a num_in. The bit operation
 * is skipped for the elements with the value, false.
 * <br/>must-be-aligned
 * @param out The output array (uint32_t) of size, @a num_in. It stores the result
 * of the bit operation between @a bit_mask and @a in. The bit operation is skipped
 * and the value in array, @a in, is adopted for the elements where corresponding
 * elements in @a edit_mask is false.
 * <br/>must-be-aligned
 * @return @a sakura_Status
 * @~japanese
 * @brief ビットマスク（uint32_t型）と一次元配列（uint32_t型）のビット積を取る。
 * @details 配列の@a i- 番目の要素に対して次の算を行い、出力@a out を返す: @n
 * @a out [i] = ( @a edit_mask[i] ? (@a bit_maks & @a in [i]) : @a in [i] )
 *
 * @param bit_mask ビットマスク（uint32_t型）
 * @param num_in @a in, @a edit_mask 及び@a out の要素の数。
 * @param in 入力配列（uint32_t型）。要素数は@a num_in でなければならない。
 * <br/>must-be-aligned
 * @param edit_mask データのマスク。要素数は@a num_in でなければならない。
 * この値が true だと、対応する入力配列@a in とビットマスク@a bit_maks のビット積を計算する。
 * この値が false だと、その要素のビット演算は行われず、対応する入力配列@a in の要素がそのまま出力となる。
 * <br/>must-be-aligned
 * @param out 結果の格納先。要素数は@a num_in でなければならない。
 * <br/>must-be-aligned
 * @return @a sakura_Status
 *@~
 * MT-safe
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(OperateBitsUint32And)(
		uint32_t bit_mask, size_t num_in, uint32_t const in[/*num_in*/],
		bool const edit_mask[/*num_in*/], uint32_t out[/*num_in*/]);

/**
 * @~
 * @brief TBD
 * @details TBD
 *
 * @param num_data TBD
 * @param data TBD
 * @param num_condition TBD
 * @param lower_bounds TBD
 * @param upper_bounds TBD
 * @param result TBD
 * @return @a sakura_Status
 *@~
 * MT-TBD
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(SetTrueFloatInRangesInclusive)(
		size_t num_data, float const data[/*num_data*/], size_t num_condition,
		float const lower_bounds[/*num_condition*/],
		float const upper_bounds[/*num_condition*/],
		bool result[/*num_data*/]);

/**
 * @~
 * @brief TBD
 * @details TBD
 *
 * @param num_data TBD
 * @param data TBD
 * @param num_condition TBD
 * @param lower_bounds TBD
 * @param upper_bounds TBD
 * @param result TBD
 * @return @a sakura_Status
 *@~
 * MT-TBD
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(SetTrueIntInRangesInclusive)(
		size_t num_data, int const data[/*num_data*/], size_t num_condition,
		int const lower_bounds[/*num_condition*/],
		int const upper_bounds[/*num_condition*/],
		bool result[/*num_data*/]);

/**
 * @~japanese
 * @brief 補間方法を定義するための列挙型。
 * @~english
 * @brief Enumerations to define interpolation types.
 */
typedef enum {
	/**
	 * @~japanese
	 * @brief 最近接補間法
	 * @~english
	 * @brief Nearest interpolation
	 */LIBSAKURA_SYMBOL(InterpolationMethod_kNearest), /**
	 * @~japanese
	 * @brief 線形補間法
	 * @~english
	 * @brief Linear interpolation
	 */LIBSAKURA_SYMBOL(InterpolationMethod_kLinear), /**
	 * @~japanese
	 * @brief 多項式補間法
	 * @~english
	 * @brief Polynomial interpolation
	 */LIBSAKURA_SYMBOL(InterpolationMethod_kPolynomial), /**
	 * @~japanese
	 * @brief スプライン補間法（三次自然スプライン補間）
	 * @~english
	 * @brief Spline interpolation (Natural cubic spline)
	 */LIBSAKURA_SYMBOL(InterpolationMethod_kSpline), /**
	 * @~japanese
	 * @brief 実装されている補間法の個数
	 * @~english
	 * @brief Number of interpolation methods implemented
	 */LIBSAKURA_SYMBOL(InterpolationMethod_kNumMethod)
}LIBSAKURA_SYMBOL(InterpolationMethod);
/**
 * @~japanese
 * @brief 一次元の補間を行う。
 * @details 長さ@a num_baseの配列@a x_baseと@a y_baseで定義されるデータ点を
 * もとにして1次元の補間を行う。補間したい点のx座標のリストを長さ
 * @a num_interpolatedの配列@a x_interpolatedに渡すと、補間結果が
 * 長さ@a num_interpolatedの配列@a y_interpolatedに格納される。
 *
 * @par
 * @pre @a x_baseおよび@a x_interpolatedは昇順または降順にソートされていなければ
 * ならない。また、@a x_baseの要素には重複があってはならない。
 *
 * @par 昇順の場合と降順の場合の速度の違いについて:
 * @a x_baseまたは @a x_interpolatedが降順にソートされている場合、
 * 内部では配列要素をコピーして昇順に並べ替えた上で補間を行う。そのため、降順の場合は
 * 昇順よりも処理が遅くなる。
 *
 * @par 多項式補間の動作について:
 * @par
 * @a polynomial_orderは0または正の整数でなければならない。
 * @par
 * @a polynomial_orderはあくまで最大次数を規定するものであり、その次数で必ず
 * 補間が行われるとは限らない。たとえば、@a polynomial_orderが2（二次多項式による補間）
 * で@a num_baseが2の場合、実際には2点を通る一次多項式が一意に決まるため、二次多項式に
 * よる補間ではなく一次多項式による補間（線形補間）が行われる。
 * @par
 * @a polynomial_orderに0を指定した場合、最近接補間が行われる。
 *
 * @par
 * @param[in] interpolation_method 補間方法
 * @param[in] polynomial_order 多項式補間法の場合の最大次数。
 * @a polynomial_orderは0または正の整数でなければならない。
 * 実際の次数は、@a num_baseとの兼ね合いで決まる。
 * @param[in] num_base 補間のためのデータ点の数。
 * @param[in] x_base 補間のためのデータ点のx座標。
 * @a x_baseは昇順または降順にソートされていなければならない。
 * @param[in] y_base 補間のためのデータ点のy座標。
 * @param[in] num_interpolated 補間したいデータ点の数。
 * @param[in] x_interpolated 補間したいデータ点のx座標。
 * @a x_interpolatedは昇順または降順にソートされていなければならない。
 * @param[out] y_interpolated 補間結果。
 * @~english
 * @brief Perform one-dimensional interpolation
 * @details
 * @param[in] interpolation_method
 * @param[in] polynomial_order
 * @param[in] num_base
 * @param[in] x_base
 * @param[in] y_base
 * @param[in] num_interpolated
 * @param[in] x_interpolated
 * @param[out] y_interpolated
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(Interpolate1dFloat)(
LIBSAKURA_SYMBOL(
		InterpolationMethod) interoplation_method, int polynomial_order,
		size_t num_base, double const x_base[], float const y_base[],
		size_t num_interpolated, double const x_interpolated[],
		float y_interpolated[]);

/**
 * @brief Perform pseudo two-dimensional interpolation
 */
//LIBSAKURA_SYMBOL(Status) InterpolatePseudo2dFloat(LIBSAKURA_SYMBOL(InterpolationMethod) interpolation_method,
//		int polynomial_order, double x_interpolated, size_t num_base, double x_base[],
//		size_t num_array, float *y_base[], float y_interpolated[]);
/**
 * @~japanese
 * @brief スムージングに使うカーネルタイプを列挙
 * @~english
 * @brief Enumerations to define kernel types.
 */
typedef enum {
	/**
	 * @brief Gaussian
	 */LIBSAKURA_SYMBOL(Convolve1DKernelType_kGaussian), /**
	 * @brief BoxCar
	 */LIBSAKURA_SYMBOL(Convolve1DKernelType_kBoxcar), /**
	 * @brief Hanning
	 */LIBSAKURA_SYMBOL(Convolve1DKernelType_kHanning), /**
	 * @brief Hamming
	 */LIBSAKURA_SYMBOL(Convolve1DKernelType_kHamming)
}LIBSAKURA_SYMBOL(Convolve1DKernelType);

/**
 * @brief Context struct for Convolution
 */
struct LIBSAKURA_SYMBOL(Convolve1DContext);

/**
 * @brief Creating Context which contains 1D Kernel with FFT or without FFT
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(CreateConvolve1DContext)(
		size_t num_channel, LIBSAKURA_SYMBOL(Convolve1DKernelType) kernel_type,
		size_t kernel_width, bool use_fft,
		struct LIBSAKURA_SYMBOL(Convolve1DContext) **context);
/**
 * @brief 1D Convolution with FFT for input spectrum
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(Convolve1D)(
		struct LIBSAKURA_SYMBOL(Convolve1DContext) **context,
		float input_spectrum[/*num_in*/], bool const input_flag[/*num_in*/],
		float output_spectrum[/*num_in*/]);
/**
 * @brief Destroy 1D Kernel with FFT or without FFT
 */
LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(DestroyConvolve1DContext)(
		struct LIBSAKURA_SYMBOL(Convolve1DContext) *context);
/**
 * @brief Logical operation AND between two boolean arrays.
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(OperateLogicalAnd)(size_t num_in,
bool const in1[/*num_in*/], bool const in2[/*num_in*/],
bool out[/*num_in*/]);
/**
 * @brief Compute subtraction between two float arrays (in1 - in2).
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(OperateFloatSubtraction)(
		size_t num_in, float const in1[/*num_in*/], float const in2[/*num_in*/],
		float out[/*num_in*/]);
/**
 * @brief Compute values for Least-Square fitting from input data and a set of model data.
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(GetLeastSquareMatrix)(size_t num_in,
		float const in_data[/*num_in*/], bool const in_mask[/*num_in*/],
		size_t num_model, double const model[/*num_model * num_in*/],
		double out[/*num_model * num_model*/],
		double out_vector[/*num_model*/]);
/**
 * @brief Solve simultaneous equations via LU decomposition.
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(SolveSimultaneousEquationsByLU)(
		size_t num_eqn, double const lsq_matrix0[/*num_eqn * num_eqn*/],
		double const lsq_vector0[/*num_eqn*/], double out[/*num_eqn*/]);
/**
 * @brief Compute the best-fit model spectrum using model spectra and coefficients.
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(DoGetBestFitModel)(size_t num_chan,
		size_t num_eqn, double const model[/*num_eqn * num_chan*/],
		double const coeff[/*num_eqn*/], float out[/*num_in*/]);
/**
 * @brief Compute the best-fit model spectrum by least-square fitting.
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(GetBestFitModel)(size_t num_in,
		float const in_data[/*num_in*/], bool const in_mask[/*num_in*/],
		size_t num_model, double const model[/*num_model * num_in*/],
		float out[/*num_in*/]);
/**
 * @brief Fit a baseline and subtract it from input spectrum.
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(SubtractBaselinePolynomial)(
		size_t num_chan, float const in_data[/*num_chan*/],
		bool const in_mask[/*num_chan*/], int order,
		float clipping_threshold_sigma, int clipping_max_iteration,
		bool get_residual, float out[/*num_chan*/]);
/**
 * @brief Compute a set of model spectra.
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(GetBaselineModel)(size_t num_chan,
		int order, double out[/*(order+1)*num_chan*/]);
/**
 * @brief Actually fit a baseline and subtract it from input spectrum.
 */LIBSAKURA_SYMBOL(Status) LIBSAKURA_SYMBOL(DoSubtractBaseline)(size_t num_chan,
		float const in_data[/*num_chan*/], bool const in_mask[/*num_chan*/],
		size_t num_model, double model_data[/*num_model * num_chan*/],
		float clipping_threshold_sigma, int clipping_max_iteration,
		bool get_residual, float out[/*num_chan*/]);

#ifdef __cplusplus
}
/* extern "C" */
#endif

#endif /* LIBSAKURA_LIBSAKURA_SAKURA_H_ */
