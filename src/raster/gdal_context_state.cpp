#include "duckdb/main/database.hpp"
#include "gdal_context_state.hpp"

namespace duckdb {

GDALClientContextState::GDALClientContextState(ClientContext &context, const string &state_id)
    : context(context), state_id(state_id) {
}

GDALClientContextState::~GDALClientContextState() {
}

void GDALClientContextState::QueryEnd(ClientContext &context) {
	std::string k = state_id;
	context.registered_state->Remove(k);
}

GDALDatasetRegistry &GDALClientContextState::GetDatasetRegistry(ClientContext &context) {
	return registry;
}

GDALClientContextState &GDALClientContextState::GetOrCreate(ClientContext &context) {
	auto keyid = CastPointerToValue(&context.GetExecutor());
	auto state_id = "query_id---" + std::to_string(keyid);
	auto gdal_state = context.registered_state->GetOrCreate<GDALClientContextState>(state_id, context, state_id);
	return *gdal_state;
}

} // namespace duckdb
