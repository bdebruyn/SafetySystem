#pragma once

namespace safety
{

   class IRefinedSafetyRules
   {
   public:
       enum class Event
       {
           EvActive,
           EvIdle,
           EvFault,
           EvLoadBuildPlate,
           EvDoorOpened,
           EvBuildPlateLoaded,
           EvDoorClosed,
           EvBuildPlateUnloaded,
   
           EvRequestDoorOpen,
           EvRequestLoadBuildPlateNotification,
           EvRequestDoorClose
       };
   
       enum class TopState
       {
           Idle,
           Active,
           Faulted,
           BuildPlateLoader
       };
   
       enum class LoaderSubstate
       {
           None,
           OpenDoor,
           DoorOpened,
           BuildPlateLoaded,
           Final
       };
   
   public:
       virtual ~IRefinedSafetyRules() = default;
   
       // Feed events into the statechart
       virtual void dispatch(Event ev) = 0;
   
       // Accessors for current state
       virtual TopState getTopState() const = 0;
       virtual LoaderSubstate getLoaderSubstate() const = 0;
   
       // Optional string helpers for debugging
       static const char* toString(TopState s);
       static const char* toString(LoaderSubstate s);
   };

} // namespace safety
