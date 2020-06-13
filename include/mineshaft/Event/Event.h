#ifndef MINESHAFT_EVENT_H
#define MINESHAFT_EVENT_H

namespace mc {

class EventDispatcher;

class Event {
public:
   enum EventKind {
      /// A keyboard button was pressed.
      KeyPressed = 0,

      /// A keyboard button was released.
      KeyReleased,

      /// A mouse button was pressed.
      MouseButtonPressed,

      /// A mouse button was released.
      MouseButtonReleased,

      /// Number of event kinds.
      _NumEvents,
   };

private:
   Event() = default;

   /// The kind of event this is.
   EventKind eventKind;

   /// Data for keyboard events.
   struct KeyPressedData {
      int pressedKey;
      bool rescheduled;
   };

   /// Data for mouse events.
   struct MouseBottonPressedData {
      int pressedButton;
      bool rescheduled;
   };

   /// Event-specific data.
   union {
      KeyPressedData keyboardData;
      MouseBottonPressedData mouseButtonData;
   };

public:
   /// Creates events.
   friend class EventDispatcher;

   /// \return The kind of event.
   EventKind getKind() const { return eventKind; }

   /// \return The pressed key.
   const KeyPressedData &getKeyboardEventData() const;

   /// \return The pressed key.
   const MouseBottonPressedData &getMouseButtonEventData() const;
};

} // namespace mc

#endif //MINESHAFT_EVENT_H
