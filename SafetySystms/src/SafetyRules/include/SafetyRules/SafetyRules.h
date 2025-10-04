#pragma once
#include "SafetyRules/ISafetyRules.h"
#include <cassert>

namespace safety 
{

   using Event = ISafetyRules::Event;
   using TopState = ISafetyRules::State;
   using LoaderSubstate = ISafetyRules::LoaderSub;

   class SafetyRules final : public ISafetyRules
   {
      public:
          // ----- Construction
          SafetyRules()
          {
              reset();
          }
      
          // ----- ISafetyRules (control)
          void reset() override
          {
              current = State::Idle;
              loader = LoaderSub::None;
      
              if (onEnterIdle)
              {
                  onEnterIdle();
              }
          }
      
          void dispatch(Event ev) override
          {
              switch (current)
              {
                  case State::Idle:
                  {
                      if (ev == Event::evPowerOn)
                      {
                          transitionTo(State::Active);
                      }
      
                      break;
                  }
      
                  case State::Active:
                  {
                      if (ev == Event::evPowerOff)
                      {
                          transitionTo(State::Idle);
                      }
      
                      else if (ev == Event::evFault)
                      {
                          transitionTo(State::Faulted);
                      }
      
                      break;
                  }
      
                  case State::Faulted:
                  {
                      if (ev == Event::evPowerOn)
                      {
                          transitionTo(State::Active);
                      }
      
                      break;
                  }
      
                  case State::BuildPlateLoader:
                  {
                      // Fault escape from any substate
                      if (ev == Event::evFault)
                      {
                          exitLoaderSubmachine();
                          transitionTo(State::Faulted);
                          break;
                      }
      
                      switch (loader)
                      {
                          case LoaderSub::OpenDoor:
                          {
                              if (ev == Event::evDoorOpened)
                              {
                                  enterLoaderSub(LoaderSub::DoorOpened);
                              }
      
                              break;
                          }
      
                          case LoaderSub::DoorOpened:
                          {
                              if (ev == Event::evBuildPlateLoaded)
                              {
                                  enterLoaderSub(LoaderSub::BuildPlateLoaded);
                              }
      
                              break;
                          }
      
                          case LoaderSub::BuildPlateLoaded:
                          {
                              if (ev == Event::evDoorClosed)
                              {
                                  // Completion of submachine -> Active
                                  exitLoaderSubmachine();
                                  transitionTo(State::Active);
                              }
      
                              break;
                          }
      
                          case LoaderSub::None:
                          default:
                          {
                              assert(false && "Invalid loader substate in BuildPlateLoader");
                              break;
                          }
                      }
      
                      break;
                  }
              }
          }
      
          void startLoader() override
          {
              if (current != State::Active)
              {
                  return; // ignore unless in Active
              }
      
              transitionTo(State::BuildPlateLoader);
      
              // Submachine initial: [*] -> OpenDoor
              enterLoaderSub(LoaderSub::OpenDoor);
          }
      
          // ----- ISafetyRules (observability)
          State getState() const override
          {
              return current;
          }
      
          LoaderSub getLoaderSubstate() const override
          {
              return loader;
          }
      
          // ----- ISafetyRules (callback setters)
          void setOnEnterIdle(VoidFn cb) override                 { onEnterIdle = std::move(cb); }
          void setOnExitIdle(VoidFn cb) override                  { onExitIdle = std::move(cb); }
      
          void setOnEnterActive(VoidFn cb) override               { onEnterActive = std::move(cb); }
          void setOnExitActive(VoidFn cb) override                { onExitActive = std::move(cb); }
      
          void setOnEnterFaulted(VoidFn cb) override              { onEnterFaulted = std::move(cb); }
          void setOnExitFaulted(VoidFn cb) override               { onExitFaulted = std::move(cb); }
      
          void setOnEnterBuildPlateLoader(VoidFn cb) override     { onEnterBuildPlateLoader = std::move(cb); }
          void setOnExitBuildPlateLoader(VoidFn cb) override      { onExitBuildPlateLoader = std::move(cb); }
      
          void setOnRequestDoorOpen(VoidFn cb) override           { onRequestDoorOpen = std::move(cb); }
          void setOnRequestLoadBuildPlate(VoidFn cb) override     { onRequestLoadBuildPlate = std::move(cb); }
          void setOnRequestDoorClose(VoidFn cb) override          { onRequestDoorClose = std::move(cb); }
      
      private:
          // ----- Top-level transitions with entry/exit hooks
          void transitionTo(State next)
          {
              if (next == current)
              {
                  return;
              }
      
              // Exit old top-level state
              switch (current)
              {
                  case State::Idle:
                  {
                      if (onExitIdle) onExitIdle();
                      break;
                  }
      
                  case State::Active:
                  {
                      if (onExitActive) onExitActive();
                      break;
                  }
      
                  case State::Faulted:
                  {
                      if (onExitFaulted) onExitFaulted();
                      break;
                  }
      
                  case State::BuildPlateLoader:
                  {
                      if (onExitBuildPlateLoader) onExitBuildPlateLoader();
                      break;
                  }
              }
      
              current = next;
      
              // Enter new top-level state
              switch (current)
              {
                  case State::Idle:
                  {
                      if (onEnterIdle) onEnterIdle();
                      break;
                  }
      
                  case State::Active:
                  {
                      if (onEnterActive) onEnterActive();
                      break;
                  }
      
                  case State::Faulted:
                  {
                      if (onEnterFaulted) onEnterFaulted();
                      break;
                  }
      
                  case State::BuildPlateLoader:
                  {
                      if (onEnterBuildPlateLoader) onEnterBuildPlateLoader();
                      break;
                  }
              }
          }
      
          // ----- Submachine helpers
          void enterLoaderSub(LoaderSub sub)
          {
              loader = sub;
      
              switch (loader)
              {
                  case LoaderSub::OpenDoor:
                  {
                      if (onRequestDoorOpen) onRequestDoorOpen(); // entry action
                      break;
                  }
      
                  case LoaderSub::DoorOpened:
                  {
                      if (onRequestLoadBuildPlate) onRequestLoadBuildPlate(); // entry action
                      break;
                  }
      
                  case LoaderSub::BuildPlateLoaded:
                  {
                      if (onRequestDoorClose) onRequestDoorClose(); // entry action
                      break;
                  }
      
                  // Dead Code 
                  case LoaderSub::None:
                  default:
                      break;
              }
          }
      
          void exitLoaderSubmachine()
          {
              loader = LoaderSub::None;
          }
      
      private:
          // ----- Data
          State     current { State::Idle };
          LoaderSub loader  { LoaderSub::None };
      
          // Entry/exit hooks
          VoidFn onEnterIdle;
          VoidFn onExitIdle;
      
          VoidFn onEnterActive;
          VoidFn onExitActive;
      
          VoidFn onEnterFaulted;
          VoidFn onExitFaulted;
      
          VoidFn onEnterBuildPlateLoader;
          VoidFn onExitBuildPlateLoader;
      
          // Substate entry actions
          VoidFn onRequestDoorOpen;
          VoidFn onRequestLoadBuildPlate;
          VoidFn onRequestDoorClose;
   };
 
} // namespace safety
