#pragma once

#include "DataSource.h"

class CustomDataSource
{
public:
	typedef std::queue<Sqf::Parameters> CustomDataQueue;
	virtual void populateQuery( string query, Sqf::Parameters& params, CustomDataQueue& queue ) = 0;
	virtual bool customExecute( string query, Sqf::Parameters& params ) = 0;
};