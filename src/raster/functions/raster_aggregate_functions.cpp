#include "raster_aggregate_functions.hpp"
#include "raster.hpp"
#include "raster_agg.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
// Spatial
#include "spatial_r/spatial_types.hpp"
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"
#include "modules/gdal/gdal_context_state.hpp"

namespace duckdb {

namespace {

//======================================================================================================================
// RT_RasterUnion_Agg
//======================================================================================================================

struct RT_RasterUnion_Agg {

	template <class T, class STATE>
	static void RasterUnionFunction(STATE &state, T &target, AggregateFinalizeData &finalize_data) {
		if (!state.is_set) {
			finalize_data.ReturnNull();
		} else {
			auto datasets = state.datasets;
			auto &bind_data = finalize_data.input.bind_data->Cast<RasterAggBindData>();
			auto &context = bind_data.context;
			auto &options = bind_data.options;

			std::vector<std::string> vrt_options;
			vrt_options.insert(vrt_options.end(), options.begin(), options.end());

			GDALDataset *result = Raster::BuildVRT(*datasets, vrt_options);
			state.Destroy();

			if (result == nullptr) {
				auto error = Raster::GetLastErrorMsg();
				throw IOException("Could not make union: (" + error + ")");
			}

			auto &ctx_state = GDALClientContextState::GetOrCreate(context);
			ctx_state.GetDatasetRegistry().RegisterDataset(result);
			target = CastPointerToValue(result);
		}
	}

	struct UnionAggUnaryOperation : RasterAggUnaryOperation {

		template <class T, class STATE>
		static void Finalize(STATE &state, T &target, AggregateFinalizeData &finalize_data) {
			RasterUnionFunction(state, target, finalize_data);
		}
	};

	struct UnionAggBinaryOperation : RasterAggBinaryOperation {

		template <class T, class STATE>
		static void Finalize(STATE &state, T &target, AggregateFinalizeData &finalize_data) {
			RasterUnionFunction(state, target, finalize_data);
		}
	};

	static void Register(DatabaseInstance &db) {

		auto fun01 = AggregateFunction::UnaryAggregate<RasterAggState, uintptr_t, uintptr_t, UnionAggUnaryOperation>(
		    RasterTypes::RASTER(), RasterTypes::RASTER());
		fun01.bind = RasterAggBindData::BindRasterAggOperation;

		auto fun02 = AggregateFunction::BinaryAggregate<RasterAggState, uintptr_t, list_entry_t, uintptr_t,
		                                                UnionAggBinaryOperation>(
		    RasterTypes::RASTER(), LogicalType::LIST(LogicalType::VARCHAR), RasterTypes::RASTER());
		fun02.bind = RasterAggBindData::BindRasterAggOperation;

		FunctionBuilder::RegisterAggregate(db, "RT_RasterUnion_Agg", [&](AggregateFunctionBuilder &func) {
			func.SetFunction(fun01);
			func.SetFunction(fun02);
			func.SetDescription(R"(
		        Returns the union of a set of raster tiles into a single raster composed of at least one band.
		        Each tiles goes into a separate band in the result dataset.
		        `options` is optional, an array of parameters like [GDALBuildVRT](https://gdal.org/programs/gdalbuildvrt.html).
		    )");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "construction");
		});
	}
};

} // namespace

// ######################################################################################################################
//  Register Scalar functions
// ######################################################################################################################

void RasterAggregateFunctions::Register(DatabaseInstance &db) {
	RT_RasterUnion_Agg::Register(db);
}

} // namespace duckdb
