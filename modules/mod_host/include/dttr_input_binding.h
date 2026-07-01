/// @file dttr_input_binding.h
/// Internal host helpers that convert a mod input binding between the "device:name" token
/// stored in config (e.g. "key:f5", "mouse:right", "pad:a") and the resolved
/// DTTR_Mods_ConfigInputBinding.

#ifndef DTTR_INPUT_BINDING_H
#define DTTR_INPUT_BINDING_H

#include <stddef.h>

#include <dttr_mods.h>
#include <dttr_result.h>

#define DTTR_INPUT_BINDING_KEY_PREFIX "key:"
#define DTTR_INPUT_BINDING_MOUSE_PREFIX "mouse:"
#define DTTR_INPUT_BINDING_GAMEPAD_PREFIX "pad:"

/// Serializes a binding into its "device:name" token. Unbound bindings produce an empty
/// string.
DTTR_Result DTTR_InputBinding_Format(
	const DTTR_Mods_ConfigInputBinding *binding,
	char *out,
	size_t out_size
);

/// Parses a "device:name" token into a binding. Empty, missing, or unrecognized tokens
/// resolve to an unbound binding (device NONE).
DTTR_Result DTTR_InputBinding_Parse(const char *token, DTTR_Mods_ConfigInputBinding *out);

/// Gives the human-readable label for a binding (e.g. "F5", "Right Mouse", "a", "Unbound").
DTTR_Result DTTR_InputBinding_DisplayName(
	const DTTR_Mods_ConfigInputBinding *binding,
	char *out,
	size_t out_size
);

#endif // DTTR_INPUT_BINDING_H
