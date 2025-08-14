#include "SVF-LLVM/LLVMUtil.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Bracelet/BraceletPass.h"
#include "Util/CommandLine.h"
#include "Util/Options.h"


using namespace llvm;
using namespace std;
using namespace SVF;

int main(int argc, char** argv)
{
    auto moduleNameVec =
        OptionBase::parseOptions(argc, argv, "Bracelet Points-to Analysis",
                                 "[options] <input-bitcode...>");

    // Refers to content of a singleton unique_ptr<SVFIR> in SVFIR.
    SVFIR* pag;

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    /// Build SVFIR
    SVFIRBuilder builder;
    pag = builder.build();

    BraceletPass bp;
    bp.runOnModule(pag);

    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}
