// Copyright (c) 2012-2015, The CryptoNote developers, The Bytecoin developers
//
// This file is part of Bytecoin.
//
// Bytecoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Bytecoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Bytecoin.  If not, see <http://www.gnu.org/licenses/>.

#include "version.h"

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include "DaemonCommandsHandler.h"

#include "Common/SignalHandler.h"
#include "Common/PathTools.h"
#include "crypto/hash.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/CoreConfig.h"
#include "cryptonote_core/Currency.h"
#include "cryptonote_core/MinerConfig.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "p2p/net_node.h"
#include "p2p/NetNodeConfig.h"
#include "rpc/RpcServer.h"
#include "rpc/RpcServerConfig.h"
#include "version.h"

#include <Logging/LoggerManager.h>

#if defined(WIN32)
#include <crtdbg.h>
#endif

using Common::JsonValue;
using namespace CryptoNote;
using namespace Logging;

namespace po = boost::program_options;

namespace
{
const command_line::arg_descriptor<std::string> arg_config_file = {"config-file", "Specify configuration file", "./configs/-.conf"};
  const command_line::arg_descriptor<bool>        arg_os_version  = {"os-version", ""};
  const command_line::arg_descriptor<std::string> arg_log_file    = {"log-file", "", ""};
  const command_line::arg_descriptor<int>         arg_log_level   = {"log-level", "", 2}; // info level
  const command_line::arg_descriptor<bool>        arg_console     = {"no-console", "Disable daemon console commands"};
const command_line::arg_descriptor<bool>        arg_print_genesis_tx = { "print-genesis-tx", "Prints genesis' block tx hex to insert it to config and exits" };
const command_line::arg_descriptor<std::string> arg_GENESIS_COINBASE_TX_HEX  = {"GENESIS_COINBASE_TX_HEX", "Genesis transaction hex", CryptoNote::parameters::GENESIS_COINBASE_TX_HEX};  
const command_line::arg_descriptor<uint64_t>    arg_CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX  = {"CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX", "uint64_t", CryptoNote::parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX};
const command_line::arg_descriptor<uint64_t>    arg_MONEY_SUPPLY  = {"MONEY_SUPPLY", "uint64_t", CryptoNote::parameters::MONEY_SUPPLY};
const command_line::arg_descriptor<unsigned int>    arg_EMISSION_SPEED_FACTOR  = {"EMISSION_SPEED_FACTOR", "unsigned int", CryptoNote::parameters::EMISSION_SPEED_FACTOR};
const command_line::arg_descriptor<uint64_t>    arg_CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE  = {"CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE", "uint64_t", CryptoNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE};
const command_line::arg_descriptor<uint64_t>    arg_CRYPTONOTE_DISPLAY_DECIMAL_POINT  = {"CRYPTONOTE_DISPLAY_DECIMAL_POINT", "size_t", CryptoNote::parameters::CRYPTONOTE_DISPLAY_DECIMAL_POINT};
const command_line::arg_descriptor<uint64_t>    arg_MINIMUM_FEE  = {"MINIMUM_FEE", "uint64_t", CryptoNote::parameters::MINIMUM_FEE};
const command_line::arg_descriptor<uint64_t>    arg_DEFAULT_DUST_THRESHOLD  = {"DEFAULT_DUST_THRESHOLD", "uint64_t", CryptoNote::parameters::DEFAULT_DUST_THRESHOLD};
const command_line::arg_descriptor<uint64_t>    arg_DIFFICULTY_TARGET  = {"DIFFICULTY_TARGET", "uint64_t", CryptoNote::parameters::DIFFICULTY_TARGET};
const command_line::arg_descriptor<size_t>      arg_CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW  = {"CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW", "size_t", CryptoNote::parameters::CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW};
const command_line::arg_descriptor<uint64_t>    arg_MAX_BLOCK_SIZE_INITIAL  = {"MAX_BLOCK_SIZE_INITIAL", "uint64_t", CryptoNote::parameters::MAX_BLOCK_SIZE_INITIAL};
const command_line::arg_descriptor<uint64_t>    arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY  = {"EXPECTED_NUMBER_OF_BLOCKS_PER_DAY", "uint64_t"};
const command_line::arg_descriptor<uint64_t>    arg_UPGRADE_HEIGHT  = {"UPGRADE_HEIGHT", "uint64_t", 0};
const command_line::arg_descriptor<size_t>      arg_DIFFICULTY_CUT  = {"DIFFICULTY_CUT", "uint64_t", CryptoNote::parameters::DIFFICULTY_CUT};
const command_line::arg_descriptor<size_t>      arg_DIFFICULTY_LAG  = {"DIFFICULTY_LAG", "uint64_t", CryptoNote::parameters::DIFFICULTY_LAG};
const command_line::arg_descriptor<std::string> arg_CRYPTONOTE_NAME  = {"CRYPTONOTE_NAME", "Cryptonote name. Used for storage directory", ""};
const command_line::arg_descriptor< std::vector<std::string> > arg_CHECKPOINT  = {"CHECKPOINT", "Checkpoints. Format: HEIGHT:HASH"};
  const command_line::arg_descriptor<bool>        arg_testnet_on  = {"testnet", "Used to deploy test nets. Checkpoints and hardcoded seeds are ignored, "
    "network id is changed. Use it with --data-dir flag. The wallet must be launched with --testnet flag.", false};
}

