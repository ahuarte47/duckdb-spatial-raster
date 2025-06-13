#include "raster_types.hpp"
#include "raster_value.hpp"
#include "raster.hpp"
#include "raster_table_functions.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/common/multi_file/multi_file_reader.hpp"
#include "duckdb/parser/expression/function_expression.hpp"
#include "duckdb/parser/parsed_data/create_copy_function_info.hpp"
#include "duckdb/parser/tableref/table_function_ref.hpp"
// Spatial
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"
#include "modules/gdal/gdal_dataset_factory.hpp"
#include "modules/gdal/gdal_context_state.hpp"
#include "modules/gdal/gdal_dataset_ts.hpp"

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
		Returns the list of supported GDAL RASTER drivers and file formats.

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

		InsertionOrderPreservingMap<string> tags;
		tags.insert("ext", "spatial_raster");
		FunctionBuilder::AddTableFunctionDocs(db, "RT_Drivers", DESCRIPTION, EXAMPLE, tags);
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
		names.emplace_back("path");
		return_types.emplace_back(LogicalType::VARCHAR);
		names.emplace_back("raster");
		return_types.emplace_back(RasterTypes::RASTER());

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

		GDALThreadSafeDataset *dataset_ts = new GDALThreadSafeDataset(dataset);

		// Now we can bind the dataset
		ctx_state.GetDatasetRegistry().RegisterDataset(dataset_ts);
		bind_data.loaded = true;

		// And fill the output
		output.data[0].SetValue(0, Value::CreateValue(raw_file_name));
		output.data[1].SetValue(0, RasterValue::CreateValue(dataset_ts));
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

		InsertionOrderPreservingMap<string> tags;
		tags.insert("ext", "spatial_raster");
		FunctionBuilder::AddTableFunctionDocs(db, "RT_Read", DOCUMENTATION, EXAMPLE, tags);

		// Replacement scan
		auto &config = DBConfig::GetConfig(db);
		config.replacement_scans.emplace_back(ReplacementScan);
	}
};

//======================================================================================================================
// RT_Read_Meta
//======================================================================================================================

struct RT_Read_Meta {

	//------------------------------------------------------------------------------------------------------------------
	// Bind
	//------------------------------------------------------------------------------------------------------------------

	struct BindData final : TableFunctionData {
		vector<OpenFileInfo> files;

		explicit BindData(vector<OpenFileInfo> files_p) : files(std::move(files_p)) {
		}
	};

	static unique_ptr<FunctionData> Bind(ClientContext &context, TableFunctionBindInput &input,
	                                     vector<LogicalType> &return_types, vector<string> &names) {
		names.emplace_back("file_name");
		return_types.push_back(LogicalType::VARCHAR);
		names.emplace_back("driver_short_name");
		return_types.push_back(LogicalType::VARCHAR);
		names.emplace_back("driver_long_name");
		return_types.push_back(LogicalType::VARCHAR);
		names.emplace_back("upper_left_x");
		return_types.push_back(LogicalType::DOUBLE);
		names.emplace_back("upper_left_y");
		return_types.push_back(LogicalType::DOUBLE);
		names.emplace_back("width");
		return_types.push_back(LogicalType::INTEGER);
		names.emplace_back("height");
		return_types.push_back(LogicalType::INTEGER);
		names.emplace_back("scale_x");
		return_types.push_back(LogicalType::DOUBLE);
		names.emplace_back("scale_y");
		return_types.push_back(LogicalType::DOUBLE);
		names.emplace_back("skew_x");
		return_types.push_back(LogicalType::DOUBLE);
		names.emplace_back("skew_y");
		return_types.push_back(LogicalType::DOUBLE);
		names.emplace_back("srid");
		return_types.push_back(LogicalType::INTEGER);
		names.emplace_back("num_bands");
		return_types.push_back(LogicalType::INTEGER);

		// Get the filename list
		const auto mfreader = MultiFileReader::Create(input.table_function);
		const auto mflist = mfreader->CreateFileList(context, input.inputs[0], FileGlobOptions::ALLOW_EMPTY);
		return make_uniq_base<FunctionData, BindData>(mflist->GetAllFiles());
	};

	//------------------------------------------------------------------------------------------------------------------
	// Init Global
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
	// Init Local
	//------------------------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------------------------
	// Execute
	//------------------------------------------------------------------------------------------------------------------

