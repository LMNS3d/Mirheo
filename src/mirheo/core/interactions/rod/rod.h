#pragma once

#include "base_rod.h"
#include "drivers_forces.h"
#include "drivers_states.h"
#include "kernels/parameters.h"
#include "polymorphic_states.h"

#include <mirheo/core/pvs/rod_vector.h>
#include <mirheo/core/pvs/views/rv.h>
#include <mirheo/core/utils/cpu_gpu_defines.h>
#include <mirheo/core/utils/cuda_common.h>
#include <mirheo/core/utils/kernel_launch.h>

namespace mirheo
{

static auto getBoundParams(const RodParameters& p)
{
    GPU_RodBoundsParameters dp;
    dp.ksCenter = p.ksCenter;
    dp.ksFrame  = p.ksFrame;
    dp.lcenter  = p.l0;
    dp.lcross   = p.a0;
    dp.lring    = 0.5 * math::sqrt(2.0) * p.a0;
    dp.ldiag    = 0.5 * math::sqrt(p.a0*p.a0 + p.l0*p.l0);
    return dp;
}

template <int Nstates>
static auto getBiSegmentParams(const RodParameters& p)
{
    GPU_RodBiSegmentParameters<Nstates> dp;
    dp.kBending = p.kBending;
    dp.kTwist   = p.kTwist;

    for (size_t i = 0; i < p.kappaEq.size(); ++i)
    {
        dp.kappaEq[i]  = p.kappaEq[i];
        dp.tauEq[i]    = p.tauEq[i];
        dp.groundE[i]  = p.groundE[i];
    }
    return dp;
}

template <int Nstates, class StateParameters>
class RodInteraction : public BaseRodInteraction
{
public:
    RodInteraction(const MirState *state, const std::string& name, RodParameters parameters,
                   StateParameters stateParameters, bool saveEnergies) :
        BaseRodInteraction(state, name),
        parameters_(parameters),
        stateParameters_(stateParameters),
        saveEnergies_(saveEnergies)
    {}

    ~RodInteraction() = default;

    void setPrerequisites(ParticleVector *pv1,
                          __UNUSED ParticleVector *pv2,
                          __UNUSED CellList *cl1,
                          __UNUSED CellList *cl2) override
    {
        if (auto rv = dynamic_cast<RodVector*>(pv1))
        {
            if (saveEnergies_)
                rv->requireDataPerBisegment<real>(ChannelNames::energies,   DataManager::PersistenceMode::None);

            if (Nstates > 1)
            {
                rv->requireDataPerBisegment<int>    (ChannelNames::polyStates, DataManager::PersistenceMode::None);
                rv->requireDataPerBisegment<real4>  (ChannelNames::rodKappa,   DataManager::PersistenceMode::None);
                rv->requireDataPerBisegment<real2>  (ChannelNames::rodTau_l,   DataManager::PersistenceMode::None);
            }
        }
        else
        {
            die("'%s' expects a Rod vector, given '%s'", this->getCName(), pv1->getCName());
        }
    }
    
    void local(ParticleVector *pv1,
               __UNUSED ParticleVector *pv2,
               __UNUSED CellList *cl1,
               __UNUSED CellList *cl2,
               cudaStream_t stream) override
    {
        auto rv = dynamic_cast<RodVector*>(pv1);

        debug("Computing internal rod forces for %d rods of '%s'",
              rv->local()->getNumObjects(), rv->getCName());

        _computeBoundForces                    (rv, stream);
        _updatePolymorphicStatesAndApplyForces (rv, stream);
        _computeElasticForces                  (rv, stream);
    }
    
private:
    void _computeBoundForces(RodVector *rv, cudaStream_t stream)
    {
        RVview view(rv, rv->local());

        const int nthreads = 128;
        const int nblocks  = getNblocks(view.nObjects * view.nSegments, nthreads);
        
        auto devParams = getBoundParams(parameters_);
        
        SAFE_KERNEL_LAUNCH(RodForcesKernels::computeRodBoundForces,
                           nblocks, nthreads, 0, stream,
                           view, devParams);
    }

    void _updatePolymorphicStatesAndApplyForces(RodVector *rv, cudaStream_t stream)
    {
        if (Nstates > 1)
        {
            RVview view(rv, rv->local());
            auto devParams = getBiSegmentParams<Nstates>(parameters_);

            auto kappa = rv->local()->dataPerBisegment.getData<real4>(ChannelNames::rodKappa)->devPtr();
            auto tau_l = rv->local()->dataPerBisegment.getData<real2>(ChannelNames::rodTau_l)->devPtr();

            const int nthreads = 128;
            const int nblocks  = getNblocks(view.nObjects * (view.nSegments-1), nthreads);
                
            SAFE_KERNEL_LAUNCH(RodStatesKernels::computeBisegmentData,
                               nblocks, nthreads, 0, stream,
                               view, kappa, tau_l);

            updateStatesAndApplyForces<Nstates>(rv, devParams, stateParameters_, stream);
        }
    }
    
    void _computeElasticForces(RodVector *rv, cudaStream_t stream)
    {
        RVview view(rv, rv->local());
        auto devParams = getBiSegmentParams<Nstates>(parameters_);
        
        const int nthreads = 128;
        const int nblocks  = getNblocks(view.nObjects * (view.nSegments-1), nthreads);

        SAFE_KERNEL_LAUNCH(RodForcesKernels::computeRodBiSegmentForces<Nstates>,
                           nblocks, nthreads, 0, stream,
                           view, devParams, saveEnergies_);
    }

private:
    RodParameters parameters_;
    StateParameters stateParameters_;
    bool saveEnergies_;
};

} // namespace mirheo
