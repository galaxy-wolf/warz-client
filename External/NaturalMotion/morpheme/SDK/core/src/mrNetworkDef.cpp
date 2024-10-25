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
#include "morpheme/TransitConditions/mrTransitConditionControlParamBoolSet.h"
#include "morpheme/TransitConditions/mrTransitConditionNodeActive.h"
#include "morpheme/TransitConditions/mrTransitConditionInDurationEvent.h"
#include "morpheme/TransitConditions/mrTransitConditionControlParamFloatGreater.h"
#include "morpheme/TransitConditions/mrTransitConditionControlParamFloatLess.h"
#include "morpheme/TransitConditions/mrTransitConditionControlParamFloatInRange.h"
#include "morpheme/TransitConditions/mrTransitConditionInSyncEventRange.h"
#include "morpheme/TransitConditions/mrTransitConditionCrossedDurationFraction.h"
#include "morpheme/TransitConditions/mrTransitConditionOnMessage.h"
#include <vector>
#include <set>
#include <map>

#include <string>

#include <stdio.h>
//----------------------------------------------------------------------------------------------------------------------

std::string k_base_path = "F:/horizon_files/civillian/";

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

class TransitCondition_631: public TransitConditionDef
{
public:
    uint32_t m_unknown0; 
    float m_float; 
};

class TransitCondition_634: public TransitConditionDef
{
public:
    uint32_t m_unknown0; // 0x0084
    float m_float; // 0x00007a43   = 250
};

class TransitCondition_16384 : public TransitConditionDef
{
public:
  CPConnection  m_cpConnection; ///< Connection to the input Control Parameter attribute data to check against.
  bool          m_value;        ///< Condition will be satisfied if values are equal.
};

class TransitCondition_16385 : public TransitConditionDef
{
public:
  uint16_t m_unknown;        
};

class TransitCondition_16386 : public TransitConditionDef
{
public:
  float m_float; // 0x004041     12.0   
};

class TransitCondition_16387 : public TransitConditionDef
{
public:
    void locate()
    {
        REFIX_SWAP_PTR(char, m_some_name);
    }
public:
  char* m_some_name;        
  uint16_t m_unknown;
};

