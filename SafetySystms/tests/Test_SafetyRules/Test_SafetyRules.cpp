#include <gtest/gtest.h>
#include "SafetyRules/SafetyRules.h"
#include "SafetyRules/ISafetyRules.h"

namespace Test_SafetyRules_Namespace 
{

   using namespace safety;

   class SafetyRulesTest : public ::testing::Test
   {
   protected:
       using State = ISafetyRules::State;
       using Sub   = ISafetyRules::LoaderSub;
       using Ev    = ISafetyRules::Event;
   
       void SetUp() override
       {
           // Wire callbacks to record entry/exit and sub-actions
           uut.setOnEnterIdle([this]() { onEnterIdleCount++; });
           uut.setOnExitIdle([this]()  { onExitIdleCount++;  });
   
           uut.setOnEnterActive([this]() { onEnterActiveCount++; });
           uut.setOnExitActive([this]()  { onExitActiveCount++;  });
   
           uut.setOnEnterFaulted([this]() { onEnterFaultedCount++; });
           uut.setOnExitFaulted([this]()  { onExitFaultedCount++;  });
   
           uut.setOnEnterBuildPlateLoader([this]() { onEnterLoaderCount++; });
           uut.setOnExitBuildPlateLoader([this]()  { onExitLoaderCount++;  });
   
           uut.setOnRequestDoorOpen([this]()        { requestDoorOpenCount++;        });
           uut.setOnRequestLoadBuildPlate([this]()  { requestLoadBuildPlateCount++;  });
           uut.setOnRequestDoorClose([this]()       { requestDoorCloseCount++;       });
       }
   
       void expectState(State s)
       {
           EXPECT_EQ(uut.getState(), s);
       }
   
       void expectLoader(Sub sub)
       {
           EXPECT_EQ(uut.getLoaderSubstate(), sub);
       }
   
   protected:
       SafetyRules uut;
   
       // Counters for callbacks
       int onEnterIdleCount{0};
       int onExitIdleCount{0};
   
       int onEnterActiveCount{0};
       int onExitActiveCount{0};
   
       int onEnterFaultedCount{0};
       int onExitFaultedCount{0};
   
       int onEnterLoaderCount{0};
       int onExitLoaderCount{0};
   
       int requestDoorOpenCount{0};
       int requestLoadBuildPlateCount{0};
       int requestDoorCloseCount{0};
   };
   
   // Initial reset places machine in Idle and fires onEnterIdle once
   TEST_F(SafetyRulesTest, ResetStartsInIdle)
   {
       uut.reset();
       expectState(State::Idle);
       EXPECT_EQ(onEnterIdleCount, 1);
       EXPECT_EQ(onExitIdleCount, 0);
   }
   
