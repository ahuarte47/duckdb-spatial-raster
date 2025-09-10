#include "raster_band_functions.hpp"
#include "raster_types.hpp"
#include "core/pixel_type.hpp"
#include "core/color_interpretation.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"
// Spatial
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"
#include "modules/gdal/gdal_dataset_ts.hpp"

namespace duckdb {

namespace {

//======================================================================================================================
// RT_RasterBand
//======================================================================================================================

struct RT_RasterBand {

	static void HasNoBand(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		BinaryExecutor::Execute<uintptr_t, int32_t, bool>(
		    args.data[0], args.data[1], result, args.size(), [&](uintptr_t input, int32_t band_num) {
			    GDALDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input)->get();
			    return dataset->GetRasterCount() >= band_num;
		    });
	}

	static void GetBandNoDataValue(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		BinaryExecutor::Execute<uintptr_t, int32_t, double_t>(
		    args.data[0], args.data[1], result, args.size(), [&](uintptr_t input, int32_t band_num) {
			    GDALDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input)->get();

			    if (band_num < 1) {
				    throw InvalidInputException("BandNum must be greater than 0");
			    }
			    if (dataset->GetRasterCount() < band_num) {
				    throw InvalidInputException("Dataset only has %d RasterBands", dataset->GetRasterCount());
			    }
			    GDALRasterBand *raster_band = dataset->GetRasterBand(band_num);
			    return raster_band->GetNoDataValue();
		    });
	}

