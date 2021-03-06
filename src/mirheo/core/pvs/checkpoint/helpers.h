#pragma once

#include <mirheo/core/pvs/data_manager.h>
#include <mirheo/core/xdmf/xdmf.h>

#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace mirheo
{

namespace XDMF {struct Channel;}

namespace CheckpointHelpers
{

std::tuple<std::vector<real3>,
           std::vector<real3>,
           std::vector<int64_t>>
splitAndShiftPosVel(const DomainInfo &domain,
                    const PinnedBuffer<real4>& pos4,
                    const PinnedBuffer<real4>& vel4);

std::tuple<std::vector<real3>, std::vector<RigidReal4>,
           std::vector<RigidReal3>, std::vector<RigidReal3>,
           std::vector<RigidReal3>, std::vector<RigidReal3>>
splitAndShiftMotions(DomainInfo domain, const PinnedBuffer<RigidMotion>& motions);

std::vector<XDMF::Channel> extractShiftPersistentData(const DomainInfo& domain,
                                                      const DataManager& extraData,
                                                      const std::set<std::string>& blackList={});

} // namespace CheckpointHelpers

} // namespace mirheo
