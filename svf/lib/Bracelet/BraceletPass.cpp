#include "Util/Options.h"
#include "MemoryModel/PointerAnalysisImpl.h"
#include "Bracelet/BraceletPass.h"
#include "Graphs/GraphTraits.h"
#include "WPA/Andersen.h"
#include "WPA/AndersenPWC.h"
#include "WPA/FlowSensitive.h"
#include "WPA/VersionedFlowSensitive.h"
#include "WPA/TypeAnalysis.h"
#include "WPA/Steensgaard.h"

#include <optional>
#include <regex>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <variant>

using namespace SVF;

char BraceletPass::ID = 0;

/*!
 * Destructor
 */
BraceletPass::~BraceletPass()
{
    PTAVector::const_iterator it = ptaVector.begin();
    PTAVector::const_iterator eit = ptaVector.end();
    for (; it != eit; ++it)
    {
        PointerAnalysis* pta = *it;
        delete pta;
    }
    ptaVector.clear();
}

/*!
 * We start from here
 */
void BraceletPass::runOnModule(SVFIR* pag)
{
    for (u32_t i = 0; i<= PointerAnalysis::Default_PTA; i++)
    {
        PointerAnalysis::PTATY iPtaTy = static_cast<PointerAnalysis::PTATY>(i);
        if (Options::PASelected(iPtaTy))
            runPointerAnalysis(pag, i);
    }
    assert(!ptaVector.empty() && "No pointer analysis is specified.\n");
}

/*!
 * Create pointer analysis according to a specified kind and then analyze the module.
 */
void BraceletPass::runPointerAnalysis(SVFIR* pag, u32_t kind)
{
    /// Initialize pointer analysis.
    switch (kind)
    {
    case PointerAnalysis::Andersen_WPA:
        _pta = new Andersen(pag);
        break;
    case PointerAnalysis::AndersenSCD_WPA:
        _pta = new AndersenSCD(pag);
        break;
    case PointerAnalysis::AndersenSFR_WPA:
        _pta = new AndersenSFR(pag);
        break;
    case PointerAnalysis::AndersenWaveDiff_WPA:
        _pta = new AndersenWaveDiff(pag);
        break;
    case PointerAnalysis::Steensgaard_WPA:
        _pta = new Steensgaard(pag);
        break;
    case PointerAnalysis::FSSPARSE_WPA:
        _pta = new FlowSensitive(pag);
        break;
    case PointerAnalysis::VFS_WPA:
        _pta = new VersionedFlowSensitive(pag);
        break;
    case PointerAnalysis::TypeCPP_WPA:
        _pta = new TypeAnalysis(pag);
        break;
    default:
        assert(false && "This pointer analysis has not been implemented yet.\n");
        return;
    }

    ptaVector.push_back(_pta);
    _pta->analyze();

    if (!Options::BraceletPtsTo().empty()) {
      writeBraceletPtsTo(_pta, Options::BraceletPtsTo());
    }

    if (!Options::BraceletCallGraph().empty()) {
      writeBraceletCallGraph(_pta, Options::BraceletCallGraph());
    }
}

typedef std::pair<size_t, size_t> LocalName;
typedef std::variant<size_t, LocalName> NodeName;

std::optional<NodeName> parseName(const std::string &name) {
  const std::regex local_regex("local_([0-9]+)_NODE_0x([0-9a-f]+)",
                               std::regex_constants::extended |
                               std::regex_constants::icase);
  const std::regex node_regex("NODE_0x([0-9a-f]+)",
                              std::regex_constants::extended |
                              std::regex_constants::icase);

  std::smatch local_match, node_match;
  if (std::regex_match(name, local_match, local_regex)) {
    size_t local = std::stoll(local_match[1].str());
    std::stringstream ss;
    ss << std::hex << local_match[2].str();
    size_t addr;
    ss >> addr;
    return NodeName(std::pair(addr, local));
  } else if (std::regex_match(name, node_match, node_regex)) {
    std::stringstream ss;
    ss << std::hex << node_match[1].str();
    size_t addr;
    ss >> addr;
    return NodeName(addr);
  } else {
    return std::nullopt;
  }
}

