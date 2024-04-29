// Copyright (c) 2010 NaturalMotion.  All Rights Reserved.
// Not to be copied, adapted, modified, used, distributed, sold,
// licensed or commercially exploited in any manner without the
// written consent of NaturalMotion.
//
// All non public elements of this software are the confidential
// information of NaturalMotion and may not be disclosed to any
// person nor used for any purpose not expressly approved by
// NaturalMotion in writing.

#include <fstream>

//----------------------------------------------------------------------------------------------------------------------
#include "NMPlatform/NMHash.h"
#include "morpheme/mrNetworkDef.h"
#include "morpheme/mrDefines.h"
#include "morpheme/mrManager.h"
#include "morpheme/mrNetwork.h"
#include "morpheme/Nodes/mrNodeStateMachine.h"
#include "morpheme/mrCharacterControllerAttribData.h"
#include "morpheme/mrMirroredAnimMapping.h"
#include <vector>
#include <set>
#include <map>

#include <string>

#include <stdio.h>
//----------------------------------------------------------------------------------------------------------------------

namespace MR
{

//----------------------------------------------------------------------------------------------------------------------
// MR::NodeInitData functions.
//----------------------------------------------------------------------------------------------------------------------
void NodeInitData::locate(NodeInitData* target)
{
  NMP::endianSwap(target->m_type);
  NMP::endianSwap(target->m_targetNodeID);
}

//----------------------------------------------------------------------------------------------------------------------
void NodeInitData::dislocate(NodeInitData* target)
{
  NMP::endianSwap(target->m_type);
  NMP::endianSwap(target->m_targetNodeID);
}

//----------------------------------------------------------------------------------------------------------------------
// MR::NodeInitDataArrayDef functions.
//----------------------------------------------------------------------------------------------------------------------
NMP::Memory::Format NodeInitDataArrayDef::getMemoryRequirements(uint16_t nodeInitDataCount)
{
  NMP::Memory::Format result(sizeof(NodeInitDataArrayDef), MR_NODE_INIT_DATA_ALIGNMENT);

  // Reserve some space for the AttribData's
  result.size += sizeof(NodeInitData*) * nodeInitDataCount;

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
NodeInitDataArrayDef* NodeInitDataArrayDef::init(
  NMP::Memory::Resource& resource,
  uint16_t               numNodeInitDatas)
{
  resource.align(NMP::Memory::Format(sizeof(NodeInitDataArrayDef), MR_NODE_INIT_DATA_ALIGNMENT));
  NodeInitDataArrayDef* result = (NodeInitDataArrayDef*)resource.ptr;
  resource.increment(NMP::Memory::Format(sizeof(NodeInitDataArrayDef), resource.format.alignment));

  // Init and wipe control param node IDs.
  result->m_nodeInitDataArray = (NodeInitData**)resource.ptr;
  resource.increment(sizeof(NodeInitData*) * numNodeInitDatas);
  for (uint16_t i = 0; i != numNodeInitDatas; ++i)
  {
    result->m_nodeInitDataArray[i] = (NodeInitData*)NULL;
  }
  result->m_numNodeInitDatas = numNodeInitDatas;

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
bool NodeInitDataArrayDef::locate()
{
  NMP::endianSwap(m_numNodeInitDatas);

  NMP::endianSwap(m_nodeInitDataArray);
  REFIX_PTR(NodeInitData*, m_nodeInitDataArray);
  for (uint16_t i = 0; i != m_numNodeInitDatas; ++i)
  {
    NMP::endianSwap(m_nodeInitDataArray[i]);
    REFIX_PTR(NodeInitData, m_nodeInitDataArray[i]);

    NodeInitDataType type = m_nodeInitDataArray[i]->getType();

    NMP::endianSwap(type);

    NodeInitDataLocateFn locateFn = Manager::getInstance().getNodeInitDataLocateFn(type);
    NMP_ASSERT(locateFn);

    locateFn(m_nodeInitDataArray[i]);
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool NodeInitDataArrayDef::dislocate()
{
  for (uint16_t i = 0; i != m_numNodeInitDatas; ++i)
  {
    NodeInitDataDislocateFn dislocateFn = Manager::getInstance().getNodeInitDataDislocateFn(m_nodeInitDataArray[i]->getType());
    NMP_ASSERT(dislocateFn);

    dislocateFn(m_nodeInitDataArray[i]);

    UNFIX_PTR(NodeInitData, m_nodeInitDataArray[i]);
    NMP::endianSwap(m_nodeInitDataArray[i]);
  }
  UNFIX_PTR(NodeInitData*, m_nodeInitDataArray);
  NMP::endianSwap(m_nodeInitDataArray);

  NMP::endianSwap(m_numNodeInitDatas);

  return true;
}

struct TrackRefStruct
{
  int duration_num;
  uint32_t duration_asset_id = 0;
  int discrete_num = 0;
  uint32_t discrete_asset_id = 0;
  int curve_num = 0;
  uint32_t curve_asset_id = 0;
};
void output_event_ref_data(
    uint32_t node_id, 
	std::map<int, int>& semantic_2_animaton_id,
	std::map<int, TrackRefStruct>& semantic_2_track_ref,
    std::ofstream& file)
{
    if (semantic_2_animaton_id.empty() && semantic_2_track_ref.empty())
        return;

    file << node_id << std::endl;
    file << semantic_2_animaton_id.size() << std::endl;
    for (const auto & t : semantic_2_animaton_id)
    {
        file << t.first << std::endl;
        file << t.second << std::endl;
    }
    file << semantic_2_track_ref.size() << std::endl;
    for (const auto& t : semantic_2_track_ref)
    {
        file << t.first << std::endl;
        file << t.second.curve_num << std::endl;
        file << t.second.curve_asset_id << std::endl;
        file << t.second.discrete_num << std::endl;
        file << t.second.discrete_asset_id << std::endl;
        file << t.second.duration_num << std::endl;
        file << t.second.duration_asset_id << std::endl;
    }

    file << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------
// MR::NetworkDef
//----------------------------------------------------------------------------------------------------------------------
void NetworkDef::locate()
{
  // Header information
  NMP::endianSwap(m_numNodes);
  NMP::endianSwap(m_numAnimSets);
  NMP::endianSwap(m_numMessageDistributors);
  NMP::endianSwap(m_maxBonesInAnimSets);
  NMP::endianSwap(m_numNetworkInputControlParameters);

  // Shared task function tables: Need to be located before the individual node definitions
  //REFIX_SWAP_PTR(SharedTaskFnTables, m_taskQueuingFnTables);
  // m_taskQueuingFnTables->locateTaskQueuingFnTables();
  // REFIX_SWAP_PTR(SharedTaskFnTables, m_outputCPTaskFnTables);
  // m_outputCPTaskFnTables->locateOutputCPTaskFnTables();

  // Locate the semantic lookup tables. They are used when locating the nodes.
  //NMP::endianSwap(m_numSemanticLookupTables);
  //REFIX_SWAP_PTR(SemanticLookupTable*, m_semanticLookupTables);
  //for (uint32_t i = 0; i < m_numSemanticLookupTables; ++i)
  //{
  //  REFIX_SWAP_PTR(SemanticLookupTable, m_semanticLookupTables[i]);
  //  m_semanticLookupTables[i]->locate();
  //}
  
  // NodeDefs
  std::ofstream myfile;
  myfile.open("F:/horizon_files/morpheme_graph.txt");
  std::ofstream event_ref_file;
  event_ref_file.open("F:/horizon_files/morpheme_event_ref.txt");
  std::ofstream state_machine_file;  // 这个是为了方便阅读。
  state_machine_file.open("F:/horizon_files/state_machine.txt");
  std::ofstream state_machine_raw;  // 这个是为了加载到python
  state_machine_raw.open("F:/horizon_files/state_machine_raw.txt");
  std::ofstream attrib_type_file;
  attrib_type_file.open("F:/horizon_files/attrib_type.txt");
  std::ofstream condition_file;
  condition_file.open("F:/horizon_files/conditions.txt");

  std::set<int> all_condition_types;

  myfile << m_numNodes << std::endl;
  REFIX_SWAP_PTR(NodeDef*, m_nodes);
  for (uint32_t i = 0; i < m_numNodes; ++i)
  {
      REFIX_SWAP_PTR(NodeDef, m_nodes[i]);
      // m_nodes[i]->locate(this); // Fixes up the task function tables
      m_nodes[i]->zhaoqi_locate();

      NodeDef* n = m_nodes[i];
      myfile << n->getNodeID() << std::endl;
      myfile << n->getParentNodeID() << std::endl;
      myfile << n->getNodeTypeID() << std::endl;
      myfile << n->getNodeFlags() << std::endl;
      myfile << n->getNodeFlagsHigher() << std::endl;
      myfile << n->getNumChildNodes() << std::endl;
      NMP_STDOUT("%d : node id %d \n"\
          "     node type %d\n"\
          "     parent id %d child number: %d", i, n->getNodeID(), n->getNodeTypeID(), n->getParentNodeID(), n->getNumChildNodes());
      for (uint32_t c = 0; c < n->getNumChildNodes(); ++c)
      {
          NMP_STDOUT("            %d", n->getChildNodeID(c));
          myfile << n->getChildNodeID(c) << std::endl;
      }
      myfile << (int32_t)(n->getNumInputCPConnections()) << std::endl;
      myfile << (int32_t)(n->getNumOutputCPPins()) << std::endl;
      NMP_STDOUT("      input num: %d, output num %d", n->getNumInputCPConnections(), n->getNumOutputCPPins());
      for (uint32_t c = 0; c < n->getNumInputCPConnections(); ++c)
      {
          const CPConnection*input = n->getInputCPConnection(c);
          NMP_STDOUT("            %d : %d", input->m_sourceNodeID, input->m_sourcePinIndex);
          myfile << input->m_sourceNodeID << std::endl;
          myfile << input->m_sourcePinIndex << std::endl;
      }
      {
          std::set<int> animation_source_ids;
          std::map<int, int> semantic_2_animaton_id;
          std::map<int, TrackRefStruct> semantic_2_track_ref;
          std::set<int> attrib_data_types;
          bool has_state_machine = false;
          for (uint16_t i = 0; i < n->m_numAttribDataHandles; ++i)
          {
              if (n->m_nodeAttribDataHandles[i].m_attribData)
              {
                  // Locate the attrib data itself
                  AttribDataType type = n->m_nodeAttribDataHandles[i].m_attribData->getType();
                  attrib_data_types.insert(type);
                  if (type == ATTRIB_TYPE_SOURCE_ANIM)
                  {
                      AttribDataSourceAnim* a = (AttribDataSourceAnim*)(n->m_nodeAttribDataHandles[i].m_attribData);
                      NMP_STDOUT("     %d : data source anim %d %f", i, a->m_animAssetID, a->m_sourceAnimDuration);
                      animation_source_ids.insert(a->m_animAssetID);
                      semantic_2_animaton_id[i] = a->m_animAssetID;
                  }
                  else if (type == ATTRIB_TYPE_SOURCE_EVENT_TRACKS)
                  {
                      AttribDataSourceEventTrackSet* a = (AttribDataSourceEventTrackSet*)(n->m_nodeAttribDataHandles[i].m_attribData);
					  NMP_STDOUT("     %d : discrete event track %d : %u", i, a->m_numDiscreteEventTracks, ((uint32_t*)(a->m_sourceDiscreteEventTracks))[0]);
					  NMP_STDOUT("     %d : duration event track %d : %u", i, a->m_numDurEventTracks, ((uint32_t*)(a->m_sourceDurEventTracks))[0]);
                      semantic_2_track_ref[i] = TrackRefStruct();
                      semantic_2_track_ref[i].discrete_num = a->m_numDiscreteEventTracks;
                      if (a->m_sourceDiscreteEventTracks)
						  semantic_2_track_ref[i].discrete_asset_id = ((uint32_t*)(a->m_sourceDiscreteEventTracks))[0];
                      semantic_2_track_ref[i].duration_num = a->m_numDurEventTracks;
                      if (a->m_sourceDurEventTracks)
						  semantic_2_track_ref[i].duration_asset_id = ((uint32_t*)(a->m_sourceDurEventTracks))[0];
                      semantic_2_track_ref[i].curve_num = a->m_numCurveEventTracks;
                      if (a->m_sourceCurveEventTracks)
						  semantic_2_track_ref[i].curve_asset_id = ((uint32_t*)(a->m_sourceCurveEventTracks))[0];
                  }
                  else if (type == ATTRIB_TYPE_STATE_MACHINE_DEF)
                  {
                      if (has_state_machine)
                      {
                          NMP_STDOUT("error : too many state machine!!");
                      }
                      has_state_machine = true;
                      // get all condition type
                      AttribDataStateMachineDef * a = (AttribDataStateMachineDef *)(n->m_nodeAttribDataHandles[i].m_attribData);
                      condition_file << n->getNodeID() << std::endl;
                      condition_file << a->getNumConditions() << std::endl;
                      for (ConditionIndex i = 0; i < a->getNumConditions(); ++i)
                      {
                          auto condition_def = a->getConditionDef(i);
                          auto tt = condition_def->getType();
                          all_condition_types.insert(tt);
                          condition_file << condition_def->getType() << std::endl << condition_def->getInvertFlag() << std::endl;
                      }

                      state_machine_file << "node id:" << n->getNodeID() << std::endl;
                      state_machine_raw << n->getNodeID() << std::endl;

                      state_machine_file << "index: " << i << std::endl;

                      state_machine_file << "num states:" << a->getNumStates() << " num child:" << n->getNumChildNodes() << std::endl;
                      state_machine_raw << a->getNumStates() << std::endl;

                      state_machine_file << "num conditions:" << a->getNumConditions() << std::endl;
                      if (a->getNumStates() != n->getNumChildNodes())
                          NMP_STDOUT("error: state num != child num");

                      state_machine_file << "default state: " << a->getDefaultStartingStateID() << std::endl;
                      state_machine_raw << a->getDefaultStartingStateID() << std::endl;

                      auto output_list = [&state_machine_file, &state_machine_raw, &a](uint16_t* ids, uint16_t num, uint16_t index) {
                          state_machine_file << "\t\t list " << index << " count:[" << num << "]";
                          state_machine_raw << num << std::endl;
                          if (ids && num > 0)
                          {
							  bool in_range_state = true;
							  bool in_range_condition = true;
                              for (int index_id = 0; index_id < num; index_id++)
                              {
                                  if (index ==3)
									  state_machine_file << ids[index_id] <<"(" << a->getStateDef(ids[index_id])->m_nodeID << ")" << " ";
                                  else
									  state_machine_file << ids[index_id] << " ";
                                  if (ids[index_id] >= a->getNumStates())
                                      in_range_state = false;
                                  if (ids[index_id] >= a->getNumConditions())
                                      in_range_condition = false;

                                  state_machine_raw << ids[index_id] << std::endl;
                              }
                              state_machine_file << "{" << std::hex << ids[num] << ", " << ids[num + 1] << "}" << std::dec << std::endl;
                              if (!in_range_state)
                                  state_machine_file << "\t\tLIST_" << index << " out of state range." << std::endl;
                              if (!in_range_condition)
                                  state_machine_file << "\t\tLIST_" << index << " out of condition range." << std::endl;
                          }
                          else
                          {
                              state_machine_file << std::endl;
                          }
					  };
                      auto output_state_def = [&state_machine_file, &state_machine_raw, output_list, &a](StateDef* sd, std::string& name_or_id, uint16_t childid) {
                          state_machine_file << "\t" << name_or_id << "\tnode id: " << sd->getNodeID();
                          state_machine_raw << sd->getNodeID() << std::endl;
                          state_machine_file << "\tchild ID:" << childid;
                          state_machine_file << "\tunkown id is: " << sd->m_UnknownNum << std::endl;
                          state_machine_raw << sd->m_UnknownNum << std::endl;
                          if (sd->m_UnknownNum > 0 && sd->m_UnknownNum > a->getNumConditions())
                              state_machine_file << "unknown num out of conditin range" << std::endl;
                          output_list(sd->m_entryConditionIndexes, sd->m_numEntryConditions, 0);
                          output_list(sd->m_exitConditionIndexes, sd->m_numExitConditions, 1);
                          output_list(sd->m_exitBreakoutConditions, sd->m_numExitBreakoutConditions, 2);
                          output_list(sd->m_exitTransitionStateIDs, sd->m_numExitTransitionStates, 3);
                          state_machine_file << "\t\t src id:" << sd->getTransitSourceStateID();
                          state_machine_raw << sd->getTransitSourceStateID() << std::endl;
                          state_machine_file << "  dst id:" << sd->getTransitDestinationStateID();
                          state_machine_raw << sd->getTransitDestinationStateID() << std::endl;
                          state_machine_file << std::endl;
                          };
                      for (StateID sid = 0; sid < a->getNumStates(); ++sid)
                      {
                          StateDef* sd = a->getStateDef(sid);
                          output_state_def(sd, std::to_string(sid), n->getChildNodeID(sid));
                      }
                      if (StateDef* sd = a->getGlobalStateDef())
                      {
                          output_state_def(sd, std::string("global"), 0xFFFF);
                      }
                      state_machine_file << std::endl;
                  }
              }
          }
		  attrib_type_file << "Node: " << n->getNodeID() << std::endl << "\t";
          for (auto tt : attrib_data_types)
          {
			  attrib_type_file << tt << " ";
          }
          attrib_type_file << std::endl;
          attrib_type_file << std::endl;

          output_event_ref_data(n->getNodeID(), semantic_2_animaton_id, semantic_2_track_ref, event_ref_file);
          myfile << animation_source_ids.size() << std::endl;
          for (auto id : animation_source_ids)
          {
              myfile << id << std::endl;
          }
      }
	  myfile << std::endl;


               
      NMP_STDOUT("");
  }
  myfile << "hello" << std::endl;
  myfile.close();
  event_ref_file.close();
  state_machine_file.close();
  state_machine_raw.close();
  attrib_type_file.close();
  condition_file.close();

  NMP_STDOUT(" condition type count %d", all_condition_types.size());
  for (auto tt : all_condition_types)
  {
      NMP_STDOUT(" \t%d", tt);
  }

  // Output control parameter Node IDs and semantics
  if (m_emittedControlParamsInfo)
  {
    REFIX_SWAP_PTR(EmittedControlParamsInfo, m_emittedControlParamsInfo);
    m_emittedControlParamsInfo->locate();
  }

  // State machine Node IDs array
  if (m_stateMachineNodeIDs)
  {
    REFIX_SWAP_PTR(NodeIDsArray, m_stateMachineNodeIDs);
    m_stateMachineNodeIDs->locate();
  }

  // Request Emitter Nodes ID array
  if (m_messageEmitterNodeIDs)
  {
    REFIX_SWAP_PTR(NodeIDsArray, m_messageEmitterNodeIDs);
    m_messageEmitterNodeIDs->locate();
  }

  // Multiply connected Node IDs array
  if (m_multiplyConnectedNodeIDs)
  {
    REFIX_SWAP_PTR(NodeIDsArray, m_multiplyConnectedNodeIDs);
    m_multiplyConnectedNodeIDs->locate();
  }

  // Node OnExit Message array
  if (m_nodeEventOnExitMessages)
  {
    REFIX_SWAP_PTR(NodeEventOnExitMessage, m_nodeEventOnExitMessages);
    NMP::endianSwap(m_numNodeEventOnExitMessages);

    for(uint32_t i = 0; i < m_numNodeEventOnExitMessages; i++)
    {
      NMP::endianSwap(m_nodeEventOnExitMessages[i].m_nodeID);
      NMP::endianSwap(m_nodeEventOnExitMessages[i].m_nodeEventMessage.m_msgID);
    }
  }

  // State machine state to state ID mapping table
  if (m_stateMachineStateIDStringTable)
  {
    REFIX_SWAP_PTR(NMP::IDMappedStringTable, m_stateMachineStateIDStringTable);
    m_stateMachineStateIDStringTable->locate();
  }

  // NodeID to Node name mapping table
  if (m_nodeIDNamesTable)
  {
    REFIX_SWAP_PTR(NMP::IDMappedStringTable, m_nodeIDNamesTable);
    m_nodeIDNamesTable->locate();
  }

  // RequestID to Request name mapping table
  if (m_messageIDNamesTable)
  {
    REFIX_SWAP_PTR(NMP::OrderedStringTable, m_messageIDNamesTable);
    m_messageIDNamesTable->locate();
  }

  // Mapping table between event track names and runtime IDs
  if (m_eventTrackIDNamesTable)
  {
    REFIX_SWAP_PTR(NMP::OrderedStringTable, m_eventTrackIDNamesTable);
    m_eventTrackIDNamesTable->locate();
  }

  // MessageDistributors
  REFIX_SWAP_PTR(MessageDistributor*, m_messageDistributors);
  for (uint32_t i = 0; i < m_numMessageDistributors; ++i)
  {
    // message ids can be sparse so this array may have gaps
    if (m_messageDistributors[i])
    {
      REFIX_SWAP_PTR(MessageDistributor, m_messageDistributors[i]);
      m_messageDistributors[i]->locate();
    }
  }

  if (m_tagTable)
  {
    REFIX_SWAP_PTR(NodeTagTable, m_tagTable);
    m_tagTable->locate();
  }

  REFIX_SWAP_PTR(uint32_t*, m_rigToUberrigMaps);
  REFIX_SWAP_PTR(uint32_t*, m_uberrigToRigMaps);
  for (uint32_t i = 0; i < m_numAnimSets; ++i)
  {
    REFIX_SWAP_PTR(uint32_t, m_rigToUberrigMaps[i]);
    REFIX_SWAP_PTR(uint32_t, m_uberrigToRigMaps[i]);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NetworkDef::dislocate()
{
  for (uint32_t i = 0; i < m_numAnimSets; ++i)
  {
    UNFIX_SWAP_PTR(uint32_t, m_rigToUberrigMaps[i]);
    UNFIX_SWAP_PTR(uint32_t, m_uberrigToRigMaps[i]);
  }
  UNFIX_SWAP_PTR(uint32_t*, m_rigToUberrigMaps);
  UNFIX_SWAP_PTR(uint32_t*, m_uberrigToRigMaps);

  if (m_tagTable)
  {
    m_tagTable->dislocate();
    UNFIX_SWAP_PTR(NodeTagTable, m_tagTable);
  }

  // MessageDistributors
  for (uint32_t i = 0; i < m_numMessageDistributors; ++i)
  {
    // message ids can be sparse so this array may have gaps
    if (m_messageDistributors[i])
    {
      m_messageDistributors[i]->dislocate();
      UNFIX_SWAP_PTR(MessageDistributor, m_messageDistributors[i]);
    }
  }
  UNFIX_SWAP_PTR(MessageDistributor*, m_messageDistributors);

  // Mapping table between event track names and runtime IDs
  if (m_eventTrackIDNamesTable)
  {
    m_eventTrackIDNamesTable->dislocate();
    UNFIX_SWAP_PTR(NMP::OrderedStringTable, m_eventTrackIDNamesTable);
  }

  // RequestID to Request name mapping table
  if (m_messageIDNamesTable)
  {
    m_messageIDNamesTable->dislocate();
    UNFIX_SWAP_PTR(NMP::OrderedStringTable, m_messageIDNamesTable);
  }

  // State machine state to state ID mapping table
  if (m_stateMachineStateIDStringTable)
  {
    m_stateMachineStateIDStringTable->dislocate();
    UNFIX_SWAP_PTR(NMP::IDMappedStringTable, m_stateMachineStateIDStringTable);
  }

  // NodeID to Node name mapping table
  if (m_nodeIDNamesTable)
  {
    m_nodeIDNamesTable->dislocate();
    UNFIX_SWAP_PTR(NMP::IDMappedStringTable, m_nodeIDNamesTable);
  }

  // Node OnExit Message array
  if (m_nodeEventOnExitMessages)
  {
    for(uint32_t i = 0; i < m_numNodeEventOnExitMessages; i++)
    {
      NMP::endianSwap(m_nodeEventOnExitMessages[i].m_nodeID);
      NMP::endianSwap(m_nodeEventOnExitMessages[i].m_nodeEventMessage.m_msgID);
    }

    NMP::endianSwap(m_numNodeEventOnExitMessages);
    UNFIX_SWAP_PTR(NodeEventOnExitMessage, m_nodeEventOnExitMessages);
  }

  // Multiply connected Node IDs array
  if (m_multiplyConnectedNodeIDs)
  {
    m_multiplyConnectedNodeIDs->dislocate();
    UNFIX_SWAP_PTR(NodeIDsArray, m_multiplyConnectedNodeIDs);
  }

  // Request Emitter Nodes ID array
  if (m_messageEmitterNodeIDs)
  {
    m_messageEmitterNodeIDs->dislocate();
    UNFIX_SWAP_PTR(NodeIDsArray, m_messageEmitterNodeIDs);
  }

  // State machine Node IDs array
  if (m_stateMachineNodeIDs)
  {
    m_stateMachineNodeIDs->dislocate();
    UNFIX_SWAP_PTR(NodeIDsArray, m_stateMachineNodeIDs);
  }

  // Output control parameter Node IDs and semantics
  if (m_emittedControlParamsInfo)
  {
    m_emittedControlParamsInfo->dislocate();
    UNFIX_SWAP_PTR(EmittedControlParamsInfo, m_emittedControlParamsInfo);
  }

  // NodeDefs
  for (uint32_t i = 0; i < m_numNodes; ++i)
  {
    m_nodes[i]->dislocate();
    UNFIX_SWAP_PTR(NodeDef, m_nodes[i]);
  }
  UNFIX_SWAP_PTR(NodeDef*, m_nodes);

  // Dislocate the semantic lookup tables.
  for (uint32_t i = 0; i < m_numSemanticLookupTables; ++i)
  {
    m_semanticLookupTables[i]->dislocate();
    UNFIX_SWAP_PTR(SemanticLookupTable, m_semanticLookupTables[i]);
  }
  NMP::endianSwap(m_numSemanticLookupTables);
  UNFIX_SWAP_PTR(SemanticLookupTable*, m_semanticLookupTables);

  // Shared task function tables
  m_outputCPTaskFnTables->dislocateOutputCPTaskFnTables();
  UNFIX_SWAP_PTR(SharedTaskFnTables, m_outputCPTaskFnTables);
  m_taskQueuingFnTables->dislocateTaskQueuingFnTables();
  UNFIX_SWAP_PTR(SharedTaskFnTables, m_taskQueuingFnTables);

  // Header information
  NMP::endianSwap(m_numNetworkInputControlParameters);
  NMP::endianSwap(m_maxBonesInAnimSets);
  NMP::endianSwap(m_numMessageDistributors);
  NMP::endianSwap(m_numAnimSets);
  NMP::endianSwap(m_numNodes);
}

//----------------------------------------------------------------------------------------------------------------------
// MR::NetworkDef::getNodeIDFromNodeName(const char* name) assumes that NMP_STRING_NOT_FOUND when casted to a NodeID is
// the same value as INVALID_NODE_ID.
#ifdef NM_COMPILER_MSVC
  #pragma warning (push)
  #pragma warning (disable : 4310) //cast truncates constant value
#endif

NM_ASSERT_COMPILE_TIME((NodeID)NMP_STRING_NOT_FOUND == INVALID_NODE_ID);

#ifdef NM_COMPILER_MSVC
  #pragma warning (pop)
#endif

//----------------------------------------------------------------------------------------------------------------------
StateID NetworkDef::getStateIDFromStateName(const char* stateName) const
{
  NMP_ASSERT_MSG(m_stateMachineStateIDStringTable, "m_stateMachineStateIDStringTable doesn't exist.");
  NMP_ASSERT_MSG(m_stateMachineStateIDStringTable->findNumEntriesForString(stateName) < 2, "More than one instance of state name %s found.", stateName);
  // Note that casting NMP_STRING_NOT_FOUND into StateID results in INVALID_STATE_ID 
  // (even though the values are different).
  return (StateID)m_stateMachineStateIDStringTable->getIDForString(stateName);
}

//----------------------------------------------------------------------------------------------------------------------
NodeID NetworkDef::getNodeIDFromStateName(const char* stateMachineAndStateName) const
{
  NMP_ASSERT_MSG(m_stateMachineStateIDStringTable, "m_stateMachineStateIDStringTable doesn't exist.");
  NMP_ASSERT_MSG(m_stateMachineStateIDStringTable->findNumEntriesForString(stateMachineAndStateName) < 2, "More than one instance of state name %s found.", stateMachineAndStateName);

  // Get the name of the state machine.  
  // Remember the 'stateMachineAndStateName' will have the state machine name pre-pended.
  // Hence remove the state name and the '|' before it. 
  const char * pCh = strchr(stateMachineAndStateName,'|');
  size_t tokenFoundAtPosition = 0;
  while (pCh != NULL)
  {
    tokenFoundAtPosition = pCh - stateMachineAndStateName;
    pCh = strchr(pCh+1,'|');
  }
  NMP_ASSERT(tokenFoundAtPosition < strlen(stateMachineAndStateName));

  const uint32_t STATE_MACHINE_NAME_SIZE = 1024;
  char stateMachineName[STATE_MACHINE_NAME_SIZE];
  NMP_STRNCPY_S(stateMachineName, STATE_MACHINE_NAME_SIZE, stateMachineAndStateName);
  NMP_ASSERT(tokenFoundAtPosition < STATE_MACHINE_NAME_SIZE);
  stateMachineName[tokenFoundAtPosition] = 0;

  // Get the state machine nodeID.
  NodeID smNodeID = (NodeID)m_nodeIDNamesTable->getIDForString(stateMachineName);
  NMP_ASSERT_MSG(smNodeID != INVALID_NODE_ID,"Could find state machine name");

  // Get the state machine def.
  MR::AttribDataStateMachineDef* smDef = getAttribData<MR::AttribDataStateMachineDef>(ATTRIB_SEMANTIC_NODE_SPECIFIC_DEF, smNodeID);
  NMP_ASSERT_MSG(smDef != NULL,"Couldn't find the state machine def");

  // Get the root node of the state machine
  return smDef->getNodeIDFromStateID(getStateIDFromStateName(stateMachineAndStateName));
}

//----------------------------------------------------------------------------------------------------------------------
NodeID NetworkDef::getNodeIDFromNodeName(const char* name) const
{
  NMP_ASSERT_MSG(m_nodeIDNamesTable, "NodeID name table doesn't exist.");
  NMP_ASSERT_MSG(m_nodeIDNamesTable->findNumEntriesForString(name) < 2, "More than one instance of node name %s found.", name);
  // Note that casting NMP_STRING_NOT_FOUND into NodeID results in INVALID_NODE_ID 
  // (even though the values are different).
  return (NodeID)m_nodeIDNamesTable->getIDForString(name);
}

//----------------------------------------------------------------------------------------------------------------------
const char* NetworkDef::getNodeNameFromNodeID(NodeID nodeID) const
{
  NMP_ASSERT(m_nodeIDNamesTable && m_nodeIDNamesTable->getStringForID(nodeID));
  return m_nodeIDNamesTable->getStringForID(nodeID);
}

//----------------------------------------------------------------------------------------------------------------------
// find the message id for the given name
MessageID NetworkDef::getMessageIDFromMessageName(const char* name) const
{
  if (m_messageIDNamesTable)
    return m_messageIDNamesTable->getIDForString(name);
  return INVALID_MESSAGE_ID;
}
//----------------------------------------------------------------------------------------------------------------------
// find the name for the given request ID
const char* NetworkDef::getMessageNameFromMessageID(MessageID requestID) const
{
  if (m_messageIDNamesTable)
    return m_messageIDNamesTable->getStringForID(requestID);
  return "Unknown";
}

//----------------------------------------------------------------------------------------------------------------------
uint32_t NetworkDef::getNumMessages() const
{
  if (m_messageIDNamesTable)
    return m_messageIDNamesTable->getNumEntries();
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
bool NetworkDef::loadAnimations(AnimSetIndex animSetIndex, void* userdata)
{
  MR::Manager& manager = MR::Manager::getInstance();

  // Search through all of the network defs attributes for this anim set and fix up any
  // animation sources, rigToAnimMaps and trajectory sources. Note that this directly
  // abuses the static nodeDef data by modifying the required attribute data pointers!
  for (NodeID nodeIndex = 0; nodeIndex < m_numNodes; ++nodeIndex)
  {
    // Check if the network def is partially built
    NodeDef* nodeDef = getNodeDef(nodeIndex);
    if (!nodeDef)
      continue;

    //--------------------------
    // Load source animation
    AttribDataHandle* attribHandleAnim = getOptionalAttribDataHandle(
                                           ATTRIB_SEMANTIC_SOURCE_ANIM,
                                           nodeIndex,
                                           animSetIndex);
    if (!attribHandleAnim)
      continue;

    AttribDataSourceAnim* sourceAnim = (AttribDataSourceAnim*)attribHandleAnim->m_attribData;
    NMP_ASSERT(sourceAnim);
    MR::AnimSourceBase* animData = manager.requestAnimation(sourceAnim->m_animAssetID, userdata);
    NMP_ASSERT_MSG(animData != NULL, "Unable to load animation!");
    if (!animData->isLocated())
    {
      AnimType animType = animData->getType();
      NMP::endianSwap(animType);
      const MR::Manager::AnimationFormatRegistryEntry* animFormatRegistryEntry =
        MR::Manager::getInstance().getInstance().findAnimationFormatRegistryEntry(animType);
      NMP_ASSERT_MSG(animFormatRegistryEntry, "Unable to get AnimationFormatRegistryEntry entry for animation type %d!", animType);
      animFormatRegistryEntry->m_locateAnimFormatFn(animData);
    }
    sourceAnim->setAnimation(animData);

    sourceAnim->fixupRigToAnimMap();

    //--------------------------
    // Trajectory channel
    const TrajectorySourceBase* trajChannelSource = animData->animGetTrajectorySourceData();
    sourceAnim->setTrajectorySource(trajChannelSource);
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool NetworkDef::unloadAnimations(AnimSetIndex animSetIndex, void* userdata)
{
  MR::Manager& manager = MR::Manager::getInstance();

  for (NodeID nodeIndex = 0; nodeIndex < m_numNodes; ++nodeIndex)
  {
    // Check if the network def is partially built
    NodeDef* nodeDef = getNodeDef(nodeIndex);
    if (!nodeDef)
      continue;

    //--------------------------
    // Unload source animation
    AttribDataHandle* attribHandleAnim = getOptionalAttribDataHandle(
                                           ATTRIB_SEMANTIC_SOURCE_ANIM,
                                           nodeIndex,
                                           animSetIndex);
    if (attribHandleAnim)
    {
      AttribDataSourceAnim* sourceAnim = (AttribDataSourceAnim*)attribHandleAnim->m_attribData;
      NMP_ASSERT(sourceAnim);
      manager.releaseAnimation(sourceAnim->m_animAssetID, sourceAnim->m_anim, userdata);
      sourceAnim->setAnimation(NULL);
    }
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
MR::AnimRigDef* NetworkDef::getRig(AnimSetIndex animSetIndex)
{
  AttribDataHandle* handle = getAttribDataHandle(ATTRIB_SEMANTIC_RIG, NETWORK_NODE_ID, animSetIndex);
  NMP_ASSERT(handle);
  AttribDataRig* rigAttrib = (AttribDataRig*)handle->m_attribData;
  NMP_ASSERT(rigAttrib && rigAttrib->m_rig);
  return rigAttrib->m_rig;
}

//----------------------------------------------------------------------------------------------------------------------
MR::CharacterControllerDef* NetworkDef::getCharacterControllerDef(AnimSetIndex animSetIndex)
{
  AttribDataHandle* handle = getAttribDataHandle(ATTRIB_SEMANTIC_CHARACTER_CONTROLLER_DEF, NETWORK_NODE_ID, animSetIndex);
  NMP_ASSERT(handle);
  AttribDataCharacterControllerDef* characterControllerDefAttrib = (AttribDataCharacterControllerDef*)handle->m_attribData;
  return characterControllerDefAttrib->m_characterControllerDef;
}

//----------------------------------------------------------------------------------------------------------------------
uint32_t NetworkDef::getNumControlParameterNodes() const
{
  return m_numNetworkInputControlParameters;
}

//----------------------------------------------------------------------------------------------------------------------
uint32_t NetworkDef::getControlParameterNodeIDs(
  NodeID*   nodeIDs,                             // Out.
  uint32_t  NMP_USED_FOR_ASSERTS(maxNumNodeIDs) // Size of node IDs array.
) const
{
  uint32_t numControlParams = 0;

  for (uint32_t i = 0; i < m_numNodes; ++i)
  {
    NodeFlags nodeFlags = m_nodes[i]->getNodeFlags();
    if (nodeFlags.isSet(NODE_FLAG_IS_CONTROL_PARAM))
    {
      NMP_ASSERT(numControlParams < maxNumNodeIDs);
      nodeIDs[numControlParams] = m_nodes[i]->getNodeID();
      ++numControlParams;
    }
  }

  return numControlParams;
}

//----------------------------------------------------------------------------------------------------------------------
bool NetworkDef::containsNodeWithFlagsSet(NodeFlags flags) const
{
  for (uint32_t i = 0; i < m_numNodes; ++i)
  {
    if (m_nodes[i]->getNodeFlags().areSet(flags))
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
uint32_t NetworkDef::getMaxBoneCount()
{
  uint32_t largestCount = 0;

  for (AnimSetIndex i = 0; i < getNumAnimSets(); ++i)
  {
    AnimRigDef* r = getRig(i);
    if (r)
    {
      if (r->getNumBones() > largestCount)
      {
        largestCount = r->getNumBones();
      }
    }
  }

  return largestCount;
}

//----------------------------------------------------------------------------------------------------------------------
size_t NetworkDef::getStringTableMemoryFootprint() const
{
  size_t footprint = 0;

  if (m_stateMachineStateIDStringTable)
  {
    footprint += m_stateMachineStateIDStringTable->getInstanceMemoryRequirements().size;
  }
  if (m_nodeIDNamesTable)
  {
    footprint += m_nodeIDNamesTable->getInstanceMemoryRequirements().size;
  }
  if (m_eventTrackIDNamesTable)
  {
    footprint += m_eventTrackIDNamesTable->getInstanceMemoryRequirements().size;
  }
  if (m_messageIDNamesTable)
  {
    footprint += m_messageIDNamesTable->getInstanceMemoryRequirements().size;
  }

  return footprint;
}

//----------------------------------------------------------------------------------------------------------------------
const MessageDistributor* NetworkDef::getMessageDistributorAndPresetMessage(
  MR::MessageType       messageType, 
  const char *          messageName, 
  const MR::Message **  presetMessageOut ) const
{
  for(uint32_t i = 0; i < m_numMessageDistributors; ++i )
  {
    const MessageDistributor * messageDistributor = m_messageDistributors[i];
    if( messageDistributor->m_messageType == messageType )
    {
      uint32_t stringIndex = messageDistributor->m_messagePresetIndexNamesTable->getIDForString( messageName );
      if( stringIndex != NMP_STRING_NOT_FOUND )
      {
        // because this is an ordered string table which can use this as an index to get the correct message.
        *presetMessageOut = messageDistributor->getMessagePreset( stringIndex );
        return messageDistributor;
      }
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------------------------------------------------
SemanticLookupTable* NetworkDef::findSemanticLookupTable(NodeType nodeType) const
{
  for (uint32_t i = 0; i < m_numSemanticLookupTables; ++i)
  {
    SemanticLookupTable* const table = m_semanticLookupTables[i];
    if (table->getNodeType() == nodeType)
    {
      return table;
    }
  }

  // No specific lookup table for this NodeType so use the default empty table.
  return m_semanticLookupTables[0];
}

//----------------------------------------------------------------------------------------------------------------------
bool NetworkDef::isPhysical() const
{
  return m_isPhysical;
}

//----------------------------------------------------------------------------------------------------------------------
void NetworkDef::mapCopyTransformBuffers(NMP::DataBuffer* sourceBuffer, AnimSetIndex sourceAnimSetIndex, NMP::DataBuffer* targetBuffer, AnimSetIndex targetAnimSetIndex)
{
  NMP_ASSERT(sourceAnimSetIndex != ANIMATION_SET_ANY);
  NMP_ASSERT(targetAnimSetIndex != ANIMATION_SET_ANY);
  uint32_t* sourceToUberrigMap = m_rigToUberrigMaps[sourceAnimSetIndex];
  uint32_t* uberrigToTargetMap = m_uberrigToRigMaps[targetAnimSetIndex];

  targetBuffer->getUsedFlags()->clearAll();
  for (uint32_t sourceIndex = 0; sourceIndex != sourceBuffer->getLength(); ++sourceIndex)
  {
    uint32_t targetIndex = uberrigToTargetMap[sourceToUberrigMap[sourceIndex]];
    targetBuffer->setPosQuatChannelPos(targetIndex, *sourceBuffer->getPosQuatChannelPos(sourceIndex));
    targetBuffer->setPosQuatChannelQuat(targetIndex, *sourceBuffer->getPosQuatChannelQuat(sourceIndex));
    targetBuffer->setChannelUsed(targetIndex);
  }

  targetBuffer->setPosQuatChannelPos(0, *sourceBuffer->getPosQuatChannelPos(0));
  targetBuffer->setPosQuatChannelQuat(0, *sourceBuffer->getPosQuatChannelQuat(0));
  targetBuffer->setChannelUsed(0);

  targetBuffer->calculateFullFlag();
}

//----------------------------------------------------------------------------------------------------------------------
void NetworkDef::mapCopyVelocityBuffers(NMP::DataBuffer* sourceBuffer, AnimSetIndex sourceAnimSetIndex, NMP::DataBuffer* targetBuffer, AnimSetIndex targetAnimSetIndex)
{
  NMP_ASSERT(sourceAnimSetIndex != ANIMATION_SET_ANY);
  NMP_ASSERT(targetAnimSetIndex != ANIMATION_SET_ANY);
  uint32_t* sourceToUberrigMap = m_rigToUberrigMaps[sourceAnimSetIndex];
  uint32_t* uberrigToTargetMap = m_uberrigToRigMaps[targetAnimSetIndex];

  targetBuffer->getUsedFlags()->clearAll();
  for (uint32_t sourceIndex = 0; sourceIndex != sourceBuffer->getLength(); ++sourceIndex)
  {
    uint32_t targetIndex = uberrigToTargetMap[sourceToUberrigMap[sourceIndex]];
    targetBuffer->setPosVelAngVelChannelPosVel(targetIndex, *sourceBuffer->getPosVelAngVelChannelPosVel(sourceIndex));
    targetBuffer->setPosVelAngVelChannelAngVel(targetIndex, *sourceBuffer->getPosVelAngVelChannelAngVel(sourceIndex));
    targetBuffer->setChannelUsed(targetIndex);
  }

  targetBuffer->setPosVelAngVelChannelPosVel(0, *sourceBuffer->getPosVelAngVelChannelPosVel(0));
  targetBuffer->setPosVelAngVelChannelAngVel(0, *sourceBuffer->getPosVelAngVelChannelAngVel(0));
  targetBuffer->setChannelUsed(0);

  targetBuffer->calculateFullFlag();
}

//----------------------------------------------------------------------------------------------------------------------
bool locateNetworkDef(uint32_t NMP_USED_FOR_ASSERTS(assetType), void* assetMemory)
{
  NMP_ASSERT(assetType == MR::Manager::kAsset_NetworkDef);
  MR::NetworkDef* networkDef = (MR::NetworkDef*)assetMemory;
  networkDef->locate();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
// MR::EmittedControlParamsInfo functions.
//----------------------------------------------------------------------------------------------------------------------
NMP::Memory::Format EmittedControlParamsInfo::getMemoryRequirements(uint32_t numNodes)
{
  NMP::Memory::Format memReqs(sizeof(EmittedControlParamsInfo), NMP_NATURAL_TYPE_ALIGNMENT);
  memReqs += NMP::Memory::Format(sizeof(EmittedControlParamData) * numNodes, NMP_NATURAL_TYPE_ALIGNMENT);
  memReqs.align();
  return memReqs;
}

//----------------------------------------------------------------------------------------------------------------------
EmittedControlParamsInfo* EmittedControlParamsInfo::init(
  NMP::Memory::Resource& memRes,
  uint32_t numNodes)
{
  NMP::Memory::Format memReqs(sizeof(EmittedControlParamsInfo), NMP_NATURAL_TYPE_ALIGNMENT);
  EmittedControlParamsInfo* result = (EmittedControlParamsInfo*)memRes.alignAndIncrement(memReqs);
  result->m_numEmittedControlParamNodes = numNodes;
  if (numNodes > 0)
  {
    result->m_emittedControlParamsData = (EmittedControlParamData*)memRes.alignAndIncrement(NMP::Memory::Format(sizeof(EmittedControlParamData) * numNodes, NMP_NATURAL_TYPE_ALIGNMENT));
  }
  else
  {
    result->m_emittedControlParamsData = NULL;
  }
  memRes.align(memReqs.alignment);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
void EmittedControlParamsInfo::locate()
{
  NMP::endianSwap(m_numEmittedControlParamNodes);
  if (m_emittedControlParamsData)
  {
    REFIX_SWAP_PTR(EmittedControlParamData, m_emittedControlParamsData);
    for (uint32_t i = 0; i < m_numEmittedControlParamNodes; ++i)
    {
      NMP::endianSwap(m_emittedControlParamsData[i].m_nodeID);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void EmittedControlParamsInfo::dislocate()
{
  if (m_emittedControlParamsData)
  {
    for (uint32_t i = 0; i < m_numEmittedControlParamNodes; ++i)
    {
      NMP::endianSwap(m_emittedControlParamsData[i].m_nodeID);
    }
    UNFIX_SWAP_PTR(EmittedControlParamData, m_emittedControlParamsData);
  }
  NMP::endianSwap(m_numEmittedControlParamNodes);
}

//----------------------------------------------------------------------------------------------------------------------
void EmittedControlParamsInfo::relocate()
{
  if (m_numEmittedControlParamNodes > 0)
  {
    void* ptr = this;
    NMP_ASSERT(NMP_IS_ALIGNED(ptr, NMP_NATURAL_TYPE_ALIGNMENT));
    ptr = (void*)NMP::Memory::align((((uint8_t*)ptr) + sizeof(EmittedControlParamsInfo)), NMP_NATURAL_TYPE_ALIGNMENT);
    m_emittedControlParamsData = (EmittedControlParamData*)ptr;
  }
  else
  {
    m_emittedControlParamsData = NULL;
  }
}

//----------------------------------------------------------------------------------------------------------------------
// MR::NodeIDsArray functions.
//----------------------------------------------------------------------------------------------------------------------
NMP::Memory::Format NodeIDsArray::getMemoryRequirements(uint32_t numEntries)
{
  NMP::Memory::Format memReqs(sizeof(NodeIDsArray), NMP_NATURAL_TYPE_ALIGNMENT);
  memReqs += NMP::Memory::Format(sizeof(NodeID) * numEntries, NMP_NATURAL_TYPE_ALIGNMENT);
  memReqs.align();
  return memReqs;
}

//----------------------------------------------------------------------------------------------------------------------
NodeIDsArray* NodeIDsArray::init(
                                 NMP::Memory::Resource& memRes,
                                 uint32_t numEntries)
{
  NMP::Memory::Format memReqs(sizeof(NodeIDsArray), NMP_NATURAL_TYPE_ALIGNMENT);
  NodeIDsArray* result = (NodeIDsArray*)memRes.alignAndIncrement(memReqs);
  result->m_numEntries = numEntries;
  if (numEntries > 0)
  {
    result->m_nodeIDs = (NodeID*)memRes.alignAndIncrement(NMP::Memory::Format(sizeof(NodeID) * numEntries, NMP_NATURAL_TYPE_ALIGNMENT));
  }
  else
  {
    result->m_nodeIDs = NULL;
  }
  memRes.align(memReqs.alignment);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
void NodeIDsArray::locate()
{
  NMP::endianSwap(m_numEntries);
  if (m_nodeIDs)
  {
    REFIX_SWAP_PTR(NodeID, m_nodeIDs);
    NMP::endianSwapArray(m_nodeIDs, m_numEntries, sizeof(NodeID));
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NodeIDsArray::dislocate()
{
  if (m_nodeIDs)
  {
    NMP::endianSwapArray(m_nodeIDs, m_numEntries, sizeof(NodeID));
    UNFIX_SWAP_PTR(NodeID, m_nodeIDs);
  }
  NMP::endianSwap(m_numEntries);
}

//----------------------------------------------------------------------------------------------------------------------
void NodeIDsArray::relocate()
{
  if (m_numEntries > 0)
  {
    void* ptr = this;
    NMP_ASSERT(NMP_IS_ALIGNED(ptr, NMP_NATURAL_TYPE_ALIGNMENT));
    ptr = (void*)NMP::Memory::align((((uint8_t*)ptr) + sizeof(NodeIDsArray)), NMP_NATURAL_TYPE_ALIGNMENT);
    m_nodeIDs = (NodeID*)ptr;
  }
  else
  {
    m_nodeIDs = NULL;
  }
}

} // namespace MR

//----------------------------------------------------------------------------------------------------------------------
