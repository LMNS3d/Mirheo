#pragma once

#include <core/pvs/data_manager.h>
#include <core/utils/cpu_gpu_defines.h>

#include <cassert>
#include <functional>

using PackPredicate = std::function< bool (const DataManager::NamedChannelDesc&) >;

/**
 * Class that packs nChannels of arbitrary data into a chunk of contiguous memory
 * or unpacks it in the same manner
 */
struct DevicePacker
{
    int packedSize_byte = 0;

    int nChannels    = 0;        ///< number of data channels to pack / unpack
    CudaVarPtr *channelData = nullptr;  ///< device pointers of the packed data

    /**
     * Pack entity with id srcId into memory starting with dstAddr
     * with no shift
     */
    inline __D__ void pack(int srcId, char *dstAddr) const
    {
        for (int i = 0; i < nChannels; i++)
        {
            cuda_variant::apply_visitor([&](auto srcPtr)
            {
                using T = typename std::remove_pointer<decltype(srcPtr)>::type;
                *reinterpret_cast<T*>(dstAddr) = srcPtr[srcId];
                dstAddr += paddedSize<T>();
            }, channelData[i]);
        }
    }

    /**
     * Pack entity with id srcId into another packer at id dstId
     * with no shift
     * Assumes that the 2 packers contain the same channels
     */
    inline __D__ void pack(int srcId, DevicePacker& dst, int dstId) const
    {
        assert (nChannels == dst.nChannels);
        
        for (int i = 0; i < nChannels; i++)
        {
            cuda_variant::apply_visitor([&](auto srcPtr)
            {
                using T = typename std::remove_pointer<decltype(srcPtr)>::type;
                auto dstPtr = cuda_variant::get_alternative<T*>(dst.channelData[i]);

                dstPtr[dstId]= srcPtr[srcId];
                
            }, channelData[i]);
        }
    }    
    
    /**
     * Unpack entity from memory by srcAddr to the channels to id dstId
     */
    inline __D__ void unpack(const char *srcAddr, int dstId) const
    {
        for (int i = 0; i < nChannels; i++)
        {
            cuda_variant::apply_visitor([&](auto dstPtr)
            {
                using T = typename std::remove_pointer<decltype(dstPtr)>::type;
                dstPtr[dstId] = *reinterpret_cast<const T*>(srcAddr);
                srcAddr += paddedSize<T>();
            }, channelData[i]);
        }
    }

    
private:

    template <typename T, typename PaddedType = float4>
    inline __D__ size_t paddedSize() const
    {
        constexpr int npads = (sizeof(T) + sizeof(PaddedType) - 1) / sizeof(PaddedType);
        return npads * sizeof(PaddedType);
    }
    
protected:

    void registerChannel (DataManager& manager, CudaVarPtr varPtr, bool& needUpload, cudaStream_t stream);
    void registerChannels(PackPredicate predicate, DataManager& manager, const std::string& pvName, bool& needUpload, cudaStream_t stream);
    void setAndUploadData(DataManager& manager, bool needUpload, cudaStream_t stream);
};