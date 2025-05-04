#pragma once

#include "duckdb/main/client_context_state.hpp"
#include "gdal_dataset_registry.hpp"

namespace duckdb {

//! A ClientContextState to collect Rasters (GDALDatasets) of a query.
class GDALClientContextState final : public ClientContextState {
	ClientContext &context;
	string state_id;
	GDALDatasetRegistry registry;

public:
	explicit GDALClientContextState(ClientContext &context, const string &state_id);
	~GDALClientContextState() override;

	void QueryEnd(ClientContext &context) override;

	//! Get or create a ClientContextState for active query.
	static GDALClientContextState &GetOrCreate(ClientContext &context);

	//! Get the registry where collecting GDALDatasets
	GDALDatasetRegistry &GetDatasetRegistry(ClientContext &context);
};

} // namespace duckdb
