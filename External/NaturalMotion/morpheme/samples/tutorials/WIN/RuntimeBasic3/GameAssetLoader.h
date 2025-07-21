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
#ifdef _MSC_VER
#pragma once
#endif
#ifndef GAME_ASSET_LOADER_H
#define GAME_ASSET_LOADER_H
#include <string>

//----------------------------------------------------------------------------------------------------------------------
#include "GameCharacterDef.h"
//----------------------------------------------------------------------------------------------------------------------

#ifndef ZQ_PATH_DEF
#define ZQ_PATH_DEF
extern std::string core_file_path;
extern std::string animation_base_path;

inline void auto_set_animation_base_path_by_core_file_path()
{
    if (core_file_path.empty())
        return;
    size_t pos = core_file_path.rfind("/core/");
    
    if (pos == std::string::npos)
    {
		pos = core_file_path.rfind("\\core\\");
    }
    if (pos == std::string::npos)
    {
        NMP_STDOUT("\nError: can not get animation path: '/core/' or '\\core\\' not found in core file %s", core_file_path.c_str());
        return;
    }

    animation_base_path = core_file_path.substr(0, pos) + "/animations/";
	NMP_STDOUT("\n animation base path is %s", animation_base_path.c_str());

}


inline void try_read_core_file_path()
{
    if (!core_file_path.empty())
        return;
    char buff[1024];
    NMP_STDOUT("\nplease input core file path:")
    scanf("%s", &buff[0]);
    core_file_path = buff;
    NMP_STDOUT("\ncore file path is:%s", core_file_path.c_str());
    auto_set_animation_base_path_by_core_file_path();
}
#endif //ZQ_PATH_DEF 


namespace Game
{


class AssetLoaderBasic
{
public:
  //----------------------------
  // parse core file for HDZ 
  static void ParseCoreFile(
    void*     buffer,
    size_t    bufferSize);



  //----------------------------
  // Evaluate the asset requirements for the network stored in a simple bundle. This will allow us to create arrays large
  // enough to store the assets
  static void evalBundleRequirements(
    uint32_t& numRegisteredAssets,
    uint32_t& numClientAssets,
    void*     buffer,
    size_t    bufferSize);

  //----------------------------
  // This function iterates through the objects in a simple bundle and registers them with the morpheme runtime library.
  // It passes back a pointer to the (last) networkDefinition found in the bundle. This function is very simple and
  // simply fixes up the objects in-place, inside the bundle.
  //
  // This would be replaced with your own bundle loader and unloader if you do not use the simple bundle file format.
  static MR::NetworkDef* loadBundle(
    void*            bundle,
    size_t           bundleSize,
    uint32_t*        registeredAssetIDs,
    void**           clientAssets,
    uint32_t         NMP_USED_FOR_ASSERTS(numRegisteredAssets),
    uint32_t         NMP_USED_FOR_ASSERTS(numClientAssets),
    MR::UTILS::SimpleAnimRuntimeIDtoFilenameLookup*& animFileLookup);

  //----------------------------
  // This unloads the objects loaded in the GameAnimModule::loadBundle. We could avoid an iteration over the bundle file
  // by reusing object count information from the loadBundle() functions but this function has been designed to be
  // self-contained.
  static void unLoadBundle(
    const uint32_t* registeredAssetIDs,
    uint32_t        numRegisteredAssets,
    void* const*    clientAssets,
    uint32_t        numClientAssets);

};

class HZDAssetLoader
{
public:
  //----------------------------
  // parse core file for HDZ 
  static void ParseCoreFile(
    void*     buffer,
    size_t    bufferSize);




  ////----------------------------
  //// Evaluate the asset requirements for the network stored in a simple bundle. This will allow us to create arrays large
  //// enough to store the assets
  //static void evalBundleRequirements(
  //  uint32_t& numRegisteredAssets,
  //  uint32_t& numClientAssets,
  //  void*     buffer,
  //  size_t    bufferSize);

  //----------------------------
  // This function iterates through the objects in a simple bundle and registers them with the morpheme runtime library.
  // It passes back a pointer to the (last) networkDefinition found in the bundle. This function is very simple and
  // simply fixes up the objects in-place, inside the bundle.
  //
  // This would be replaced with your own bundle loader and unloader if you do not use the simple bundle file format.
  //static MR::NetworkDef* loadBundle(
  //  void*            bundle,
  //  size_t           bundleSize,
  //  uint32_t*        registeredAssetIDs,
  //  void**           clientAssets,
  //  uint32_t         NMP_USED_FOR_ASSERTS(numRegisteredAssets),
  //  uint32_t         NMP_USED_FOR_ASSERTS(numClientAssets),
  //  MR::UTILS::SimpleAnimRuntimeIDtoFilenameLookup*& animFileLookup);

  static MR::NetworkDef* HZDAssetLoader::loadBundle(
      void* bundle,
      size_t           bundleSize);

  //----------------------------
  // This unloads the objects loaded in the GameAnimModule::loadBundle. We could avoid an iteration over the bundle file
  // by reusing object count information from the loadBundle() functions but this function has been designed to be
  // self-contained.
  static void unLoadBundle(
    const uint32_t* registeredAssetIDs,
    uint32_t        numRegisteredAssets,
    void* const*    clientAssets,
    uint32_t        numClientAssets);

};


}

//----------------------------------------------------------------------------------------------------------------------
#endif // GAME_ASSET_LOADER_H
//----------------------------------------------------------------------------------------------------------------------
