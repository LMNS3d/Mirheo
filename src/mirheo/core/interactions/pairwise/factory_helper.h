#pragma once

#include "kernels/parameters.h"

#include <mirheo/core/interactions/parameters_wrap.h>

#include <limits>

namespace mirheo
{

namespace FactoryHelper
{
constexpr auto defaultReal = std::numeric_limits<real>::infinity();

struct ParamsReader
{
    enum Mode {FailIfNotFound, DefaultIfNotFound};
    
    template <typename T>
    T read(ParametersWrap& desc, const std::string& key) const
    {
        if (mode == Mode::FailIfNotFound)
        {
            return desc.read<T>(key);
        }
        else
        {
            if (desc.exists<T>(key))
                return desc.read<T>(key);
            else
                return makeDefault<T>();
        }
    }

    Mode mode {Mode::FailIfNotFound};

    template <class T> T makeDefault() const;
};


template <class Params> void readParams(Params& p, ParametersWrap& desc, ParamsReader reader);

DPDParams       readDPDParams     (ParametersWrap& desc);
LJParams        readLJParams      (ParametersWrap& desc);
MDPDParams      readMDPDParams    (ParametersWrap& desc);
DensityParams   readDensityParams (ParametersWrap& desc);
SDPDParams      readSDPDParams    (ParametersWrap& desc);

VarStressParams readStressParams  (ParametersWrap& desc);



template <class Params>
void readSpecificParams(Params& p, ParametersWrap& desc)
{
    using namespace FactoryHelper;
    readParams(p, desc, {ParamsReader::Mode::DefaultIfNotFound});
}

void readSpecificParams(LJParams&      p, ParametersWrap& desc);
void readSpecificParams(DensityParams& p, ParametersWrap& desc);
void readSpecificParams(SDPDParams&    p, ParametersWrap& desc);

} // FactoryHelper

} // namespace mirheo
