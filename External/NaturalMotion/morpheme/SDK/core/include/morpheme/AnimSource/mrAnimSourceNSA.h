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
#ifndef MR_ANIMATION_SOURCE_NSA_H
#define MR_ANIMATION_SOURCE_NSA_H
//----------------------------------------------------------------------------------------------------------------------
#if !defined(NM_HOST_CELL_SPU) && !defined(NM_HOST_CELL_PPU)
  #ifdef NM_DEBUG
    #include <stdio.h>
  #endif // NM_DEBUG
#endif //#if !defined(NM_HOST_CELL_SPU) && !defined(NM_HOST_CELL_PPU)

#include <vector>

#include "NMPlatform/NMVector3.h"
#include "NMPlatform/NMBuffer.h"
#include "morpheme/mrTask.h"
#include "morpheme/AnimSource/mrAnimSourceUtils.h"
#include "morpheme/AnimSource/mrAnimSectionNSA.h"
#include "morpheme/AnimSource/mrTrajectorySourceNSA.h"
//----------------------------------------------------------------------------------------------------------------------

namespace MR
{

//----------------------------------------------------------------------------------------------------------------------
/// \defgroup SourceAnimationCompressedModule Source Animation Compressed
/// \ingroup SourceAnimationModule
///
/// Implementation of MR::AnimSourceBase with some level of compression.
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// \class MR::AnimSourceNSA
/// \brief Implementation of AnimSourceBase virtual base class.
/// \ingroup SourceAnimationCompressedModule
/// \see MR::AnimSourceBase
///
/// Fixed sample rate, quantised quaternion Quat and vector Pos channels.
/// The last animation frame must be the same as the first in cyclic animations.
/// Note this is a lossy quantisation scheme.
//----------------------------------------------------------------------------------------------------------------------
class AnimSourceNSA : public AnimSourceBase
{
public:
  // get HZD animation at frame
    static void HZDComputeAtFrame(
        const AnimSourceBase* sourceAnimation,
        int frameIndex,
		std::vector<int>& unchangingPosCompToAnimMap,
        std::vector<float>& oneFrame  // posX, posY, posZ, posW, quatX, quatY, quatZ, quatW; posX .... 
    );
  //-----------------------
  /// \brief Calculates the transforms for the requested rig bones.
  ///
  /// Samples the source animation at the requested time through its playback duration.
  /// Inserts the results into the appropriate bone indexes in the output buffer.
  static void computeAtTime(
    const AnimSourceBase* sourceAnimation,       ///< IN: Animation to sample.
    float                 time,                  ///< IN: Time at which to sample the animation (seconds).
    const AnimRigDef*     rig,                   ///< IN: Hierarchy and bind poses
    const RigToAnimMap*   mapFromRigToAnim,      ///< IN: So that results from sampled anim channels can be
                                                 ///<     stored in the correct rig bone ordered transform buffer slot.
    uint32_t              outputSubsetSize,      ///< IN: \see outputSubsetArray
    const uint16_t*       outputSubsetArray,     ///< IN: Channel set values will only be calculated if
                                                 ///<     output indexes appear both in this array and the 
                                                 ///<     mapping array. Ignored if size is zero.
    NMP::DataBuffer*      outputTransformBuffer, ///< OUT: Calculated transform data is stored and returned
                                                 ///<      in this provided set.
    NMP::MemoryAllocator* allocator);

  //-----------------------
  /// \brief Calculate the transform for the requested Animation bone index.
  ///
  /// Samples the request source animation channel at the specified time through its playback duration.
  static void computeAtTimeSingleTransform(
    const AnimSourceBase* sourceAnimation,      ///< IN: Animation to sample.
    float                 time,                 ///< IN: Time at which to sample the animation (seconds).
    uint32_t              rigChannelIndex,      ///< IN: Index of the rig bone to evaluate
    const AnimRigDef*     rig,                  ///< IN: Hierarchy and bind poses
    const RigToAnimMap*   mapFromRigToAnim,     ///< IN: So that results from sampled anim channels can be
                                                ///<     stored in the correct rig bone ordered transform buffer slot.
    NMP::Vector3*         pos,                  ///< OUT: The resulting position is filled in here.
    NMP::Quat*            quat,                 ///< OUT: The resulting orientation is filled in here.
    NMP::MemoryAllocator* allocator);

  //-----------------------
  /// \brief Returns the duration of this animation in seconds.
  static NM_INLINE float getDuration(
    const AnimSourceBase* sourceAnimation       ///< Animation to query.
  );

  /// \brief Returns the number of channel sets contained within this animation.
  ///
  /// A channel set usually contains the key frame animation data for a bone on a rig.
  static NM_INLINE uint32_t getNumChannelSets(
    const AnimSourceBase* sourceAnimation       ///< Animation to query.
  );

  static NM_INLINE uint32_t getNumFrameSections(
    const AnimSourceBase* sourceAnimation       ///< Animation to query.
  );
  static NM_INLINE uint32_t getNumChannelSections(
    const AnimSourceBase* sourceAnimation       ///< Animation to query.
  );