	static void Execute(ClientContext &context, TableFunctionInput &input, DataChunk &output) {
		auto &bind_data = input.bind_data->Cast<BindData>();
		auto &state = input.global_state->Cast<State>();

		auto out_size = MinValue<idx_t>(STANDARD_VECTOR_SIZE, bind_data.files.size() - state.current_idx);

		for (idx_t out_idx = 0; out_idx < out_size; out_idx++, state.current_idx++) {
			auto file = bind_data.files[state.current_idx];

			GDALDatasetUniquePtr dataset;
			try {
				dataset =
				    GDALDatasetUniquePtr(GDALDataset::Open(file.path.c_str(), GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR));
			} catch (...) {
				// Just skip anything we cant open
				out_idx--;
				out_size--;
				continue;
			}

			GDALThreadSafeDataset dataset_ts(dataset.get());
			Raster raster(&dataset_ts);
			double gt[6] = {0};
			raster.GetGeoTransform(gt);

			output.data[0].SetValue(out_idx, file.path);
			output.data[1].SetValue(out_idx, dataset->GetDriver()->GetDescription());
			output.data[2].SetValue(out_idx, dataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));
			output.data[3].SetValue(out_idx, gt[0]);
			output.data[4].SetValue(out_idx, gt[3]);
			output.data[5].SetValue(out_idx, raster.GetRasterXSize());
			output.data[6].SetValue(out_idx, raster.GetRasterYSize());
			output.data[7].SetValue(out_idx, gt[1]);
			output.data[8].SetValue(out_idx, gt[5]);
			output.data[9].SetValue(out_idx, gt[2]);
			output.data[10].SetValue(out_idx, gt[4]);
			output.data[11].SetValue(out_idx, raster.GetSrid());
			output.data[12].SetValue(out_idx, raster.GetRasterCount());

			dataset.release();
		}
		output.SetCardinality(out_size);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Cardinality
	//------------------------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------------------------
	// Replacement Scan
	//------------------------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------------------------
	// Documentation
	//------------------------------------------------------------------------------------------------------------------

	static constexpr auto DOCUMENTATION = R"(
	    Read the metadata from a variety of geospatial raster file formats using the GDAL library.

	    The `RT_Read_Meta` table function accompanies the `RT_Read` table function, but instead of reading the contents of a file, this function scans the metadata instead.
	)";

	static constexpr auto EXAMPLE = R"(
		SELECT
			driver_short_name,
			driver_long_name,
			upper_left_x,
			upper_left_y,
			width,
			height,
			scale_x,
			scale_y,
			skew_x,
			skew_y,
			srid,
			num_bands
		FROM
			RT_Read_Meta('./test/data/mosaic/SCL.tif-land-clip00.tiff')
		;

		┌───────────────────┬──────────────────┬──────────────┬──────────────┬───────┬────────┬─────────┬─────────┬────────┬────────┬───────┬───────────┐
		│ driver_short_name │ driver_long_name │ upper_left_x │ upper_left_y │ width │ height │ scale_x │ scale_y │ skew_x │ skew_y │ srid  │ num_bands │
		│      varchar      │     varchar      │    double    │    double    │ int32 │ int32  │ double  │ double  │ double │ double │ int32 │   int32   │
		├───────────────────┼──────────────────┼──────────────┼──────────────┼───────┼────────┼─────────┼─────────┼────────┼────────┼───────┼───────────┤
		│ GTiff             │ GeoTIFF          │     541020.0 │    4796640.0 │  3438 │   5322 │    20.0 │   -20.0 │    0.0 │    0.0 │ 32630 │         1 │
		└───────────────────┴──────────────────┴──────────────┴──────────────┴───────┴────────┴─────────┴─────────┴────────┴────────┴───────┴───────────┘
	)";

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------

	static void Register(DatabaseInstance &db) {
		const TableFunction func("RT_Read_Meta", {LogicalType::VARCHAR}, Execute, Bind, Init);
		ExtensionUtil::RegisterFunction(db, MultiFileReader::CreateFunctionSet(func));

		InsertionOrderPreservingMap<string> tags;
		tags.insert("ext", "spatial_raster");
		FunctionBuilder::AddTableFunctionDocs(db, "RT_Read_Meta", DOCUMENTATION, EXAMPLE, tags);
	}
};

//======================================================================================================================
// RT_Write
//======================================================================================================================

struct RT_Write {

	//------------------------------------------------------------------------------------------------------------------
	// Bind
	//------------------------------------------------------------------------------------------------------------------

	struct BindData : public TableFunctionData {

		string file_path;
		vector<LogicalType> field_sql_types;
		vector<string> field_names;
		string driver_name;
		vector<string> creation_options;

		BindData(string file_path, vector<LogicalType> field_sql_types, vector<string> field_names)
		    : file_path(std::move(file_path)), field_sql_types(std::move(field_sql_types)),
		      field_names(std::move(field_names)) {
		}
	};

