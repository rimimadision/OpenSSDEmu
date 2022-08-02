#ifndef EMU_CONFIG_H
#define EMU_CONFIG_H

/*
 * Emulator Configuration
 */
#define EMU

#ifdef EMU
#define EMU_SINGLE_CORE
#define NO_LOG

// #define EMU_DATA_FLOW
#endif // EMU == ON

#endif // EMU_CONFIG_H