#pragma once

#include "SqlDataSource.h"
#include "DataSourceCustom.h"

class SqlCustDataSource : public SqlDataSource, public CustomDataSource
{
public:
	SqlCustDataSource(Poco::Logger& logger, shared_ptr<Database> db);
	~SqlCustDataSource();

	void populateQuery(string query, Sqf::Parameters& params, CustomDataQueue& queue) override;
	bool customExecute(string query, Sqf::Parameters& params) override;
};