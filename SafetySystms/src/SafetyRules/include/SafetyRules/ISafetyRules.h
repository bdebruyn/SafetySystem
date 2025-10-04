#pragma once
#include <functional>

namespace safety
{


   class ISafetyRules
   {
      public:
          // ----- Public types shared by interface and implementation
          enum class State
          {
              Idle,
              Active,
              Faulted,
              BuildPlateLoader
          };
      
          enum class LoaderSub
          {
              None,
              OpenDoor,
              DoorOpened,
              BuildPlateLoaded
          };
      
          enum class Event
          {
              evPowerOn,          // Idle -> Active, Faulted -> Active
              evPowerOff,         // Active -> Idle
              evFault,            // Active/BuildPlateLoader -> Faulted
              evDoorOpened,       // OpenDoor -> DoorOpened
              evBuildPlateLoaded, // DoorOpened -> BuildPlateLoaded
              evDoorClosed        // BuildPlateLoaded -> completion (-> Active)
          };
      
          using VoidFn = std::function<void()>;
      
      public:
          virtual ~ISafetyRules() = default;
      
          // ----- Lifecycle / control
          virtual void reset() = 0;
          virtual void dispatch(Event ev) = 0;
          virtual void startLoader() = 0;
      
          // ----- Observability
          virtual State getState() const = 0;
          virtual LoaderSub getLoaderSubstate() const = 0;
      
          // ----- Callbacks (entry/exit and substate entry actions)
          virtual void setOnEnterIdle(VoidFn cb) = 0;
          virtual void setOnExitIdle(VoidFn cb) = 0;
      
          virtual void setOnEnterActive(VoidFn cb) = 0;
          virtual void setOnExitActive(VoidFn cb) = 0;
      
          virtual void setOnEnterFaulted(VoidFn cb) = 0;
          virtual void setOnExitFaulted(VoidFn cb) = 0;
      
          virtual void setOnEnterBuildPlateLoader(VoidFn cb) = 0;
          virtual void setOnExitBuildPlateLoader(VoidFn cb) = 0;
      
          // Substate entry actions per diagram comments
          virtual void setOnRequestDoorOpen(VoidFn cb) = 0;        // OpenDoor : entry / requestDoorOpen()
          virtual void setOnRequestLoadBuildPlate(VoidFn cb) = 0;  // DoorOpened : entry / requestLoadBuildPlate()
          virtual void setOnRequestDoorClose(VoidFn cb) = 0;       // BuildPlateLoaded : entry / requestDoorClose()
   };

} // namespace safety