bool command_line_preprocessor(const boost::program_options::variables_map& vm, LoggerRef& logger);
void print_genesis_tx_hex(const po::variables_map& vm, LoggerManager& logManager) {
CryptoNote::CurrencyBuilder currencyBuilder(logManager);
currencyBuilder.genesisCoinbaseTxHex(command_line::get_arg(vm, arg_GENESIS_COINBASE_TX_HEX));
currencyBuilder.publicAddressBase58Prefix(command_line::get_arg(vm, arg_CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX));
currencyBuilder.moneySupply(command_line::get_arg(vm, arg_MONEY_SUPPLY));
currencyBuilder.emissionSpeedFactor(command_line::get_arg(vm, arg_EMISSION_SPEED_FACTOR));
currencyBuilder.blockGrantedFullRewardZone(command_line::get_arg(vm, arg_CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE));
currencyBuilder.numberOfDecimalPlaces(command_line::get_arg(vm, arg_CRYPTONOTE_DISPLAY_DECIMAL_POINT));
currencyBuilder.mininumFee(command_line::get_arg(vm, arg_MINIMUM_FEE));
currencyBuilder.defaultDustThreshold(command_line::get_arg(vm, arg_DEFAULT_DUST_THRESHOLD));
currencyBuilder.difficultyTarget(command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
currencyBuilder.minedMoneyUnlockWindow(command_line::get_arg(vm, arg_CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW));
currencyBuilder.maxBlockSizeInitial(command_line::get_arg(vm, arg_MAX_BLOCK_SIZE_INITIAL));

if (command_line::has_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY) && command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY) != 0)
{
  currencyBuilder.difficultyWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
  currencyBuilder.upgradeVotingWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
  currencyBuilder.upgradeWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
} else {
  currencyBuilder.difficultyWindow(24 * 60 * 60 / command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
}
currencyBuilder.maxBlockSizeGrowthSpeedDenominator(365 * 24 * 60 * 60 / command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
currencyBuilder.lockedTxAllowedDeltaSeconds(command_line::get_arg(vm, arg_DIFFICULTY_TARGET) * CryptoNote::parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS);  

if (command_line::has_arg(vm, arg_UPGRADE_HEIGHT) && command_line::get_arg(vm, arg_UPGRADE_HEIGHT) != 0)
{
  currencyBuilder.upgradeHeight(command_line::get_arg(vm, arg_UPGRADE_HEIGHT));
}

currencyBuilder.difficultyLag(command_line::get_arg(vm, arg_DIFFICULTY_CUT));
currencyBuilder.difficultyCut(command_line::get_arg(vm, arg_DIFFICULTY_LAG));
CryptoNote::Transaction tx = currencyBuilder.generateGenesisTransaction();
currencyBuilder.genesisCoinbaseTxHex(command_line::get_arg(vm, arg_GENESIS_COINBASE_TX_HEX));
currencyBuilder.publicAddressBase58Prefix(command_line::get_arg(vm, arg_CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX));
currencyBuilder.moneySupply(command_line::get_arg(vm, arg_MONEY_SUPPLY));
currencyBuilder.emissionSpeedFactor(command_line::get_arg(vm, arg_EMISSION_SPEED_FACTOR));
currencyBuilder.blockGrantedFullRewardZone(command_line::get_arg(vm, arg_CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE));
currencyBuilder.numberOfDecimalPlaces(command_line::get_arg(vm, arg_CRYPTONOTE_DISPLAY_DECIMAL_POINT));
currencyBuilder.mininumFee(command_line::get_arg(vm, arg_MINIMUM_FEE));
currencyBuilder.defaultDustThreshold(command_line::get_arg(vm, arg_DEFAULT_DUST_THRESHOLD));
currencyBuilder.difficultyTarget(command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
currencyBuilder.minedMoneyUnlockWindow(command_line::get_arg(vm, arg_CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW));
currencyBuilder.maxBlockSizeInitial(command_line::get_arg(vm, arg_MAX_BLOCK_SIZE_INITIAL));

if (command_line::has_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY) && command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY) != 0)
{
  currencyBuilder.difficultyWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
  currencyBuilder.upgradeVotingWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
  currencyBuilder.upgradeWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
} else {
  currencyBuilder.difficultyWindow(24 * 60 * 60 / command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
}
currencyBuilder.maxBlockSizeGrowthSpeedDenominator(365 * 24 * 60 * 60 / command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
currencyBuilder.lockedTxAllowedDeltaSeconds(command_line::get_arg(vm, arg_DIFFICULTY_TARGET) * CryptoNote::parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS);  

if (command_line::has_arg(vm, arg_UPGRADE_HEIGHT) && command_line::get_arg(vm, arg_UPGRADE_HEIGHT) != 0)
{
  currencyBuilder.upgradeHeight(command_line::get_arg(vm, arg_UPGRADE_HEIGHT));
}

currencyBuilder.difficultyLag(command_line::get_arg(vm, arg_DIFFICULTY_CUT));
currencyBuilder.difficultyCut(command_line::get_arg(vm, arg_DIFFICULTY_LAG));
  CryptoNote::blobdata txb = tx_to_blob(tx);
  std::string tx_hex = blobToHex(txb);

  std::cout << "Insert this line into your coin configuration file as is: " << std::endl;
std::cout << "GENESIS_COINBASE_TX_HEX=" << tx_hex << std::endl;

  return;
}

JsonValue buildLoggerConfiguration(Level level, const std::string& logfile) {
  JsonValue loggerConfiguration(JsonValue::OBJECT);
  loggerConfiguration.insert("globalLevel", static_cast<int64_t>(level));

  JsonValue& cfgLoggers = loggerConfiguration.insert("loggers", JsonValue::ARRAY);

  JsonValue& fileLogger = cfgLoggers.pushBack(JsonValue::OBJECT);
  fileLogger.insert("type", "file");
  fileLogger.insert("filename", logfile);
  fileLogger.insert("level", static_cast<int64_t>(TRACE));

  JsonValue& consoleLogger = cfgLoggers.pushBack(JsonValue::OBJECT);
  consoleLogger.insert("type", "console");
  consoleLogger.insert("level", static_cast<int64_t>(TRACE));
  consoleLogger.insert("pattern", "%T %L ");

  return loggerConfiguration;
}


int main(int argc, char* argv[])
{

#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

  LoggerManager logManager;
  LoggerRef logger(logManager, "daemon");

  try {

    po::options_description desc_cmd_only("Command line options");
    po::options_description desc_cmd_sett("Command line options and settings options");

    command_line::add_arg(desc_cmd_only, command_line::arg_help);
    command_line::add_arg(desc_cmd_only, command_line::arg_version);
    command_line::add_arg(desc_cmd_only, arg_os_version);
    // tools::get_default_data_dir() can't be called during static initialization
    command_line::add_arg(desc_cmd_only, command_line::arg_data_dir, tools::get_default_data_dir());
    command_line::add_arg(desc_cmd_only, arg_config_file);

    command_line::add_arg(desc_cmd_sett, arg_log_file);
    command_line::add_arg(desc_cmd_sett, arg_log_level);
    command_line::add_arg(desc_cmd_sett, arg_console);
    command_line::add_arg(desc_cmd_sett, arg_testnet_on);
command_line::add_arg(desc_cmd_sett, arg_GENESIS_COINBASE_TX_HEX);
command_line::add_arg(desc_cmd_sett, arg_CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
command_line::add_arg(desc_cmd_sett, arg_MONEY_SUPPLY);
command_line::add_arg(desc_cmd_sett, arg_EMISSION_SPEED_FACTOR);
command_line::add_arg(desc_cmd_sett, arg_CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
command_line::add_arg(desc_cmd_sett, arg_CRYPTONOTE_DISPLAY_DECIMAL_POINT);
command_line::add_arg(desc_cmd_sett, arg_MINIMUM_FEE);
command_line::add_arg(desc_cmd_sett, arg_DEFAULT_DUST_THRESHOLD);
command_line::add_arg(desc_cmd_sett, arg_DIFFICULTY_TARGET);
command_line::add_arg(desc_cmd_sett, arg_CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);
command_line::add_arg(desc_cmd_sett, arg_MAX_BLOCK_SIZE_INITIAL);
command_line::add_arg(desc_cmd_sett, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY);
command_line::add_arg(desc_cmd_sett, arg_UPGRADE_HEIGHT);
command_line::add_arg(desc_cmd_sett, arg_DIFFICULTY_CUT);
command_line::add_arg(desc_cmd_sett, arg_DIFFICULTY_LAG);
command_line::add_arg(desc_cmd_sett, arg_CRYPTONOTE_NAME);
command_line::add_arg(desc_cmd_sett, arg_CHECKPOINT);
command_line::add_arg(desc_cmd_sett, arg_print_genesis_tx);

    RpcServerConfig::initOptions(desc_cmd_sett);
    CoreConfig::initOptions(desc_cmd_sett);
    NetNodeConfig::initOptions(desc_cmd_sett);
    MinerConfig::initOptions(desc_cmd_sett);

    po::options_description desc_options("Allowed options");
    desc_options.add(desc_cmd_only).add(desc_cmd_sett);

    po::variables_map vm;
    bool r = command_line::handle_error_helper(desc_options, [&]()
    {
      po::store(po::parse_command_line(argc, argv, desc_options), vm);

      if (command_line::get_arg(vm, command_line::arg_help))
      {
        std::cout << CryptoNote::CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << ENDL << ENDL;
        std::cout << desc_options << std::endl;
        return false;
      }

      std::string data_dir = command_line::get_arg(vm, command_line::arg_data_dir);
      std::string config = command_line::get_arg(vm, arg_config_file);

      boost::filesystem::path data_dir_path(data_dir);
      boost::filesystem::path config_path(config);
      if (!config_path.has_parent_path()) {
        config_path = data_dir_path / config_path;
      }

      boost::system::error_code ec;
      if (boost::filesystem::exists(config_path, ec)) {
po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), desc_cmd_sett, true), vm);      }
      po::notify(vm);
if (command_line::get_arg(vm, arg_print_genesis_tx)) {
  print_genesis_tx_hex(vm, logManager);
  return false;
}
      return true;
    });

    if (!r)
      return 1;
  
    auto modulePath = Common::NativePathToGeneric(argv[0]);
    auto cfgLogFile = Common::NativePathToGeneric(command_line::get_arg(vm, arg_log_file));

    if (cfgLogFile.empty()) {
      cfgLogFile = Common::ReplaceExtenstion(modulePath, ".log");
    } else {
      if (!Common::HasParentPath(cfgLogFile)) {
        cfgLogFile = Common::CombinePath(Common::GetPathDirectory(modulePath), cfgLogFile);
      }
    }

    Level cfgLogLevel = static_cast<Level>(static_cast<int>(Logging::ERROR) + command_line::get_arg(vm, arg_log_level));

    // configure logging
    logManager.configure(buildLoggerConfiguration(cfgLogLevel, cfgLogFile));

    logger(INFO) << CryptoNote::CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG;

    if (command_line_preprocessor(vm, logger)) {
      return 0;
    }

    logger(INFO) << "Module folder: " << argv[0];

    bool testnet_mode = command_line::get_arg(vm, arg_testnet_on);
    if (testnet_mode) {
      logger(INFO) << "Starting in testnet mode!";
    }

    //create objects and link them
    CryptoNote::CurrencyBuilder currencyBuilder(logManager);
currencyBuilder.genesisCoinbaseTxHex(command_line::get_arg(vm, arg_GENESIS_COINBASE_TX_HEX));
currencyBuilder.publicAddressBase58Prefix(command_line::get_arg(vm, arg_CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX));
currencyBuilder.moneySupply(command_line::get_arg(vm, arg_MONEY_SUPPLY));
currencyBuilder.emissionSpeedFactor(command_line::get_arg(vm, arg_EMISSION_SPEED_FACTOR));
currencyBuilder.blockGrantedFullRewardZone(command_line::get_arg(vm, arg_CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE));
currencyBuilder.numberOfDecimalPlaces(command_line::get_arg(vm, arg_CRYPTONOTE_DISPLAY_DECIMAL_POINT));
currencyBuilder.mininumFee(command_line::get_arg(vm, arg_MINIMUM_FEE));
currencyBuilder.defaultDustThreshold(command_line::get_arg(vm, arg_DEFAULT_DUST_THRESHOLD));
currencyBuilder.difficultyTarget(command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
currencyBuilder.minedMoneyUnlockWindow(command_line::get_arg(vm, arg_CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW));
currencyBuilder.maxBlockSizeInitial(command_line::get_arg(vm, arg_MAX_BLOCK_SIZE_INITIAL));

if (command_line::has_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY) && command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY) != 0)
{
  currencyBuilder.difficultyWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
  currencyBuilder.upgradeVotingWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
  currencyBuilder.upgradeWindow(command_line::get_arg(vm, arg_EXPECTED_NUMBER_OF_BLOCKS_PER_DAY));
} else {
  currencyBuilder.difficultyWindow(24 * 60 * 60 / command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
}
currencyBuilder.maxBlockSizeGrowthSpeedDenominator(365 * 24 * 60 * 60 / command_line::get_arg(vm, arg_DIFFICULTY_TARGET));
currencyBuilder.lockedTxAllowedDeltaSeconds(command_line::get_arg(vm, arg_DIFFICULTY_TARGET) * CryptoNote::parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS);  

if (command_line::has_arg(vm, arg_UPGRADE_HEIGHT) && command_line::get_arg(vm, arg_UPGRADE_HEIGHT) != 0)
{
  currencyBuilder.upgradeHeight(command_line::get_arg(vm, arg_UPGRADE_HEIGHT));
}

currencyBuilder.difficultyLag(command_line::get_arg(vm, arg_DIFFICULTY_CUT));
currencyBuilder.difficultyCut(command_line::get_arg(vm, arg_DIFFICULTY_LAG));
    currencyBuilder.testnet(testnet_mode);
try {
  currencyBuilder.currency();
} catch (std::exception&) {
  std::cout << "GENESIS_COINBASE_TX_HEX constant has an incorrect value. Please launch: " << CryptoNote::CRYPTONOTE_NAME << "d --" << arg_print_genesis_tx.name;
  return 1;
}
    CryptoNote::Currency currency = currencyBuilder.currency();
    CryptoNote::core ccore(currency, nullptr, logManager);

    CryptoNote::checkpoints checkpoints(logManager);
std::vector<CryptoNote::CheckpointData> checkpoint_input;
std::vector<std::string> checkpoint_args = command_line::get_arg(vm, arg_CHECKPOINT);
std::vector<std::string> checkpoint_blockIds;

if (command_line::has_arg(vm, arg_CHECKPOINT) && checkpoint_args.size() != 0)
{
  for(const std::string& str: checkpoint_args) {
    std::string::size_type p = str.find(':');
    if(p != std::string::npos)
    {
      uint64_t checkpoint_height = std::stoull(str.substr(0, p));
      checkpoint_blockIds.push_back(str.substr(p+1, str.size()));
      checkpoint_input.push_back({ checkpoint_height, checkpoint_blockIds.back().c_str() });
    }
  }
}
else
{
  if (!command_line::has_arg(vm, arg_UPGRADE_HEIGHT) || command_line::get_arg(vm, arg_UPGRADE_HEIGHT) == 0 || command_line::get_arg(vm, arg_UPGRADE_HEIGHT) == 1) {
      checkpoint_input = CryptoNote::CHECKPOINTS;
  }
}
for (const auto& cp : checkpoint_input) {
      checkpoints.add_checkpoint(cp.height, cp.blockId);
    }

    if (!testnet_mode) {
      ccore.set_checkpoints(std::move(checkpoints));
    }

    CoreConfig coreConfig;
    coreConfig.init(vm);
    NetNodeConfig netNodeConfig;
    netNodeConfig.init(vm);
    MinerConfig minerConfig;
    minerConfig.init(vm);
std::string default_data_dir = tools::get_default_data_dir();
if (command_line::has_arg(vm, arg_CRYPTONOTE_NAME) && !command_line::get_arg(vm, arg_CRYPTONOTE_NAME).empty()) {
  boost::replace_all(default_data_dir, CryptoNote::CRYPTONOTE_NAME, command_line::get_arg(vm, arg_CRYPTONOTE_NAME));
}
coreConfig.configFolder = default_data_dir;
netNodeConfig.configFolder = default_data_dir;
    RpcServerConfig rpcConfig;
    rpcConfig.init(vm);

    System::Dispatcher dispatcher;

    CryptoNote::cryptonote_protocol_handler cprotocol(currency, dispatcher, ccore, nullptr, logManager);
    CryptoNote::node_server p2psrv(dispatcher, cprotocol, logManager);
    CryptoNote::RpcServer rpcServer(dispatcher, logManager, ccore, p2psrv);

    cprotocol.set_p2p_endpoint(&p2psrv);
    ccore.set_cryptonote_protocol(&cprotocol);
    DaemonCommandsHandler dch(ccore, p2psrv, logManager);

    // initialize objects
    logger(INFO) << "Initializing p2p server...";
    if (!p2psrv.init(netNodeConfig, testnet_mode)) {
      logger(ERROR, BRIGHT_RED) << "Failed to initialize p2p server.";
      return 1;
    }
    logger(INFO) << "P2p server initialized OK";

    //logger(INFO) << "Initializing core rpc server...";
    //if (!rpc_server.init(vm)) {
    //  logger(ERROR, BRIGHT_RED) << "Failed to initialize core rpc server.";
    //  return 1;
    //}
    // logger(INFO, BRIGHT_GREEN) << "Core rpc server initialized OK on port: " << rpc_server.get_binded_port();

    // initialize core here
    logger(INFO) << "Initializing core...";
    if (!ccore.init(coreConfig, minerConfig, true)) {
      logger(ERROR, BRIGHT_RED) << "Failed to initialize core";
      return 1;
    }
    logger(INFO) << "Core initialized OK";

    // start components
    if (!command_line::has_arg(vm, arg_console)) {
      dch.start_handling();
    }

    logger(INFO) << "Starting core rpc server on address " << rpcConfig.getBindAddress();
    rpcServer.start(rpcConfig.bindIp, rpcConfig.bindPort);
    logger(INFO) << "Core rpc server started ok";

    tools::SignalHandler::install([&dch, &p2psrv] {
      dch.stop_handling();
      p2psrv.send_stop_signal();
    });

    logger(INFO) << "Starting p2p net loop...";
    p2psrv.run();
    logger(INFO) << "p2p net loop stopped";

    dch.stop_handling();

    //stop components
    logger(INFO) << "Stopping core rpc server...";
    rpcServer.stop();

    //deinitialize components
    logger(INFO) << "Deinitializing core...";
    ccore.deinit();
    logger(INFO) << "Deinitializing p2p...";
    p2psrv.deinit();

    ccore.set_cryptonote_protocol(NULL);
    cprotocol.set_p2p_endpoint(NULL);

  } catch (const std::exception& e) {
    logger(ERROR, BRIGHT_RED) << "Exception: " << e.what();
    return 1;
  }

  logger(INFO) << "Node stopped.";
  return 0;
}

bool command_line_preprocessor(const boost::program_options::variables_map &vm, LoggerRef &logger) {
  bool exit = false;

  if (command_line::get_arg(vm, command_line::arg_version)) {
    std::cout << CryptoNote::CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << ENDL;
    exit = true;
  }
  if (command_line::get_arg(vm, arg_os_version)) {
    std::cout << "OS: " << tools::get_os_version_string() << ENDL;
    exit = true;
  }

  if (exit) {
    return true;
  }

  return false;
}
