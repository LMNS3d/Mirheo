#pragma once

#include "../fetchers.h"
#include "../parameters.h"

#include <mirheo/core/utils/cpu_gpu_defines.h>
#include <mirheo/core/utils/helper_math.h>

#include <cmath>

namespace mirheo
{

class DihedralJuelicher : public VertexFetcherWithMeanCurvatures
{
public:    

    using ParametersType = JuelicherBendingParameters;
    
    DihedralJuelicher(ParametersType p, mReal lscale) :
        scurv_(0)
    {
        kb_    = p.kb         * lscale*lscale;
        kadPi_ = p.kad * M_PI * lscale*lscale;

        H0_  = p.C0 / (2*lscale);        
        DA0_ = p.DA0 / (lscale*lscale);
    }

    __D__ inline void computeCommon(const ViewType& view, int rbcId)
    {
        scurv_ = _getScurv(view, rbcId);
    }
    
    __D__ inline mReal3 operator()(VertexType v0, VertexType v1, VertexType v2, VertexType v3, mReal3 &f1) const
    {
        mReal3 f0;
        const mReal theta = supplementaryDihedralAngle(v0.r, v1.r, v2.r, v3.r);
        
        f0  = _forceLen   (theta, v0,     v2        );
        f0 += _forceTheta (       v0, v1, v2, v3, f1);
        f0 += _forceArea  (       v0, v1, v2        );

        return f0;
    }

private:

    __D__ inline mReal3 _forceLen(mReal theta, VertexType v0, VertexType v2) const
    {
        const mReal3 d = normalize(v0.r - v2.r);
        return - ( kb_ * (v0.H + v2.H - 2 * H0_) + kadPi_ * scurv_ ) * theta * d;
    }

    __D__ inline mReal3 _forceTheta(VertexType v0, VertexType v1, VertexType v2, VertexType v3, mReal3 &f1) const
    {
        const mReal3 v20 = v0.r - v2.r;
        const mReal3 v21 = v1.r - v2.r;
        const mReal3 v23 = v3.r - v2.r;
    
        const mReal3 n = cross(v21, v20);
        const mReal3 k = cross(v20, v23);

        const mReal inv_lenn = math::rsqrt(dot(n,n));
        const mReal inv_lenk = math::rsqrt(dot(k,k));

        const mReal cotangent2n = dot(v20, v21) * inv_lenn;
        const mReal cotangent2k = dot(v23, v20) * inv_lenk;
    
        const mReal3 d1 = (dot(v20, v20)  * inv_lenn*inv_lenn) * n;
        const mReal3 d0 =
            (-cotangent2n * inv_lenn) * n +
            (-cotangent2k * inv_lenk) * k;

        const mReal coef = kb_ * (v0.H + v2.H - 2*H0_)  +  kadPi_ * scurv_;

        f1 = -coef * d1;
        return -coef * d0;
    }

    __D__ inline mReal3 _forceArea(VertexType v0, VertexType v1, VertexType v2) const
    {
        const mReal coef =
            0.6666667_mr * kb_     * (v0.H * v0.H + v1.H * v1.H + v2.H * v2.H - 3 * H0_ * H0_)
            + 0.5_mr     * kadPi_ * scurv_ * scurv_;

        const mReal3 n  = normalize(cross(v1.r-v0.r, v2.r-v0.r));
        const mReal3 d0 = 0.5_mr * cross(n, v2.r - v1.r);

        return coef * d0;
    }

    __D__ inline mReal _getScurv(const ViewType& view, int rbcId) const
    {
        const mReal totArea     = view.area_volumes[rbcId].x;
        const mReal totLenTheta = view.lenThetaTot [rbcId];

        return (0.5_mr * totLenTheta - DA0_) / totArea;
    }

    
private:    
    
    mReal kb_;
    mReal H0_;
    mReal kadPi_; ///< kAD * Pi
    mReal DA0_;
    mReal scurv_;
};

MIRHEO_TYPE_NAME_AUTO(DihedralJuelicher);

} // namespace mirheo
