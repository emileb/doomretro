


#include "game_interface.h"


#include "SDL.h"
#include "SDL_keycode.h"

#include "i_system.h"
#include "m_fixed.h"
#include "m_controls.h"
#include "doomstat.h"

#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"JNI", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "JNI", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"JNI", __VA_ARGS__))



// FIFO STUFF ////////////////////
// Copied from FTEQW, I don't know if this is thread safe, but it's safe enough for a game :)
#define EVENTQUEUELENGTH 128

//This event_t is from choc Doom d_event.h
event_t  eventlist[EVENTQUEUELENGTH];

volatile int events_avail; /*volatile to make sure the cc doesn't try leaving these cached in a register*/
volatile int events_used;

static  event_t *in_newevent(void)
{
	if (events_avail >= events_used + EVENTQUEUELENGTH)
		return 0;
	return &eventlist[events_avail & (EVENTQUEUELENGTH-1)];
}

static void in_finishevent(void)
{
	events_avail++;
}

int add_choc_event(evtype_t type, int data1, int data2, int data3){

	//LOGI("PortableKeyEvent %d %d",state,code);
	event_t *ev = in_newevent();
	if (!ev)
		return 0;

	ev->type = type;
	ev->data1 = data1;
	ev->data2 = data2;
	ev->data3 = data3;

	in_finishevent();
	return 0;
}
///////////////////////


extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);

int PortableKeyEvent(int state, int code, int unicode){

	LOGI("PortableKeyEvent %d %d %d",state,code,unicode);

	if (state)
		SDL_SendKeyboardKey(SDL_PRESSED, (SDL_Scancode)code);
	else
		SDL_SendKeyboardKey(SDL_RELEASED, (SDL_Scancode) code);

	return 0;

}

void PortableBackButton()
{
    PortableKeyEvent(1, SDL_SCANCODE_ESCAPE,0 );
    PortableKeyEvent(0, SDL_SCANCODE_ESCAPE, 0);
}

void ActionKey(int state,int key)
{
	add_choc_event(state?ev_keydown:ev_keyup,key,0,0);
}

int newweapon = -1;

void PortableAction(int state, int action)
{
	LOGI("PortableAction %d   %d",state,action);

	int key = -1;

	if ((action >= PORT_ACT_MENU_UP && action <= PORT_ACT_MENU_ABORT) &&
	    ( PortableGetScreenMode() == TS_MENU ) || ( PortableGetScreenMode() == TS_Y_N ))
	{
	    SDL_Scancode scanCode = 0;

	    switch (action)
		{
		case PORT_ACT_MENU_UP:
		    scanCode = SDL_SCANCODE_UP;
		    break;
        case PORT_ACT_MENU_DOWN:
            scanCode = SDL_SCANCODE_DOWN;
            break;
        case PORT_ACT_MENU_LEFT:
            scanCode = SDL_SCANCODE_LEFT;
            break;
        case PORT_ACT_MENU_RIGHT:
            scanCode = SDL_SCANCODE_RIGHT;
            break;
        case PORT_ACT_MENU_SELECT:
            scanCode = SDL_SCANCODE_RETURN;
            break;
        case PORT_ACT_MENU_BACK:
            scanCode = SDL_SCANCODE_ESCAPE;
            break;
        case PORT_ACT_MENU_CONFIRM:
            scanCode = SDL_SCANCODE_Y;
            break;
        case PORT_ACT_MENU_ABORT:
            scanCode = SDL_SCANCODE_N;
            break;
		}

		PortableKeyEvent( state, scanCode, 0);
	}
	else
	{
		switch (action)
		{
		case PORT_ACT_LEFT:
			key = keyboardleft;
			break;
		case PORT_ACT_RIGHT:
			key = keyboardright;
			break;
		case PORT_ACT_FWD:
			key = keyboardforward;
			break;
		case PORT_ACT_BACK:
			key = keyboardback;
			break;
		case PORT_ACT_MOVE_LEFT:
			key = keyboardstrafeleft;
			break;
		case PORT_ACT_MOVE_RIGHT:
			key = keyboardstraferight;
			break;
		case PORT_ACT_USE:
			key = keyboarduse;
			break;
		case PORT_ACT_ATTACK:
			key = keyboardfire;
			break;
		case PORT_ACT_STRAFE:
			key = keyboardstrafe;
			break;
		case PORT_ACT_SPEED:
			key = keyboardrun;
			break;
		case PORT_ACT_MAP:
			key = keyboardautomap;
			break;
		case PORT_ACT_MAP_ZOOM_IN:
			key = keyboardautomapzoomin;
			break;
		case PORT_ACT_MAP_ZOOM_OUT:
			key = keyboardautomapzoomout;
			break;
		case PORT_ACT_NEXT_WEP:
			key = keyboardnextweapon;
			break;
		case PORT_ACT_PREV_WEP:
			key = keyboardprevweapon;
			break;
		case PORT_ACT_QUICKSAVE:
			PortableKeyEvent(state, SDL_SCANCODE_F6, 0);
			break;
		case PORT_ACT_QUICKLOAD:
			PortableKeyEvent(state, SDL_SCANCODE_F9, 0);
			break;
		case PORT_ACT_GAMMA:
			//key = key_menu_gamma;
			break;
		case PORT_ACT_WEAP1:
			key = keyboardweapon1;
			break;
		case PORT_ACT_WEAP2:
			key = keyboardweapon2;
			break;
		case PORT_ACT_WEAP3:
			key = keyboardweapon3;
			break;
		case PORT_ACT_WEAP4:
			key = keyboardweapon4;
			break;
		case PORT_ACT_WEAP5:
			key = keyboardweapon5;
			break;
		case PORT_ACT_WEAP6:
			key = keyboardweapon6;
			break;
		case PORT_ACT_WEAP7:
			key = keyboardweapon7;
			break;
        case PORT_ACT_CONSOLE:
            key = keyboardconsole;
            break;
		}

		if (key != -1)
			ActionKey(state,key);
	}
}



