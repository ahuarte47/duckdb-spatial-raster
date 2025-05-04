#include "raster_types.hpp"
#include "raster_value.hpp"
#include "raster.hpp"
#include "raster_table_functions.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/parser/expression/function_expression.hpp"
#include "duckdb/parser/tableref/table_function_ref.hpp"
// Spatial
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"
#include "gdal_dataset_factory.hpp"
#include "gdal_context_state.hpp"

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

//======================================================================================================================
// RT_Read
//======================================================================================================================

struct RT_Read {

	//------------------------------------------------------------------------------------------------------------------
	// Bind
	//------------------------------------------------------------------------------------------------------------------

	struct BindData final : TableFunctionData {
		string file_name;
		named_parameter_map_t parameters;
		bool loaded;
	};

	static unique_ptr<FunctionData> Bind(ClientContext &context, TableFunctionBindInput &input,
	                                     vector<LogicalType> &return_types, vector<string> &names) {
		return_types.emplace_back(LogicalType::VARCHAR);
		return_types.emplace_back(RasterTypes::RASTER());
		names.emplace_back("path");
		names.emplace_back("raster");

		auto raw_file_name = input.inputs[0].GetValue<string>();
		auto parameters = input.named_parameters;

		auto result = make_uniq<BindData>();
		result->file_name = raw_file_name;
		result->parameters = parameters;
		result->loaded = false;
		return std::move(result);
	};

	//------------------------------------------------------------------------------------------------------------------
	// Init Global
	//------------------------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------------------------
	// Init Local
	//------------------------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------------------------
	// Execute
	//------------------------------------------------------------------------------------------------------------------

	static void Execute(ClientContext &context, TableFunctionInput &input, DataChunk &output) {
		auto &bind_data = (BindData &)*input.bind_data;

		if (bind_data.loaded) {
			output.SetCardinality(0);
			return;
		}

		auto &config = DBConfig::GetConfig(context);
		if (!config.options.enable_external_access) {
			throw PermissionException("Scanning GDAL files is disabled through configuration");
		}

		// First scan for "options" parameter
		auto gdal_open_options = GDALDatasetFactory::FromNamedParameters(bind_data.parameters, "open_options");

		auto gdal_allowed_drivers = GDALDatasetFactory::FromNamedParameters(bind_data.parameters, "allowed_drivers");

		auto gdal_sibling_files = GDALDatasetFactory::FromNamedParameters(bind_data.parameters, "sibling_files");

		// Now we can open the dataset
		auto raw_file_name = bind_data.file_name;
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);
		auto dataset = GDALDataset::Open(raw_file_name.c_str(), GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR,
		                                 gdal_allowed_drivers.empty() ? nullptr : gdal_allowed_drivers.data(),
		                                 gdal_open_options.empty() ? nullptr : gdal_open_options.data(),
		                                 gdal_sibling_files.empty() ? nullptr : gdal_sibling_files.data());

		if (dataset == nullptr) {
			auto error = Raster::GetLastErrorMsg();
			throw IOException("Could not open file: " + raw_file_name + " (" + error + ")");
		}

		// Now we can bind the dataset
		ctx_state.GetDatasetRegistry(context).RegisterDataset(dataset);
		bind_data.loaded = true;

