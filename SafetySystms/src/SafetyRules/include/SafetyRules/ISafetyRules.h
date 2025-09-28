#pragma once

namespace safety
{
   class ISafetyRules
   {
   public:
      virtual ~ISafetyRules() = default;
   
      // Top-level states
      enum class State
      {
         Idle,
         Active,
         Faulted,
         BuildPlateLoader
      };
   
      // Submachine states
      enum class BuildPlateLoaderState
      {
         None,
         OpenDoor,
         DoorOpened,
         BuildPlateLoaded,
         Final
      };
   
      // Initialization
      virtual void init() = 0;
   
      // Events
      virtual void evPowerOn() = 0;
      virtual void evPowerOff() = 0;
      virtual void evFault() = 0;
      virtual void evDoorOpened() = 0;
      virtual void evBuildPlateLoaded() = 0;
      virtual void evDoorClosed() = 0;
   
      // Introspection
      virtual State getState() const = 0;
      virtual BuildPlateLoaderState getLoaderState() const = 0;
   };

} // namespace safety