// =================== FORWARD and SIDE MOVMENT ==============

float forwardmove_android, sidemove_android; //Joystick mode

void PortableMoveFwd(float fwd)
{
	if (fwd > 1)
		fwd = 1;
	else if (fwd < -1)
		fwd = -1;

	forwardmove_android = fwd;
}

void PortableMoveSide(float strafe)
{
	if (strafe > 1)
		strafe = 1;
	else if (strafe < -1)
		strafe = -1;

	sidemove_android = strafe;
}

void PortableMove(float fwd, float strafe)
{
	PortableMoveFwd(fwd);
	PortableMoveSide(strafe);
}

//======================================================================

//Look up and down
int look_pitch_mode;
float look_pitch_mouse,look_pitch_abs,look_pitch_joy;
void PortableLookPitch(int mode, float pitch)
{
	look_pitch_mode = mode;
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_pitch_mouse += pitch;
		break;
	case LOOK_MODE_ABSOLUTE:
		look_pitch_abs = pitch;
		break;
	case LOOK_MODE_JOYSTICK:
		look_pitch_joy = pitch;
		break;
	}
}

//left right
int look_yaw_mode;
float look_yaw_mouse,look_yaw_joy;
void PortableLookYaw(int mode, float yaw)
{
	look_yaw_mode = mode;
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_yaw_mouse += yaw;
		break;
	case LOOK_MODE_JOYSTICK:
		look_yaw_joy = yaw;
		break;
	}
}




extern int main_android(int argc, char *argv[]);
void PortableInit(int argc,const char ** argv){
	main_android(argc,(char **)argv);
}


static float am_zoom = 0;
static float am_pan_x = 0;
static float am_pan_y = 0;

void PortableAutomapControl(float zoom, float x, float y)
{
	am_zoom += zoom;
	am_pan_x += x;
	am_pan_y += y;
}


extern dboolean menuactive;
extern dboolean paused;
extern dboolean	messageNeedsInput;
extern dboolean automapactive;


touchscreemode_t PortableGetScreenMode()
{
    if(menuactive || paused)
    {
        if( messageNeedsInput )
            return TS_Y_N;
        else
            return TS_MENU;
    }
    else if(gamestate == GS_LEVEL)
    {
        if(automapactive)
            return TS_MAP;
         else
            return TS_GAME;
    }
    else
        return TS_BLANK;
}

int PortableShowKeyboard(void){

	return 0;
}


void I_UpdateAndroid(void)
{
	event_t *ev;

	while (events_used != events_avail)
	{
		ev = &eventlist[events_used & (EVENTQUEUELENGTH-1)];


		D_PostEvent(ev);

		events_used++;
	}
}

void Mobile_AM_controls(double *zoom, fixed_t *pan_x, fixed_t *pan_y )
{
	if (am_zoom)
	{
        *zoom = am_zoom * 10;
		am_zoom = 0;
	}

	*pan_x += (fixed_t)(am_pan_x * 20000000);
	*pan_y += -(fixed_t)(am_pan_y * 10000000);
	am_pan_x = am_pan_y = 0;
	//LOGI("zoom = %f",*zoom);
}

// From g_game.c
#define FORWARDMOVE0    0x19
#define FORWARDMOVE1    0x32

#define SIDEMOVE0       0x18
#define SIDEMOVE1       0x28

static int mlooky = 0;
//Called from the game
void G_AndroidBuildTiccmd(ticcmd_t *cmd)
{
	cmd->forwardmove  += forwardmove_android * FORWARDMOVE1;
	cmd->sidemove  += sidemove_android   *  SIDEMOVE1;

	switch(look_pitch_mode)
	{
	case LOOK_MODE_MOUSE:
		mlooky += look_pitch_mouse * 10000;
		look_pitch_mouse = 0;
		break;
	case LOOK_MODE_ABSOLUTE:
		mlooky = look_pitch_abs * 80;
		break;
	case LOOK_MODE_JOYSTICK:
		mlooky += look_pitch_joy * 300;
		break;
	}

	//LOGI("mlooky = %d",mlooky);

	if (abs(mlooky) > 100)
	{
		int look = -mlooky/100;
		if (look > 7) look = 7;
		if (look < -7) look = -7;

		if (look < 0)
		{
			look += 16;
		}

		mlooky = 0;
	}


	switch(look_yaw_mode)
	{
	case LOOK_MODE_MOUSE:
		cmd->angleturn += look_yaw_mouse * 80000;
		look_yaw_mouse = 0;
		break;
	case LOOK_MODE_JOYSTICK:
		cmd->angleturn += look_yaw_joy * 1000;
		break;
	}
	if (newweapon != -1)
	{
		cmd->buttons |= BT_CHANGE;
		cmd->buttons |= (newweapon-1)<<BT_WEAPONSHIFT;
		newweapon = -1;
	}
}



