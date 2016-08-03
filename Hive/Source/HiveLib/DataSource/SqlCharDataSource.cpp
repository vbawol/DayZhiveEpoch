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

#include "SqlCharDataSource.h"
#include "Database/Database.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

SqlCharDataSource::SqlCharDataSource( Poco::Logger& logger, shared_ptr<Database> db, const string& idFieldName, const string& wsFieldName ) : SqlDataSource(logger,db)
{
	_idFieldName = getDB()->escape(idFieldName);
	_wsFieldName = getDB()->escape(wsFieldName);
}

SqlCharDataSource::~SqlCharDataSource() {}

Sqf::Value SqlCharDataSource::fetchCharacterInitial( string playerId, int serverId, const string& playerName )
{
	Sqf::Value playerGroup = lexical_cast<Sqf::Value>("[]");
	bool newPlayer = false;
	Int64 playerCoins = 0;
	Int64 BankCoins = 0;
	//make sure player exists in db
	{
		auto playerRes(getDB()->queryParams(("SELECT `PlayerName`, `PlayerSex`, `playerGroup`, `PlayerCoins`, `BankCoins` FROM `Player_DATA` WHERE `"+_idFieldName+"`='%s'").c_str(), getDB()->escape(playerId).c_str()));
		if (playerRes && playerRes->fetchRow())
		{
			newPlayer = false;
			//update player name if not current
			if (playerRes->at(0).getString() != playerName)
			{
				auto stmt = getDB()->makeStatement(_stmtChangePlayerName, "UPDATE `Player_DATA` SET `PlayerName`=? WHERE `"+_idFieldName+"`=?");
				stmt->addString(playerName);
				stmt->addString(playerId);
				bool exRes = stmt->execute();
				poco_assert(exRes == true);
				_logger.information("Changed name of player " + playerId + " from '" + playerRes->at(0).getString() + "' to '" + playerName + "'");
			}
			playerGroup = lexical_cast<Sqf::Value>(playerRes->at(2).getString());
			playerCoins = playerRes->at(3).getInt64();
			BankCoins = playerRes->at(4).getInt64();
		}
		else
		{
			newPlayer = true;
			//insert new player into db
			auto stmt = getDB()->makeStatement(_stmtInsertPlayer, "INSERT INTO `Player_DATA` (`"+_idFieldName+"`, `PlayerName`, playerGroup) VALUES (?, ?, ?)");
			stmt->addString(playerId);
			stmt->addString(playerName);
			stmt->addString(lexical_cast<string>(playerGroup));
			bool exRes = stmt->execute();
			poco_assert(exRes == true);
			_logger.information("Created a new player " + playerId + " named '" + playerName + "'");
		}
	}

	//get characters from db
	auto charsRes = getDB()->queryParams(
		("SELECT `CharacterID`, `"+_wsFieldName+"`, `Inventory`, `Backpack`, "
		"TIMESTAMPDIFF(MINUTE,`Datestamp`,`LastLogin`) as `SurvivalTime`, "
		"TIMESTAMPDIFF(MINUTE,`LastAte`,NOW()) as `MinsLastAte`, "
		"TIMESTAMPDIFF(MINUTE,`LastDrank`,NOW()) as `MinsLastDrank`, "
		"`Model`, `Coins` FROM `Character_DATA` WHERE `"+_idFieldName+"` = '%s' AND `Alive` = 1 ORDER BY `CharacterID` DESC LIMIT 1").c_str(), getDB()->escape(playerId).c_str());
	int infected = 0;
	bool newChar = false; //not a new char
	Int64 characterId = -1; //invalid charid
	Sqf::Value worldSpace = Sqf::Parameters(); //empty worldspace
	Sqf::Value inventory = lexical_cast<Sqf::Value>("[]"); //empty inventory
	Sqf::Value backpack = lexical_cast<Sqf::Value>("[]"); //empty backpack
	Sqf::Value survival = lexical_cast<Sqf::Value>("[0,0,0]"); //0 mins alive, 0 mins since last ate, 0 mins since last drank
	string model = ""; //empty models will be defaulted by scripts
	Int64 CharacterCoins = 0; //Coins
	if (charsRes && charsRes->fetchRow())
	{
		newChar = false;
		characterId = charsRes->at(0).getInt64();
		try
		{
			worldSpace = lexical_cast<Sqf::Value>(charsRes->at(1).getString());
		}
		catch(bad_lexical_cast)
		{
			_logger.warning("Invalid Worldspace for CharacterID("+lexical_cast<string>(characterId)+"): "+charsRes->at(1).getString());
		}
		if (!charsRes->at(2).isNull()) //inventory can be null
		{
			try
			{
				inventory = lexical_cast<Sqf::Value>(charsRes->at(2).getString());
				try { SanitiseInv(boost::get<Sqf::Parameters>(inventory)); } catch (const boost::bad_get&) {}
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Inventory for CharacterID("+lexical_cast<string>(characterId)+"): "+charsRes->at(2).getString());
			}
		}		
		if (!charsRes->at(3).isNull()) //backpack can be null
		{
			try
			{
				backpack = lexical_cast<Sqf::Value>(charsRes->at(3).getString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Backpack for CharacterID("+lexical_cast<string>(characterId)+"): "+charsRes->at(3).getString());
			}
		}
		//set survival info
		{
			Sqf::Parameters& survivalArr = boost::get<Sqf::Parameters>(survival);
			survivalArr[0] = charsRes->at(4).getInt32();
			survivalArr[1] = charsRes->at(5).getInt32();
			survivalArr[2] = charsRes->at(6).getInt32();
		}
		try
		{
			model = boost::get<string>(lexical_cast<Sqf::Value>(charsRes->at(7).getString()));
		}
		catch(...)
		{
			model = charsRes->at(7).getString();
		}
		CharacterCoins = charsRes->at(8).getInt64();

		//update last login
		{
			//update last character login
			auto stmt = getDB()->makeStatement(_stmtUpdateCharacterLastLogin, "UPDATE `Character_DATA` SET `LastLogin` = CURRENT_TIMESTAMP WHERE `CharacterID` = ?");
			stmt->addInt64(characterId);
			bool exRes = stmt->execute();
			poco_assert(exRes == true);
		}
	}
	else //inserting new character
	{
		newChar = true;

		int generation = 1;
		int humanity = 2500;
		//try getting previous character info
		{
			auto prevCharRes = getDB()->queryParams(
				("SELECT `Generation`, `Humanity`, `Model`, `Infected` FROM `Character_DATA` WHERE `"+_idFieldName+"` = '%s' AND `Alive` = 0 ORDER BY `CharacterID` DESC LIMIT 1").c_str(), getDB()->escape(playerId).c_str());
			if (prevCharRes && prevCharRes->fetchRow())
			{
				generation = prevCharRes->at(0).getInt32();
				generation++; //apparently this was the correct behaviour all along

				humanity = prevCharRes->at(1).getInt32();
				try
				{
					model = boost::get<string>(lexical_cast<Sqf::Value>(prevCharRes->at(2).getString()));
				}
				catch(...)
				{
					model = prevCharRes->at(2).getString();
				}
				infected = prevCharRes->at(3).getInt32();


			}
		}
		Sqf::Value medical = Sqf::Parameters(); //script will fill this in if empty
		//insert new char into db
		{
			auto stmt = getDB()->makeStatement(_stmtInsertNewCharacter, 
				"INSERT INTO `Character_DATA` (`"+_idFieldName+"`, `InstanceID`, `"+_wsFieldName+"`, `Inventory`, `Backpack`, `Medical`, `Generation`, `Datestamp`, `LastLogin`, `LastAte`, `LastDrank`, `Humanity`) "
				"VALUES (?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, ?)");
			stmt->addString(playerId);
			stmt->addInt32(serverId);
			stmt->addString(lexical_cast<string>(worldSpace));
			stmt->addString(lexical_cast<string>(inventory));
			stmt->addString(lexical_cast<string>(backpack));
			stmt->addString(lexical_cast<string>(medical));
			stmt->addInt32(generation);
			stmt->addInt32(humanity);
			bool exRes = stmt->directExecute(); //need sync as we will be getting the CharacterID right after this
			if (exRes == false)
			{
				_logger.error("Error creating character for playerId " + playerId);
				Sqf::Parameters retVal;
				retVal.push_back(string("ERROR"));
				return retVal;
			}
		}
		//get the new character's id
		{
			auto newCharRes = getDB()->queryParams(
				("SELECT `CharacterID` FROM `Character_DATA` WHERE `"+_idFieldName+"` = '%s' AND `Alive` = 1 ORDER BY `CharacterID` DESC LIMIT 1").c_str(), getDB()->escape(playerId).c_str());
			if (!newCharRes || !newCharRes->fetchRow())
			{
				_logger.error("Error fetching created character for playerId " + playerId);
				Sqf::Parameters retVal;
				retVal.push_back(string("ERROR"));
				return retVal;
			}
			characterId = newCharRes->at(0).getInt64();
		}
		_logger.information("Created a new character " + lexical_cast<string>(characterId) + " for player '" + playerName + "' (" + playerId + ")" );
	}

	Sqf::Parameters retVal;
	retVal.push_back(string("PASS"));
	retVal.push_back(newPlayer);
	retVal.push_back(lexical_cast<string>(characterId));
	if (!newChar)
	{
		retVal.push_back(worldSpace);
		retVal.push_back(inventory);
		retVal.push_back(backpack);
		retVal.push_back(survival);
		retVal.push_back(CharacterCoins); //Character coins
	} else {
		retVal.push_back(infected);
	}
	retVal.push_back(model);
	retVal.push_back(playerGroup); //Persistent group
	retVal.push_back(playerCoins); //Global player coins
	retVal.push_back(BankCoins); //Global bank coins (BANK SALDO)
	//hive interface version
	retVal.push_back(0.97f);
	

	return retVal;
}

Sqf::Value SqlCharDataSource::fetchCharacterDetails( Int64 characterId )
{
	Sqf::Parameters retVal;
	//get details from db
	auto charDetRes = getDB()->queryParams(
		"SELECT `%s`, `Medical`, `Generation`, `KillsZ`, `HeadshotsZ`, `KillsH`, `KillsB`, `CurrentState`, `Humanity`, `InstanceID`,  `Coins`  "
		"FROM `Character_DATA` WHERE `CharacterID`=%d", _wsFieldName.c_str(), characterId);

	if (charDetRes && charDetRes->fetchRow())
	{
		Sqf::Value worldSpace = Sqf::Parameters(); //empty worldspace
		Sqf::Value medical = Sqf::Parameters(); //script will fill this in if empty
		int generation = 1;
		Sqf::Value stats = lexical_cast<Sqf::Value>("[0,0,0,0]"); //killsZ, headZ, killsH, killsB
		Sqf::Value currentState = Sqf::Parameters(); //empty state (aiming, etc)
		int humanity = 2500;
		int instance = 1;
		Int64 coins = 0;
		//get stuff from row
		{
			try
			{
				worldSpace = lexical_cast<Sqf::Value>(charDetRes->at(0).getString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Worldspace (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+charDetRes->at(0).getString());
			}
			try
			{
				medical = lexical_cast<Sqf::Value>(charDetRes->at(1).getString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Medical (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+charDetRes->at(1).getString());
			}
			generation = charDetRes->at(2).getInt32();
			//set stats
			{
				Sqf::Parameters& statsArr = boost::get<Sqf::Parameters>(stats);
				statsArr[0] = charDetRes->at(3).getInt32();
				statsArr[1] = charDetRes->at(4).getInt32();
				statsArr[2] = charDetRes->at(5).getInt32();
				statsArr[3] = charDetRes->at(6).getInt32();
			}
			try
			{
				currentState = lexical_cast<Sqf::Value>(charDetRes->at(7).getString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid CurrentState (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+charDetRes->at(7).getString());
			}
			humanity = charDetRes->at(8).getInt32();
			instance = charDetRes->at(9).getInt32();
			coins = charDetRes->at(10).getInt64();
		}

		retVal.push_back(string("PASS"));
		retVal.push_back(medical);
		retVal.push_back(stats);
		retVal.push_back(currentState);
		retVal.push_back(worldSpace);
		retVal.push_back(humanity);
		retVal.push_back(instance);
	}
	else
	{
		retVal.push_back(string("ERROR"));
	}

	return retVal;
}

bool SqlCharDataSource::updateCharacter( Int64 characterId, int serverId, const FieldsType& fields )
{
	map<string,string> sqlFields;

	for (auto it=fields.begin();it!=fields.end();++it)
	{
		const string& name = it->first;
		const Sqf::Value& val = it->second;
		//arrays
		if (name == "Worldspace" || name == "Inventory" || name == "Backpack" || name == "Medical" || name == "CurrentState")
			sqlFields[name] = "'"+getDB()->escape(lexical_cast<string>(val))+"'";
		//booleans
		else if (name == "JustAte" || name == "JustDrank")
		{
			if (boost::get<bool>(val))
			{
				string newName = "LastAte";
				if (name == "JustDrank")
					newName = "LastDrank";

				sqlFields[newName] = "CURRENT_TIMESTAMP";
			}
		}
		//addition integeroids
		else if (name == "KillsZ" || name == "HeadshotsZ" || name == "DistanceFoot" || name == "Duration" ||
			name == "KillsH" || name == "KillsB" || name == "Humanity")
		{
			int integeroid = static_cast<int>(Sqf::GetDouble(val));
			char intSign = '+';
			if (integeroid < 0)
			{
				intSign = '-';
				integeroid = abs(integeroid);
			}

			if (integeroid > 0) 
				sqlFields[name] = "(`"+name+"` "+intSign+" "+lexical_cast<string>(integeroid)+")";
		}
		//strings
		else if (name == "Model")
		{
			sqlFields[name] = "'" + getDB()->escape(boost::get<string>(val)) + "'";
		}
		else if (name == "Coins")
		{
			Int64 coinNum = static_cast<Int64>(Sqf::GetBigInt(val));
			if (coinNum >= 0) {
				sqlFields[name] = lexical_cast<string>(coinNum);
			}
		}
	}

	if (sqlFields.size() > 0)
	{
		string query = "UPDATE `Character_DATA` SET ";
		for (auto it=sqlFields.begin();it!=sqlFields.end();)
		{
			string fieldName = it->first;
			if (fieldName == "Worldspace")
				fieldName = _wsFieldName;

			query += "`" + fieldName + "` = " + it->second;
			++it;
			if (it != sqlFields.end())
				query += " , ";
		}
		query += ", `InstanceID` = " + lexical_cast<string>(serverId) + "  WHERE `CharacterID` = " + lexical_cast<string>(characterId);

		bool exRes = getDB()->execute(query.c_str());
		poco_assert(exRes == true);

		return exRes;
	}

	return true;
}

bool SqlCharDataSource::updateCharacterGroup(string playerId, int serverId, const Sqf::Value& playerGroup)
{
	auto stmt = getDB()->makeStatement(_stmtUpdatePlayerGroup, "UPDATE `Player_DATA` SET `playerGroup`=? WHERE `" + _idFieldName + "`=?");
	stmt->addString(lexical_cast<string>(playerGroup));
	stmt->addString(playerId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlCharDataSource::updatePlayerCoins(string playerId, int serverId, Int64 coinsValue, Int64 playerBank)
{
	unique_ptr<SqlStatement> stmt;
	bool exRes = true;
	if (coinsValue >= 0 && playerBank >= 0) {
		stmt = getDB()->makeStatement(_stmtUpdatePlayerCoinsA, "UPDATE `Player_DATA` SET `PlayerCoins`=?, `BankCoins`=? WHERE `" + _idFieldName + "`=?");
		stmt->addInt64(coinsValue); //BANK
		stmt->addInt64(playerBank); //BANKSALDO
		stmt->addString(playerId);
		exRes = stmt->execute();
	}
	else if (coinsValue >= 0) {
		stmt = getDB()->makeStatement(_stmtUpdatePlayerCoinsB, "UPDATE `Player_DATA` SET `PlayerCoins`=? WHERE `" + _idFieldName + "`=?");
		_logger.information("SQF Failed to pass player bank value, skipping column: `BankCoins` update");
		stmt->addInt64(coinsValue); //BANK
		stmt->addString(playerId);
		exRes = stmt->execute();
	}
	else if (playerBank >= 0) {
		stmt = getDB()->makeStatement(_stmtUpdatePlayerCoinsC, "UPDATE `Player_DATA` SET `BankCoins`=? WHERE `" + _idFieldName + "`=?");
		_logger.information("SQF Failed to pass player coins value, skipping column: `PlayerCoins` update");
		stmt->addInt64(playerBank); //BANKSALDO
		stmt->addString(playerId);
		exRes = stmt->execute();
	}
	else 
	{
		_logger.information("SQF Failed to pass both player coins and player bank values skipping update");
	}
	poco_assert(exRes == true);

	return exRes;
}

bool SqlCharDataSource::initCharacter( Int64 characterId, const Sqf::Value& inventory, const Sqf::Value& backpack )
{
	auto stmt = getDB()->makeStatement(_stmtInitCharacter, "UPDATE `Character_DATA` SET `Inventory` = ? , `Backpack` = ? WHERE `CharacterID` = ?");
	stmt->addString(lexical_cast<string>(inventory));
	stmt->addString(lexical_cast<string>(backpack));
	stmt->addInt64(characterId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlCharDataSource::killCharacter( Int64 characterId, int duration, int infected )
{
	auto stmt = getDB()->makeStatement(_stmtKillCharacter, 
		"UPDATE `Character_DATA` SET `Alive` = 0, `Infected` = ?, `LastLogin` = DATE_SUB(CURRENT_TIMESTAMP, INTERVAL ? MINUTE) WHERE `CharacterID` = ? AND `Alive` = 1");
	stmt->addInt32(infected);
	stmt->addInt32(duration);
	stmt->addInt64(characterId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlCharDataSource::recordLogin( string playerId, Int64 characterId, int action )
{
	auto stmt = getDB()->makeStatement(_stmtRecordLogin, 
		"INSERT INTO `Player_LOGIN` (`"+_idFieldName+"`, `CharacterID`, `Datestamp`, `Action`) VALUES (?, ?, CURRENT_TIMESTAMP, ?)");
	stmt->addString(playerId);
	stmt->addInt64(characterId);
	stmt->addInt32(action);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

Sqf::Value SqlCharDataSource::fetchTraderObject( int traderObjectId, int action)
{
	Sqf::Parameters retVal;
	//get details from db

	if(action == 0) { 
		auto charDetRes = getDB()->queryParams(
		"SELECT `qty` FROM `Traders_DATA` WHERE `id`=%d", traderObjectId);

		if (charDetRes && charDetRes->fetchRow())
		{
			int qty = 0;
			//get stuff from row	
			qty = charDetRes->at(0).getInt32();
	
			if(qty != 0)
			{
		
				auto stmt = getDB()->makeStatement(_stmtTradeObjectBuy, 
					"UPDATE `Traders_DATA` SET qty = qty - 1 WHERE `id`= ?");
				stmt->addInt32(traderObjectId);

				bool exRes = stmt->directExecute();
				if(exRes == true)
				{
					retVal.push_back(string("PASS"));
					return retVal;
				}
				else 
				{
					retVal.push_back(string("ERROR"));
					return retVal;
				}	
			}
			else 
			{
				retVal.push_back(string("ERROR"));
				return retVal;
			}
		}
		else
		{
			retVal.push_back(string("ERROR"));
			return retVal;
		}
	} else {
	
		
		auto stmt = getDB()->makeStatement(_stmtTradeObjectSell, 
			"UPDATE `Traders_DATA` SET qty = qty + 1 WHERE `id`= ?");
		stmt->addInt32(traderObjectId);

		bool exRes = stmt->directExecute(); 
		if(exRes == true)
		{
			retVal.push_back(string("PASS"));
			return retVal;
		}
		else 
		{
			retVal.push_back(string("ERROR"));
			return retVal;
		}	

	}

}


