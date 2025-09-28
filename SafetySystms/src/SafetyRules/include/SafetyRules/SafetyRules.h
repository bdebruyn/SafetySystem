#pragma once
#include "SafetyRules/ISafetyRules.h"

namespace safety 
{

   class SafetyRules : public ISafetyRules
   {
   public:
      // Expose base enums locally for convenience
      using State = ISafetyRules::State;
      using BuildPlateLoaderState = ISafetyRules::BuildPlateLoaderState;
   
      // Support interfaces for actions
      struct IMachine
      {
         virtual ~IMachine() = default;
         virtual void requestDoorOpen() = 0;
         virtual void requestLoadBuildPlate() = 0;
         virtual void requestDoorClose() = 0;
         virtual void log() = 0; // Faulted entry
      };
   
      struct ILoader
      {
         virtual ~ILoader() = default;
         virtual void start() = 0; // Active -> BuildPlateLoader transition action
      };
   
   public:
      explicit SafetyRules(IMachine& m, ILoader& loader)
         : m_(m), loader_(loader)
      {
      }
   
      // ---------- Interface methods (inline) ----------
      void init() override
      {
         state_ = State::Idle;
         loaderState_ = BuildPlateLoaderState::None;
      }
   
      void evPowerOn() override
      {
         switch (state_)
         {
            case State::Idle:
            {
               exitState(State::Idle);
   
               enterState(State::Active);
               break;
            }
   
            case State::Faulted:
            {
               // Faulted --> Active : evPowerOn
               exitState(State::Faulted);
   
               enterState(State::Active);
               break;
            }
   
            default:
            {
               // no transition
               break;
            }
         }
      }
   
      void evPowerOff() override
      {
         if (state_ == State::Active)
         {
            exitState(State::Active);
   
            enterState(State::Idle);
         }
      }
   
      void evFault() override
      {
         if (state_ == State::BuildPlateLoader)
         {
            // BuildPlateLoader --> Faulted : evFault
            exitLoaderState(loaderState_);
            loaderState_ = BuildPlateLoaderState::None;
   
            exitState(State::BuildPlateLoader);
   
            enterState(State::Faulted);
            return;
         }
   
         if (state_ == State::Active)
         {
            // Active -> Faulted : evFault
            exitState(State::Active);
   
            enterState(State::Faulted);
         }
      }
   
      void evDoorOpened() override
      {
         if (state_ != State::BuildPlateLoader)
         {
            return;
         }
   
         if (loaderState_ == BuildPlateLoaderState::OpenDoor)
         {
            exitLoaderState(BuildPlateLoaderState::OpenDoor);
   
            enterLoaderState(BuildPlateLoaderState::DoorOpened);
         }
      }
   
      void evBuildPlateLoaded() override
      {
         if (state_ != State::BuildPlateLoader)
         {
            return;
         }
   
         if (loaderState_ == BuildPlateLoaderState::DoorOpened)
         {
            exitLoaderState(BuildPlateLoaderState::DoorOpened);
   
            enterLoaderState(BuildPlateLoaderState::BuildPlateLoaded);
         }
      }
   
      void evDoorClosed() override
      {
         if (state_ != State::BuildPlateLoader)
         {
            return;
         }
   
         if (loaderState_ == BuildPlateLoaderState::BuildPlateLoaded)
         {
            exitLoaderState(BuildPlateLoaderState::BuildPlateLoaded);
   
            enterLoaderState(BuildPlateLoaderState::Final);
   
            // Completion: BuildPlateLoaded --> [*] -> default transition back to Active
            completeBuildPlateLoader();
         }
      }
   
      State getState() const override
      {
         return state_;
      }
   
      BuildPlateLoaderState getLoaderState() const override
      {
         return loaderState_;
      }
   
      // ---------- Former private helpers (now public, inline) ----------
      void enterState(State s)
      {
         state_ = s;
   
         switch (s)
         {
            case State::Active:
            {
               // "Active : wait for safety event" (note only)
               break;
            }
   
            case State::Faulted:
            {
               // Faulted : entry / log()
               m_.log();
               break;
            }
   
            case State::BuildPlateLoader:
            {
               startBuildPlateLoader();
               break;
            }
   
            case State::Idle:
            default:
            {
               break;
            }
         }
      }
   
      void exitState(State /*s*/)
      {
         // No explicit exit actions in this diagram
      }
   
      void enterLoaderState(BuildPlateLoaderState s)
      {
         loaderState_ = s;
   
         switch (s)
         {
            case BuildPlateLoaderState::OpenDoor:
            {
               // OpenDoor : entry / m.requestDoorOpen()
               m_.requestDoorOpen();
               break;
            }
   
            case BuildPlateLoaderState::DoorOpened:
            {
               // DoorOpened : entry / m.requestLoadBuildPlate()
               m_.requestLoadBuildPlate();
               break;
            }
   
            case BuildPlateLoaderState::BuildPlateLoaded:
            {
               // BuildPlateLoaded : entry / m.requestDoorClose()
               m_.requestDoorClose();
               break;
            }
   
            case BuildPlateLoaderState::Final:
            case BuildPlateLoaderState::None:
            default:
            {
               break;
            }
         }
      }
   
      void exitLoaderState(BuildPlateLoaderState /*s*/)
      {
         // No explicit substate exit actions defined
      }
   
      void startBuildPlateLoader()
      {
         // Transition action on Active -> BuildPlateLoader
         loader_.start();
   
         // [*] -> OpenDoor with entry action
         loaderState_ = BuildPlateLoaderState::None;
   
         enterLoaderState(BuildPlateLoaderState::OpenDoor);
      }
   
      void completeBuildPlateLoader()
      {
         // BuildPlateLoader -> Active : default transition
         exitState(State::BuildPlateLoader);
   
         loaderState_ = BuildPlateLoaderState::None;
   
         enterState(State::Active);
      }
   
   private:
      IMachine& m_;
      ILoader&  loader_;
      State     state_{ State::Idle };
      BuildPlateLoaderState loaderState_{ BuildPlateLoaderState::None };
   };
   
} // namespace safety