void output_condition(std::ofstream& condition_file, TransitConditionDef* condition_def)
{
  auto tt = condition_def->getType();
  condition_file << condition_def->getType() << std::endl;
  // 看过内存, 有些特殊时候会是00， 大部分时候是CD，变量未初始化？？
  condition_file << condition_def->getInvertFlag() << std::endl;

  auto output_attr_address = [&condition_file](const AttribAddress* attr_address) {
      condition_file << attr_address->m_owningNodeID << std::endl;
      condition_file << attr_address->m_targetNodeID << std::endl;
      condition_file << attr_address->m_semantic << std::endl;
      condition_file << attr_address->m_animSetIndex << std::endl;
      condition_file << attr_address->m_validFrame << std::endl;
      };

  if (tt == TRANSCOND_ON_MESSAGE_ID)
  { 
      TransitConditionDefOnMessage* cd = (TransitConditionDefOnMessage*)condition_def;
      condition_file << cd->getMessageID() << std::endl;
  }
  else if (tt == TRANSCOND_CONTROL_PARAM_BOOL_SET_ID)
  {
	  TransitConditionDefControlParamBoolSet* cd = (TransitConditionDefControlParamBoolSet*)condition_def;
	  condition_file << cd->getCPConnection()->m_sourceNodeID << std::endl;
	  condition_file << cd->getCPConnection()->m_sourcePinIndex << std::endl;
      condition_file << cd->getTestValue() << std::endl;
  }
  else if (tt == TRANSCOND_CROSSED_DURATION_FRACTION_ID)
  {
      TransitConditionDefCrossedDurationFraction* cd = (TransitConditionDefCrossedDurationFraction*)condition_def;
      output_attr_address(cd->getSourceNodeFractionalPosAttribAddress());
      condition_file << cd->getTriggerFraction() << std::endl;
      // todo:bool last???
  }
  else if (tt == TRANSCOND_CONTROL_PARAM_FLOAT_LESS_ID)
  {
      TransitConditionDefControlParamFloatLess* cd = (TransitConditionDefControlParamFloatLess*)condition_def;
      condition_file << cd->getCPConnection()->m_sourceNodeID << std::endl;
      condition_file << cd->getCPConnection()->m_sourcePinIndex<< std::endl;
      condition_file << cd->getTestValue() << std::endl;
      condition_file << cd->getOrEqual() << std::endl;
  }
  else if (tt == TRANSCOND_CONTROL_PARAM_FLOAT_GREATER_ID)
  {
      TransitConditionDefControlParamFloatGreater* cd = (TransitConditionDefControlParamFloatGreater*)condition_def;
      condition_file << cd->getCPConnection()->m_sourceNodeID << std::endl;
      condition_file << cd->getCPConnection()->m_sourcePinIndex<< std::endl;
      condition_file << cd->getTestValue() << std::endl;
      condition_file << cd->getOrEqual() << std::endl;
  }
  else if (tt == TRANSCOND_IN_SYNC_EVENT_RANGE_ID)
  {
      TransitConditionDefInSyncEventRange* cd = (TransitConditionDefInSyncEventRange*)condition_def;
      output_attr_address(cd->getSourceNodeSyncEventPlaybackPosAttribAddress());
      output_attr_address(cd->getSourceNodeSyncEventTrackAttribAddress());
      condition_file << cd->m_eventRangeStart << std::endl;
      condition_file << cd->m_eventRangeEnd << std::endl;
  }
  else if (tt == TRANSCOND_NODE_ACTIVE_ID)
  {
	  TransitConditionDefNodeActive* cd = (TransitConditionDefNodeActive*)condition_def;
	  condition_file << cd->getNodeID() << std::endl;
  }
  else if (tt == TRANSCOND_IN_DURATION_EVENT_ID)
  {
	  TransitConditionDefInDurationEvent * cd = (TransitConditionDefInDurationEvent *)condition_def;
      output_attr_address(cd->getSourceNodeSampledEventsAttribAddress());
	  condition_file << cd->m_eventTrackUserData << std::endl;
	  condition_file << cd->m_eventUserData << std::endl;
  }
  else if (tt == TRANSCOND_CONTROL_PARAM_FLOAT_IN_RANGE_ID)
  {
	  TransitConditionDefControlParamFloatInRange* cd = (TransitConditionDefControlParamFloatInRange*)condition_def;
      condition_file << cd->getCPConnection()->m_sourceNodeID << std::endl;
      condition_file << cd->getCPConnection()->m_sourcePinIndex << std::endl;
      condition_file << cd->getLowerTestValue() << std::endl;
      condition_file << cd->getUpperTestValue() << std::endl;
  }
  else if (tt == 631)
  {
	  TransitCondition_631* cd = (TransitCondition_631*)condition_def;
      condition_file << cd->m_unknown0 << std::endl;
      condition_file << cd->m_float<< std::endl;
  }
  else if (tt == 634)
  {
      TransitCondition_634* cd = (TransitCondition_634*)condition_def;
      condition_file << cd->m_unknown0 << std::endl;
      condition_file << cd->m_float << std::endl;
  }
  else if (tt == 16384)
  {
	  // 猜测这玩意就是条件取反？？
	  TransitCondition_16384* cd = (TransitCondition_16384*)condition_def;
	  condition_file << cd->m_cpConnection.m_sourceNodeID << std::endl;
	  condition_file << cd->m_cpConnection.m_sourcePinIndex << std::endl;
      condition_file << cd->m_value << std::endl;
  }
  else if (tt == 16385)
  {
	  // 猜测这玩意就是条件取反？？
	  TransitCondition_16385* cd = (TransitCondition_16385*)condition_def;
	  // 猜测这个可能是 1. node id， 判断node 是否active。 
	  // 2.其他condition 的id， 然后这个将别人的结果取反。
	  condition_file << cd->m_unknown << std::endl;
  }
  else if (tt == 16386)
  {
	  TransitCondition_16386* cd = (TransitCondition_16386*)condition_def;
      // 比大小吗？
	  condition_file << cd->m_float << std::endl;
  }
  else if (tt == 16387)
  {
	  TransitCondition_16387* cd = (TransitCondition_16387*)condition_def;
	  cd->locate();
	  condition_file << cd->m_some_name << std::endl;
	  condition_file << cd->m_unknown << std::endl;
  }
  else
  {
	  NMP_STDOUT("not process condition type");
  }

  condition_file << std::endl;
}

auto output_vector3 = [](std::ofstream& os, const NMP::Vector3& v)
	{
		os << v.x << ", " << v.y << ", " << v.z << std::endl;
	};