	static unique_ptr<FunctionData> Bind(ClientContext &context, CopyFunctionBindInput &input,
	                                     const vector<string> &names, const vector<LogicalType> &sql_types) {

		auto bind_data = make_uniq<BindData>(input.info.file_path, sql_types, names);

		// check all the options in the copy info and set
		for (auto &option : input.info.options) {
			if (StringUtil::Upper(option.first) == "DRIVER") {
				auto set = option.second.front();
				if (set.type().id() == LogicalTypeId::VARCHAR) {
					bind_data->driver_name = set.GetValue<string>();
				} else {
					throw BinderException("Driver name must be a string");
				}
			} else if (StringUtil::Upper(option.first) == "CREATION_OPTIONS") {
				auto set = option.second;
				for (auto &s : set) {
					if (s.type().id() != LogicalTypeId::VARCHAR) {
						throw BinderException("Creation options must be strings");
					}
					bind_data->creation_options.push_back(s.GetValue<string>());
				}
			} else {
				throw BinderException("Unknown option '%s'", option.first);
			}
		}

		if (bind_data->driver_name.empty()) {
			throw BinderException("Driver name must be specified");
		}

		auto driver = GetGDALDriverManager()->GetDriverByName(bind_data->driver_name.c_str());
		if (!driver) {
			throw BinderException("Unknown driver '%s'", bind_data->driver_name);
		}

		// Try get the file extension from the driver
		auto file_ext = driver->GetMetadataItem(GDAL_DMD_EXTENSION);
		if (file_ext) {
			input.file_extension = file_ext;
		} else {
			// Space separated list of file extensions
			auto file_exts = driver->GetMetadataItem(GDAL_DMD_EXTENSIONS);
			if (file_exts) {
				auto exts = StringUtil::Split(file_exts, ' ');
				if (!exts.empty()) {
					input.file_extension = exts[0];
				}
			}
		}

		return std::move(bind_data);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Init Global
	//------------------------------------------------------------------------------------------------------------------

	struct GlobalState final : GlobalFunctionData {

		explicit GlobalState(ClientContext &context) {
		}
	};

	static unique_ptr<GlobalFunctionData> InitGlobal(ClientContext &context, FunctionData &bind_data,
	                                                 const string &file_path) {

		auto global_data = make_uniq<GlobalState>(context);
		return std::move(global_data);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Init Local
	//------------------------------------------------------------------------------------------------------------------

	struct LocalState : public LocalFunctionData {

		explicit LocalState(ClientContext &context) {
		}
	};

	static unique_ptr<LocalFunctionData> InitLocal(ExecutionContext &context, FunctionData &bind_data) {

		auto local_data = make_uniq<LocalState>(context.client);
		return std::move(local_data);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Sink
	//------------------------------------------------------------------------------------------------------------------

	static void Sink(ExecutionContext &context, FunctionData &bdata, GlobalFunctionData &gstate,
	                 LocalFunctionData &lstate, DataChunk &input) {

		auto &bind_data = bdata.Cast<BindData>();

		// Create the raster
		input.Flatten();
		for (idx_t row_idx = 0; row_idx < input.size(); row_idx++) {

			for (idx_t col_idx = 0; col_idx < input.ColumnCount(); col_idx++) {
				auto &type = bind_data.field_sql_types[col_idx];

				if (type == RasterTypes::RASTER()) {
					auto value = input.GetValue(col_idx, row_idx);

					GDALThreadSafeDataset *dataset =
					    reinterpret_cast<GDALThreadSafeDataset *>(value.GetValueUnsafe<uint64_t>());

					auto raw_file_name = bind_data.file_path;
					auto driver_name = bind_data.driver_name;
					auto creation_options = bind_data.creation_options;

					if (!GDALDatasetFactory::WriteFile(dataset, raw_file_name, driver_name, creation_options)) {
						auto error = Raster::GetLastErrorMsg();
						throw IOException("Could not save file: " + raw_file_name + " (" + error + ")");
					}
					break;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	// Combine
	//------------------------------------------------------------------------------------------------------------------

	static void Combine(ExecutionContext &context, FunctionData &bind_data, GlobalFunctionData &gstate,
	                    LocalFunctionData &lstate) {
	}

	//------------------------------------------------------------------------------------------------------------------
	// Finalize
	//------------------------------------------------------------------------------------------------------------------

	static void Finalize(ClientContext &context, FunctionData &bind_data, GlobalFunctionData &gstate) {
	}

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------

	static void Register(DatabaseInstance &db) {
		// register the copy function
		CopyFunction info("RASTER");
		info.copy_to_bind = Bind;
		info.copy_to_initialize_local = InitLocal;
		info.copy_to_initialize_global = InitGlobal;
		info.copy_to_sink = Sink;
		info.copy_to_combine = Combine;
		info.copy_to_finalize = Finalize;
		info.extension = "raster";

		ExtensionUtil::RegisterFunction(db, info);
	}
};

} // namespace

// ######################################################################################################################
//  Register Raster Table Functions
// ######################################################################################################################

void RasterTableFunctions::Register(DatabaseInstance &db) {

	// Register functions
	RT_Drivers::Register(db);
	RT_Read::Register(db);
	RT_Read_Meta::Register(db);
	RT_Write::Register(db);
}

} // namespace duckdb
