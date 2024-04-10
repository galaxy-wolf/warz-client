// Copyright (c) 2010 NaturalMotion.  All Rights Reserved.
// Not to be copied, adapted, modified, used, distributed, sold,
// licensed or commercially exploited in any manner without the
// written consent of NaturalMotion.
//
// All non public elements of this software are the confidential
// information of NaturalMotion and may not be disclosed to any
// person nor used for any purpose not expressly approved by
// NaturalMotion in writing.

//----------------------------------------------------------------------------------------------------------------------
#include "GameAssetLoader.h"

#include "GameCharacterManager.h"
#include "simpleBundle/simpleBundle.h"
#include "morpheme/AnimSource/mrAnimSourceNSA.h"
#include <fstream>
#include <string>
#include <algorithm>
//----------------------------------------------------------------------------------------------------------------------

namespace Game
{
void AssetLoaderBasic::ParseCoreFile(
  void* buffer, 
  size_t bufferSize)
{
  if (!buffer || !bufferSize)
  {
    NMP_DEBUG_MSG("error: Valid bundle data expected (%p, size=%u)!\n", buffer, bufferSize);
    return;
  }

}
    //----------------------------------------------------------------------------------------------------------------------
void AssetLoaderBasic::evalBundleRequirements(
  uint32_t& numRegisteredAssets,
  uint32_t& numClientAssets,
  void*     buffer,
  size_t    bufferSize)
{
  numRegisteredAssets = 0;
  numClientAssets = 0;

  if (!buffer || !bufferSize)
  {
    NMP_DEBUG_MSG("error: Valid bundle data expected (%p, size=%u)!\n", buffer, bufferSize);
    return;
  }

  //----------------------------
  // Start parsing the bundle. 
  MR::UTILS::SimpleBundleReader bundleReader(buffer, bufferSize);

  MR::Manager::AssetType assetType;
  uint32_t assetID;
  uint8_t* fileGuid;
  void* asset;
  NMP::Memory::Format assetMemReqs;

  while (bundleReader.readNextAsset(assetType, assetID, fileGuid, asset, assetMemReqs))
  {
    if (assetType < MR::Manager::kAsset_NumAssetTypes)
    {
      // The pluginList is used only when loading the bundle and isn't registered with the manager
      if (assetType != MR::Manager::kAsset_PluginList)
      {
        ++numRegisteredAssets;
      }
    }
    else
    {
      ++numClientAssets;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
MR::NetworkDef* AssetLoaderBasic::loadBundle(
  void*            bundle,
  size_t           bundleSize,
  uint32_t*        registeredAssetIDs,
  void**           clientAssets,
  uint32_t         NMP_USED_FOR_ASSERTS(numRegisteredAssets),
  uint32_t         NMP_USED_FOR_ASSERTS(numClientAssets),
  MR::UTILS::SimpleAnimRuntimeIDtoFilenameLookup*& animFileLookup)
{
  animFileLookup = NULL;

  if (!bundle || !bundleSize)
  {
    NMP_DEBUG_MSG("error: Valid bundle data expected (%p, size=%u)!\n", bundle, bundleSize);
    return NULL;
  }

  MR::NetworkDef* networkDef = NULL;

  //----------------------------
  // Start parsing the bundle. The simple bundle has been written by the asset compiler and contains various types of 
  // assets like the network definition, rig definitions and animation markup but not the actual animation data.
  MR::UTILS::SimpleBundleReader bundleReader(bundle, (uint32_t)bundleSize);

  MR::Manager::AssetType assetType;
  void* asset;
  NMP::Memory::Format assetMemReqs;
  MR::RuntimeAssetID assetID;
  uint8_t* fileGuid = 0;

  uint32_t registeredAssetIndex = 0;
  uint32_t clientAssetIndex = 0;

  while (bundleReader.readNextAsset(assetType, assetID, fileGuid, asset, assetMemReqs))
  {
    //----------------------------
    // Only consider core runtime asset for registration with the manager. The locate process is also different for 
    // core and client assets, while core assets can be located using the manager, client assets need to be located 
    // explicitly - which could also be handled outside this method
    if (assetType < MR::Manager::kAsset_NumAssetTypes)
    {
      //----------------------------
      // Special case for plugin list.
      if (assetType == MR::Manager::kAsset_PluginList)
      {
        // The basic tutorials only the morpheme core so doesn't have any plugin restrictions
        continue;
      }

      // Grab locate function for this asset type
      const MR::AssetLocateFn locateFn = MR::Manager::getInstance().getAssetLocateFn(assetType);
      if (!locateFn)
      {
        //----------------------------
        // This may happen if you compiled your assets using an asset compiler with additional plug-ins registered
        // but are using a runtime with different modules. For more details see MR::registerCoreAssets() (called from
        // MR::Manager::initMorphemeLib()) and MR::initMorphemePhysics()
        NMP_DEBUG_MSG("error: Failed to locate core asset (type=%u, ID=%u)!\n", assetType, assetID);
        return NULL;
      }

      //----------------------------
      // If the asset is already registered just bump the reference count, if its a new ID the asset is loaded below
      void* const registeredAsset = (void*)MR::Manager::getInstance().getObjectPtrFromObjectID(assetID);
      if (registeredAsset)
      {
        asset = registeredAsset;
      }
      else
      {
        //----------------------------
        // Allocate memory to store the asset for runtime use. The memory is freed as the reference count goes to 
        // zero in unloadAssets() while the bundle memory can be freed right after this methods has completed
        void* const bundleAsset = asset;
        asset = NMPMemoryAllocateFromFormat(assetMemReqs).ptr;
        NMP::Memory::memcpy(asset, bundleAsset, assetMemReqs.size);

        //----------------------------
        // Locate the asset (in-place pointer fix-up)
        if (!locateFn(assetType, asset))
        {
          NMP_DEBUG_MSG("error: Failed to locate core asset (type=%u, ID=%u)!\n", assetType, assetID);
          return NULL;
        }

        //----------------------------
        // Register the object (initialises the reference count to zero)
        if (!MR::Manager::getInstance().registerObject(asset, assetType, assetID))
        {
          NMP_DEBUG_MSG("error: Failed to register asset (type=%u, ID=%u)!\n", assetType, assetID);
          return NULL;
        }
      }

      //----------------------------
      // Increment reference count
      MR::Manager::incObjectRefCount(assetID);

      //----------------------------
      // Special case for the network definition
      if (assetType == MR::Manager::kAsset_NetworkDef)
      {
        NMP_ASSERT(!networkDef);  // We only expect one network definition per bundle
        networkDef = (MR::NetworkDef*)asset;
      }

      //----------------------------
      // Log the asset ID for use in UnloadMorphemeNetwork().
      NMP_ASSERT(registeredAssetIndex < numRegisteredAssets);
      registeredAssetIDs[registeredAssetIndex++] = assetID;
    }
    else
    {
      //----------------------------
      // Allocate memory to store the asset for runtime use. The memory is freed in unLoadBundle() while the 
      // bundle memory can be freed right after this methods has completed.
      void* const bundleAsset = asset;
      asset = NMPMemoryAllocateFromFormat(assetMemReqs).ptr;
      NMP::Memory::memcpy(asset, bundleAsset, assetMemReqs.size);

      //----------------------------
      // Locate the asset (in-place pointer fix-up).
      switch (assetType)
      {
      case MR::UTILS::SimpleAnimRuntimeIDtoFilenameLookup::kAsset_SimpleAnimRuntimeIDtoFilenameLookup:
        NMP_ASSERT(!animFileLookup); // We only expect one filename lookup per bundle.
        animFileLookup = (MR::UTILS::SimpleAnimRuntimeIDtoFilenameLookup*)asset;
        animFileLookup->locate();
        break;

      default:
        NMP_DEBUG_MSG("warning: Failed to locate client asset (type=%u, ID=%u)!\n", assetType, assetID);
        break;
      }

      //----------------------------
      // Log the asset pointer for use in UnloadMorphemeNetwork().
      NMP_ASSERT(clientAssetIndex < numClientAssets);
      clientAssets[clientAssetIndex++] = asset;
    }
  }

  return networkDef;
}

//----------------------------------------------------------------------------------------------------------------------
void AssetLoaderBasic::unLoadBundle(
  const uint32_t* registeredAssetIDs,
  uint32_t        numRegisteredAssets,
  void* const*    clientAssets,
  uint32_t        numClientAssets)
{
  //----------------------------
  // Release registered assets but only free the associated memory if the reference count goes to zero.
  for (uint32_t i = 0; i < numRegisteredAssets; ++i)
  {
    const uint32_t assetId = registeredAssetIDs[i];

    //----------------------------
    // Unregister and free the asset if it's no longer referenced.
    if (MR::Manager::decObjectRefCount(assetId) == 0)
    {
      void* const asset = const_cast<void*>(MR::Manager::getInstance().getObjectPtrFromObjectID(assetId));
      MR::Manager::getInstance().unregisterObject(assetId);
      NMP::Memory::memFree(asset);
    }
  }

  //----------------------------
  // Free client assets.
  for (uint32_t i = 0; i < numClientAssets; ++i)
  {
    NMP::Memory::memFree(clientAssets[i]);
  }
}

//////////////////////////////////////////////////
// HZD asset loader
//////////////////////////////////////////////////

void HZDAssetLoader::ParseCoreFile(
  void* buffer, 
  size_t bufferSize)
{
  if (!buffer || !bufferSize)
  {
    NMP_DEBUG_MSG("error: Valid bundle data expected (%p, size=%u)!\n", buffer, bufferSize);
    return;
  }

}
    //----------------------------------------------------------------------------------------------------------------------
//void HZDAssetLoader::evalBundleRequirements(
//  uint32_t& numRegisteredAssets,
//  uint32_t& numClientAssets,
//  void*     buffer,
//  size_t    bufferSize)
//{
//  numRegisteredAssets = 0;
//  numClientAssets = 0;
//
//  if (!buffer || !bufferSize)
//  {
//    NMP_DEBUG_MSG("error: Valid bundle data expected (%p, size=%u)!\n", buffer, bufferSize);
//    return;
//  }
//
//  //----------------------------
//  // Start parsing the bundle. 
//  MR::UTILS::SimpleBundleReader bundleReader(buffer, bufferSize);
//
//  MR::Manager::AssetType assetType;
//  uint32_t assetID;
//  uint8_t* fileGuid;
//  void* asset;
//  NMP::Memory::Format assetMemReqs;
//
//  while (bundleReader.readNextAsset(assetType, assetID, fileGuid, asset, assetMemReqs))
//  {
//    if (assetType < MR::Manager::kAsset_NumAssetTypes)
//    {
//      // The pluginList is used only when loading the bundle and isn't registered with the manager
//      if (assetType != MR::Manager::kAsset_PluginList)
//      {
//        ++numRegisteredAssets;
//      }
//    }
//    else
//    {
//      ++numClientAssets;
//    }
//  }
//}

//----------------------------------------------------------------------------------------------------------------------
MR::NetworkDef* HZDAssetLoader::loadBundle(
  void*            bundle,
  size_t           bundleSize)
{
  if (!bundle || !bundleSize)
  {
    NMP_DEBUG_MSG("error: Valid bundle data expected (%p, size=%u)!\n", bundle, bundleSize);
    return NULL;
  }

  MR::NetworkDef* networkDef = NULL;

  //----------------------------
  // Start parsing the bundle. The simple bundle has been written by the asset compiler and contains various types of 
  // assets like the network definition, rig definitions and animation markup but not the actual animation data.
  MR::UTILS::HZDBundleReader bundleReader(bundle, (uint32_t)bundleSize);

  //MR::Manager::AssetType assetType;
  void* asset;
  // NMP::Memory::Format assetMemReqs;
  // MR::RuntimeAssetID assetID;
  // uint8_t* fileGuid = 0;

  // uint32_t registeredAssetIndex = 0;
  // uint32_t clientAssetIndex = 0;

  uint32_t unkown1;
  uint32_t unkown2;
  size_t size;

  size_t anim_type_ok_count = 0;
  size_t anim_asset_id = 2;

  while (bundleReader.readNextAsset(unkown1, unkown2, asset, size))
  {
      uint8_t * bytes = (uint8_t*)asset;
      if (unkown1 == 0xf90d8e41 && unkown2 == 0x344f9419) // MorphemeEventMappingResource
      {
      }
      else if (unkown1 == 0x42258efd && unkown2 == 0x9cb9b024) // MorphemeNetworkDefresource
      {

      }
      else if (unkown1 == 0x5c07569f && unkown2 == 0x985d2cd6) // MorphemeAnimationResource
      { 
          ++anim_asset_id;
          uint32_t max_of_channel_num = 0;
          // skip uuid
          bytes += 16;
          // finally found MorphemeAssets
          NMP::Memory::Format assetMemReqs;
          assetMemReqs.size = ((uint32_t*)bytes)[0];
          assetMemReqs.alignment = NMP_VECTOR_ALIGNMENT;
          bytes += sizeof(uint32_t);

          void* animation_data = NMPMemoryAllocateFromFormat(assetMemReqs).ptr;
          NMP::Memory::memcpy(animation_data, bytes, assetMemReqs.size);
          MR::AnimSourceBase* anim = (MR::AnimSourceBase*)animation_data;
          MR::AnimSourceNSA* nsa_anim = (MR::AnimSourceNSA*)animation_data;
          nsa_anim->zhaoqi_locate();

          // 修复 m_unchangingPosCompToAnimMap 这个是空的， 其实要从m_sampledPosCompToAnimMap 这个推断出来。
          std::vector<int> unchangingPosCompToAnimMap;
          {
              int sampledPoseCount = nsa_anim->m_sampledPosCompToAnimMap->m_numChannels;
              int iSamplePoseIndex = 0;
              for (uint16_t iChannel = 0; iChannel < nsa_anim->m_numChannelSets; ++iChannel)
              {
                  if (iSamplePoseIndex < sampledPoseCount &&
                      nsa_anim->m_sampledPosCompToAnimMap->m_animChannels[iSamplePoseIndex] == iChannel)
                  {
                      ++iSamplePoseIndex;
                  }
                  else {
                      unchangingPosCompToAnimMap.push_back(iChannel);
                  }
              }
              NMP_ASSERT(unchangingPosCompToAnimMap.size() + sampledPoseCount == nsa_anim->m_numChannelSets);
          }

          if (nsa_anim->m_sectionDataGood)
          {
              if (fabs(nsa_anim->m_duration * nsa_anim->m_sampleFrequency - (nsa_anim->m_sectionDataGood->m_numSectionAnimFrames - 1)) > 1e-3)
              {
                  NMP_STDOUT("num section anim frame not equal to duration * frequency!!");
              }
          }

          // 导出文本文件。
          std::ofstream animfile;
          std::string path = "F:/horizon_files/aloy_animations/";
          path = path + std::to_string(anim_asset_id) + ".txt";
          animfile.open(path);
          animfile << nsa_anim->m_duration << std::endl;
          animfile << nsa_anim->m_sampleFrequency << std::endl;
          animfile << nsa_anim->m_numChannelSets << std::endl;
          if (nsa_anim->m_sectionDataGood)
          {
              animfile << nsa_anim->m_sectionDataGood->m_numSectionAnimFrames << std::endl;
              std::vector<float> one_frame;
              for (int frameIndex = 0; frameIndex < nsa_anim->m_sectionDataGood->m_numSectionAnimFrames + 1; ++frameIndex)
              {
                  MR::AnimSourceNSA::HZDComputeAtFrame(anim, frameIndex, unchangingPosCompToAnimMap, one_frame);
                  for (int iChannel = 0; iChannel < nsa_anim->m_numChannelSets; ++iChannel)
                  {
                      for (int iComp = 0; iComp < 8; ++iComp)
                          animfile << one_frame[iChannel * 8 + iComp] << ",";
                      animfile << std::endl;
                  }
                  animfile << std::endl;
              }
          }
          else
          {
              animfile << 1 << std::endl;
			  std::vector<float> one_frame;
              // 有多帧， 但是只有一个固定帧， 那只能输出一帧喽。
			  MR::AnimSourceNSA::HZDComputeAtFrame(anim, 0, unchangingPosCompToAnimMap, one_frame);
              for (int iChannel = 0; iChannel < nsa_anim->m_numChannelSets; ++iChannel)
              {
                  for (int iComp = 0; iComp < 8; ++iComp)
                      animfile << one_frame[iChannel * 8 + iComp] << ",";
                  animfile << std::endl;
              }
			  animfile << std::endl;
          }

          animfile.close();

          if (nsa_anim->m_unknown1 != 0 ||
              nsa_anim->m_unknown2 != 0 ||
              nsa_anim->m_unknown3 != 0 ||
              nsa_anim->m_unknown4 != 0 ||
              nsa_anim->m_unknown5 != 0 ||
              nsa_anim->m_unknown6 != 0 ||
              nsa_anim->m_unknown7 != 0 ||
              nsa_anim->m_unknown8 != 0 )
          {
              NMP_STDOUT("found not zero unknown!!");
          }

          if (nsa_anim->m_unchangingData->m_unknown1 != 0 || nsa_anim->m_unchangingData->m_unknown2 != 0 || nsa_anim->m_unchangingData->m_unknown3 != 0 || nsa_anim->m_unchangingData->m_unknown4 != 0) 
          {
              NMP_STDOUT("found m_unchangingData not zero unknown!!");
          }

          //animation asset uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x unkown1: %x unkown2: %x \n
		  //bytes[3], bytes[2], bytes[1], bytes[0],
		  //bytes[5], bytes[4],
		  //bytes[7], bytes[6],
		  //bytes[8], bytes[9],
		  //bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15],
		  //unkown1, unkown2,
          NMP_STDOUT("%d : m_numChannelSet %d m_duration %fs m_numSectionAnimFrames %d, m_maxNumCompChannels %d\nsampledPosNumQuantis %d sampledQuatNumQuantis %d",
              anim_asset_id,
              MR::AnimSourceNSA::getNumChannelSets(anim),
              MR::AnimSourceNSA::getDuration(anim),
              nsa_anim->m_sectionDataGood ? nsa_anim->m_sectionDataGood->m_numSectionAnimFrames : 0,
              nsa_anim->m_maxNumCompChannels,
              nsa_anim->m_sampledPosNumQuantisationSets,
              nsa_anim->m_sampledQuatNumQuantisationSets
              );
          NMP_STDOUT("unchanging data pos num %d  quat num %d", nsa_anim->m_unchangingData->m_unchangingPosNumChannels,
              nsa_anim->m_unchangingData->m_unchangingQuatNumChannels);
		  max_of_channel_num = std::max(max_of_channel_num, nsa_anim->m_unchangingData->m_unchangingPosNumChannels);
		  max_of_channel_num = std::max(max_of_channel_num, nsa_anim->m_unchangingData->m_unchangingQuatNumChannels);

          int unchanging_pos_num = nsa_anim->m_unchangingPosCompToAnimMap->m_numChannels;
          NMP_STDOUT("unchange pos num %d", unchanging_pos_num);
          for (int i = 0; i < unchanging_pos_num; ++i)
          {
              int c = nsa_anim->m_unchangingPosCompToAnimMap->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not in channel set: %d:%d", i, c);
              }
			  // NMP_STDOUT("%d", c);
          }
          if (unchanging_pos_num > 0)
          {
              NMP_STDOUT("found not unchanging pos num > 0");
          }

          uint8_t channel_to_map_id[82];
          memset(channel_to_map_id, 0, sizeof(channel_to_map_id));

          int unchangeQuatChannel = nsa_anim->m_unchangingQuatCompToAnimMap->m_numChannels;
          NMP_STDOUT("unchange quat num %d", unchangeQuatChannel);
          for (int i = 0; i < nsa_anim->m_unchangingQuatCompToAnimMap->m_numChannels; ++i)
          {
              int c = nsa_anim->m_unchangingQuatCompToAnimMap->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not in Quat channel set: %d:%d", i, c);
              }
              //NMP_STDOUT("%d", c);
              channel_to_map_id[c] = 1;
          }

          int sample_pos_num = nsa_anim->m_sampledPosCompToAnimMap->m_numChannels;
          NMP_STDOUT("sample pos num %d", sample_pos_num);
          for (int i = 0; i < sample_pos_num; ++i)
          {
              int c = nsa_anim->m_sampledPosCompToAnimMap->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not sample pos map 1 channel set: %d:%d", i, c);
              }
              // NMP_STDOUT("%d", c);
          }
          max_of_channel_num = std::max(max_of_channel_num, (uint32_t)( nsa_anim->m_sampledPosCompToAnimMap->m_numChannels));

          int sample_quat_num = nsa_anim->m_sampledQuatCompToAnimMap->m_numChannels;
          NMP_STDOUT("sample quat num %d", sample_quat_num);
          for (int i = 0; i < sample_quat_num; ++i)
          {
              int c = nsa_anim->m_sampledQuatCompToAnimMap->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not in sample quat channel set: %d:%d", i, c);
              }
              // NMP_STDOUT("%d", c);
              if (channel_to_map_id[c] != 0)
              {
                  NMP_STDOUT("Error found channel used asset id : %d channel id %d", anim_asset_id, c);
              }
              channel_to_map_id[c] = 2;
          }
          max_of_channel_num = std::max(max_of_channel_num, (uint32_t)( nsa_anim->m_sampledQuatCompToAnimMap->m_numChannels));
          int unknownMap3Num = nsa_anim->m_unknownMap3->m_numChannels;
          NMP_STDOUT("unkonwn map 3 num %d", unknownMap3Num);
          if (unknownMap3Num == 0)
          {
              uint16_t unknown3 = nsa_anim->m_unknownMap3->m_animChannels[0];
              if (unknown3 != 65535)
				  NMP_STDOUT("unknown map3 %x", unknown3)
          }
          if (unknownMap3Num != 0)
          {
			  NMP_STDOUT("unknown map3 not zero ", unknownMap3Num)
          }
          for (int i = 0; i < unknownMap3Num; ++i)
          {
              int c = nsa_anim->m_unknownMap3->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not in unkown map 3 channel set: %d:%d", i, c);
              }
              // NMP_STDOUT("%d", c);
          }
          for (int i = 0; i < 82; ++i)
          {
              if (channel_to_map_id[i] == 0)
              {
                  NMP_STDOUT("Error found channel not used! anim asset id: %d : channel %d ", anim_asset_id, i);
              }
          }
          if (nsa_anim->m_maxOfChannelNum != max_of_channel_num)
          {
              NMP_STDOUT("max channel num %d not equal to real max of channel %d", nsa_anim->m_maxOfChannelNum, max_of_channel_num);
          }

          //if (animType != 2)
          //{
          //    NMP_STDOUT("read asset uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x unkown1: %x unkown2: %x size %d",
          //        bytes[3], bytes[2], bytes[1], bytes[0],
          //        bytes[5], bytes[4],
          //        bytes[7], bytes[6],
          //        bytes[8], bytes[9],
          //        bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15],
          //        unkown1, unkown2, size);
          //    NMP_STDOUT("found morphemeassets size %d anim type %d", animationLength, animType);
          //}
          //else {
          //    ++anim_type_ok_count;
          //}
      }
  }

  NMP_STDOUT("anim type ok count is %d", anim_type_ok_count);

  return networkDef;
}

//----------------------------------------------------------------------------------------------------------------------
void HZDAssetLoader::unLoadBundle(
  const uint32_t* registeredAssetIDs,
  uint32_t        numRegisteredAssets,
  void* const*    clientAssets,
  uint32_t        numClientAssets)
{
  //----------------------------
  // Release registered assets but only free the associated memory if the reference count goes to zero.
  for (uint32_t i = 0; i < numRegisteredAssets; ++i)
  {
    const uint32_t assetId = registeredAssetIDs[i];

    //----------------------------
    // Unregister and free the asset if it's no longer referenced.
    if (MR::Manager::decObjectRefCount(assetId) == 0)
    {
      void* const asset = const_cast<void*>(MR::Manager::getInstance().getObjectPtrFromObjectID(assetId));
      MR::Manager::getInstance().unregisterObject(assetId);
      NMP::Memory::memFree(asset);
    }
  }

  //----------------------------
  // Free client assets.
  for (uint32_t i = 0; i < numClientAssets; ++i)
  {
    NMP::Memory::memFree(clientAssets[i]);
  }
}


} // namespace Game