static void writeHex(std::ostream &out, size_t value) {
  out << "0x" << std::hex << std::setfill('0') << std::setw(16) << value;
  out << std::dec;
}

static void writeHexShort(std::ostream &out, size_t value) {
  out << "0x" << std::hex << std::setfill('0') << std::setw(4) << value;
  out << std::dec;
}

void BraceletPass::writeBraceletPtsTo(PointerAnalysis *pta, const std::string &filename) {
  std::fstream out(filename.c_str(), std::ios_base::out);
  if (!out.good()) {
    std::cerr << "error opening file \"" << filename << "\"for writing!\n";
    return;
  }

  SVFIR *pag = pta->getPAG();

  std::unordered_map<NodeID,std::variant<size_t, std::pair<size_t,size_t>>> node_mapping;

  for (const auto &item: *pag) {
    NodeID id = item.first;
    SVFVar *var = item.second;

    if (const HeapObjVar *hov = SVFUtil::dyn_cast<HeapObjVar>(var)) {
      if (hov->hasOutgoingEdges(SVFStmt::Addr)) {
        for (auto it = hov->getOutgoingEdgesBegin(SVFStmt::Addr),
               eit = hov->getOutgoingEdgesEnd(SVFStmt::Addr);
             it != eit; ++it) {
          const ValVar *vv = SVFUtil::cast<ValVar>((*it)->getDstNode());
          if (vv->hasOutgoingEdges(SVFStmt::Store)) {
            for (auto vit = vv->getOutgoingEdgesBegin(SVFStmt::Store),
                   evit = vv->getOutgoingEdgesEnd(SVFStmt::Store);
                 vit != evit; ++vit) {
              const GlobalValVar *gvv = SVFUtil::cast<GlobalValVar>((*vit)->getDstNode());
              if (auto node = parseName(gvv->getName())) {
                node_mapping[id] = *node;
              }
            }
          }
        }
      }
    } else if (const GlobalObjVar *gov = SVFUtil::dyn_cast<GlobalObjVar>(var)) {
      if (gov->hasOutgoingEdges(SVFStmt::Addr)) {
        for (auto it = gov->getOutgoingEdgesBegin(SVFStmt::Addr),
               eit = gov->getOutgoingEdgesEnd(SVFStmt::Addr);
             it != eit; ++it) {
          const GlobalValVar *gvv = SVFUtil::cast<GlobalValVar>((*it)->getDstNode());
          if (auto node = parseName(gvv->getName())) {
            node_mapping[id] = *node;
          }
        }
      }
    } else if (const FunObjVar *fov = SVFUtil::dyn_cast<FunObjVar>(var)) {
      if (fov->hasOutgoingEdges(SVFStmt::Addr)) {
        for (auto it = fov->getOutgoingEdgesBegin(SVFStmt::Addr),
               eit = fov->getOutgoingEdgesEnd(SVFStmt::Addr);
             it != eit; ++it) {
          const FunValVar *fvv = SVFUtil::cast<FunValVar>((*it)->getDstNode());
          if (auto node = parseName(fvv->getName())) {
            node_mapping[id] = *node;
          }
        }
      }
    } else if (const ValVar *vv = SVFUtil::dyn_cast<ValVar>(var)) {
      if (auto node = parseName(vv->getName())) {
        node_mapping[id] = *node;
      }
    }
  }

  for (const auto &item: *pag) {
    NodeID id = item.first;
    SVFVar *var = item.second;

    if (SVFUtil::isa<GlobalValVar>(var)
        || SVFUtil::isa<FunValVar>(var)
        || SVFUtil::isa<GlobalObjVar>(var)
        || SVFUtil::isa<FunObjVar>(var)) {
      if (node_mapping.find(id) != node_mapping.end()) {
        auto node = node_mapping[id];
        const PointsTo &pt = pta->getPts(id);
        for (NodeID target : pt) {
          auto targetNode = node_mapping.find(target);
          if (targetNode != node_mapping.end()) {
            if (std::holds_alternative<LocalName>(node)) {
              LocalName ln = std::get<LocalName>(node);
	      writeHex(out, ln.first);
              out << "\t" << ln.second << "\t";
            } else {
	      writeHex(out, std::get<size_t>(node));
              out << "\t-\t";
            }
            if (std::holds_alternative<LocalName>(targetNode->second)) {
              LocalName ln = std::get<LocalName>(targetNode->second);
	      writeHex(out, ln.first);
	      out << "\t" << ln.second << "\n";
            } else {
	      writeHex(out, std::get<size_t>(targetNode->second));
	      out << "\t-\n";
            }
          }
        }
      }
    }
  }

  return;
}

