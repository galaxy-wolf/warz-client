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
  size_t anim_asset_id = 0;

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
          MR::AnimSourceNSA* qsa_anim = (MR::AnimSourceNSA*)animation_data;
          qsa_anim->zhaoqi_locate();

          if (qsa_anim->m_unknown1 != 0 ||
              qsa_anim->m_unknown2 != 0 ||
              qsa_anim->m_unknown3 != 0 ||
              qsa_anim->m_unknown4 != 0 ||
              qsa_anim->m_unknown5 != 0 ||
              qsa_anim->m_unknown6 != 0)
          {
              NMP_STDOUT("found not zero unknown!!");
          }

          //animation asset uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x unkown1: %x unkown2: %x \n
		  //bytes[3], bytes[2], bytes[1], bytes[0],
		  //bytes[5], bytes[4],
		  //bytes[7], bytes[6],
		  //bytes[8], bytes[9],
		  //bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15],
		  //unkown1, unkown2,
          NMP_STDOUT("%d : m_numChannelSet %d m_numFrameSections %f %d m_numChannelSections %d",
              anim_asset_id,
              MR::AnimSourceNSA::getNumChannelSets(anim),
              MR::AnimSourceNSA::getDuration(anim),
              MR::AnimSourceNSA::getNumFrameSections(anim),
              MR::AnimSourceNSA::getNumChannelSections(anim)
              );
          uint8_t channel_to_map_id[82];
          memset(channel_to_map_id, 0, sizeof(channel_to_map_id));

          int unchangePosChannel = qsa_anim->m_unchangingPosCompToAnimMap->m_numChannels;
          NMP_STDOUT("unchange pos num %d", unchangePosChannel);
          for (int i = 0; i < qsa_anim->m_unchangingPosCompToAnimMap->m_numChannels; ++i)
          {
              int c = qsa_anim->m_unchangingPosCompToAnimMap->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not in channel set: %d:%d", i, c);
              }
			  // NMP_STDOUT("%d", c);
              channel_to_map_id[c] = 1;
          }

          int unchangeQuatChannel = qsa_anim->m_unchangingQuatCompToAnimMap->m_numChannels;
          NMP_STDOUT("unchange quat num %d", unchangeQuatChannel);
          for (int i = 0; i < qsa_anim->m_unchangingQuatCompToAnimMap->m_numChannels; ++i)
          {
              int c = qsa_anim->m_unchangingQuatCompToAnimMap->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not in Quat channel set: %d:%d", i, c);
              }
              //NMP_STDOUT("%d", c);
          }

          int unknownMap1Num = qsa_anim->m_unknownMap1->m_numChannels;
          NMP_STDOUT("unkonwn map 1 num %d", unknownMap1Num);
          for (int i = 0; i < unknownMap1Num; ++i)
          {
              int c = qsa_anim->m_unknownMap1->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not in unkown map 1 channel set: %d:%d", i, c);
              }
              NMP_STDOUT("%d", c);
          }

          int unknownMap2Num = qsa_anim->m_sampledPosCompToAnimMap->m_numChannels;
          NMP_STDOUT("unkonwn map 2 num %d", unknownMap2Num);
          for (int i = 0; i < unknownMap2Num; ++i)
          {
              int c = qsa_anim->m_sampledPosCompToAnimMap->m_animChannels[i];
              if (c<0 || c> MR::AnimSourceNSA::getNumChannelSets(anim))
              {
                  NMP_STDOUT("Error found not in unkown map 2 channel set: %d:%d", i, c);
              }
              // NMP_STDOUT("%d", c);
              if (channel_to_map_id[c] != 0)
              {
                  NMP_STDOUT("Error found channel used asset id : %d channel id %d", anim_asset_id, c);
              }
              channel_to_map_id[c] = 2;
          }
          int unknownMap3Num = qsa_anim->m_unknownMap3->m_numChannels;
          NMP_STDOUT("unkonwn map 3 num %d", unknownMap3Num);
          if (unknownMap3Num == 0)
          {
              uint16_t unknown3 = qsa_anim->m_unknownMap3->m_animChannels[0];
              if (unknown3 != 65535)
				  NMP_STDOUT("unknown map3 %x", unknown3)
          }
          for (int i = 0; i < unknownMap3Num; ++i)
          {
              int c = qsa_anim->m_unknownMap3->m_animChannels[i];
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
    ////----------------------------
    //// Only consider core runtime asset for registration with the manager. The locate process is also different for 
    //// core and client assets, while core assets can be located using the manager, client assets need to be located 
    //// explicitly - which could also be handled outside this method
    //if (assetType < MR::Manager::kAsset_NumAssetTypes)
    //{
    //  //----------------------------
    //  // Special case for plugin list.
    //  if (assetType == MR::Manager::kAsset_PluginList)
    //  {
    //    // The basic tutorials only the morpheme core so doesn't have any plugin restrictions
    //    continue;
    //  }

    //  // Grab locate function for this asset type
    //  const MR::AssetLocateFn locateFn = MR::Manager::getInstance().getAssetLocateFn(assetType);
    //  if (!locateFn)
    //  {
    //    //----------------------------
    //    // This may happen if you compiled your assets using an asset compiler with additional plug-ins registered
    //    // but are using a runtime with different modules. For more details see MR::registerCoreAssets() (called from
    //    // MR::Manager::initMorphemeLib()) and MR::initMorphemePhysics()
    //    NMP_DEBUG_MSG("error: Failed to locate core asset (type=%u, ID=%u)!\n", assetType, assetID);
    //    return NULL;
    //  }

    //  //----------------------------
    //  // If the asset is already registered just bump the reference count, if its a new ID the asset is loaded below
    //  void* const registeredAsset = (void*)MR::Manager::getInstance().getObjectPtrFromObjectID(assetID);
    //  if (registeredAsset)
    //  {
    //    asset = registeredAsset;
    //  }
    //  else
    //  {
    //    //----------------------------
    //    // Allocate memory to store the asset for runtime use. The memory is freed as the reference count goes to 
    //    // zero in unloadAssets() while the bundle memory can be freed right after this methods has completed
    //    void* const bundleAsset = asset;
    //    asset = NMPMemoryAllocateFromFormat(assetMemReqs).ptr;
    //    NMP::Memory::memcpy(asset, bundleAsset, assetMemReqs.size);

    //    //----------------------------
    //    // Locate the asset (in-place pointer fix-up)
    //    if (!locateFn(assetType, asset))
    //    {
    //      NMP_DEBUG_MSG("error: Failed to locate core asset (type=%u, ID=%u)!\n", assetType, assetID);
    //      return NULL;
    //    }

    //    //----------------------------
    //    // Register the object (initialises the reference count to zero)
    //    if (!MR::Manager::getInstance().registerObject(asset, assetType, assetID))
    //    {
    //      NMP_DEBUG_MSG("error: Failed to register asset (type=%u, ID=%u)!\n", assetType, assetID);
    //      return NULL;
    //    }
    //  }

    //  //----------------------------
    //  // Increment reference count
    //  MR::Manager::incObjectRefCount(assetID);

    //  //----------------------------
    //  // Special case for the network definition
    //  if (assetType == MR::Manager::kAsset_NetworkDef)
    //  {
    //    NMP_ASSERT(!networkDef);  // We only expect one network definition per bundle
    //    networkDef = (MR::NetworkDef*)asset;
    //  }

    //  //----------------------------
    //  // Log the asset ID for use in UnloadMorphemeNetwork().
    //  NMP_ASSERT(registeredAssetIndex < numRegisteredAssets);
    //  registeredAssetIDs[registeredAssetIndex++] = assetID;
    //}
    //else
    //{
    //  //----------------------------
    //  // Allocate memory to store the asset for runtime use. The memory is freed in unLoadBundle() while the 
    //  // bundle memory can be freed right after this methods has completed.
    //  void* const bundleAsset = asset;
    //  asset = NMPMemoryAllocateFromFormat(assetMemReqs).ptr;
    //  NMP::Memory::memcpy(asset, bundleAsset, assetMemReqs.size);

    //  //----------------------------
    //  // Locate the asset (in-place pointer fix-up).
    //  switch (assetType)
    //  {
    //  case MR::UTILS::SimpleAnimRuntimeIDtoFilenameLookup::kAsset_SimpleAnimRuntimeIDtoFilenameLookup:
    //    NMP_ASSERT(!animFileLookup); // We only expect one filename lookup per bundle.
    //    animFileLookup = (MR::UTILS::SimpleAnimRuntimeIDtoFilenameLookup*)asset;
    //    animFileLookup->locate();
    //    break;

    //  default:
    //    NMP_DEBUG_MSG("warning: Failed to locate client asset (type=%u, ID=%u)!\n", assetType, assetID);
    //    break;
    //  }

    //  //----------------------------
    //  // Log the asset pointer for use in UnloadMorphemeNetwork().
    //  NMP_ASSERT(clientAssetIndex < numClientAssets);
    //  clientAssets[clientAssetIndex++] = asset;
    //}
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

