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
#ifndef GAME_CHARACTER_H
#define GAME_CHARACTER_H
//----------------------------------------------------------------------------------------------------------------------
#include "morpheme/mrDispatcher.h"
#include "GameCharacterController.h"
#include "GameCharacterDef.h"
#include <string>
//----------------------------------------------------------------------------------------------------------------------


#ifndef ZQ_PATH_DEF
#define ZQ_PATH_DEF
extern std::string core_file_path;
extern std::string graph_base_path;

inline void auto_set_graph_base_path_by_core_file_path()
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
        NMP_STDOUT("\nError: can not get graph path: '/core/' or '\\core\\' not found in core file %s", core_file_path.c_str());
        return;
    }

    graph_base_path = core_file_path.substr(0, pos) + "/graph/";
	NMP_STDOUT("\ngraph base path is %s", graph_base_path.c_str());

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
    auto_set_graph_base_path_by_core_file_path();
}
#endif //ZQ_PATH_DEF 


namespace Game
{


//----------------------------------------------------------------------------------------------------------------------
// Game::Character
// Character represents an instance of a GameCharacterDef. Essentially the class wraps up the instantiated morpheme
// network and the information required to update and run an individual character. In a game scenario a GameCharacter
// Class could also store data for a character instance (current health/ammo etc).
//----------------------------------------------------------------------------------------------------------------------
class CharacterBasic
{
public:
  
  ~CharacterBasic() {};

  static CharacterBasic* create(HZDCharacterDef* networkDef);
  //----------------------------
  // Static function to create an instance of a morpheme network (GameCharacter)
  static CharacterBasic* create(CharacterDefBasic* gameCharacterDef);

  //----------------------------
  // Static function to release an instance of Character
  static void destroy(CharacterBasic* character);  

  //----------------------------
  // Update the world transforms from the network transforms. Root position and root rotation are just taken from the
  // character root bone here as we have an empty character controller.
  void updateWorldTransforms(const NMP::Matrix34& rootTransform);

  //----------------------------
  // Runs an update step to initialise the network.
  // The initial update with a delta time of zero will initialize active connections etc. It is important that the
  // initial active animation set is set before the first update so the resulting transform output matches the active
  // rig. This should be called exactly once after the network was created.
  void runInitialisingUpdateStep();

  //----------------------------
  // A simple update function that wraps all the required functionality to update an instance of a morpheme network in
  // one step.
  // For a more detailed implementation of a non-physics network see Game::CharacterNoPhysics update functions in
  // Game Management library.
  bool update(float timeDelta);

  //----------------------------
  // Accessors
  MR::Network*      getNetwork() const { return m_net; }

  // Returns the world transforms as evaluated by the most recent network update.
  NMP::DataBuffer*    getWorldTransforms() const { return m_worldTransforms; }

protected:

  CharacterBasic(CharacterDefBasic* gameCharacterDef):
    m_characterDef(gameCharacterDef),
    m_net(NULL),
    m_worldTransforms(NULL),
    m_temporaryMemoryAllocator(NULL),
    m_persistentMemoryAllocator(NULL)
  {
  };

  //----------------------------
  // Initialise the Game Character, allocate memory etc
  bool init(CharacterDefBasic* characterDef);

  //----------------------------
  // Release any memory allocated by this class
  bool term();

protected:

  CharacterDefBasic*    m_characterDef;    // The character definition that this class is an instance of.
  MR::Network*          m_net;             // The network instance for this character.

  NMP::DataBuffer* m_worldTransforms; // The transforms buffer (pose) of the character.

  NMP::FastHeapAllocator*   m_temporaryMemoryAllocator;
  NMP::MemoryAllocator*     m_persistentMemoryAllocator;

  NMP::Memory::Resource   m_bufferMemory;
};



} // namespace Game

//----------------------------------------------------------------------------------------------------------------------
#endif // GAME_CHARACTER_H
//----------------------------------------------------------------------------------------------------------------------

