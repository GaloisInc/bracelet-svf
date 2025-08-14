#ifndef Bracelet_H_
#define Bracelet_H_

#include "MemoryModel/PointerAnalysisImpl.h"

namespace SVF
{

class SVFG;

class BraceletPass
{
    typedef std::vector<PointerAnalysis*> PTAVector;

public:
    /// Pass ID
    static char ID;

    BraceletPass() {}

    /// Destructor
    virtual ~BraceletPass();

    /// Run pointer analysis on SVFModule
    virtual void runOnModule(SVFIR* svfModule);

    /// PTA name
    virtual inline std::string getPassName() const
    {
        return "BraceletPass";
    }

private:
    void runPointerAnalysis(SVFIR* pag, u32_t kind);
    void writeBraceletPtsTo(PointerAnalysis *pta, const std::string &filename);
    void writeBraceletCallGraph(PointerAnalysis *pta, const std::string &filename);

    PTAVector ptaVector;
    PointerAnalysis* _pta;	///<  pointer analysis to be executed.
    SVFG* _svfg;  ///< svfg generated through -ander pointer analysis
};

} // End namespace SVF

#endif /* Bracelet_H_ */