	static void GetBandPixelType(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		BinaryExecutor::Execute<uintptr_t, int32_t, int32_t>(
		    args.data[0], args.data[1], result, args.size(), [&](uintptr_t input, int32_t band_num) {
			    GDALDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input)->get();

			    if (band_num < 1) {
				    throw InvalidInputException("BandNum must be greater than 0");
			    }
			    if (dataset->GetRasterCount() < band_num) {
				    throw InvalidInputException("Dataset only has %d RasterBands", dataset->GetRasterCount());
			    }
			    GDALRasterBand *raster_band = dataset->GetRasterBand(band_num);
			    return (int32_t)raster_band->GetRasterDataType();
		    });
	}

	static void GetBandPixelTypeName(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		BinaryExecutor::Execute<uintptr_t, int32_t, string_t>(
		    args.data[0], args.data[1], result, args.size(), [&](uintptr_t input, int32_t band_num) {
			    GDALDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input)->get();

			    if (band_num < 1) {
				    throw InvalidInputException("BandNum must be greater than 0");
			    }
			    if (dataset->GetRasterCount() < band_num) {
				    throw InvalidInputException("Dataset only has %d RasterBands", dataset->GetRasterCount());
			    }
			    GDALRasterBand *raster_band = dataset->GetRasterBand(band_num);
			    return PixelTypes::GetPixelTypeName((PixelType)raster_band->GetRasterDataType());
		    });
	}

	static void GetBandColorInterpretation(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		BinaryExecutor::Execute<uintptr_t, int32_t, int32_t>(
		    args.data[0], args.data[1], result, args.size(), [&](uintptr_t input, int32_t band_num) {
			    GDALDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input)->get();

			    if (band_num < 1) {
				    throw InvalidInputException("BandNum must be greater than 0");
			    }
			    if (dataset->GetRasterCount() < band_num) {
				    throw InvalidInputException("Dataset only has %d RasterBands", dataset->GetRasterCount());
			    }
			    GDALRasterBand *raster_band = dataset->GetRasterBand(band_num);
			    return (int32_t)raster_band->GetColorInterpretation();
		    });
	}

	static void GetBandColorInterpretationName(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		BinaryExecutor::Execute<uintptr_t, int32_t, string_t>(
		    args.data[0], args.data[1], result, args.size(), [&](uintptr_t input, int32_t band_num) {
			    GDALDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input)->get();

			    if (band_num < 1) {
				    throw InvalidInputException("BandNum must be greater than 0");
			    }
			    if (dataset->GetRasterCount() < band_num) {
				    throw InvalidInputException("Dataset only has %d RasterBands", dataset->GetRasterCount());
			    }
			    GDALRasterBand *raster_band = dataset->GetRasterBand(band_num);
			    return ColorInterps::GetColorInterpretationName((ColorInterp)raster_band->GetColorInterpretation());
		    });
	}

	static void Register(ExtensionLoader &loader) {

		FunctionBuilder::RegisterScalar(loader, "RT_HasNoBand", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("band_number", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::BOOLEAN);
				variant.SetFunction(HasNoBand);
			});
			func.SetDescription(R"(
				Returns true if there is no band with given band number.
				Band numbers start at 1 and band is assumed to be 1 if not specified.
			)");
			func.SetExample(R"(
				SELECT
					RT_HasNoBand(raster, 1)
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌─────────────────────────┐
				│ rt_hasnoband(raster, 1) │
				│         boolean         │
				├─────────────────────────┤
				│ true                    │
				└─────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(loader, "RT_GetBandNoDataValue", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("band_number", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetBandNoDataValue);
			});
			func.SetDescription(R"(
				Returns the NODATA value of a band in the raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_GetBandNoDataValue(raster, 1)
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌──────────────────────────────────┐
				│ rt_getbandnodatavalue(raster, 1) │
				│              double              │
				├──────────────────────────────────┤
				│             -9999.0              │
				└──────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(loader, "RT_GetBandPixelType", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("band_number", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(GetBandPixelType);
			});
			func.SetDescription(R"(
				Returns the pixel type of a band in the raster.

				This is a code in the enumeration:
				+ Unknown = 0: Unknown or unspecified type
				+ Byte = 1: Eight bit unsigned integer
				+ Int8 = 14: 8-bit signed integer
				+ UInt16 = 2: Sixteen bit unsigned integer
				+ Int16 = 3: Sixteen bit signed integer
				+ UInt32 = 4: Thirty two bit unsigned integer
				+ Int32 = 5: Thirty two bit signed integer
				+ UInt64 = 12: 64 bit unsigned integer
				+ Int64 = 13: 64 bit signed integer
				+ Float32 = 6: Thirty two bit floating point
				+ Float64 = 7: Sixty four bit floating point
				+ CInt16 = 8: Complex Int16
				+ CInt32 = 9: Complex Int32
				+ CFloat32 = 10: Complex Float32
				+ CFloat64 = 11: Complex Float64
			)");
			func.SetExample(R"(
				SELECT
					RT_GetBandPixelType(raster, 1)
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────────────────────────────┐
				│ rt_getbandpixeltype(raster, 1) │
				│             int32              │
				├────────────────────────────────┤
				│               3                │
				└────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(loader, "RT_GetBandPixelTypeName", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("band_number", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::VARCHAR);
				variant.SetFunction(GetBandPixelTypeName);
			});
			func.SetDescription(R"(
				Returns the pixel type name of a band in the raster.

				This is a string in the enumeration:
				+ Unknown: Unknown or unspecified type
				+ Byte: Eight bit unsigned integer
				+ Int8: 8-bit signed integer
				+ UInt16: Sixteen bit unsigned integer
				+ Int16: Sixteen bit signed integer
				+ UInt32: Thirty two bit unsigned integer
				+ Int32: Thirty two bit signed integer
				+ UInt64: 64 bit unsigned integer
				+ Int64: 64 bit signed integer
				+ Float32: Thirty two bit floating point
				+ Float64: Sixty four bit floating point
				+ CInt16: Complex Int16
				+ CInt32: Complex Int32
				+ CFloat32: Complex Float32
				+ CFloat64: Complex Float64
			)");
			func.SetExample(R"(
				SELECT
					RT_GetBandPixelTypeName(raster, 1)
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────────────────────────────────┐
				│ rt_getbandpixeltypename(raster, 1) │
				│              varchar               │
				├────────────────────────────────────┤
				│ Int16                              │
				└────────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(loader, "RT_GetBandColorInterpretation", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("band_number", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(GetBandColorInterpretation);
			});
			func.SetDescription(R"(
				Returns the color interpretation of a band in the raster.

				This is a code in the enumeration:
				+ Undefined = 0: Undefined
				+ GrayIndex = 1: Greyscale
				+ PaletteIndex = 2: Paletted (see associated color table)
				+ RedBand = 3: Red band of RGBA image
				+ GreenBand = 4: Green band of RGBA image
				+ BlueBand = 5: Blue band of RGBA image
				+ AlphaBand = 6: Alpha (0=transparent, 255=opaque)
				+ HueBand = 7: Hue band of HLS image
				+ SaturationBand = 8: Saturation band of HLS image
				+ LightnessBand = 9: Lightness band of HLS image
				+ CyanBand = 10: Cyan band of CMYK image
				+ MagentaBand = 11: Magenta band of CMYK image
				+ YellowBand = 12: Yellow band of CMYK image
				+ BlackBand = 13: Black band of CMYK image
				+ YCbCr_YBand = 14: Y Luminance
				+ YCbCr_CbBand = 15: Cb Chroma
				+ YCbCr_CrBand = 16: Cr Chroma
			)");
			func.SetExample(R"(
				SELECT
					RT_GetBandColorInterpretation(raster, 1)
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌──────────────────────────────────────────┐
				│ rt_getbandcolorinterpretation(raster, 1) │
				│                  int32                   │
				├──────────────────────────────────────────┤
				│                    1                     │
				└──────────────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(loader, "RT_GetBandColorInterpretationName", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("band_number", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::VARCHAR);
				variant.SetFunction(GetBandColorInterpretationName);
			});
			func.SetDescription(R"(
				Returns the color interpretation name of a band in the raster.

				This is a string in the enumeration:
				+ Undefined: Undefined
				+ Greyscale: Greyscale
				+ Paletted: Paletted (see associated color table)
				+ Red: Red band of RGBA image
				+ Green: Green band of RGBA image
				+ Blue: Blue band of RGBA image
				+ Alpha: Alpha (0=transparent, 255=opaque)
				+ Hue: Hue band of HLS image
				+ Saturation: Saturation band of HLS image
				+ Lightness: Lightness band of HLS image
				+ Cyan: Cyan band of CMYK image
				+ Magenta: Magenta band of CMYK image
				+ Yellow: Yellow band of CMYK image
				+ Black: Black band of CMYK image
				+ YLuminance: Y Luminance
				+ CbChroma: Cb Chroma
				+ CrChroma: Cr Chroma
			)");
			func.SetExample(R"(
				SELECT
					RT_GetBandColorInterpretationName(raster, 1)
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌──────────────────────────────────────────────┐
				│ rt_getbandcolorinterpretationname(raster, 1) │
				│                   varchar                    │
				├──────────────────────────────────────────────┤
				│ Greyscale                                    │
				└──────────────────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});
	}
};

} // namespace

// ######################################################################################################################
//  Register File functions
// ######################################################################################################################

void RasterBandFunctions::Register(ExtensionLoader &loader) {
	RT_RasterBand::Register(loader);
}

} // namespace duckdb
