add_subdirectory(core)

find_package(PkgConfig REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(cppnetlib 0.12 EXACT REQUIRED)
find_package(libtins REQUIRED)
find_package(TinyProcess REQUIRED)

pkg_check_modules(FLUID_BASE REQUIRED libfluid_base)
pkg_check_modules(FLUID_MSG REQUIRED libfluid_msg)
pkg_check_modules(EDITLINE REQUIRED libedit)

find_path(RANGE_V3_INCLUDE_DIRS
    NAMES meta/meta.hpp range/v3/all.hpp
)

add_library(runos STATIC
    # API
    Config.hpp
    Config.cc
    IGMP.hpp
    ArpTable.hpp
    api/OFConnection.hpp
    api/Port.hpp
    api/Statistics.hpp
    api/Switch.hpp
    api/OFDriver.hpp
    api/FunctionalTraits.hpp
    # Base
    lib/ethaddr.cc
    lib/ipv4addr.cc
    lib/pipe_exec.cc
    Application.cc
    Application.hpp
    Controller.cc
    Controller.hpp
    FluidOXMAdapter.cc
    FluidOXMAdapter.hpp
    Loader.cc
    Loader.hpp
    # Aux
    lib/action_parsing.cc
    lib/action_parsing.hpp
    lib/poller.cc
    lib/poller.hpp

    OFDPA.hpp
    LinkDiscoveryDriver.cc
    LinkDiscoveryDriver.hpp

    Logger.cc
    Logger.hpp
    OFAgentImpl.cc
    OFAgentImpl.hpp
    OFServer.cc
    OFServer.hpp
    OFDriver.cc
    PacketParser.cc
    PacketParser.hpp
    PortImpl.cc
    PortImpl.hpp
    StatsBucket.cc
    StatsBucket.hpp
    PropertySheet.cc
    Utility.cc
    ResourceLocator.cc
    DeviceDb.cc

    # Apps
    TunnelMaker.cc
    TunnelMaker.hpp

    CiscoDiscovery.cc
    CiscoDiscovery.hpp
    DatabaseConnector.cc
    DatabaseConnector.hpp
    DOSControl.cc
    DOSControl.hpp
    FlowEntriesVerifier.cc
    FlowEntriesVerifier.hpp
    InBand.cc
    InBand.hpp
    InBandRouteBlackList.cc
    InBandRouteBlackList.hpp
    IdGen.cc
    LAGBalancer.cc
    LAGBalancer.hpp
    LinkDiscovery.cc
    LinkDiscovery.hpp
    MirrorManager.cc
    MirrorManager.hpp
    Multicast.cc
    Multicast.hpp
    OFMsgSender.cc
    OFMsgSender.hpp
    OVSLacp.cc
    OVSLacp.hpp
    OVSQueues.cc
    OVSQueues.hpp
    QoSClassifier.cc
    QoSClassifier.hpp
    RuntimeConfigManager.cc
    RuntimeConfigManager.hpp
    SnmpAgent.cc
    SnmpAgent.hpp
    StatsRulesManager.cc
    StatsRulesManager.hpp
    StormControl.cc
    StormControl.hpp
    SwitchImpl.cc
    SwitchImpl.hpp
    SwitchManager.cc
    SwitchManager.hpp
    SwitchOrdering.cc
    SwitchOrdering.hpp
    Topology.cc
    Topology.hpp
    RTKManager.cc
    RTKManager.hpp
    SdwanManager.cc
    SdwanManager.hpp
    IcmpManager.cc
    IcmpManager.hpp
    OvsBfd.cc
    OvsBfd.hpp
    OvsBfdDefinitions.hpp
    ArpManager.cc
    ArpManager.hpp

    DpidChecker.cc
    DpidChecker.hpp
    DpidDatabaseSync.cc
    DpidDatabaseSync.hpp

    # HA
    Recovery.cc
    Recovery.hpp
    RecoveryModeChecker.cc
    RecoveryModeChecker.hpp
)

add_library(runos_cli STATIC
    CommandLine.cc
    CommandLine.hpp
    LinkDiscoveryCli.cc
    OFServerCli.cc
    RuntimeConfigManagerCli.cc
    SwitchManagerCli.cc
    QoSClassifierCli.cc
    IcmpManagerCli.cc
)

add_library(runos_rest STATIC
    CiscoDiscoveryRest.cc
    DOSControlRest.cc
    LAGBalancerRest.cc
    LinkDiscoveryRest.cc
    MirrorManagerRest.cc
    MulticastRest.cc
    OVSLacpRest.cc
    OVSQueuesRest.cc
    QoSClassifierRest.cc
    RTKManagerRest.cc
    OFServerRest.cc
    RecoveryRest.cc
    RestListener.cc
    RestListener.hpp
    StatsBucketRest.cc
    StatsRulesManagerRest.cc
    StormControlRest.cc
    SwitchManagerRest.cc
    TopologyRest.cc
    InBandRest.cc
    FlowTableRest.cc
    GroupTableRest.cc
    MeterTableRest.cc
    AuxDevicesRest.cc
    SdwanManagerRest.cc
    OvsBfdRest.cc
)

#add_library(runos_drivers STATIC
#    drivers/OFDPA.hpp
#    drivers/LinkDiscoveryDriver.сpp
#    drivers/LinkDiscoveryDriver.hpp
#)

# Instruct CMake to run moc automatically when needed.
target_include_directories(runos SYSTEM
    PUBLIC
      ${RANGE_V3_INCLUDE_DIRS}
      ${FLUID_BASE_INCLUDE_DIRS}
      ${FLUID_MSG_INCLUDE_DIRS}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR} # for Qt-generated moc files
      ${GLOG_INCLUDE_DIRS}
      ${LIBTINS_INCLUDE_DIRS}
    )

target_include_directories(runos_cli SYSTEM
    PRIVATE
      ${EDITLINE_INCLUDE_DIRS}
    )

target_include_directories(runos_rest SYSTEM
    PUBLIC
      ${CPPNETLIB_INCLUDE_DIRS}
    )

link_directories(
    ${EDITLINE_LIBRARY_DIRS}
    ${FLUID_BASE_LIBRARY_DIRS}
    ${FLUID_MSG_LIBRARY_DIRS}
    ${GLOG_LIBRARY_DIRS}
)

target_compile_definitions(runos
    PUBLIC ${BOOST_THREAD_CONFIG}
    )

target_link_libraries(runos
    PUBLIC
      Qt5::Core
      runos_core
    PRIVATE
      ${FLUID_BASE_LIBRARIES}
      ${FLUID_MSG_LIBRARIES}
      ${GLOG_LIBRARIES}
      ${LIBTINS_LIBRARIES}
      snmp++
      agent++
      TinyProcess::TinyProcess
      heartbeatcore
      redisdb
      cpqd_print
      domain_drivers
      oxm
      curl
      curlpp
    )

target_link_libraries(runos_cli
    PRIVATE
      runos
      ${EDITLINE_LIBRARIES}
    )

target_link_libraries(runos_rest
    PRIVATE
      runos
      ${CPPNETLIB_LIBRARIES}
    )

set_target_properties(runos PROPERTIES
    LANGUAGE CXX
    AUTOMOC ON
    )

add_subdirectory(domains)
add_subdirectory(hb)
add_subdirectory(redisdb)
add_subdirectory(oflog)
add_subdirectory(oxm)
