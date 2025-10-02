#pragma once
#include <functional>
#include "RefinedSafetyRules/IRefinedSafetyRules.h"

namespace safety 
{

   using Event = IRefinedSafetyRules::Event;
   using TopState = IRefinedSafetyRules::TopState;
   using LoaderSubstate = IRefinedSafetyRules::LoaderSubstate;

   class RefinedSafetyRules
   {
      public:
          explicit RefinedSafetyRules(std::function<void(Event)> onSendCallback = {})
              : onSend(std::move(onSendCallback))
          {
              // [*] --> Idle
              top = TopState::Idle;
              loader = LoaderSubstate::None;
          }
      
          // Dispatch an event into the state machine
          void dispatch(Event ev)
          {
              // Global (top-level) handling with delegation to substates when needed.
              switch (top)
              {
                  case TopState::Idle:
                  {
                      if (ev == Event::EvActive)
                      {
                          transitionToActive();
                          return;
                      }
      
                      // no other transitions from Idle
                      return;
                  }
      
                  case TopState::Active:
                  {
                      if (ev == Event::EvIdle)
                      {
                          transitionToIdle();
                          return;
                      }
      
                      if (ev == Event::EvFault)
                      {
                          transitionToFaulted();
                          return;
                      }
      
                      if (ev == Event::EvLoadBuildPlate)
                      {
                          enterBuildPlateLoader();
                          return;
                      }
      
                      return;
                  }
      
                  case TopState::Faulted:
                  {
                      if (ev == Event::EvIdle)
                      {
                          transitionToIdle();
                          return;
                      }
      
                      return;
                  }
      
                  case TopState::BuildPlateLoader:
                  {
                      // Superstate-level transition: fault can occur anytime inside the submachine
                      if (ev == Event::EvFault)
                      {
                          transitionToFaulted();
                          return;
                      }
      
                      // Route to the substate handler
                      handleLoaderSubstate(ev);
                      return;
                  }
              }
          }
      
          // Accessors (useful for tests / instrumentation)
          TopState getTopState() const
          {
              return top;
          }
      
          LoaderSubstate getLoaderSubstate() const
          {
              return loader;
          }
      
          // Optional helper: stringify state (handy for logs)
          static const char* toString(TopState s)
          {
              switch (s)
              {
                  case TopState::Idle: return "Idle";
                  case TopState::Active: return "Active";
                  case TopState::Faulted: return "Faulted";
                  case TopState::BuildPlateLoader: return "BuildPlateLoader";
              }
      
              return "Unknown";
          }
      
          static const char* toString(LoaderSubstate s)
          {
              switch (s)
              {
                  case LoaderSubstate::None: return "None";
                  case LoaderSubstate::OpenDoor: return "OpenDoor";
                  case LoaderSubstate::DoorOpened: return "DoorOpened";
                  case LoaderSubstate::BuildPlateLoaded: return "BuildPlateLoaded";
                  case LoaderSubstate::Final: return "Final";
              }
      
              return "Unknown";
          }
      
      public:
          // ----- Top-state transitions -----
          void transitionToIdle()
          {
              exitCurrentTopIfNeeded();
      
              top = TopState::Idle;
              loader = LoaderSubstate::None;
              // Idle entry (none defined in model)
          }
      
          void transitionToActive()
          {
              exitCurrentTopIfNeeded();
      
              top = TopState::Active;
              loader = LoaderSubstate::None;
              // Active entry comments:
              // // Power on.
              // // Wait for evLoadBuildPlate
          }
      
          void transitionToFaulted()
          {
              exitCurrentTopIfNeeded();
      
              top = TopState::Faulted;
              loader = LoaderSubstate::None;
              // Faulted entry:
              // // Violation detected. Power is off.
          }
      
          void enterBuildPlateLoader()
          {
              exitCurrentTopIfNeeded();
      
              top = TopState::BuildPlateLoader;
      
              // [*] -> OpenDoor
              enterLoaderSubstate(LoaderSubstate::OpenDoor);
          }
      
          void exitCurrentTopIfNeeded()
          {
              if (top == TopState::BuildPlateLoader)
              {
                  // Exit from submachine: reset substate
                  loader = LoaderSubstate::None;
              }
          }
      
          // ----- BuildPlateLoader substate handling -----
          void handleLoaderSubstate(Event ev)
          {
              switch (loader)
              {
                  case LoaderSubstate::OpenDoor:
                  {
                      if (ev == Event::EvDoorOpened)
                      {
                          enterLoaderSubstate(LoaderSubstate::DoorOpened);
                          return;
                      }
      
                      return;
                  }
      
                  case LoaderSubstate::DoorOpened:
                  {
                      if (ev == Event::EvBuildPlateLoaded)
                      {
                          enterLoaderSubstate(LoaderSubstate::BuildPlateLoaded);
                          return;
                      }
      
                      return;
                  }
      
                  case LoaderSubstate::BuildPlateLoaded:
                  {
                      if (ev == Event::EvDoorClosed)
                      {
                          // Reaching final inside submachine triggers completion transition
                          enterLoaderSubstate(LoaderSubstate::Final);
                          onSubmachineComplete();
                          return;
                      }
      
                      if (ev == Event::EvBuildPlateUnloaded)
                      {
                          transitionToFaulted();
                          return;
                      }
      
                      return;
                  }
      
                  case LoaderSubstate::Final:
                  case LoaderSubstate::None:
                  default:
                  {
                      // No behavior
                      return;
                  }
              }
          }
      
          void enterLoaderSubstate(LoaderSubstate next)
          {
              loader = next;
      
              if (loader == LoaderSubstate::OpenDoor)
              {
                  // OpenDoor : entry / send(evRequestDoorOpen)
                  emit(Event::EvRequestDoorOpen);
              }
      
              if (loader == LoaderSubstate::DoorOpened)
              {
                  // DoorOpened : entry / send(evRequestLoadBuildPlateNotification)
                  emit(Event::EvRequestLoadBuildPlateNotification);
              }
      
              if (loader == LoaderSubstate::BuildPlateLoaded)
              {
                  // BuildPlateLoaded : entry / send(evRequestDoorClose)
                  emit(Event::EvRequestDoorClose);
              }
          }
      
          void onSubmachineComplete()
          {
              // BuildPlateLoader -> Active (completion transition)
              if (top == TopState::BuildPlateLoader && loader == LoaderSubstate::Final)
              {
                  transitionToActive();
              }
          }
      
          void emit(Event e)
          {
              if (onSend)
              {
                  onSend(e);
              }
          }
      
      private:
          TopState top { TopState::Idle };
          LoaderSubstate loader { LoaderSubstate::None };
          std::function<void(Event)> onSend;
   };
 
} // namespace safety
