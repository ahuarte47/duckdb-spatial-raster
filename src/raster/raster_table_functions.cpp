#include "raster_table_functions.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
// Spatial
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"

namespace duckdb {

namespace {

//======================================================================================================================
// RT_Drivers
//======================================================================================================================

struct RT_Drivers {

	//------------------------------------------------------------------------------------------------------------------
	// Bind
	//------------------------------------------------------------------------------------------------------------------

	struct BindData final : TableFunctionData {
		idx_t driver_count;
		explicit BindData(const idx_t driver_count_p) : driver_count(driver_count_p) {
		}
	};

	static unique_ptr<FunctionData> Bind(ClientContext &context, TableFunctionBindInput &input,
	                                     vector<LogicalType> &return_types, vector<string> &names) {

		return_types.emplace_back(LogicalType::VARCHAR);
		return_types.emplace_back(LogicalType::VARCHAR);
		return_types.emplace_back(LogicalType::BOOLEAN);
		return_types.emplace_back(LogicalType::BOOLEAN);
		return_types.emplace_back(LogicalType::BOOLEAN);
		return_types.emplace_back(LogicalType::VARCHAR);
		names.emplace_back("short_name");
		names.emplace_back("long_name");
		names.emplace_back("can_create");
		names.emplace_back("can_copy");
		names.emplace_back("can_open");
		names.emplace_back("help_url");

		return make_uniq_base<FunctionData, BindData>(GDALGetDriverCount());
	}

	//------------------------------------------------------------------------------------------------------------------
	// Init
	//------------------------------------------------------------------------------------------------------------------

	struct State final : GlobalTableFunctionState {
		idx_t current_idx;
		explicit State() : current_idx(0) {
		}
	};

	static unique_ptr<GlobalTableFunctionState> Init(ClientContext &context, TableFunctionInitInput &input) {
		return make_uniq_base<GlobalTableFunctionState, State>();
	}

	//------------------------------------------------------------------------------------------------------------------
	// Execute
	//------------------------------------------------------------------------------------------------------------------

	static void Execute(ClientContext &context, TableFunctionInput &input, DataChunk &output) {
		auto &state = input.global_state->Cast<State>();
		auto &bind_data = input.bind_data->Cast<BindData>();

		idx_t count = 0;
		auto next_idx = MinValue<idx_t>(state.current_idx + STANDARD_VECTOR_SIZE, bind_data.driver_count);

		for (; state.current_idx < next_idx; state.current_idx++) {
			auto driver = GDALGetDriver(static_cast<int>(state.current_idx));

			// Check if the driver is a raster driver
			if (GDALGetMetadataItem(driver, GDAL_DCAP_RASTER, nullptr) == nullptr) {
				continue;
			}

			auto short_name = Value::CreateValue(GDALGetDriverShortName(driver));
			auto long_name = Value::CreateValue(GDALGetDriverLongName(driver));

			const char *create_flag = GDALGetMetadataItem(driver, GDAL_DCAP_CREATE, nullptr);
			auto create_value = Value::CreateValue(create_flag != nullptr);

			const char *copy_flag = GDALGetMetadataItem(driver, GDAL_DCAP_CREATECOPY, nullptr);
			auto copy_value = Value::CreateValue(copy_flag != nullptr);
			const char *open_flag = GDALGetMetadataItem(driver, GDAL_DCAP_OPEN, nullptr);
			auto open_value = Value::CreateValue(open_flag != nullptr);

			auto help_topic_flag = GDALGetDriverHelpTopic(driver);
			auto help_topic_value = help_topic_flag == nullptr
			                            ? Value(LogicalType::VARCHAR)
			                            : Value(StringUtil::Format("https://gdal.org/%s", help_topic_flag));

			output.data[0].SetValue(count, short_name);
			output.data[1].SetValue(count, long_name);
			output.data[2].SetValue(count, create_value);
			output.data[3].SetValue(count, copy_value);
			output.data[4].SetValue(count, open_value);
			output.data[5].SetValue(count, help_topic_value);
			count++;
		}
		output.SetCardinality(count);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Documentation
	//------------------------------------------------------------------------------------------------------------------

	// static constexpr DocTag DOC_TAGS[] = {{"ext", "spatial_raster"}};

	static constexpr auto DESCRIPTION = R"(
		Returns the list of supported GDAL RASTER drivers and file formats

		Note that far from all of these drivers have been tested properly.
		Some may require additional options to be passed to work as expected.
		If you run into any issues please first consult the [consult the GDAL docs](https://gdal.org/drivers/raster/index.html).
	)";

	static constexpr auto EXAMPLE = R"(
		SELECT * FROM RT_Drivers();
	)";

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------

	static void Register(DatabaseInstance &db) {
		const TableFunction func("RT_Drivers", {}, Execute, Bind, Init);
		ExtensionUtil::RegisterFunction(db, func);

		FunctionBuilder::AddTableFunctionDocs(db, "RT_Drivers", DESCRIPTION, EXAMPLE, {{"ext", "spatial_raster"}});
	}
};

} // namespace

// ######################################################################################################################
//  Register Raster Table Functions
// ######################################################################################################################

void GdalRasterTableFunctions::Register(DatabaseInstance &db) {

	// Register functions
	RT_Drivers::Register(db);
}

} // namespace duckdb
