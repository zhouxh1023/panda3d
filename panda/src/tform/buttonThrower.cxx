// Filename: buttonThrower.cxx
// Created by:  drose (12Mar02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "buttonThrower.h"

#include "buttonEvent.h"
#include "buttonEventList.h"
#include "dataNodeTransmit.h"
#include "throw_event.h"
#include "event.h"
#include "indent.h"
#include "dcast.h"

TypeHandle ButtonThrower::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
ButtonThrower::
ButtonThrower(const string &name) :
  DataNode(name)
{
  _button_events_input = define_input("button_events", ButtonEventList::get_class_type());
  _button_events_output = define_output("button_events", ButtonEventList::get_class_type());

  _button_events = new ButtonEventList;

  _specific_flag = true;
  _time_flag = false;
  _throw_buttons_active = false;
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::Destructor
//       Access: Published, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
ButtonThrower::
~ButtonThrower() {
}


////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::add_parameter
//       Access: Public
//  Description: Adds the indicated parameter to the list of
//               parameters that will be passed with each event
//               generated by this ButtonThrower.
////////////////////////////////////////////////////////////////////
void ButtonThrower::
add_parameter(const EventParameter &obj) {
  _parameters.push_back(obj);
}


////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::get_num_parameters
//       Access: Public
//  Description: Returns the number of parameters that have been added
//               to the list of parameters to be passed with each
//               event generated by this ButtonThrower.
////////////////////////////////////////////////////////////////////
int ButtonThrower::
get_num_parameters() const {
  return _parameters.size();
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::get_parameter
//       Access: Public
//  Description: Returns the nth parameter that has been added to the
//               list of parameters passed with each event generated
//               by this ButtonThrower.
////////////////////////////////////////////////////////////////////
EventParameter ButtonThrower::
get_parameter(int n) const {
  nassertr(n >= 0 && n < (int)_parameters.size(), EventParameter(0));
  return _parameters[n];
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::add_throw_button
//       Access: Published
//  Description: Adds a new button to the set of buttons that the
//               ButtonThrower explicitly processes.
//
//               If set_throw_buttons_active is false (which is the
//               default), the ButtonThrower will process all buttons.
//               Otherwise, the ButtonThrower will only process events
//               for the button(s) explicitly named by this function;
//               buttons not on the list will be ignored by this
//               object and passed on downstream to the child node(s)
//               in the data graph.  A button that *is* on the list
//               will be processed by the ButtonThrower and not passed
//               on to the child node(s).
//
//               The return value is true if the button is added, or
//               false if it was already in the set.
////////////////////////////////////////////////////////////////////
bool ButtonThrower::
add_throw_button(const ModifierButtons &mods, const ButtonHandle &button) {
  ThrowButtonDef &def = _throw_buttons[button];

  // This is a vector of ModifierButtons for which the indicated
  // button is handled.  Make sure the current ModifierButtons object
  // is not already on the list.
  ThrowButtonDef::iterator di;
  for (di = def.begin(); di != def.end(); ++di) {
    if (mods.matches(*di)) {
      return false;
    }
  }

  def.push_back(mods);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::remove_throw_button
//       Access: Published
//  Description: Removes the indicated button from the set of buttons
//               that the ButtonThrower explicitly processes.  See
//               add_throw_button().
//
//               The return value is true if the button is removed, or
//               false if it was not on the set.
////////////////////////////////////////////////////////////////////
bool ButtonThrower::
remove_throw_button(const ModifierButtons &mods, const ButtonHandle &button) {
  ThrowButtons::iterator ti = _throw_buttons.find(button);
  if (ti == _throw_buttons.end()) {
    // No buttons of this kind are in the set.
    return false;
  }

  ThrowButtonDef &def = (*ti).second;

  // This is a vector of ModifierButtons for which the indicated
  // button is handled.
  ThrowButtonDef::iterator di;
  for (di = def.begin(); di != def.end(); ++di) {
    if (mods.matches(*di)) {
      def.erase(di);
      if (def.empty()) {
        _throw_buttons.erase(ti);
      }
      return true;
    }
  }

  // The indicated ModifierButtons are not applied to this button in
  // the set.
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::has_throw_button
//       Access: Published
//  Description: Returns true if the indicated button is on the set of
//               buttons that will be processed by the ButtonThrower,
//               false otherwise.  See add_throw_button().
////////////////////////////////////////////////////////////////////
bool ButtonThrower::
has_throw_button(const ModifierButtons &mods, const ButtonHandle &button) const {
  ThrowButtons::const_iterator ti = _throw_buttons.find(button);
  if (ti == _throw_buttons.end()) {
    // No buttons of this kind are in the set.
    return false;
  }

  const ThrowButtonDef &def = (*ti).second;

  // This is a vector of ModifierButtons for which the indicated
  // button is handled.
  ThrowButtonDef::const_iterator di;
  for (di = def.begin(); di != def.end(); ++di) {
    if (mods.matches(*di)) {
      return true;
    }
  }

  // The indicated ModifierButtons are not applied to this button in
  // the set.
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::has_throw_button
//       Access: Published
//  Description: Returns true if the indicated button, in conjunction
//               with any nonspecified modifier buttons, is on the set
//               of buttons that will be processed by the
//               ButtonThrower.  That is to say, returns true if this
//               button was ever passed as the second parameter
//               add_throw_button(), regardless of what the first
//               parameter was.
////////////////////////////////////////////////////////////////////
bool ButtonThrower::
has_throw_button(const ButtonHandle &button) const {
  ThrowButtons::const_iterator ti = _throw_buttons.find(button);
  if (ti == _throw_buttons.end()) {
    // No buttons of this kind are in the set.
    return false;
  }

  const ThrowButtonDef &def = (*ti).second;
  return !def.empty();
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::clear_throw_buttons
//       Access: Published
//  Description: Empties the set of buttons that were added via
//               add_throw_button().  See add_throw_button().
////////////////////////////////////////////////////////////////////
void ButtonThrower::
clear_throw_buttons() {
  _throw_buttons.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::write
//       Access: Public, Virtual
//  Description: Throw all events for button events found in the data
//               element.
////////////////////////////////////////////////////////////////////
void ButtonThrower::
write(ostream &out, int indent_level) const {
  DataNode::write(out, indent_level);
  if (_throw_buttons_active) {
    indent(out, indent_level)
      << "Processing keys:\n";
    // Write the list of buttons that we're processing.
    ThrowButtons::const_iterator ti;
    for (ti = _throw_buttons.begin(); ti != _throw_buttons.end(); ++ti) {
      ButtonHandle button = (*ti).first;
      const ThrowButtonDef &def = (*ti).second;
      ThrowButtonDef::const_iterator di;
      for (di = def.begin(); di != def.end(); ++di) {
        indent(out, indent_level + 2)
          << (*di).get_prefix() << button.get_name() << "\n";
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::do_specific_event
//       Access: Private
//  Description: Generates an event of the indicated name, adding on
//               all of the user-requested parameters.
////////////////////////////////////////////////////////////////////
void ButtonThrower::
do_specific_event(const string &event_name, double time) {
  if (_specific_flag) {
    PT(Event) event = new Event(_prefix + event_name);

    if (_time_flag) {
      event->add_parameter(time);
    }

    ParameterList::const_iterator pi;
    for (pi = _parameters.begin(); pi != _parameters.end(); ++pi) {
      event->add_parameter(*pi);
    }

    throw_event(event);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::do_general_event
//       Access: Private
//  Description: Generates an appropriate general event, if one is
//               configured.
////////////////////////////////////////////////////////////////////
void ButtonThrower::
do_general_event(const ButtonEvent &button_event, const string &button_name) {
  string event_name;
  switch (button_event._type) {
  case ButtonEvent::T_down:
    event_name = _button_down_event;
    break;

  case ButtonEvent::T_resume_down:
    break;

  case ButtonEvent::T_up:
    event_name = _button_up_event;
    break;

  case ButtonEvent::T_repeat:
    event_name = _button_repeat_event;
    break;

  case ButtonEvent::T_keystroke:
    event_name = _keystroke_event;
    break;

  case ButtonEvent::T_candidate:
    event_name = _candidate_event;
    break;

  case ButtonEvent::T_move:
    event_name = _move_event;
    break;

  case ButtonEvent::T_raw_down:
    event_name = _raw_button_down_event;
    break;

  case ButtonEvent::T_raw_up:
    event_name = _raw_button_up_event;
    break;
  }
  if (event_name.empty()) {
    // This general event is not configured.
    return;
  }

  PT(Event) event = new Event(event_name);

  if (_time_flag) {
    event->add_parameter(button_event._time);
  }

  // Now add the appropriate parameters.
  switch (button_event._type) {
  case ButtonEvent::T_down:
  case ButtonEvent::T_resume_down:
  case ButtonEvent::T_up:
  case ButtonEvent::T_repeat:
  case ButtonEvent::T_raw_down:
  case ButtonEvent::T_raw_up:
    event->add_parameter(button_name);
    break;

  case ButtonEvent::T_keystroke:
    event->add_parameter(wstring(1, button_event._keycode));
    break;

  case ButtonEvent::T_candidate:
    event->add_parameter(button_event._candidate_string);
    break;

  case ButtonEvent::T_move:
    event_name = _move_event;
    break;
  }

  ParameterList::const_iterator pi;
  for (pi = _parameters.begin(); pi != _parameters.end(); ++pi) {
    event->add_parameter(*pi);
  }

  throw_event(event);
}

////////////////////////////////////////////////////////////////////
//     Function: ButtonThrower::do_transmit_data
//       Access: Protected, Virtual
//  Description: The virtual implementation of transmit_data().  This
//               function receives an array of input parameters and
//               should generate an array of output parameters.  The
//               input parameters may be accessed with the index
//               numbers returned by the define_input() calls that
//               were made earlier (presumably in the constructor);
//               likewise, the output parameters should be set with
//               the index numbers returned by the define_output()
//               calls.
////////////////////////////////////////////////////////////////////
void ButtonThrower::
do_transmit_data(DataGraphTraverser *, const DataNodeTransmit &input,
                 DataNodeTransmit &output) {
  // Clear our outgoing button events.  We'll fill it up again with
  // just those events that want to carry on.
  _button_events->clear();

  if (input.has_data(_button_events_input)) {
    const ButtonEventList *button_events;
    DCAST_INTO_V(button_events, input.get_data(_button_events_input).get_ptr());

    int num_events = button_events->get_num_events();
    for (int i = 0; i < num_events; i++) {
      const ButtonEvent &be = button_events->get_event(i);
      string event_name = be._button.get_name();

      if (be._type == ButtonEvent::T_down || be._type == ButtonEvent::T_repeat) {
        // Button down.
        if (!_mods.button_down(be._button)) {
          // We only prepend modifier names on the button-down events,
          // and only for buttons which are not themselves modifiers.
          event_name = _mods.get_prefix() + event_name;
        }

        if (!_throw_buttons_active || has_throw_button(_mods, be._button)) {
          // Process this button.
          if (be._type == ButtonEvent::T_repeat) {
            do_specific_event(event_name + "-repeat", be._time);
          } else {
            do_specific_event(event_name, be._time);
          }
          do_general_event(be, event_name);

        } else {
          // Don't process this button; instead, pass it down to future
          // generations.
          _button_events->add_event(be);
        }

      } else if (be._type == ButtonEvent::T_resume_down) {
        // Button resume down.  The button was pressed at some earlier
        // time, and the event was only just now detected.  Don't
        // throw an event now (since we already missed it), but do
        // make sure our modifiers are up-to-date.
        _mods.button_down(be._button);

      } else if (be._type == ButtonEvent::T_up) {
        // Button up.
        _mods.button_up(be._button);

        // We always throw button "up" events if we have any
        // definition for the button at all, regardless of the state
        // of the modifier keys.
        if (!_throw_buttons_active || has_throw_button(be._button)) {
          do_specific_event(event_name + "-up", be._time);
          do_general_event(be, event_name);
        }
        if (_throw_buttons_active) {
          // Now pass the event on to future generations.  We always
          // pass "up" events, even if we are intercepting this
          // particular button; unless we're processing all buttons in
          // which case it doesn't matter.
          _button_events->add_event(be);
        }

      } else if (be._type == ButtonEvent::T_raw_down) {
        // Raw button down.
        if (!_throw_buttons_active || has_throw_button(be._button)) {
          // Process this button.
          do_specific_event("raw-" + event_name, be._time);
          do_general_event(be, event_name);

        } else {
          // Don't process this button; instead, pass it down to future
          // generations.
          _button_events->add_event(be);
        }

      } else if (be._type == ButtonEvent::T_raw_up) {
        // Raw button up.
        if (!_throw_buttons_active || has_throw_button(be._button)) {
          // Process this button.
          do_specific_event("raw-" + event_name + "-up", be._time);
          do_general_event(be, event_name);
        }
        if (_throw_buttons_active) {
          // Now pass the event on to future generations.  We always
          // pass "up" events, even if we are intercepting this
          // particular button; unless we're processing all buttons in
          // which case it doesn't matter.
          _button_events->add_event(be);
        }

      } else {
        // Some other kind of button event (e.g. keypress).  Don't
        // throw an event for this, but do pass it down.
        _button_events->add_event(be);
        do_general_event(be, "");
      }
    }
  }

  output.set_data(_button_events_output, EventParameter(_button_events));
}
