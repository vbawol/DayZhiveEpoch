/*
* Copyright (C) 2009-2012 Rajko Stojadinovic <http://github.com/rajkosto/hive>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#pragma once

#include "SqlDataSource.h"
#include "CharDataSource.h"
#include "Database/SqlStatement.h"

class SqlCharDataSource : public SqlDataSource, public CharDataSource
{
public:
	SqlCharDataSource(Poco::Logger& logger, shared_ptr<Database> db, const string& idFieldName, const string& wsFieldName);
	~SqlCharDataSource();

	Sqf::Value fetchCharacterInitial( string playerId, int serverId, const string& playerName ) override;
	Sqf::Value fetchCharacterDetails( Int64 characterId ) override;
	Sqf::Value fetchTraderObject( int traderObjectId, int action) override;
	bool updateCharacter( Int64 characterId, int serverId, const FieldsType& fields ) override;
	bool updateCharacterGroup( string playerId, int serverId, const Sqf::Value& playerGroup)  override;
	bool updatePlayerCoins(string playerId, int serverId, Int64 coinsValue, Int64 playerBank)  override;
	bool initCharacter( Int64 characterId, const Sqf::Value& inventory, const Sqf::Value& backpack ) override;
	bool killCharacter( Int64 characterId, int duration, int infected ) override;
	bool recordLogin( string playerId, Int64 characterId, int action ) override;

private:
	string _idFieldName;
	string _wsFieldName;

	//statement ids
	SqlStatementID _stmtChangePlayerName;
	SqlStatementID _stmtUpdatePlayerGroup;
	SqlStatementID _stmtUpdatePlayerCoinsA;
	SqlStatementID _stmtUpdatePlayerCoinsB;
	SqlStatementID _stmtUpdatePlayerCoinsC;
	SqlStatementID _stmtInsertPlayer;
	SqlStatementID _stmtUpdateCharacterLastLogin;
	SqlStatementID _stmtInsertNewCharacter;
	SqlStatementID _stmtInitCharacter;
	SqlStatementID _stmtKillCharacter;
	SqlStatementID _stmtRecordLogin;
	SqlStatementID _stmtTradeObjectBuy;
	SqlStatementID _stmtTradeObjectSell;
};