void BraceletPass::writeBraceletCallGraph(PointerAnalysis *pta, const std::string &filename) {
  std::fstream out(filename.c_str(), std::ios_base::out);
  if (!out.good()) {
    std::cerr << "error opening file \"" << filename << "\"for writing!\n";
    return;
  }
  SVFIR *pag = pta->getPAG();
  CallGraph *cg = pta->getCallGraph();
  std::string ndc_name = "__nondeterministic_choice";
  const CallGraphNode *ndc = cg->getCallGraphNode(ndc_name);
  for (const auto& item : cg->getCallInstToCallGraphEdgesMap()) {
    for (const CallGraphEdge *edge : item.second) {
      const CallGraphNode *dst = cg->getCallGraphNode(edge->getDstID());
      if (dst == ndc)
	continue;
      if (!pag->callsiteHasRet(item.first->getRetICFGNode()))
	continue;
      const ValVar *src_rv = SVFUtil::cast<ValVar>(pag->getCallSiteRet(item.first->getRetICFGNode()));
      if (!src_rv->hasOutgoingEdges(SVFStmt::Store))
	continue;
      const ValVar *src_vv = NULL;
      for (auto it = src_rv->getOutgoingEdgesBegin(SVFStmt::Store),
	     eit = src_rv->getOutgoingEdgesEnd(SVFStmt::Store);
	   it != eit; ++it) {
	src_vv = SVFUtil::cast<ValVar>((*it)->getDstNode());
      }
      assert(src_vv != NULL);
      auto srcNodeName = parseName(src_vv->getName());
      const FunObjVar *dst_fov = dst->getFunction();
      const FunValVar *dst_fvv = NULL;
      assert(dst_fov->hasOutgoingEdges(SVFStmt::Addr));
      for (auto it = dst_fov->getOutgoingEdgesBegin(SVFStmt::Addr),
             eit = dst_fov->getOutgoingEdgesEnd(SVFStmt::Addr);
           it != eit; ++it) {
        dst_fvv = SVFUtil::cast<FunValVar>((*it)->getDstNode());
      }
      auto dstNodeName = parseName(dst_fvv->getName());
      if (srcNodeName) {
        if (std::holds_alternative<LocalName>(*srcNodeName)) {
          LocalName ln = std::get<LocalName>(*srcNodeName);
	  writeHex(out, ln.first);
	  out << ":";
	  writeHexShort(out, ln.second);
          out << "\t";
        } else {
	  writeHex(out, std::get<size_t>(*srcNodeName));
	  out << "\t";
        }
      } else {
        out << src_vv->getName() << "\t";
      }
      if (dstNodeName) {
        if (std::holds_alternative<LocalName>(*dstNodeName)) {
          LocalName ln = std::get<LocalName>(*dstNodeName);
	  writeHex(out, ln.first);
	  out << ":";
	  writeHexShort(out, ln.second);
          out << "\t";
        } else {
	  writeHex(out, std::get<size_t>(*dstNodeName));
	  out << "\t";
        }
      } else {
        out << dst_fvv->getName() << "\t";
      }
      if (item.first->isIndirectCall()) {
	out << "Indirect\n";
      } else {
	out << "Direct\n";
      }
    }
  }
}
