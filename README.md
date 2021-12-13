this is w9wm window manager version 0.4.2 with these changes:
------------------------------------------------------------------------
1. Open new windows automatically instead of just showing a cursor.
2. Hotkey support for virtual desktops and right click menu items. (See usage below, some code borrowed from 999)
3. Alt + left click desktop for virtual desk menu.
4. Middle click desktop for applications menu from $HOME/.w9wmrc.
5. Right click desktop for system menu.


------------------------------------------------------------------------
HOTKEYS:

Alt+(1-9) - switches virtual desktops

Alt+Tab, (Alt+Shift+Tab) - switch applications on same desk

Alt+r - exec dmenu

Alt+t - exec terminal

Alt+m - move client

Alt+n - make client a new size

Alt+d - close current window

Alt+shift+w - restart w9wm

Alt+shift+q - quit w9wm


------------------------------------------------------------------------
BUGS: (To be looked at in future)

after starting w9wm, hotkeys arent working until selecting "new" from system menu or opening a program from applications menu.

Alt-d pressed on root will close window manager


------------------------------------------------------------------------
about w9wm:

w9wm is an improved version of 9wm, the excellent 8-1/2 window manager emulation. It supports virtual screens as well as keybindings.
It is free software, under 9wm license although I would prefer to ship it under the GNU GPL (as most of w9wm's code is borrowed from 9wm, I have to keep the same license). Please have a look at README for more information.
Current stable version is 0.4.2
