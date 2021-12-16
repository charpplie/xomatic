#pragma once

// EXPECTED macro is used in the situations where you want to have an assertion
// combine with a runtime check/action, that is done in all configurations.
//
// Examples of use:
//
//   EXPECTED(connect(button, SIGNAL(triggered()), this, SLOT(OnButtonTriggered())));
//
//   if (!EXPECTED(argument != nullptr))
//     return;
//
// This will break under the debugger, but still will performs check and calls in production build.

#define EXPECTED(x) ((x) || (ExpectedIsDebuggerPresent() && (__debugbreak(), true), false))

bool ExpectedIsDebuggerPresent();

