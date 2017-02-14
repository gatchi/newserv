This server is by no means bug-free. The source contained in this archive is
released as-is, wich no guarantees about anything. It is likely that if you
were to build this project as it is right now, without modifying anything, that
it wouldn't work at all. Some modifications may have to be made in order to get
it working. It's likely that the patch server routines are too far out of date;
PSO may not recognize the patch server as valid anymore and may not continue past
the patching stage.

There is no makefile included. It's not like you need one: just compile everything
in this directory into one executable. Any good IDE will generate a makefile for
you anyway.

Almost all functions in this project return 0 on success, and a nonzero error code
on failure. The error codes are completely random and have no meaning. If you come
across any error code while playing, though, you can often search the entire
project for that error code to find exactly where that error occurred.

That said, here is a list of the source files and what they do:

battleparamentry: functions for loading battleparamentry files
blockserver: the PSOBB data server. (It was once the PSOGC block server, and never got renamed.)
chatcommands: functions for handling chat commands
client: routines for sending to, receiving from, and deleting clients
command-functions: routines that translate commands between versions
command-input-subs: routines for handling game commands (60, 62, 6C, 6D)
command-input: routines for handling received commands
command-output: routines that generate and send commands to clients
command-procedures: routines for quest loading, game joining, and changing lobbies
commonservers: the server 'new client' routine
console: code for color text in Windows consoles.
dns-server: exactly what it says it is. I did not comment this code; it just needs to be rewritten.
encryption: PSO's encryption libraries. Also, a function to print blocks of data to the console.
fuzzshipgateclient: was going to be the client for the Fuzziqer Software shipgate. I never got around to writing it.
item-procedures: procedures for manipulating items in a player's inventory (namely initializing IDs and sorting)
itemraretable: loads a specific set of rare item data for a game.
items: procedures for generating non-rare items
levels: functions for handling level-up data and level-ups
license: a terrible license handling library. So terrible, in fact, that I didn't comment it and it should just be rewritten.
listenthread: a library that listens for connections in a separate thread and calls a callback function when a client connects.
lobby: functions that handle clients in lobbies/games and dropped items in games
lobbyserver: handles clients in lobbies and games
loginserver: handles the information menu, server switching, and such
main: duh. starts the server and all that.
map: builds lists of monsters in .dat files (used for EXP and items and such)
netconfig: a simple configuration file library. most human-readable files that newserv uses are in netconfig format.
network-addresses: a terribly crude set of network address routines
patchserver: the PSOPC/PSOBB patch server (untested with PSOPC)
player: player data, account data, and item manipulation routines
privilege: privileges used by players on this server
prs: PRS compression/recompression routines
psobb-crypt: PSOBB encryption routines (reversed by Myria)
psogc-crypt: PSOGC encryption routines
psopc-crypt-old: PSOPC/PSODC encryption routines
quest: routines for manipulating quests and quest lists
server: general server data management functions
shipgateserver: the Fuzziqer Software shipgate server
text: functions for manipulating and converting text
updates: library for loading and using a list of files to update
version: contains defines and flags that describe the various versions of PSO

There are also several minor programs and source files included in this package.
Here are descriptions of those programs and sources:

system/blueburst/battleparamentry: displays the contents of a battleparamentry file in a (very large) table.
    for best results, run it like battleparamentry.exe file.dat > table.txt
system/blueburst/droprate: expands/compresses those annoying one-byte PSOGC drop rates (in ItemRT.gsl, for example)
system/blueburst/enemies: displays the contents of enemies.nsi in a table format. (Not commented because the source is really pretty self-explanatory)
system/blueburst/splitstreamfile: splits the large chunk of data that PSOBB sends (which includes the battleparamentry files) into separate files.
    The index (header) of this chunk should be in streamfile.ind, and the data should be in streamfile.pbb.
    The resulting files will be saved in the same folder.
system/blueburst/translateparams: builds the enemies.pbb and boxrares.pbb files for the server.
    The following files must be present for this to work:
        itemraretable.pbb (take ItemRT.gsl from PSOGC/PSOX and rename it to itemraretable.pbb)
        BattleParamEntry_on.dat (from PSOBB data server)
        BattleParamEntry_lab_on.dat (from PSOBB data server)
        BattleParamEntry_ep4_on.dat (from PSOBB data server)
        BattleParamEntry.dat (from PSOBB data server)
        BattleParamEntry_lab.dat (from PSOBB data server)
        BattleParamEntry_ep4.dat (from PSOBB data server)
        dropchart.ini (see included example)
        enemies.ini (see included example)
        droptype.ini (see included example)
system/ep3/mina: a simple, probably buggy Ep3 emulator. Included is an early version; there have been bug fixes and improvements
    since this version was built, but the new version is much larger. If people want the newer version, I can post it too (separately).
system/addlicense: modifies the license list. See addlicense.cpp for more information.
