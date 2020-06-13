#ifndef MINESHAFT_EVENTDISPATCHER_H
#define MINESHAFT_EVENTDISPATCHER_H

#include "mineshaft/Event/Event.h"

#include <llvm/ADT/DenseMap.h>
#include <queue>

namespace mc {

class Application;
using EventHandler = void(*)(EventDispatcher&, const Event&);

class EventDispatcher {
   /// Reference to the context object.
   Application &Ctx;

   /// Currently scheduled events.
   std::queue<Event> scheduledEvents;

   /// Registered event handlers.
   llvm::DenseMap<unsigned, std::vector<EventHandler>> eventHandlers;

public:
   explicit EventDispatcher(Application &Ctx);

   /// \return The context instance.
   Application &getContext() const { return Ctx; }

   /// Create a key press event.
   void keyPressed(int pressedKey, bool rescheduled = false);

   /// Create a key release event.
   void keyReleased(int pressedKey);

   /// Create a key press event.
   void mouseButtonPressed(int pressedKey, bool rescheduled = false);

   /// Create a key release event.
   void mouseButtonReleased(int pressedKey);

   /// Register an event handler.
   void registerEventHandler(Event::EventKind eventKind, EventHandler handler);

   /// Handle the scheduled events.
   void dispatchEvents();
};

} // namespace mc

#endif //MINESHAFT_EVENTDISPATCHER_H