auto output_quat = [](std::ofstream& os, const NMP::Quat& q)
	{
		os << q.x << ", " << q.y << ", " << q.z << ", " << q.w << std::endl;
	};

void output_AttribDataBool_0(std::ofstream& os, AttribData* data)
{
    AttribDataBool* d = (AttribDataBool*)data;
    os << d->m_value << std::endl;
}

void output_AttribDataUInt_1(std::ofstream& os, AttribData* data)
{
    AttribDataUInt* d = (AttribDataUInt*)data;
    os << d->m_value << std::endl;
}

void output_AttribDataFloat_3(std::ofstream& os, AttribData* data)
{
    AttribDataFloat* d = (AttribDataFloat*)data;
    os << d->m_value << std::endl;
}

void output_AttribDataFloatArray_10(std::ofstream& os, AttribData* data)
{
    AttribDataFloatArray* d = (AttribDataFloatArray*)data;
    os << d->m_numValues << std::endl;
    for (int i = 0; i < d->m_numValues; ++i)
        os << d->m_values[i] << std::endl;
}

void output_AttribDataSyncEventTrack_19(std::ofstream& os, AttribData* data)
{
    AttribDataSyncEventTrack* d = (AttribDataSyncEventTrack*)data;
    EventTrackSync* ets = &(d->m_syncEventTrack);
    os << ets->m_startEventIndex << std::endl;
    os << ets->m_numEvents << std::endl;
    for (int i = 0; i < ets->m_numEvents; ++i)
    {
        EventDefDiscrete* e = &(ets->m_events[i]);
        os << e->m_startTime << std::endl;
        os << e->m_duration << std::endl;
        os << e->m_userData << std::endl;
    }
    os << ets->m_duration << std::endl;
    os << ets->m_durationReciprocal << std::endl;
    os << d->m_transitionOffset << std::endl;
}

void output_AttribDataSourceAnim_23(std::ofstream& os, AttribData* data)
{
    AttribDataSourceAnim * d = (AttribDataSourceAnim*)data;

    output_vector3(os, d->m_transformAtStartPos);
    output_quat(os, d->m_transformAtStartQuat);
    output_vector3(os, d->m_transformAtEndPos);
    output_quat(os, d->m_transformAtEndQuat);

    os << d->m_animAssetID << std::endl;

    os << (uint32_t) d->m_startSyncEventIndex << std::endl;
    os << (uint32_t) d->m_clipStartSyncEventIndex << std::endl;
    os << (uint32_t) d->m_clipEndSyncEventIndex << std::endl;
    os << (uint32_t) d->m_unknown2 << std::endl;

    os << d->m_clipStartFraction << std::endl;
    os << d->m_clipEndFraction << std::endl;

    os << d->m_sourceAnimDuration << std::endl;
    os << d->m_syncEventTrackIndex << std::endl;
    os << d->m_playBackwards << std::endl;
}

void output_AttribDataSourceEventTracks_25(std::ofstream& os, AttribData* data)
{
    AttribDataSourceEventTrackSet* d = (AttribDataSourceEventTrackSet*)(data);
    auto output_uint32_vector = [&os](uint32_t num, void* input_uint64_array)
        {
            os << num << std::endl;
            for (int i = 0; i < num; ++i)
                os << (uint32_t)((uint64_t*)input_uint64_array)[i] << std::endl;
        };
    output_uint32_vector(d->m_numDiscreteEventTracks, d->m_sourceDiscreteEventTracks);
    output_uint32_vector(d->m_numDurEventTracks, d->m_sourceDurEventTracks);
    output_uint32_vector(d->m_numCurveEventTracks, d->m_sourceCurveEventTracks);
}

auto output_CompressedDataBufferQuat= [](std::ofstream& os, MR::CompressedDataBufferQuat* data)
    {
        os << (data != nullptr) << std::endl;
        if (!data)
            return;
		// 256 个骨骼， 应该超不了。
        NMP_ASSERT(data->m_length < 256);
		NMP::Quat q_list[256];
		data->decode(q_list);
        os << data->m_length << std::endl;
        for (int i = 0; i < data->m_length; ++i)
            output_quat(os, q_list[i]);
    };

