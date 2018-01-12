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

#include "DataSource.h"

class CharDataSource
{
public:
	virtual ~CharDataSource() {}

	virtual Sqf::Value fetchCharacterInitial( string playerId, int serverId, const string& playerName ) = 0;
	virtual Sqf::Value fetchCharacterDetails( Int64 characterId ) = 0;
	virtual Sqf::Value fetchTraderObject( int traderObjectId, int action ) = 0;
	typedef map<string,Sqf::Value> FieldsType;
	virtual bool updateCharacter( Int64 characterId, int serverId, const FieldsType& fields ) = 0;
	virtual bool updateCharacterGroup(string playerId, int serverId, const Sqf::Value& playerGroup ) = 0;
	virtual bool updatePlayerCoins(string playerId, int serverId, Int64 coinsValue, Int64 playerBank ) = 0;
	virtual bool initCharacter( Int64 characterId, const Sqf::Value& inventory, const Sqf::Value& backpack ) = 0;
	virtual bool killCharacter( Int64 characterId, int duration, int infected ) = 0;
	virtual bool recordLogin( string playerId, Int64 characterId, int action ) = 0;
protected:
	static int SanitiseInv(Sqf::Parameters& origInv);
};