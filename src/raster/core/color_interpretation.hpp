#pragma once

#include "duckdb/common/string.hpp"

namespace duckdb {

//! Supported Types of color interpretation for raster bands (GDALColorInterp).
typedef enum {
	Undefined = 0,      /**< Undefined                             */
	GrayIndex = 1,      /**< Greyscale                             */
	PaletteIndex = 2,   /**< Paletted (see associated color table) */
	RedBand = 3,        /**< Red band of RGBA image                */
	GreenBand = 4,      /**< Green band of RGBA image              */
	BlueBand = 5,       /**< Blue band of RGBA image               */
	AlphaBand = 6,      /**< Alpha (0=transparent, 255=opaque)     */
	HueBand = 7,        /**< Hue band of HLS image                 */
	SaturationBand = 8, /**< Saturation band of HLS image          */
	LightnessBand = 9,  /**< Lightness band of HLS image           */
	CyanBand = 10,      /**< Cyan band of CMYK image               */
	MagentaBand = 11,   /**< Magenta band of CMYK image            */
	YellowBand = 12,    /**< Yellow band of CMYK image             */
	BlackBand = 13,     /**< Black band of CMYK image              */
	YCbCr_YBand = 14,   /**< Y Luminance                           */
	YCbCr_CbBand = 15,  /**< Cb Chroma                             */
	YCbCr_CrBand = 16   /**< Cr Chroma                             */
} ColorInterp;

struct ColorInterps {

	//! Returns the name of given ColorInterpretation
	static std::string GetColorInterpretationName(const ColorInterp &color_interp);
};

} // namespace duckdb
