# Lasertrap (C) Christian Guckelsberger 2015
# Implemented with the octet framework
# https://www.youtube.com/watch?v=A7u715On4Pc

Lasertrap is a 2d stealth-game prototype for the first assessment of the "Introduction to Games Programming" class at Goldsmiths, 2015.

Game outline:
You play a thief with the task to steal a famous painting. Unfortunately, the painting is well protected and you have to overcome obstacles, such as lasertraps, to get it. This prototype showcases a first simple level, and stealing the target painting allows the player to finish the game successfully, while being trapped ends the game immediately. The player can overcome traps, by placing a limited number of mirrors. The challenge is in navigating and re-using the thief's kit efficiently.
In a fully realized game, stealing the target painting would allow to go to the next level with higher-ranking objectives, and more sophisticated protection mechanism, such as moving lasers, guards, sonic sensors, etc. Stealing paintings allows the player to earn money which can be spent on more kit to challenge these advanced protection mechanism. Denying the player to proceed to the next level without stealing the targer painting allows to try a level multiple time, stealing minor paintings to buy the kit required to master it.

Controls:
-Move forward/backward with key UP/DOWN. 
-Rotate counterclockwise/clockwise with key LEFT/RIGHT. 
-Steal a painting if close enough by hitting SPACE. 
-Place a mirror, if any mirrors left in inventory, by pressing SPACE. 
-Pick up a mirror, back into the inventory, by pressing SPACE. Only works if mirror is close enough to player.

Assessment requirements:
-1) Level loading mechanism: The level is loaded from several .csv files, each representing either a semantic (e.g. walls that player can collide with, vs. shadows) or depth layer (e.g. shadows over floor). Most tokens are stored as sprites in a vector container, but paintings, lasers and the player character are stored as distinct objects that encapsulate further functionality.
-2) Fragment shaders: The prototype implements two customized fragment shaders, by extending the texture_shader class from octet's "Invaderers" example.
-2.1) Noisy texture shader: Used to let the laser beams appear mottled and non-static. It receives a random number from the shader program, and plugs it into a very chaotic hash function, which eventually determines a grey-scale colour. The latter is then multiplied with the current pixel supplied by the sampler, to slightly darken or brighten up the pixel's color. The external random number replaces missing facilities for random number generation in OpenGL ES, and allows to make the noise time-variant. 
-2.2) Fading texture shader: Used to highlight paintings that can be picked up. Could alternatively be used to generate an "alarm triggered" effect. Implemented by means of a successively increased and externally provided phase progress number in interval [0,2*PI], which correlates with the number of times the rendering method was triggered. This progress number is fed into a term involving a sine function, which outputs smoothly alternating values in [0,1]. These values in turn are used to control the alpha of a color with which each pixel is multiplied.
-3) Clearly formatted code with comments.
-4) (Low-res) video on YouTube: https://www.youtube.com/watch?v=A7u715On4Pc

Minimal future work to make prototype more playable:
-Improve collision detection and avoid "backjumps" of player character
-Include mirrors in collision detection
-Fix camera to player position