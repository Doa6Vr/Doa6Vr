Virtual Reality Mod for Dead or Alive 6 v1.12

This is a free-camera mod for DOA6 with a VR component. The free camera can be used without VR.
I heavily based this on the work by https://bryanyora2525.com/

This is very much in an Alpha state. If you try running this, don't expect it to be in a user-friendly state.


Quick directions if you want to try to get it to work:
    * Cam Mod:
        * Get Cheat Engine. I used 7.0 : https://www.cheatengine.org/downloads.php
        * Place Doa6_VrMod.ct and CamMod.dll together in a folder, and open Doa6_VrMod.ct in Cheat Engine
            * Say yes to run the Lua script if prompted
        * Start Dead or Alive 6
        * Select "Open process" and select DOA6.exe
        * Click the checkbox next to "VR Mod" to inject the DLL and start the mod
        *Press F2 to start the Lua script. It's needed for some controls to work. You'll hear a 'beep' after pressing F2 to indicate the script is active.
        
        
    Most of these controls only work when the game camera is active (during an unpaused match)
        
    * Cam Controls:
        * Controls are a mess. Here's the basics:
        * Left joystick - Move around
        * Right joystick - Look around  (click the stick to toggle Y axis between 'look up/down' and 'move/up down'
            * Moving joysticks while clicking the left stick will move the camera *target* instead of camera position
        * Right trigger - speed up joystick movements by amount trigger pressed
        * RB+Right joystick - Up/Down zooms, left/right rolls
        * Left trigger - Pause/Unpause game
        * Left stick click - Slow down game (hold LB to slow down faster)
        * Right stick click - Speed up game (hold LB to speed up faster)
        * RB + A,B,X,Y - Speed presets
        * Back - Switch forward through P1/P2 target/source camera presets
        * LB + X - Switch backward through P1/P2 target/source camera presets
        * LB + Back - Toggle UI  (control is not active when paused. Must unpause first to bring back pause menu)
        * RB + Right-Stick - Switch through 'Camera modes'
            * See Camera Modes details below
        * RB + Back - Toggle character heads/bodies
        * Left stick click + Right stick click - Reset camera position
        * RB + left stick click - Toggle height lock modes (lock source height, target height, both)
        * RB + LB + Back - Toggle camera lock. Useful to stop the game changing scenes from moving the camera (I think this is broken right now)
        
   * Other controls:
        * 3D  (these controls are set in dxdi.ini and are handled by 3d migoto)
            * While holding in DPAD_LEFT, pressing A/B will cycle forward/backward between convergence/depth presets
            * While holding in DPAD_DOWN, pressing A/B will cycle through convergence settings
            * While holding in DPAD_DOWN, pressing X/Y will cycle through depth settings
        * CPU controls (most of these controls only take affect after passing through the Character Select menu)
            * While holding in RB and DPAD_RIGHT, X toggles CPU control of Player 1
            * While holding in RB and DPAD_RIGHT, Y toggles CPU control of Player 2
            * While holding in RightTrigger, B cycles through P1 Cpu difficulty level (0 means use level chosen in game)
            * While holding in RightTrigger and clicking the left stick, B cycles through P2 Cpu difficulty level
        * Gameplayer
            * While holding in LeftTrigger, A cycles through P1 Break Meter ( 0 - don't touch, 1 - always full, 2 - always empty ). Visual bar glit
            * While holding in LeftTrigger, B cycles through P2 Break Meter
            * While holding in LeftTrigger, X toggles P1 always full health
            * While holding in LeftTrigger, Y toggles P2 always full health
            

    * Camera Modes - control how the base camera position and target are set. Camera presets are cycled through with BACK
        * Game Cam - use the normal game camera.
        * Game, no target - Position is game camera, the target is not updated
        * Game, with target - Game camera, but looking at the target from the current camera preset
        * Cheat Engine Values - Don't modify any values, use whatever cheat engine set
        * Source and Target On - Set the position and target to the current camera preset
        * POV - Set the source to the camera preset, then set the target to 'look out' from the source
        * Source Off Target On - leave the camera alone, but keep following the target
        * Source and target off - leave the camera completely alone. Position is entirely under player control
        
    You'll get lost a lot. Sometimes the camera doesn't seem to stick where you want it to. Best bet is to try combinations
    of 'reset camera position',  switching point of views, and changing camera mode until the camera decides to get back on task.

    
    VR Mod:
        VR is done using Nvidia 3D Vision to render the 3D cameras, 3d-migoto for tweaks and to provide a side-by-side image, then viewing it in Virtual Desktop.
        
        You'll need to be running NVidia drivers 418.91, the last release with 3D vision support.
            There is an attempt, "3D Fix Manager", to re-add the 3D vision driver to newer drivers, but when I tried the 3D driver it installed was very old and didn't work in DOA.

        Extract all of the mod files into a directory. 
        
        Get the 3d-Migoto Launcher: http://helixmod.blogspot.com/2019/03/dead-or-alive-6.html
            * Place the launcher in a folder *next to* DOA's folder. It will not work if it is in a folder.
            * You should use my d3dx.ini as a starting point, and just configure the 'launch=' line in d3dx.ini for your DOA6 path. Works best with absolute path.
        Get SBSMode.vbs from the post here: https://www.nvidia.com/en-us/geforce/forums/discover/252329/guide-how-to-change-3d-vision-discover-colors-or-enable-sbs-in-3dmigoto-via-discover-/
            I recommend changing "RunFor = Timer + 10" to "RunFor = Timer + 30"
            
        Load up the NVidia Control panel, and there should be a menu for Stereoscopic 3d. Go and enable it. The first time you do this you'll have to set up 3D. Tell it you are using 3D Discover glasses.
        
        
        Ok, everything is gathered and 3D Vision is enabled. Now to run it:
            * Load up Virtual Desktop. On Oculus this must be done first, because Oculus won't let you launch Virtual Desktop if anything else is already running.
            * Run HmdServer.exe . This should pop up a console and say that it is waiting for connections
            * Run "SBSMode.vbs".  For the next 30 seconds it will run in the background repeatedly overwriting the "3D Discover" settings to remove the red/blue tint. You must launch DOA6 in this window.
            * Run "3DMigoto Loader.exe".  This should launch DOA6 in Side-by-Side 3D
                * If you are not in 3D, try pressing "Ctrl+T" to toggle 3D Vision
                * You can try pressing F11 to toggle through 3D Migoto 3D modes
            * Switch to Virtual Desktop, set up the screen (I currently use low 'Screen Distance', like 0.5m, medium screen size, like 200 degrees, and high curve, like 70%.
            * Enable Head Lock, No Delay.   I'd recommend hotkeying this so you can unlock it to see menus.
            * Open up the Cheat Engine table, DOA6_VrMod.ct, open the DOA6.exe process, and enable the checkbox next to VR mod.
            * Once a match starts the mod should kick in, and HmdServer.exe should indicate a connection.
            * You should now have headtracking. See Cam Mod directions for other details.
