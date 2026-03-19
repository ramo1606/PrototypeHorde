/*
 * external_impl.c — Implementation definitions for header-only libraries.
 *
 * Each header-only library needs exactly ONE .c file that defines
 * its IMPLEMENTATION macro before including it. This is that file.
 * Every other file in the project includes these headers normally
 * (without the IMPLEMENTATION define) and gets only declarations.
 */

#define RMEM_IMPLEMENTATION
#include "rmem.h"

#define RINI_IMPLEMENTATION
#include "rini.h"

 /*
  * reasings.h is static inline by default (REASINGS_STATIC_INLINE),
  * so it doesn't need an implementation compilation unit.
  * Just #include "reasings.h" wherever you need easing functions.
  */

  /*
   * raygui: RAYGUI_IMPLEMENTATION is defined in debug_ui.c because
   * it depends on raylib being included first with specific setup.
   * If raygui is needed outside debug (e.g. temp menus in Phase 9),
   * move the implementation define here and remove it from debug_ui.c.
   */