auto output_CompressedDataBufferVector3 = [](std::ofstream& os, MR::CompressedDataBufferVector3* data)
    {
        os << (data != nullptr) << std::endl;
        if (!data)
            return;
        NMP_ASSERT(data->m_length < 256);
		// 256 个骨骼， 应该超不了。
		NMP::Vector3 v_list[256];
		data->decode(v_list);
        os << data->m_length << std::endl;
        for (int i = 0; i < data->m_length; ++i)
            output_vector3(os, v_list[i]);
    };

auto output_ClosestAnimSourceData = [](std::ofstream& os, AttribDataClosestAnimDef::ClosestAnimSourceData* data)
    {
        output_CompressedDataBufferVector3(os, data->m_sourceTransformsPos);
        output_CompressedDataBufferQuat(os, data->m_sourceTransformsAtt);
        output_CompressedDataBufferVector3(os, data->m_sourceTransformsPosVel);
        output_CompressedDataBufferVector3(os, data->m_sourceTransformsAngVel);
    };

void output_AttribDataClosestAnimDef_39(std::ofstream& os, AttribData* data)
{
    AttribDataClosestAnimDef* d = (AttribDataClosestAnimDef*)(data);
    output_vector3(os, d->m_rootRotationAxis);
    output_quat(os, d->m_upAlignAtt);
    os << d->m_useRootRotationBlending << std::endl;
    os << d->m_fractionThroughSource << std::endl;
    os << d->m_maxRootRotationAngle << std::endl;

    os << d->m_numAnimJoints << std::endl;
    os << d->m_precomputeSourcesOffline << std::endl;
    os << d->m_useVelocity << std::endl;
    os << d->m_Unknown1 << std::endl;
    os << d->m_positionScaleFactor << std::endl;
    os << d->m_orientationScaleFactor << std::endl;
    os << d->m_influenceBetweenPosAndOrient << std::endl;

    os << d->m_numSources << std::endl;
    if (d->m_precomputeSourcesOffline)
    {
        for (int i = 0; i < d->m_numSources; ++i)
        {
            AttribDataClosestAnimDef::ClosestAnimSourceData* source_data = d->m_sourceDataArray[i];
            output_ClosestAnimSourceData(os, source_data);
        }
    }
    os << d->m_numDescendants << std::endl;
    for (int i = 0; i < d->m_numDescendants; ++i)
    {
        os << d->m_descendantIDs[i] << std::endl;
    }
}

void output_AttribDataClosestAnimDefAnimSet_40(std::ofstream& os, AttribData* data)
{
    AttribDataClosestAnimDefAnimSet* d = (AttribDataClosestAnimDefAnimSet*)data;
    os << d->m_numAnimWeights << std::endl;
    for (int i=0; i< d->m_numAnimWeights; ++i)
		os << d->m_weights[i] << std::endl;

    os << d->m_numEntries << std::endl;
    for (int i = 0; i < d->m_numEntries; ++i)
        os << d->m_animChannels[i] << std::endl;
    
    os << d->m_numEntries << std::endl;
    for (int i = 0; i < d->m_numEntries; ++i)
        os << d->m_rigChannels[i] << std::endl;
}