  /// \brief Returns the trajectory channel data related to this animation. If this function pointer is NULL then
  /// AnimSourceBase::animGetTrajectoryChannelData() returns a NULL trajectory control.
  static NM_INLINE const TrajectorySourceBase* getTrajectoryChannelData(
    const AnimSourceBase* sourceAnimation       ///< Animation to query.
  );

  /// \brief Return the string table which contains the names of the animation channels which this animation contains
  ///
  /// Note that this function may return a NULL pointer if no string table exists.
  static NM_INLINE const NMP::OrderedStringTable* getChannelNameTable(
    const AnimSourceBase* sourceAnimation       ///< Animation to query.
  );

  /// Accessors.
  float getSampleFrequency() const { return m_sampleFrequency; }

public:
  AnimSourceNSA();
  ~AnimSourceNSA() {}

  void locate();
  void zhaoqi_locate();
  void dislocate();
  void relocate();

  NM_INLINE uint32_t findSectionIndexFromFrameIndex(uint32_t animFrameIndex) const;

public:
  //-----------------------
  // Header information
  static AnimFunctionTable        m_functionTable;              ///< Function table needed by each source animation type that inherits
                                                                ///< from AnimSourceBase. Provides basic runtime polymorphisms, removing the
                                                                ///< requirement for virtual functions.
  float                           m_duration;                   ///< Playback length of this animation in seconds.
  float                           m_sampleFrequency;            ///< Number of key frame samples per second.
                                                                ///< The sample frequency is uniform across all the channels in an
                                                                ///< animation, but it can vary between animations.
  uint32_t                        m_numChannelSets;             ///< The number of channel sets contained within this animation.
                                                                ///< A channel set usually contains the key frame animation data for a
                                                                ///< bone on a rig.
  uint32_t                       m_maxOfChannelNum;         // unchanged; changed; pos, quat 四个Num 中最大的那个。

  //-----------------------
  // Compression to animation channel maps
  uint32_t                        m_maxNumCompChannels;               ///< The maximum number of compression channels that are used
  uint32_t                        m_unknown0;           // 全部等于0
  CompToAnimChannelMap*           m_unchangingQuatCompToAnimMap;      ///< The unchanging quat comp to anim channel map

  //                    todo: m_unchangingPosCompToAnimMap这部分找不到对应的内存， 但是我们可以从m_sampledPosCompToAnimMap 反推出来。
///< 长度都是0， 猜测： 大概率是unchanging 的sacale 部分。
  CompToAnimChannelMap*           m_unchangingPosCompToAnimMap;       ///< The unchanging pos comp to anim channel map

  CompToAnimChannelMap*           m_sampledPosCompToAnimMap;      
  CompToAnimChannelMap*           m_sampledQuatCompToAnimMap;     
  CompToAnimChannelMap*           m_unknownMap3;                    // 长度都是 0，猜测： 大概率是sample 的 scale部分。

  //-----------------------
  // Quantisation scale and offset information (Common to all sections)
  QuantisationScaleAndOffsetVec3  m_posMeansQuantisationInfo;         ///< Global quantisation range for the pos means.

  uint32_t                        m_unknown1;   // 全部等于0
  uint32_t                        m_unknown2;   // 全部等于0
                                                                      ///< Global quantisation range for the quat means is between [-1:1] (tan quarter angle rotvec)
  uint32_t                        m_sampledPosNumQuantisationSets;    ///< The number of quantisation sets used to encode the sectioned position channels
  uint32_t                        m_sampledQuatNumQuantisationSets;   ///< The number of quantisation sets used to encode the sectioned orientation channels                                                                      
  uint32_t                        m_unknown3; // 全部等于0
  uint32_t                        m_unknown4;  // 全部等于0
  QuantisationScaleAndOffsetVec3* m_sampledPosQuantisationInfo;       ///< Quantisation scale and offset info for all sampled pos channels
  QuantisationScaleAndOffsetVec3* m_sampledQuatQuantisationInfo;      ///< Quantisation scale and offset info for all sampled quat channels
  uint32_t                        m_unknown5;  // 全部等于0
  uint32_t                        m_unknown6;  // 全部等于0 


  //-----------------------
  // Unchanging channel set data
  UnchangingDataNSA*              m_unchangingData;             ///< Section data for the unchanging channels

  // 上面的十分确定是正确的解释。 因为UnchangingDataNSA 可以完全对应上。
  //---------------------------------------------------------

  //-----------------------
  // Sectioning information
  SectionDataNSA* m_sectionDataGood;  // 这玩意只有一个，不是2d的。
  ///< A 2D grid of data references to sectioned data (frame major, channel minor) - DMA alignmnent
  // 这个被上面的 m_sectionDataGood 替换掉了。可能是因为pc 上没有dma 读取！！
  // DataRef*                        m_sectionData;                ///< A 2D grid of data references to sectioned data (frame major, channel minor) - DMA alignmnent

  ///// 这个绝对正确， 已经和trajectoryNSA 完全对上了。
  //-----------------------
  // Trajectory
  TrajectorySourceNSA* m_trajectoryDataGood;     
  // 这个被上面的替换掉了， 可能是因为pc 上没有dma 读取！！
  // DataRef                         m_trajectoryData;             ///< Holds a set of animation data for handling a trajectory bone (can be NULL) - DMA alignment

