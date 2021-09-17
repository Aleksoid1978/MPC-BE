/* Common Blackfin assembly defines
 *
 * Copyright (C) 2005-2009 Analog Devices
 */

#if __GNUC__ <= 3
/* GCC-3.4 and older did not use hardware loops and thus did not have
 * register constraints for declaring clobbers.
 */
# define BFIN_HWLOOP0_REGS
# define BFIN_HWLOOP1_REGS
#else
# define BFIN_HWLOOP0_REGS , "LB0", "LT0", "LC0"
# define BFIN_HWLOOP1_REGS , "LB1", "LT1", "LC1"
#endif
