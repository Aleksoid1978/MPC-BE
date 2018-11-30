#ifndef _USERDEFINES_INCLUDED
#define _USERDEFINES_INCLUDED

/*

  Define these values to alter the various
  features of the coolscroll library. If you don't want
  a certain feature, then you might be able to reduce the
  size of your app by a few kb...

*/

/* allow inserted buttons. Without this, all button code will
   be excluded, resulting in a smaller build (about 4kb less). This
   may not seem much, but it is a 25% reduction! */
//#define INCLUDE_BUTTONS		

/* Allow user-resizable buttons. Makes no difference if INCLUDE_BUTTONS
   is not defined for the project */
//#define RESIZABLE_BUTTONS	

/* Include tooltip support for inserted buttons. Without this, no
   tooltip requests (TTN_GETDISPINFO's) will be sent to the window */
//#define COOLSB_TOOLTIPS

/* Define this to include the custom-draw support */
//#define CUSTOM_DRAW

/* Define to enable WM_NOTIFY messages to be sent for mouse event */
#define NOTIFY_MOUSE

/* Define this value to make the horizontal scrollbar stay visible even
   if the window is sized to small vertically. Normal scrollbars always leave
   a 1-pixel line of "client" area before hiding the horizontal scrollbar. This
   value allows the window to be sized so the client area totally disappears if
   sized too small */
//#define COOLSB_FILLWINDOW

/* minimum size of scrollbar before inserted buttons are 
   hidden to make room when the window is sized too small */
#define MIN_COOLSB_SIZE 24

/* min size of scrollbar when resizing a button, before the 
   resize is stopped because the scrollbar has gotten too small */
#define MINSCROLLSIZE   50		

/* define this to display the default mouse arrow whenever the
   the mouse is released over a button which has a user-defined cursor.
   not really very useful, just provides a different type of feedback */
#undef  HIDE_CURSOR_AFTER_MOUSEUP

/* enable HOT_TRACKING to provide visual feedback when the mouse
   moves over a scrollbar area (like Flat Scrollbars) */
#define HOT_TRACKING

/* enable FLAT_SCROLLBARS to include support for flat scrollbars
   note that they must be enabled by the user first of all */
#define FLAT_SCROLLBARS

/* a normal scrollbar "snaps" its scroll-thumb back into position if
   you move the mouse too far away from the window, whilst you are
   dragging the thumb, that is. #undeffing this results in the thumb
   never snapping back into position, no matter how far away you move
   the mouse */
#define SNAP_THUMB_BACK

/* distance (in pixels) the mouse must move away from the thumb 
   during tracking to cause the thumb bar to snap back to its 
   starting place. Has no effect unless SNAP_THUMB_BACK is defined */
#define THUMBTRACK_SNAPDIST 24

/* maximum number of inserted buttons per bar */
#define MAX_COOLSB_BUTS 1
   
/* maximum number of coolsb windows per application. 
   Set to lower if you don't need many.  */
#define MAX_COOLSB 64

#endif /* _USERDEFINES_INCLUDED */