  ///// 

  uint32_t                        m_unknown7;  // 全部等于0 
  uint32_t                        m_unknown8;  // 全部等于0



  DataRef*                        m_sectionData;                ///< A 2D grid of data references to sectioned data (frame major, channel minor) - DMA alignmnent
  DataRef                         m_trajectoryData;             ///< Holds a set of animation data for handling a trajectory bone (can be NULL) - DMA alignment





// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  // 后面的都是空没有用的！！！！
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

  // 而且我知道后面最多一个指针 或者两个int， 你们看着办。
  uint32_t                        m_maxSectionSize;             ///< The maximum section size amoungst the sectioned data
  uint32_t*                       m_sectionStartFrames;         ///< The start frames for each frame-wise section dividing up the sampled keyframe data
  uint32_t*                       m_sectionSizes;               ///< A 2D grid of memory sizes for the sectioned data (frame major, channel minor)

  uint32_t                        m_numChannelSections;         ///< The number of channel-wise sections dividing up the sampled keyframe data
  uint32_t                        m_numFrameSections;           ///< The number of frame-wise sections dividing up the sampled keyframe data

  //-----------------------
  // Channel names table
  NMP::OrderedStringTable*               m_channelNames;               ///< Optional string table holding the names of each channel set in this anim.

  CompToAnimChannelMap**          m_sampledPosCompToAnimMaps;         ///< A table of pointers to the channel-wise sampled pos comp to anim channel maps
  CompToAnimChannelMap**          m_sampledQuatCompToAnimMaps;        ///< A table of pointers to the channel-wise sampled quat comp to anim channel maps

};

//----------------------------------------------------------------------------------------------------------------------
// AnimSourceNSA inline functions.
//----------------------------------------------------------------------------------------------------------------------
NM_INLINE float AnimSourceNSA::getDuration(const AnimSourceBase* sourceAnimation)
{
  NMP_ASSERT(sourceAnimation);
  const AnimSourceNSA* compressedSource = static_cast<const AnimSourceNSA*> (sourceAnimation);
  return compressedSource->m_duration;
}

//----------------------------------------------------------------------------------------------------------------------
NM_INLINE uint32_t AnimSourceNSA::getNumChannelSets(const AnimSourceBase* sourceAnimation)
{
  NMP_ASSERT(sourceAnimation);
  const AnimSourceNSA* compressedSource = static_cast<const AnimSourceNSA*> (sourceAnimation);
  return compressedSource->m_numChannelSets;
}
NM_INLINE uint32_t AnimSourceNSA::getNumChannelSections(const AnimSourceBase* sourceAnimation)
{
  NMP_ASSERT(sourceAnimation);
  const AnimSourceNSA* compressedSource = static_cast<const AnimSourceNSA*> (sourceAnimation);
  return compressedSource->m_numChannelSections;
}

//----------------------------------------------------------------------------------------------------------------------
NM_INLINE uint32_t AnimSourceNSA::getNumFrameSections(const AnimSourceBase* sourceAnimation)
{
  NMP_ASSERT(sourceAnimation);
  const AnimSourceNSA* compressedSource = static_cast<const AnimSourceNSA*> (sourceAnimation);
  return compressedSource->m_numFrameSections;
}

//----------------------------------------------------------------------------------------------------------------------
NM_INLINE const TrajectorySourceBase* AnimSourceNSA::getTrajectoryChannelData(const AnimSourceBase* sourceAnimation)
{
  NMP_ASSERT(sourceAnimation);
  const AnimSourceNSA* compressedSource = static_cast<const AnimSourceNSA*> (sourceAnimation);
  return (const TrajectorySourceBase*) compressedSource->m_trajectoryData.m_data;
}

//----------------------------------------------------------------------------------------------------------------------
NM_INLINE const NMP::OrderedStringTable* AnimSourceNSA::getChannelNameTable(const AnimSourceBase* sourceAnimation)
{
  NMP_ASSERT(sourceAnimation);
  const AnimSourceNSA* compressedSource = static_cast<const AnimSourceNSA*> (sourceAnimation);
  return compressedSource->m_channelNames;
}

//----------------------------------------------------------------------------------------------------------------------
NM_INLINE uint32_t AnimSourceNSA::findSectionIndexFromFrameIndex(uint32_t animFrameIndex) const
{
  NMP_ASSERT(m_numFrameSections > 0);
  NMP_ASSERT(animFrameIndex <= m_sectionStartFrames[m_numFrameSections]);
  for (uint32_t iFrameSection = 0; iFrameSection < m_numFrameSections; ++iFrameSection)
  {
    if (animFrameIndex < m_sectionStartFrames[iFrameSection + 1])
      return iFrameSection;
  }
  
  return m_numFrameSections - 1;
}

} // namespace MR

//----------------------------------------------------------------------------------------------------------------------
#endif // MR_ANIMATION_SOURCE_NSA_H
//----------------------------------------------------------------------------------------------------------------------
