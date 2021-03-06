add_definitions(-DSTATICLIB)

file(GLOB_RECURSE Common Common/*)
file(GLOB_RECURSE ConnectivityTool connectivity_tool/*)
file(GLOB_RECURSE Crypto crypto/*)
file(GLOB_RECURSE CryptoNote CryptoNote/*)
file(GLOB_RECURSE CryptoNoteCore cryptonote_core/* cryptonote_config.h)
file(GLOB_RECURSE CryptoNoteProtocol cryptonote_protocol/*)
file(GLOB_RECURSE Daemon daemon/*)
file(GLOB_RECURSE Http HTTP/*)
file(GLOB_RECURSE InProcessNode InProcessNode/*)
file(GLOB_RECURSE Logging Logging/*)
file(GLOB_RECURSE NodeRpcProxy node_rpc_proxy/*)
file(GLOB_RECURSE P2p p2p/*)
file(GLOB_RECURSE Rpc rpc/*)
file(GLOB_RECURSE Serialization serialization/*)
file(GLOB_RECURSE SimpleWallet simplewallet/*)
if(MSVC)
file(GLOB_RECURSE System System/* Platform/Windows/System/*)
elseif(APPLE)
file(GLOB_RECURSE System System/* Platform/OSX/System/*)
else()
file(GLOB_RECURSE System System/* Platform/Linux/System/*)
endif()
file(GLOB_RECURSE Transfers transfers/*)
file(GLOB_RECURSE Wallet wallet/*)
file(GLOB_RECURSE PaymentService payment_service/*)

source_group("" FILES ${Common} ${ConnectivityTool} ${Crypto} ${CryptoNote} ${CryptoNoteCore} ${CryptoNoteProtocol} ${Daemon} ${Http} ${Logging} ${NodeRpcProxy} ${P2p} ${Rpc} ${Serialization} ${SimpleWallet} ${System} ${Transfers} ${Wallet})

add_library(Common ${Common})
add_library(Crypto ${Crypto})
add_library(CryptoNote ${CryptoNote})
add_library(CryptoNoteCore ${CryptoNoteCore})
add_library(Http ${Http})
add_library(InProcessNode ${InProcessNode})
add_library(Logging ${Logging})
add_library(NodeRpcProxy ${NodeRpcProxy})
add_library(Rpc ${Rpc})
add_library(P2P ${CryptoNoteProtocol} ${P2p})
add_library(Serialization ${Serialization})
add_library(System ${System})
add_library(Transfers ${Transfers})
add_library(Wallet ${Wallet})

add_executable(ConnectivityTool ${ConnectivityTool} p2p/LevinProtocol.cpp p2p/LevinProtocol.h)
add_executable(Daemon ${Daemon})
add_executable(SimpleWallet ${SimpleWallet})
add_executable(PaymentGate ${PaymentService})

target_link_libraries(ConnectivityTool epee Common Crypto Rpc Http System ${Boost_LIBRARIES})
target_link_libraries(Daemon epee CryptoNoteCore P2P Rpc Serialization System Http Logging Common Crypto upnpc-static ${Boost_LIBRARIES})
target_link_libraries(SimpleWallet epee Wallet NodeRpcProxy Transfers Rpc Http Serialization CryptoNoteCore System Logging Common Crypto ${Boost_LIBRARIES})
target_link_libraries(PaymentGate epee Wallet NodeRpcProxy Transfers CryptoNoteCore Crypto P2P Rpc Http Serialization System Logging Common InProcessNode upnpc-static ${Boost_LIBRARIES})

add_dependencies(Rpc version)

add_dependencies(ConnectivityTool version)
add_dependencies(Daemon version)
add_dependencies(SimpleWallet version)
add_dependencies(PaymentGate version)
add_dependencies(P2P version)

set_property(TARGET ConnectivityTool PROPERTY OUTPUT_NAME "connectivity_tool")
set_property(TARGET Daemon PROPERTY OUTPUT_NAME "forknoted")
set_property(TARGET SimpleWallet PROPERTY OUTPUT_NAME "simplewallet")
set_property(TARGET PaymentGate PROPERTY OUTPUT_NAME "walletd")