   // Idle -> Active on evPowerOn
   TEST_F(SafetyRulesTest, IdleToActiveOnPowerOn)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       EXPECT_EQ(onExitIdleCount, 1);
       EXPECT_EQ(onEnterActiveCount, 1);
   }
   
   // Active -> Idle on evPowerOff
   TEST_F(SafetyRulesTest, ActiveToIdleOnPowerOff)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.dispatch(Ev::evPowerOff);
   
       expectState(State::Idle);
       EXPECT_EQ(onExitActiveCount, 1);
       EXPECT_EQ(onEnterIdleCount, 2); // initial + re-enter
   }
   
   // Active -> Faulted on evFault, recover Faulted -> Active on evPowerOn
   TEST_F(SafetyRulesTest, FaultFromActiveAndRecover)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.dispatch(Ev::evFault);
   
       expectState(State::Faulted);
       EXPECT_EQ(onEnterFaultedCount, 1);
       EXPECT_EQ(onExitActiveCount, 1);
   
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
       EXPECT_EQ(onExitFaultedCount, 1);
       EXPECT_EQ(onEnterActiveCount, 2); // first enter + after recovery
   }
   
   // startLoader ignored unless in Active
   TEST_F(SafetyRulesTest, StartLoaderIgnoredWhenNotActive)
   {
       uut.reset();
       uut.startLoader(); // still in Idle -> ignore
       expectState(State::Idle);
       EXPECT_EQ(onEnterLoaderCount, 0);
       EXPECT_EQ(requestDoorOpenCount, 0);
   }
   
   // Full happy-path through BuildPlateLoader submachine back to Active
   TEST_F(SafetyRulesTest, LoaderHappyPath)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.startLoader();
       expectState(State::BuildPlateLoader);
       expectLoader(Sub::OpenDoor);
   
       // Entry action for OpenDoor
       EXPECT_EQ(onEnterLoaderCount, 1);
       EXPECT_EQ(requestDoorOpenCount, 1);
   
       // OpenDoor -> DoorOpened
       uut.dispatch(Ev::evDoorOpened);
       expectLoader(Sub::DoorOpened);
       EXPECT_EQ(requestLoadBuildPlateCount, 1);
   
       // DoorOpened -> BuildPlateLoaded
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectLoader(Sub::BuildPlateLoaded);
       EXPECT_EQ(requestDoorCloseCount, 1);
   
       // BuildPlateLoaded -> completion -> Active (via evDoorClosed)
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Active);
       expectLoader(Sub::None);
       EXPECT_EQ(onExitLoaderCount, 1);
   }
   
   // Fault during loader from any substate routes to Faulted and exits submachine
   TEST_F(SafetyRulesTest, FaultDuringLoaderFromOpenDoor)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.startLoader();
   
       expectState(State::BuildPlateLoader);
       expectLoader(Sub::OpenDoor);
   
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
       expectLoader(Sub::None);
   
       EXPECT_EQ(onExitLoaderCount, 1);
       EXPECT_EQ(onEnterFaultedCount, 1);
   }
   
   // Additional substate fault coverage: fault from DoorOpened
   TEST_F(SafetyRulesTest, FaultDuringLoaderFromDoorOpened)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.startLoader();
       uut.dispatch(Ev::evDoorOpened); // into DoorOpened
   
       expectLoader(Sub::DoorOpened);
   
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
       expectLoader(Sub::None);
   }
   
   // Additional substate fault coverage: fault from BuildPlateLoaded
   TEST_F(SafetyRulesTest, FaultDuringLoaderFromBuildPlateLoaded)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.startLoader();
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectLoader(Sub::BuildPlateLoaded);
   
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
       expectLoader(Sub::None);
   }
   
   // Redundant transitions are ignored; state does not change
   TEST_F(SafetyRulesTest, RedundantTransitionIgnored)
   {
       uut.reset();
       expectState(State::Idle);
   
       // evPowerOff in Idle does nothing
       uut.dispatch(Ev::evPowerOff);
       expectState(State::Idle);
   
       // Double evPowerOn stays Active once
       uut.dispatch(Ev::evPowerOn);
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   }
   
   // Optional: provide a main if gtest_main is not linked
   #ifndef GTEST_HAS_MAIN
   int main(int argc, char** argv)
   {
       ::testing::InitGoogleTest(&argc, argv);
       return RUN_ALL_TESTS();
   }
   #endif

   // Idle: ignored events keep Idle
   TEST_F(SafetyRulesTest, IdleIgnoresUnrelatedEvents)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOff);
       expectState(State::Idle);
       uut.dispatch(Ev::evFault);
       expectState(State::Idle);
       uut.dispatch(Ev::evDoorOpened);
       expectState(State::Idle);
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectState(State::Idle);
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Idle);
   }
   
   // Active: ignored events keep Active
   TEST_F(SafetyRulesTest, ActiveIgnoresNonPowerNonFaultEvents)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.dispatch(Ev::evDoorOpened);
       expectState(State::Active);
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectState(State::Active);
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Active);
   }
   
   // Faulted: ignored events keep Faulted
   TEST_F(SafetyRulesTest, FaultedIgnoresNonPowerOnEvents)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
   
       uut.dispatch(Ev::evPowerOff);
       expectState(State::Faulted);
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
       uut.dispatch(Ev::evDoorOpened);
       expectState(State::Faulted);
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectState(State::Faulted);
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Faulted);
   }
   
   // Loader/OpenDoor: ignored events
   TEST_F(SafetyRulesTest, LoaderOpenDoorIgnoresOtherEvents)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.startLoader();
       expectState(State::BuildPlateLoader);
       expectLoader(Sub::OpenDoor);
   
       uut.dispatch(Ev::evPowerOff);         // ignored in submachine
       expectLoader(Sub::OpenDoor);
       uut.dispatch(Ev::evBuildPlateLoaded); // ignored
       expectLoader(Sub::OpenDoor);
       uut.dispatch(Ev::evDoorClosed);       // ignored
       expectLoader(Sub::OpenDoor);
   }
   
   // Loader/DoorOpened: ignored events
   TEST_F(SafetyRulesTest, LoaderDoorOpenedIgnoresOtherEvents)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.startLoader();
       uut.dispatch(Ev::evDoorOpened);
       expectLoader(Sub::DoorOpened);
   
       uut.dispatch(Ev::evPowerOff);   // ignored
       expectLoader(Sub::DoorOpened);
       uut.dispatch(Ev::evDoorClosed); // ignored
       expectLoader(Sub::DoorOpened);
   }
   
   // Loader/BuildPlateLoaded: ignored events
   TEST_F(SafetyRulesTest, LoaderBuildPlateLoadedIgnoresOtherEvents)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.startLoader();
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectLoader(Sub::BuildPlateLoaded);
   
       uut.dispatch(Ev::evPowerOff);         // ignored
       expectLoader(Sub::BuildPlateLoaded);
       uut.dispatch(Ev::evDoorOpened);       // ignored
       expectLoader(Sub::BuildPlateLoaded);
       uut.dispatch(Ev::evBuildPlateLoaded); // ignored
       expectLoader(Sub::BuildPlateLoaded);
   }
   
   // startLoader called again while already in loader should be ignored
   TEST_F(SafetyRulesTest, StartLoaderIgnoredWhenAlreadyInLoader)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.startLoader();
       expectState(State::BuildPlateLoader);
   
       uut.startLoader(); // should be ignored because current != Active
       expectState(State::BuildPlateLoader);
       expectLoader(Sub::OpenDoor); // still in initial loader substate
   }

   // Cover false-branches of top-level entry/exit callbacks: idle<->active with callbacks cleared.
   TEST_F(SafetyRulesTest, TopLevelTransitionsWithoutCallbacksFireNoHooks)
   {
       uut.reset();
   
       // Clear all top-level entry/exit callbacks
       uut.setOnEnterIdle({});
       uut.setOnExitIdle({});
       uut.setOnEnterActive({});
       uut.setOnExitActive({});
       uut.setOnEnterFaulted({});
       uut.setOnExitFaulted({});
       uut.setOnEnterBuildPlateLoader({});
       uut.setOnExitBuildPlateLoader({});
   
       // Perform Idle -> Active -> Idle
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.dispatch(Ev::evPowerOff);
       expectState(State::Idle);
   
       // Since hooks are cleared, none of these counters should have changed
       EXPECT_EQ(onEnterIdleCount, 1);   // only the initial SetUp() wired call on reset
       EXPECT_EQ(onExitIdleCount, 0);
       EXPECT_EQ(onEnterActiveCount, 0);
       EXPECT_EQ(onExitActiveCount, 0);
       EXPECT_EQ(onEnterFaultedCount, 0);
       EXPECT_EQ(onExitFaultedCount, 0);
       EXPECT_EQ(onEnterLoaderCount, 0);
       EXPECT_EQ(onExitLoaderCount, 0);
   }
   
   // Cover false-branches of loader entry-action callbacks for each substate.
   TEST_F(SafetyRulesTest, LoaderEntryActionsDoNothingWhenCallbacksCleared)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       // Clear only the loader entry-action callbacks
       uut.setOnRequestDoorOpen({});
       uut.setOnRequestLoadBuildPlate({});
       uut.setOnRequestDoorClose({});
   
       // Enter loader and walk through substates
       uut.startLoader();
       expectState(State::BuildPlateLoader);
       expectLoader(Sub::OpenDoor);
   
       // With callbacks cleared, these should remain zero
       EXPECT_EQ(requestDoorOpenCount, 0);
   
       uut.dispatch(Ev::evDoorOpened);
       expectLoader(Sub::DoorOpened);
       EXPECT_EQ(requestLoadBuildPlateCount, 0);
   
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectLoader(Sub::BuildPlateLoaded);
       EXPECT_EQ(requestDoorCloseCount, 0);
   
       // Complete loader back to Active (also exercise onExitBuildPlateLoader being cleared in next test)
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Active);
   }
   
   // Cover false-branch of onExitBuildPlateLoader specifically.
   TEST_F(SafetyRulesTest, LoaderExitCallbackCleared_NoExitHookInvocation)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
   
       // Keep other callbacks, but clear the loader exit hook only
       uut.setOnExitBuildPlateLoader({});
   
       uut.startLoader();                           // enter loader
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       uut.dispatch(Ev::evDoorClosed);              // exit loader -> Active
   
       expectState(State::Active);
       EXPECT_EQ(onExitLoaderCount, 0);             // exit hook should NOT have fired
   }
   
   // Cover false-branch of onEnterBuildPlateLoader specifically.
   TEST_F(SafetyRulesTest, LoaderEnterCallbackCleared_NoEnterHookInvocation)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
   
       // Clear loader enter hook only
       uut.setOnEnterBuildPlateLoader({});
   
       uut.startLoader();                           // enter loader
       expectState(State::BuildPlateLoader);
   
       EXPECT_EQ(onEnterLoaderCount, 0);            // enter hook should NOT have fired
   }
   
   // Cover false-branches of onExitActive/onEnterFaulted by faulting with those hooks cleared.
   TEST_F(SafetyRulesTest, FaultTransitionWithoutHooksFiresNoCallbacks)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       // Clear only active-exit and faulted-enter hooks
       uut.setOnExitActive({});
       uut.setOnEnterFaulted({});
   
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
   
       EXPECT_EQ(onExitActiveCount, 0);
       EXPECT_EQ(onEnterFaultedCount, 0);
   }

   // Idle <-> Active transitions with ALL callbacks cleared
   TEST_F(SafetyRulesTest, TransitionsWithoutAnyCallbacks)
   {
       uut.reset();
   
       // Clear all top-level callbacks
       uut.setOnEnterIdle({});
       uut.setOnExitIdle({});
       uut.setOnEnterActive({});
       uut.setOnExitActive({});
       uut.setOnEnterFaulted({});
       uut.setOnExitFaulted({});
       uut.setOnEnterBuildPlateLoader({});
       uut.setOnExitBuildPlateLoader({});
   
       // Do transitions
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.dispatch(Ev::evPowerOff);
       expectState(State::Idle);
   
       // Verify none of the counters incremented
       EXPECT_EQ(onEnterIdleCount, 1);   // only initial SetUp()
       EXPECT_EQ(onExitIdleCount, 0);
       EXPECT_EQ(onEnterActiveCount, 0);
       EXPECT_EQ(onExitActiveCount, 0);
       EXPECT_EQ(onEnterFaultedCount, 0);
       EXPECT_EQ(onExitFaultedCount, 0);
       EXPECT_EQ(onEnterLoaderCount, 0);
       EXPECT_EQ(onExitLoaderCount, 0);
   }
   
   // Loader entry actions with callbacks cleared
   TEST_F(SafetyRulesTest, LoaderActionsWithCallbacksCleared)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       // Clear only loader callbacks
       uut.setOnRequestDoorOpen({});
       uut.setOnRequestLoadBuildPlate({});
       uut.setOnRequestDoorClose({});
   
       uut.startLoader();
       expectLoader(Sub::OpenDoor);
       EXPECT_EQ(requestDoorOpenCount, 0);
   
       uut.dispatch(Ev::evDoorOpened);
       expectLoader(Sub::DoorOpened);
       EXPECT_EQ(requestLoadBuildPlateCount, 0);
   
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectLoader(Sub::BuildPlateLoaded);
       EXPECT_EQ(requestDoorCloseCount, 0);
   
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Active);
   }
   
   // Clear onExitBuildPlateLoader specifically
   TEST_F(SafetyRulesTest, LoaderExitWithoutCallback)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
   
       uut.setOnExitBuildPlateLoader({});
   
       uut.startLoader();
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       uut.dispatch(Ev::evDoorClosed);
   
       expectState(State::Active);
       EXPECT_EQ(onExitLoaderCount, 0);
   }
   
   // Clear onEnterBuildPlateLoader specifically
   TEST_F(SafetyRulesTest, LoaderEnterWithoutCallback)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
   
       uut.setOnEnterBuildPlateLoader({});
       uut.startLoader();
   
       expectState(State::BuildPlateLoader);
       EXPECT_EQ(onEnterLoaderCount, 0);
   }
   
   // Fault transition without active-exit/faulted-enter callbacks
   TEST_F(SafetyRulesTest, FaultTransitionWithoutCallbacks)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
   
       uut.setOnExitActive({});
       uut.setOnEnterFaulted({});
   
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
   
       EXPECT_EQ(onExitActiveCount, 0);
       EXPECT_EQ(onEnterFaultedCount, 0);
   }

   TEST_F(SafetyRulesTest, TransitionsWithoutCallbacks_ReachableOnly)
   {
       uut.reset();
       uut.setOnEnterIdle({});
       uut.setOnExitIdle({});
       uut.setOnEnterActive({});
       uut.setOnExitActive({});
       uut.setOnEnterFaulted({});
       uut.setOnExitFaulted({});
       uut.setOnEnterBuildPlateLoader({});
       uut.setOnExitBuildPlateLoader({});
   
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
       uut.dispatch(Ev::evPowerOff);
       expectState(State::Idle);
   
       EXPECT_EQ(onExitIdleCount, 0);
       EXPECT_EQ(onEnterActiveCount, 0);
       EXPECT_EQ(onExitActiveCount, 0);
       EXPECT_EQ(onEnterFaultedCount, 0);
       EXPECT_EQ(onExitFaultedCount, 0);
       EXPECT_EQ(onEnterLoaderCount, 0);
       EXPECT_EQ(onExitLoaderCount, 0);
   }
   
   TEST_F(SafetyRulesTest, LoaderActionsWithoutCallbacks_ReachableOnly)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.setOnRequestDoorOpen({});
       uut.setOnRequestLoadBuildPlate({});
       uut.setOnRequestDoorClose({});
   
       uut.startLoader();
       expectState(State::BuildPlateLoader);
       expectLoader(Sub::OpenDoor);
       EXPECT_EQ(requestDoorOpenCount, 0);
   
       uut.dispatch(Ev::evDoorOpened);
       expectLoader(Sub::DoorOpened);
       EXPECT_EQ(requestLoadBuildPlateCount, 0);
   
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectLoader(Sub::BuildPlateLoaded);
       EXPECT_EQ(requestDoorCloseCount, 0);
   
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Active);
   }
   
   TEST_F(SafetyRulesTest, LoaderExitHookCleared_NoCallback_ReachableOnly)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.setOnExitBuildPlateLoader({});
   
       uut.startLoader();
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       uut.dispatch(Ev::evDoorClosed);
   
       expectState(State::Active);
       EXPECT_EQ(onExitLoaderCount, 0);
   }
   
   TEST_F(SafetyRulesTest, LoaderEnterHookCleared_NoCallback_ReachableOnly)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.setOnEnterBuildPlateLoader({});
       uut.startLoader();
   
       expectState(State::BuildPlateLoader);
       EXPECT_EQ(onEnterLoaderCount, 0);
   }
   
   TEST_F(SafetyRulesTest, FaultTransitionWithoutCallbacks_ReachableOnly)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.setOnExitActive({});
       uut.setOnEnterFaulted({});
   
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
   
       EXPECT_EQ(onExitActiveCount, 0);
       EXPECT_EQ(onEnterFaultedCount, 0);
   }

   // Idle should ignore non-power events
   TEST_F(SafetyRulesTest, Idle_IgnoresNonPowerEvents)
   {
       uut.reset();                           // Idle
       uut.setOnEnterIdle({});                // keep callbacks cleared to hit false-branches
       uut.setOnExitIdle({});
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Idle);              // still Idle
   }
   
   // Active should ignore loader-substate events (unless routed via startLoader)
   TEST_F(SafetyRulesTest, Active_IgnoresSubstateEventsWithoutLoader)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
       // Clear hooks to cover false sides of if(callback)
       uut.setOnEnterActive({});
       uut.setOnExitActive({});
       // Out-of-context events should be ignored
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Active);
   }
   
   // Faulted should ignore everything except evPowerOn
   TEST_F(SafetyRulesTest, Faulted_IgnoresNonPowerOnEvents)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.dispatch(Ev::evFault);
       expectState(State::Faulted);
       // Clear hooks to take false branches
       uut.setOnEnterFaulted({});
       uut.setOnExitFaulted({});
       // These should be ignored in Faulted
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Faulted);
   }
   
   // Loader: OpenDoor substate should ignore out-of-order events
   TEST_F(SafetyRulesTest, Loader_OpenDoor_IgnoresOutOfOrderEvents)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.setOnEnterBuildPlateLoader({});
       uut.setOnExitBuildPlateLoader({});
       uut.setOnRequestDoorOpen({});          // clear action hooks
       uut.setOnRequestLoadBuildPlate({});
       uut.setOnRequestDoorClose({});
   
       uut.startLoader();                     // -> BuildPlateLoader/OpenDoor
       expectLoader(Sub::OpenDoor);
   
       // Wrong order: these should be ignored in OpenDoor
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectLoader(Sub::OpenDoor);
       uut.dispatch(Ev::evDoorClosed);
       expectLoader(Sub::OpenDoor);
   
       // Proceed correctly so we can continue other tests later if needed
       uut.dispatch(Ev::evDoorOpened);
       expectLoader(Sub::DoorOpened);
   }
   
   // Loader: DoorOpened substate should ignore evDoorClosed (wrong order)
   TEST_F(SafetyRulesTest, Loader_DoorOpened_IgnoresDoorClosed)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.setOnRequestDoorOpen({});
       uut.setOnRequestLoadBuildPlate({});
       uut.setOnRequestDoorClose({});
   
       uut.startLoader();                     // OpenDoor
       uut.dispatch(Ev::evDoorOpened);        // -> DoorOpened
       expectLoader(Sub::DoorOpened);
   
       // Wrong order here: close before loaded -> ignore
       uut.dispatch(Ev::evDoorClosed);
       expectLoader(Sub::DoorOpened);
   
       // Proceed correctly
       uut.dispatch(Ev::evBuildPlateLoaded);
       expectLoader(Sub::BuildPlateLoaded);
   }
   
   // Loader: BuildPlateLoaded should ignore evDoorOpened
   TEST_F(SafetyRulesTest, Loader_BuildPlateLoaded_IgnoresDoorOpened)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.setOnRequestDoorOpen({});
       uut.setOnRequestLoadBuildPlate({});
       uut.setOnRequestDoorClose({});
   
       uut.startLoader();                     // OpenDoor
       uut.dispatch(Ev::evDoorOpened);        // DoorOpened
       uut.dispatch(Ev::evBuildPlateLoaded);  // BuildPlateLoaded
       expectLoader(Sub::BuildPlateLoaded);
   
       // Wrong/extra: reopening event should be ignored here
       uut.dispatch(Ev::evDoorOpened);
       expectLoader(Sub::BuildPlateLoaded);
   
       // Finish correctly
       uut.dispatch(Ev::evDoorClosed);
       expectState(State::Active);
   }
   
   // Active: startLoader twice -> second request should be ignored (already in loader)
   TEST_F(SafetyRulesTest, Active_IgnoresRedundantStartLoader)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       expectState(State::Active);
   
       uut.startLoader();
       expectState(State::BuildPlateLoader);
   
       // If API routes this as a no-op when already in loader, this exercises ignored branch
       uut.startLoader();
       expectState(State::BuildPlateLoader);
   }

   // Covers: L190 (onExitActive=false), L226 (onEnterFaulted=false)
   TEST_F(SafetyRulesTest, BrFalse_ExitActive_EnterFaulted)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);           // Idle -> Active (hooks still set from SetUp)
       uut.setOnExitActive({});               // clear exit hook
       uut.setOnEnterFaulted({});             // clear enter hook
       uut.dispatch(Ev::evFault);             // Active -> Faulted
       expectState(State::Faulted);
       // No counter assertions needed: goal is to take the false branches
   }
   
   // Covers: L196 (onExitFaulted=false), L220 (onEnterActive=false)
   TEST_F(SafetyRulesTest, BrFalse_ExitFaulted_EnterActive)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);
       uut.dispatch(Ev::evFault);             // go to Faulted first
       uut.setOnExitFaulted({});
       uut.setOnEnterActive({});
       uut.dispatch(Ev::evPowerOn);           // Faulted -> Active
       expectState(State::Active);
   }
   
   // Covers: L184 (onExitIdle=false), L220 (onEnterActive=false)
   TEST_F(SafetyRulesTest, BrFalse_ExitIdle_EnterActive)
   {
       uut.reset();                            // currently Idle
       uut.setOnExitIdle({});
       uut.setOnEnterActive({});
       uut.dispatch(Ev::evPowerOn);            // Idle -> Active
       expectState(State::Active);
   }
   
   // Covers: L190 (onExitActive=false), L214 (onEnterIdle=false)
   TEST_F(SafetyRulesTest, BrFalse_ExitActive_EnterIdle)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);            // now Active
       uut.setOnExitActive({});
       uut.setOnEnterIdle({});
       uut.dispatch(Ev::evPowerOff);           // Active -> Idle
       expectState(State::Idle);
   }
   
   // Covers: L232 (onEnterBuildPlateLoader=false) on entry,
   //         L202 (onExitBuildPlateLoader=false) on exit
   TEST_F(SafetyRulesTest, BrFalse_EnterAndExit_LoaderTopHooks)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);            // Active
       uut.setOnEnterBuildPlateLoader({});
       uut.startLoader();                       // Active -> BuildPlateLoader (enter=false)
       expectState(State::BuildPlateLoader);
   
       // Prepare to exit submachine
       uut.setOnExitBuildPlateLoader({});
       // Walk through to completion -> exit back to Active (exit=false)
       uut.dispatch(Ev::evDoorOpened);
       uut.dispatch(Ev::evBuildPlateLoaded);
       uut.dispatch(Ev::evDoorClosed);         // -> Active
       expectState(State::Active);
   }
   
   // Covers: L247 (onRequestDoorOpen=false),
   //         L253 (onRequestLoadBuildPlate=false),
   //         L259 (onRequestDoorClose=false)
   TEST_F(SafetyRulesTest, BrFalse_LoaderEntryActions_AllThree)
   {
       uut.reset();
       uut.dispatch(Ev::evPowerOn);            // Active
       uut.setOnRequestDoorOpen({});
       uut.setOnRequestLoadBuildPlate({});
       uut.setOnRequestDoorClose({});
   
       uut.startLoader();                       // enter OpenDoor, action=false
       expectState(State::BuildPlateLoader);
       expectLoader(Sub::OpenDoor);
   
       uut.dispatch(Ev::evDoorOpened);          // -> DoorOpened, action=false
       expectLoader(Sub::DoorOpened);
   
       uut.dispatch(Ev::evBuildPlateLoaded);    // -> BuildPlateLoaded, action=false
       expectLoader(Sub::BuildPlateLoaded);
   }
   
   // Covers: L214 (onEnterIdle=false) on reset path specifically
   TEST_F(SafetyRulesTest, BrFalse_ResetEnterIdle)
   {
       // Clear before reset so reset() takes the false branch at L27/L214
       uut.setOnEnterIdle({});
       uut.reset();                             // should NOT call onEnterIdle
       expectState(State::Idle);
   }

}