		// And fill the output
		output.data[0].SetValue(0, Value::CreateValue(raw_file_name));
		output.data[1].SetValue(0, RasterValue::CreateValue(dataset));
		output.SetCardinality(1);
	};

	//------------------------------------------------------------------------------------------------------------------
	// Cardinality
	//------------------------------------------------------------------------------------------------------------------

	static unique_ptr<NodeStatistics> Cardinality(ClientContext &context, const FunctionData *data) {
		auto result = make_uniq<NodeStatistics>();
		result->has_estimated_cardinality = true;
		result->estimated_cardinality = 1;
		result->has_max_cardinality = true;
		result->max_cardinality = 1;
		return result;
	}

	//------------------------------------------------------------------------------------------------------------------
	// Replacement Scan
	//------------------------------------------------------------------------------------------------------------------

	static unique_ptr<TableRef> ReplacementScan(ClientContext &, ReplacementScanInput &input,
	                                            optional_ptr<ReplacementScanData>) {
		auto &table_name = input.table_name;
		auto lower_name = StringUtil::Lower(table_name);

		// Check if the file name ends with some common raster file extensions
		if (StringUtil::EndsWith(lower_name, ".img") || StringUtil::EndsWith(lower_name, ".tiff") ||
		    StringUtil::EndsWith(lower_name, ".tif") || StringUtil::EndsWith(lower_name, ".vrt")) {

			auto table_function = make_uniq<TableFunctionRef>();
			vector<unique_ptr<ParsedExpression>> children;
			children.push_back(make_uniq<ConstantExpression>(Value(table_name)));
			table_function->function = make_uniq<FunctionExpression>("RT_Read", std::move(children));
			return std::move(table_function);
		}
		// else not something we can replace
		return nullptr;
	}

	//------------------------------------------------------------------------------------------------------------------
	// Documentation
	//------------------------------------------------------------------------------------------------------------------

	static constexpr auto DOCUMENTATION = R"(
	    Read and import a variety of geospatial raster file formats using the GDAL library.

	    The `RT_Read` table function is based on the [GDAL](https://gdal.org/index.html) translator library and enables reading raster data from a variety of geospatial raster file formats as if they were DuckDB tables.

	    > See [RT_Drivers](#rt_drivers) for a list of supported file formats and drivers.

	    Except for the `path` parameter, all parameters are optional.

	    | Parameter | Type | Description |
	    | --------- | -----| ----------- |
	    | `path` | VARCHAR | The path to the file to read. Mandatory |
	    | `open_options` | VARCHAR[] | A list of key-value pairs that are passed to the GDAL driver to control the opening of the file. |
	    | `allowed_drivers` | VARCHAR[] | A list of GDAL driver names that are allowed to be used to open the file. If empty, all drivers are allowed. |
	    | `sibling_files` | VARCHAR[] | A list of sibling files that are required to open the file. |

	    Note that GDAL is single-threaded, so this table function will not be able to make full use of parallelism.

	    By using `RT_Read`, the spatial extension also provides “replacement scans” for common geospatial file formats, allowing you to query files of these formats as if they were tables directly.

	    ```sql
	    SELECT * FROM './path/to/some/shapefile/dataset.tif';
	    ```

	    In practice this is just syntax-sugar for calling RT_Read, so there is no difference in performance. If you want to pass additional options, you should use the RT_Read table function directly.

	    The following formats are currently recognized by their file extension:

		| Format | Extension |
		| ------ | --------- |
		| GeoTiff COG | .tif, .tiff |
		| Erdas Imagine | .img |
		| GDAL Virtual | .vrt |
	)";

	static constexpr auto EXAMPLE = R"(
		-- Read a Gtiff file
		SELECT * FROM RT_Read('some/file/path/filename.tif');
	)";

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------

	static void Register(DatabaseInstance &db) {
		TableFunction func("RT_Read", {LogicalType::VARCHAR}, Execute, Bind);

		func.cardinality = Cardinality;
		func.named_parameters["open_options"] = LogicalType::LIST(LogicalType::VARCHAR);
		func.named_parameters["allowed_drivers"] = LogicalType::LIST(LogicalType::VARCHAR);
		func.named_parameters["sibling_files"] = LogicalType::LIST(LogicalType::VARCHAR);
		ExtensionUtil::RegisterFunction(db, func);

		FunctionBuilder::AddTableFunctionDocs(db, "RT_Read", DOCUMENTATION, EXAMPLE, {{"ext", "spatial_raster"}});

		// Replacement scan
		auto &config = DBConfig::GetConfig(db);
		config.replacement_scans.emplace_back(ReplacementScan);
	}
};

} // namespace

// ######################################################################################################################
//  Register Raster Table Functions
// ######################################################################################################################

void GdalRasterTableFunctions::Register(DatabaseInstance &db) {

	// Register functions
	RT_Drivers::Register(db);
	RT_Read::Register(db);
}

} // namespace duckdb