void output_AttribDataBlendFlags_116(std::ofstream& os, AttribData* data)
{
    AttribDataBlendFlags* d = (AttribDataBlendFlags*)data;
    os << d->m_alwaysBlendTrajectoryAndTransforms << std::endl;
    os << d->m_alwaysCombineSampledEvents << std::endl;
    os << d->m_unkown << std::endl;
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
    std::ofstream semantic_lookup_table_file;
    semantic_lookup_table_file.open(k_base_path + "semantic_lookup_table.txt");

    NMP::endianSwap(m_numSemanticLookupTables);
    semantic_lookup_table_file << m_numSemanticLookupTables << std::endl;
    REFIX_SWAP_PTR(SemanticLookupTable*, m_semanticLookupTables);
    for (uint32_t i = 0; i < m_numSemanticLookupTables; ++i)
    {
        REFIX_SWAP_PTR(SemanticLookupTable, m_semanticLookupTables[i]);
        m_semanticLookupTables[i]->locate();

        semantic_lookup_table_file << (uint32_t)(m_semanticLookupTables[i]->m_nodeType) << std::endl;
        semantic_lookup_table_file << (uint32_t)(m_semanticLookupTables[i]->m_numAttribsPerAnimSet) << std::endl;
        semantic_lookup_table_file << (uint32_t)(m_semanticLookupTables[i]->m_numSemantics) << std::endl;
        for (int j = 0; j < m_semanticLookupTables[i]->m_numSemantics; ++j)
        {
            semantic_lookup_table_file << (uint32_t)(m_semanticLookupTables[i]->m_semanticLookup[j]) << std::endl;
        }
        semantic_lookup_table_file << std::endl;
    }
    semantic_lookup_table_file.close();


    // misc in network.
    {
        std::ofstream network_misc_file;
        network_misc_file.open(k_base_path + "network_misc_file.txt");

	    // Output control parameter Node IDs and semantics
        if (m_emittedControlParamsInfo)
        {
            REFIX_SWAP_PTR(EmittedControlParamsInfo, m_emittedControlParamsInfo);
            m_emittedControlParamsInfo->locate();
            network_misc_file << "m_emittedControlParamsInfo" << std::endl;
            network_misc_file << m_emittedControlParamsInfo->m_numEmittedControlParamNodes << std::endl;
            for (int i = 0; i < m_emittedControlParamsInfo->m_numEmittedControlParamNodes; ++i)
            {
                network_misc_file << m_emittedControlParamsInfo->m_emittedControlParamsData[i].m_nodeID << std::endl;
            }
            network_misc_file << std::endl;
        }

        auto output_NodeIDsArray = [&network_misc_file](NodeIDsArray* a) {
            network_misc_file << a->m_numEntries << std::endl;
            for (int i=0; i< a->m_numEntries; ++i)
				network_misc_file << a->m_nodeIDs[i] << std::endl;
		};

		// State machine Node IDs array
        if (m_stateMachineNodeIDs)
        {
            REFIX_SWAP_PTR(NodeIDsArray, m_stateMachineNodeIDs);
            m_stateMachineNodeIDs->locate();
            network_misc_file << "m_stateMachineNodeIDs" << std::endl;
            output_NodeIDsArray(m_stateMachineNodeIDs);
            network_misc_file << std::endl;
        }

        if (some_string_struct)
        { 
            REFIX_SWAP_PTR(SomeStringStruct, some_string_struct);
            some_string_struct->locate();
            network_misc_file << "some_string_struct" << std::endl;
            network_misc_file << some_string_struct->id_count << std::endl;
            for (int i = 0; i < some_string_struct->id_count; ++i)
                network_misc_file << some_string_struct->id_list[i] << std::endl;
            network_misc_file << some_string_struct->string_count << std::endl;
            for (int i = 0; i < some_string_struct->string_count; ++i)
                network_misc_file << some_string_struct->string + some_string_struct->string_offset[i] << std::endl;
            network_misc_file << std::endl;
        }


		// Request Emitter Nodes ID array
        if (m_messageEmitterNodeIDs)
        {
            REFIX_SWAP_PTR(NodeIDsArray, m_messageEmitterNodeIDs);
            m_messageEmitterNodeIDs->locate();
            network_misc_file << "m_messageEmitterNodeIDs" << std::endl;
            output_NodeIDsArray(m_messageEmitterNodeIDs);
            network_misc_file << std::endl;
        }

		// Multiply connected Node IDs array
        if (m_multiplyConnectedNodeIDs)
        {
            REFIX_SWAP_PTR(NodeIDsArray, m_multiplyConnectedNodeIDs);
            m_multiplyConnectedNodeIDs->locate();
            network_misc_file << "m_multiplyConnectedNodeIDs" << std::endl;
            output_NodeIDsArray(m_multiplyConnectedNodeIDs);
            network_misc_file << std::endl;
        }

        auto output_IDMappedStringTable = [&network_misc_file](NMP::IDMappedStringTable* d) 
        {
                network_misc_file << d->m_NumEntrys << std::endl;
                for (int i = 0; i < d->m_NumEntrys; ++i)
                {
                    network_misc_file << d->m_IDs[i] << std::endl;
                    network_misc_file << d->m_HashTable[i] << std::endl;
                    network_misc_file << &(d->m_Data[d->m_Offsets[i]]) << std::endl;
                }
		};

	    //// State machine state to state ID mapping table
     //   if (m_stateMachineStateIDStringTable)
     //   {
     //       REFIX_SWAP_PTR(NMP::IDMappedStringTable, m_stateMachineStateIDStringTable);
     //       m_stateMachineStateIDStringTable->locate();
     //       network_misc_file << "m_stateMachineStateIDStringTable" << std::endl;
     //       output_IDMappedStringTable(m_stateMachineStateIDStringTable);
     //       network_misc_file << std::endl;
     //   }

		// NodeID to Node name mapping table
        // 没啥用的信息， 先不导出了。
        //if (m_nodeIDNamesTable)
        //{
        //    REFIX_SWAP_PTR(NMP::IDMappedStringTable, m_nodeIDNamesTable);
        //    m_nodeIDNamesTable->locate();
        //    network_misc_file << "m_nodeIDNamesTable" << std::endl;
        //    output_IDMappedStringTable(m_nodeIDNamesTable);
        //    network_misc_file << std::endl;
        //}

        auto output_OrderedStringTable = [&network_misc_file](NMP::OrderedStringTable* d)
            {
                network_misc_file << d->m_NumEntrys << std::endl;
                for (int i=0; i<d->m_NumEntrys; ++i)
					network_misc_file << &(d->m_Data[d->m_Offsets[i]]) << std::endl;
            };

	    // RequestID to Request name mapping table
        if (m_messageIDNamesTable)
        {
            REFIX_SWAP_PTR(NMP::OrderedStringTable, m_messageIDNamesTable);
            m_messageIDNamesTable->locate();
            network_misc_file << "m_messageIDNamesTable" << std::endl;
            output_OrderedStringTable(m_messageIDNamesTable);
            network_misc_file << std::endl;
        }

        if (m_unknown_ptr2)
        {
            REFIX_SWAP_PTR(NMP::OrderedStringTable, m_unknown_ptr2);
            m_unknown_ptr2->locate();
            network_misc_file << "m_unknown_ptr2" << std::endl;
            output_OrderedStringTable(m_unknown_ptr2);
            network_misc_file << std::endl;
        }

        if (m_eventTrackIDNamesTable)
        {
            REFIX_SWAP_PTR(NMP::OrderedStringTable, m_eventTrackIDNamesTable);
            m_eventTrackIDNamesTable->locate();
            network_misc_file << "m_eventTrackIDNamesTable" << std::endl;
            output_OrderedStringTable(m_eventTrackIDNamesTable);
            network_misc_file << std::endl;
        }

        auto output_NodeTagTable = [&network_misc_file](NodeTagTable* t) {
            uint32_t count = 0;
            for (int nodeID = 0; nodeID < t->m_numNodes; ++nodeID)
            {
                if (t->m_tagListLengths[nodeID] > 0)
                    ++count;
            }
            network_misc_file << count << std::endl;

            for (int nodeID = 0; nodeID < t->m_numNodes; ++nodeID)
            {
                if (t->m_tagListLengths[nodeID] > 0)
                {
                    network_misc_file << nodeID << std::endl;
                    network_misc_file << t->m_tagListLengths[nodeID] << std::endl;
                    for (int i = 0; i < t->m_tagListLengths[nodeID]; ++i)
                    {
                        network_misc_file << t->getTagOnNode(nodeID, i) << std::endl;
                    }
                }
            }
		};

        if (m_tagTable)
        {
            REFIX_SWAP_PTR(NodeTagTable, m_tagTable);
            m_tagTable->locate();
            network_misc_file << "m_tagTable" << std::endl;
            output_NodeTagTable(m_tagTable);
            network_misc_file << std::endl;
        }

        auto output_SomeIDMap = [&network_misc_file](SomeIDMap* m) {
            network_misc_file << m->allEntryListNum << std::endl;
            network_misc_file << m->oneEtryListNum << std::endl;
            for (int i = 0; i < m->allEntryListNum; ++i)
            {
                OneIDMapEntry* l = m->allEntryList[i];
                for (int j = 0; j < m->oneEtryListNum; ++j)
                {
                    network_misc_file << l[j].data0 << std::endl;
                    network_misc_file << l[j].data1 << std::endl;
                }
            }
		};

        if (some_id_map1)
        {
            REFIX_SWAP_PTR(SomeIDMap, some_id_map1);
            some_id_map1->locate();
            network_misc_file << "some_id_map1" << std::endl;
            output_SomeIDMap(some_id_map1);
            network_misc_file << std::endl;
        }

        if (some_id_map2)
        {
            REFIX_SWAP_PTR(SomeIDMap, some_id_map2);
            some_id_map2->locate();
            network_misc_file << "some_id_map2" << std::endl;
            output_SomeIDMap(some_id_map2);
            network_misc_file << std::endl;
        }

        auto output_MessageDistributor = [&network_misc_file](MessageDistributor* d)
            {
                network_misc_file << d->m_messageID << std::endl;
                network_misc_file << d->m_messageType << std::endl;
                network_misc_file << d->m_numNodeIDs << std::endl;
                for (int i=0; i< d->m_numNodeIDs; ++i)
					network_misc_file << d->m_nodeIDs[i] << std::endl;
				network_misc_file << d->m_numMessagePresets << std::endl;
                for (int i = 0; i < d->m_numMessagePresets; ++i)
                {
                    Message* m = d->m_messagePresets[i];
                    network_misc_file << d->m_messagePresetIndexNamesTable->getStringForID(i) << std::endl;
                    network_misc_file << m->m_data << std::endl;
                    network_misc_file << m->m_dataSize<< std::endl;
                    network_misc_file << m->m_id<< std::endl;
                    network_misc_file << m->m_type<< std::endl;
                    network_misc_file << m->m_status<< std::endl;
                }
            };

	  // MessageDistributors
        {
            REFIX_SWAP_PTR(MessageDistributor*, m_messageDistributors);
            network_misc_file << "m_messageDistributors" << std::endl;
            network_misc_file << m_numMessageDistributors << std::endl;
            for (uint32_t i = 0; i < m_numMessageDistributors; ++i)
            {
                // message ids can be sparse so this array may have gaps
				network_misc_file << "#" << i << std::endl;
                if (m_messageDistributors[i])
                {
                    network_misc_file << true << std::endl;
                    REFIX_SWAP_PTR(MessageDistributor, m_messageDistributors[i]);
                    m_messageDistributors[i]->locate();
                    output_MessageDistributor(m_messageDistributors[i]);
                }
                else {
                    network_misc_file << false << std::endl;
                }
            }
            network_misc_file << std::endl;
        }


        network_misc_file.close();
    }
  
  // NodeDefs
  std::ofstream myfile;
  myfile.open(k_base_path + "morpheme_graph.txt");
  std::ofstream state_machine_file;  // 这个是为了方便阅读。
  state_machine_file.open(k_base_path + "state_machine.txt");
  std::ofstream state_machine_raw;  // 这个是为了加载到python
  state_machine_raw.open(k_base_path + "state_machine_raw.txt");
  std::ofstream attrib_type_file;
  attrib_type_file.open(k_base_path + "attrib_type.txt");
  std::ofstream condition_file;
  condition_file.open(k_base_path + "conditions.txt");
  std::ofstream transition_def_file;
  transition_def_file.open(k_base_path + "transition_def.txt");
  std::ofstream all_attri_data_file;
  all_attri_data_file.open(k_base_path + "all_attri_data_file.txt");

  std::set<int> all_condition_types;

  std::set<int> all_attrib_data_types;

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
          all_attri_data_file << n->getNodeID() << std::endl;
		  all_attri_data_file << n->m_numAttribDataHandles << std::endl;
          std::set<int> animation_source_ids;
          //std::map<int, TrackRefStruct> semantic_2_track_ref;
          std::vector<int> attrib_data_types;
          bool has_state_machine = false;
          for (uint16_t i = 0; i < n->m_numAttribDataHandles; ++i)
          {
			  attrib_data_types.push_back(INVALID_ATTRIB_TYPE);
              all_attri_data_file << "#" << i << std::endl;
              if (n->m_nodeAttribDataHandles[i].m_attribData)
              {
                  // Locate the attrib data itself
                  AttribDataType type = n->m_nodeAttribDataHandles[i].m_attribData->getType();
				  all_attri_data_file << type << std::endl;

                  attrib_data_types[attrib_data_types.size() - 1] = type;
                  all_attrib_data_types.insert(type);

                  if (type == ATTRIB_TYPE_BOOL) // 0)
                  {
                      output_AttribDataBool_0(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_UINT) // 1)
                  {
                      output_AttribDataUInt_1(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_FLOAT) // 3)
                  {
				      output_AttribDataFloat_3(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_FLOAT_ARRAY) //10)
                  {
                      output_AttribDataFloatArray_10(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_SYNC_EVENT_TRACK)//19)
                  {
                      output_AttribDataSyncEventTrack_19(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_SOURCE_ANIM) // 23)
                  {
                      AttribDataSourceAnim* a = (AttribDataSourceAnim*)(n->m_nodeAttribDataHandles[i].m_attribData);
                      NMP_STDOUT("     %d : data source anim %d %f", i, a->m_animAssetID, a->m_sourceAnimDuration);
                      animation_source_ids.insert(a->m_animAssetID);

                      output_AttribDataSourceAnim_23(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_SOURCE_EVENT_TRACKS) //25)
                  {
                      output_AttribDataSourceEventTracks_25(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_CLOSEST_ANIM_DEF) //39)
                  {
                      output_AttribDataClosestAnimDef_39(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_CLOSEST_ANIM_DEF_ANIM_SET) //40)
                  {
				      output_AttribDataClosestAnimDefAnimSet_40(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
                  }
                  else if (type == ATTRIB_TYPE_BLEND_FLAGS) //116)
                  {
                      output_AttribDataBlendFlags_116(all_attri_data_file, n->m_nodeAttribDataHandles[i].m_attribData);
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
                      condition_file << "##" << std::endl;
                      for (ConditionIndex i = 0; i < a->getNumConditions(); ++i)
                      {
                          auto condition_def = a->getConditionDef(i);

                          {
                              auto tt = condition_def->getType();
                              all_condition_types.insert(tt);
                          }
                          output_condition(condition_file, condition_def);
                      }

                      state_machine_file << "node id:" << n->getNodeID() << std::endl;
                      state_machine_raw << n->getNodeID() << std::endl;

                      state_machine_file << "index: " << i << std::endl;
                      state_machine_file << "prioritiseGlobalTransitions: " << a->m_prioritiseGlobalTransitions << std::endl;
                      state_machine_raw << a->m_prioritiseGlobalTransitions << std::endl;

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
						  state_machine_file << "\t\t list " << 1 << " origin count:[" << sd->m_numExitBreakoutConditions << "]" << std::endl;
						  state_machine_raw << sd->m_numExitBreakoutConditions << std::endl;
                          if (sd->m_numExitBreakoutConditions > 1)
                          {
                              output_list(sd->m_exitBreakoutConditions, sd->m_numExitBreakoutConditions - 1, 1);
                          }
                          output_list(sd->m_exitConditionIndexes, sd->m_numExitConditions, 2);
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
                  else if (type == ATTRIB_TYPE_TRANSIT_DEF)
                  {
                      AttribDataTransitDef* a = (AttribDataTransitDef*)(n->m_nodeAttribDataHandles[i].m_attribData);
                      transition_def_file << n->getNodeID() << std::endl;
                      transition_def_file << a->m_duration << std::endl;
                      transition_def_file << a->m_destinationInitMethod << std::endl;
					  transition_def_file << a->m_destinationStartFraction << std::endl;
                      transition_def_file << a->m_destinationStartSyncEvent << std::endl;
					  transition_def_file << a->m_slerpTrajectoryPosition << std::endl;
					  transition_def_file << a->m_blendMode << std::endl;
					  transition_def_file << a->m_freezeSource << std::endl;
					  transition_def_file << a->m_freezeDest << std::endl;
					  transition_def_file << a->m_reversible << std::endl;
                      transition_def_file << a->m_reverseInputCPConnection.m_sourceNodeID << std::endl;
                      transition_def_file << a->m_reverseInputCPConnection.m_sourcePinIndex << std::endl;
					  transition_def_file << a->m_unknown << std::endl;  // 验证这个一直是0xfffffff << std::endlf
                      transition_def_file << std::endl;
				  }
              }
              else {
                  all_attri_data_file << -1 << std::endl;
              }
          }
          all_attri_data_file << std::endl;

          if (n->getNodeTypeID() == 104)
          {
              attrib_type_file << n->getNodeID() << " " << attrib_data_types.size() << " " << n->getNodeTypeID() << std::endl;
              for (auto tt : attrib_data_types)
              {
                  attrib_type_file << tt << std::endl;
              }
              attrib_type_file << std::endl;
              attrib_type_file << std::endl;
          }

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
  state_machine_file.close();
  state_machine_raw.close();
  attrib_type_file.close();
  condition_file.close();
  transition_def_file.close();
  all_attri_data_file.close();

  NMP_STDOUT(" condition type count %d", all_condition_types.size());
  for (auto tt : all_condition_types)
  {
      NMP_STDOUT(" \t%d", tt);
  }

  //NMP_STDOUT(" all attrib data types count %d", all_attrib_data_types.size());
  //for (auto tt : all_attrib_data_types)
  //{
  //    NMP_STDOUT(" \t%d", tt);
  //}


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
