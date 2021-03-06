#pragma once

#include <mirheo/core/mirheo_object.h>

#include <mpi.h>
#include <vector>

namespace mirheo
{

class Simulation;

class Plugin
{    
public:
    Plugin();
    virtual ~Plugin();
    
    virtual void handshake();

    void setTag(int tag);
    
protected:
    MPI_Comm comm_, interComm_;
    int rank_, nranks_;
    
    void _setup(const MPI_Comm& comm, const MPI_Comm& interComm);

    int _sizeTag() const;
    int _dataTag() const;

private:
    void _checkTag() const;
    static constexpr int invalidTag = -1;
    int tag_ {invalidTag};
};

class SimulationPlugin : public Plugin, public MirSimulationObject
{
public:
    SimulationPlugin(const MirState *state, const std::string& name);
    virtual ~SimulationPlugin();

    virtual void beforeCellLists            (cudaStream_t stream);
    virtual void beforeForces               (cudaStream_t stream);
    virtual void beforeIntegration          (cudaStream_t stream);
    virtual void afterIntegration           (cudaStream_t stream);
    virtual void beforeParticleDistribution (cudaStream_t stream);

    virtual void serializeAndSend (cudaStream_t stream);

    virtual bool needPostproc() = 0;

    virtual void setup(Simulation *simulation, const MPI_Comm& comm, const MPI_Comm& interComm);
    virtual void finalize();    

protected:
    void waitPrevSend();
    void send(const std::vector<char>& data);
    void send(const void *data, size_t sizeInBytes);

    /** \brief Implementation of the snapshot saving. Reusable by potential derived classes.
        \param [in,out] saver The \c Saver object. Provides save context and serialization functions.
        \param [in] typeName The name of the type being saved.
      */
    ConfigObject _saveSnapshot(Saver& saver, const std::string& typeName);

private:
    int localSendSize_;
    MPI_Request sizeReq_, dataReq_;
};




class PostprocessPlugin : public Plugin, public MirObject
{
public:
    PostprocessPlugin(const std::string& name);
    virtual ~PostprocessPlugin();

    MPI_Request waitData();
    void recv();
    
    virtual void deserialize();

    virtual void setup(const MPI_Comm& comm, const MPI_Comm& interComm);    

protected:
    /** \brief Implementation of the snapshot saving. Reusable by potential derived classes.
        \param [in,out] saver The \c Saver object. Provides save context and serialization functions.
        \param [in] typeName The name of the type being saved.
      */
    ConfigObject _saveSnapshot(Saver& saver, const std::string& typeName);

    std::vector<char> data_;
private:
    int size_;
};

} // namespace mirheo
