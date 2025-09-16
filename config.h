/**
 * Miscellaneous Configuration Constants 
 */
#ifndef SPLINTER_CONFIG_H
#define SPLINTER_CONFIG_H

#define SPLINTER_VERSION "0.0.4"

// so tools can be used without many / any arguments
#define DEFAULT_BUS "splinter_debug"
#define DEFAULT_KEY "__debug"

// do we have the valgrind development headers?
// comment out (undefine) if not.
#define HAVE_VALGRIND_H 1

// more can go here as needed.

#endif // SPLINTER_CONFIG_H