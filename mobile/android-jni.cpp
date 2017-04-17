/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <unistd.h>

#include "SDL_beloko_extra.h"

#include "TouchControlsContainer.h"
#include "JNITouchControlsUtils.h"


extern "C"
{


#include "in_android.h"
#include "SDL_keycode.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"JNI", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "JNI", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"JNI", __VA_ARGS__))

#define JAVA_FUNC(x) Java_org_libsdl_app_NativeLib_##x

int android_screen_width;
int android_screen_height;


#define KEY_SHOW_WEAPONS 0x1000
#define KEY_SHOOT        0x1001

#define KEY_SHOW_INV     0x1006
#define KEY_QUICK_CMD    0x1007
#define KEY_SHOW_KBRD    0x1008



float gameControlsAlpha = 0.5;
bool showWeaponCycle = false;
bool turnMouseMode = true;
bool invertLook = false;
bool precisionShoot = false;
bool showSticks = true;
bool hideTouchControls = true;
bool enableWeaponWheel = true;

bool shooting = false;

//set when holding down reload
bool sniperMode = false;

static int controlsCreated = 0;
touchcontrols::TouchControlsContainer controlsContainer;

touchcontrols::TouchControls *tcMenuMain=0;
touchcontrols::TouchControls *tcGameMain=0;
touchcontrols::TouchControls *tcGameWeapons=0;
touchcontrols::TouchControls *tcWeaponWheel=0;
touchcontrols::TouchControls *tcInventory=0;

//So can hide and show these buttons
touchcontrols::Button *nextWeapon=0;
touchcontrols::Button *prevWeapon=0;
touchcontrols::TouchJoy *touchJoyLeft;
touchcontrols::TouchJoy *touchJoyRight;

extern JNIEnv* env_;

void openGLStart()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, android_screen_width, android_screen_height);
    glOrthof(0.0f, android_screen_width, android_screen_height, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY );

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	//glDisable(GL_CULL_FACE);
	glMatrixMode(GL_MODELVIEW);

}

void openGLEnd()
{

	glDisable (GL_BLEND);
	glColor4f(1,1,1,1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

}

void gameSettingsButton(int state)
{
	if (state == 1)
	{
		showTouchSettings();
	}
}

//Bit strange! This is called by the setup program to launch the game..
void launchChocGame(int val)
{
	jclass helloWorldClass;
	jmethodID mainMethod;
	helloWorldClass = env_->FindClass("com/beloko/opengames/chocdoom/Setup");
	mainMethod = env_->GetStaticMethodID(helloWorldClass, "launchGame", "()V");
	env_->CallStaticVoidMethod(helloWorldClass, mainMethod);
}



extern unsigned int Sys_Milliseconds(void);

static unsigned int reload_time_down;
void gameButton(int state,int code)
{
	if (code == KEY_SHOOT)
	{
		shooting = state;
		PortableAction(state,PORT_ACT_ATTACK);
	}
	else if (code == PORT_ACT_RELOAD)
	{


		sniperMode = state; //Use reload button for precision aim also
	}
	else if (code == KEY_SHOW_WEAPONS)
	{
		if (state == 1)
			if (!tcGameWeapons->enabled)
			{

				tcGameWeapons->animateIn(5);
			}
	}
	else if (code == KEY_SHOW_INV)
	{
		if (state == 1)
		{
			if (!tcInventory->enabled)
			{
				tcInventory->animateIn(5);
			}
			else
				tcInventory->animateOut(5);
		}
	}
	else if  (code == KEY_SHOW_KBRD)
	{
		if (state)
			toggleKeyboard();
	}
	else
	{
		PortableAction(state, code);
	}
}


//Weapon wheel callbacks
void weaponWheelSelected(int enabled)
{
	if (enabled)
		tcWeaponWheel->fade(touchcontrols::FADE_IN,5); //fade in
}

void weaponWheel(int segment)
{
	LOGI("weaponWheel %d",segment);
	PortableAction(1,PORT_ACT_WEAP1 + segment);
}

void menuButton(int state,int code)
{
	if (code == KEY_SHOW_KBRD)
	{
		if (state)
			toggleKeyboard();
	}
	else if (code == SDL_SCANCODE_F10) //for choch setup f10 to work!
	{
		PortableKeyEvent(state, code, 0);
	}
	else
		PortableAction(state, code);
}



int left_double_action;
int right_double_action;

void left_double_tap(int state)
{
	//LOGTOUCH("L double %d",state);
	if (left_double_action)
		PortableAction(state,left_double_action);
}

void right_double_tap(int state)
{
	//LOGTOUCH("R double %d",state);
	if (right_double_action)
		PortableAction(state,right_double_action);
}



//To be set by android
static float strafe_sens = 1;
static float forward_sens = 1;

static float pitch_sens = 1;
static float yaw_sens = 1;

void left_stick(float joy_x, float joy_y,float mouse_x, float mouse_y)
{
	joy_x *=10;
	//float strafe = joy_x*joy_x;
	float strafe = joy_x;
	//if (joy_x < 0)
	//	strafe *= -1;

	PortableMove(joy_y * 15 * forward_sens,-strafe * strafe_sens);
}
void right_stick(float joy_x, float joy_y,float mouse_x, float mouse_y)
{
	//LOGI(" mouse x = %f",mouse_x);
	int invert = invertLook?-1:1;

	float scale;

	if (sniperMode)
		scale = 0.1;
	else
		scale = (shooting && precisionShoot)?0.3:1;

	PortableLookPitch(LOOK_MODE_MOUSE,-mouse_y  * pitch_sens * invert * scale);

	if (turnMouseMode)
		PortableLookYaw(LOOK_MODE_MOUSE,mouse_x*2*yaw_sens * scale);
	else
		PortableLookYaw(LOOK_MODE_JOYSTICK,joy_x*6*yaw_sens * scale);

}

void inventoryButton(int state,int code)
{
	PortableAction(state,code);
}

//Weapon select callbacks
void selectWeaponButton(int state, int code)
{
	PortableKeyEvent(state, code, 0);
	if (state == 0)
		tcGameWeapons->animateOut(5);
}

void weaponCycle(bool v)
{
	if (v)
	{
		if (nextWeapon) nextWeapon->setEnabled(true);
		if (prevWeapon) prevWeapon->setEnabled(true);
	}
	else
	{
		if (nextWeapon) nextWeapon->setEnabled(false);
		if (prevWeapon) prevWeapon->setEnabled(false);
	}
}

void setHideSticks(bool v)
{
	if (touchJoyLeft) touchJoyLeft->setHideGraphics(v);
	if (touchJoyRight) touchJoyRight->setHideGraphics(v);
}


void initControls(int width, int height,const char * graphics_path)
{
	touchcontrols::GLScaleWidth = (float)width;
	touchcontrols::GLScaleHeight = (float)height;

	LOGI("initControls %d x %d,x path = %s",width,height,graphics_path);

	if (!controlsCreated)
	{
		LOGI("creating controls");

		touchcontrols::setGraphicsBasePath(graphics_path);
		setControlsContainer(&controlsContainer);

		controlsContainer.openGL_start.connect( sigc::ptr_fun(&openGLStart));
		controlsContainer.openGL_end.connect( sigc::ptr_fun(&openGLEnd));


		tcMenuMain = new touchcontrols::TouchControls("menu",true,false);
		tcGameMain = new touchcontrols::TouchControls("game",false,true,1);
		tcGameWeapons = new touchcontrols::TouchControls("weapons",false,true,1);
		tcInventory  = new touchcontrols::TouchControls("inventory",false,true,1);
		tcWeaponWheel = new touchcontrols::TouchControls("weapon_wheel",false,true,1);

		tcGameMain->signal_settingsButton.connect(  sigc::ptr_fun(&gameSettingsButton) );

		//Menu
		tcMenuMain->addControl(new touchcontrols::Button("down_arrow",touchcontrols::RectF(20,13,23,16),"arrow_down",PORT_ACT_MENU_DOWN));
		tcMenuMain->addControl(new touchcontrols::Button("up_arrow",touchcontrols::RectF(20,10,23,13),"arrow_up",PORT_ACT_MENU_UP));
		tcMenuMain->addControl(new touchcontrols::Button("left_arrow",touchcontrols::RectF(17,13,20,16),"arrow_left",PORT_ACT_MENU_LEFT));
		tcMenuMain->addControl(new touchcontrols::Button("right_arrow",touchcontrols::RectF(23,13,26,16),"arrow_right",PORT_ACT_MENU_RIGHT));
		tcMenuMain->addControl(new touchcontrols::Button("enter",touchcontrols::RectF(0,12,4,16),"enter",PORT_ACT_MENU_SELECT));
		tcMenuMain->addControl(new touchcontrols::Button("keyboard",touchcontrols::RectF(0,0,3,3),"keyboard",KEY_SHOW_KBRD));


#ifdef CHOC_SETUP
		tcMenuMain->addControl(new touchcontrols::Button("f10",touchcontrols::RectF(23,0,26,3),"f10",SDL_SCANCODE_F10));
#endif
		tcMenuMain->signal_button.connect(  sigc::ptr_fun(&menuButton) );

		tcMenuMain->setAlpha(0.8);


		//Game
		tcGameMain->setAlpha(gameControlsAlpha);
		tcGameMain->addControl(new touchcontrols::Button("attack",touchcontrols::RectF(20,7,23,10),"shoot",KEY_SHOOT));
		tcGameMain->addControl(new touchcontrols::Button("use",touchcontrols::RectF(23,6,26,9),"use",PORT_ACT_USE));
		tcGameMain->addControl(new touchcontrols::Button("quick_save",touchcontrols::RectF(24,0,26,2),"save",PORT_ACT_QUICKSAVE));
		tcGameMain->addControl(new touchcontrols::Button("quick_load",touchcontrols::RectF(20,0,22,2),"load",PORT_ACT_QUICKLOAD));
		tcGameMain->addControl(new touchcontrols::Button("map",touchcontrols::RectF(2,0,4,2),"map",PORT_ACT_MAP));
		//tcGameMain->addControl(new touchcontrols::Button("plus",touchcontrols::RectF(17,0,19,2),"zoom_in",PORT_ACT_MAP_ZOOM_IN));
		//tcGameMain->addControl(new touchcontrols::Button("minus",touchcontrols::RectF(15,0,17,2),"zoom_out",PORT_ACT_MAP_ZOOM_OUT));
		tcGameMain->addControl(new touchcontrols::Button("keyboard",touchcontrols::RectF(8,0,10,2),"keyboard",KEY_SHOW_KBRD));
		tcGameMain->addControl(new touchcontrols::Button("gamma",touchcontrols::RectF(14,0,16,2),"gamma",PORT_ACT_GAMMA));

#if defined(CHOC_STRIFE)
		tcGameMain->addControl(new touchcontrols::Button("show_strife_weapons",touchcontrols::RectF(4,0,6,2),"ammo",PORT_ACT_SHOW_WEAPONS));
		tcGameMain->addControl(new touchcontrols::Button("show_keys",touchcontrols::RectF(6,0,8,2),"key",PORT_ACT_SHOW_KEYS));
		tcGameMain->addControl(new touchcontrols::Button("mission",touchcontrols::RectF(18,0,20,2),"notebook",PORT_ACT_HELPCOMP));

#endif

#if defined(CHOC_HEXEN) || defined(CHOC_STRIFE)
		tcGameMain->addControl(new touchcontrols::Button("use_inventory",touchcontrols::RectF(16,0,18,2),"inv",KEY_SHOW_INV));
		tcGameMain->addControl(new touchcontrols::Button("jump",touchcontrols::RectF(24,3,26,5),"jump",PORT_ACT_JUMP));
#endif

#if defined(CHOC_HERETIC)
		tcGameMain->addControl(new touchcontrols::Button("use_inventory",touchcontrols::RectF(16,0,18,2),"inv",KEY_SHOW_INV));
#endif

		tcGameMain->addControl(new touchcontrols::Button("show_weapons",touchcontrols::RectF(12,14,14,16),"show_weapons",KEY_SHOW_WEAPONS));
		nextWeapon = new touchcontrols::Button("next_weapon",touchcontrols::RectF(0,3,3,5),"next_weap",PORT_ACT_NEXT_WEP);
		tcGameMain->addControl(nextWeapon);
		prevWeapon = new touchcontrols::Button("prev_weapon",touchcontrols::RectF(0,5,3,7),"prev_weap",PORT_ACT_PREV_WEP);
		tcGameMain->addControl(prevWeapon);

		touchJoyLeft = new touchcontrols::TouchJoy("stick",touchcontrols::RectF(0,7,8,16),"strafe_arrow");
		tcGameMain->addControl(touchJoyLeft);
		touchJoyLeft->signal_move.connect(sigc::ptr_fun(&left_stick) );
		touchJoyLeft->signal_double_tap.connect(sigc::ptr_fun(&left_double_tap) );

		touchJoyRight = new touchcontrols::TouchJoy("touch",touchcontrols::RectF(17,4,26,16),"look_arrow");
		tcGameMain->addControl(touchJoyRight);
		touchJoyRight->signal_move.connect(sigc::ptr_fun(&right_stick) );
		touchJoyRight->signal_double_tap.connect(sigc::ptr_fun(&right_double_tap) );

		tcGameMain->signal_button.connect(  sigc::ptr_fun(&gameButton) );

		//Weapons
		tcGameWeapons->addControl(new touchcontrols::Button("weapon1",touchcontrols::RectF(1,14,3,16),"key_1",SDL_SCANCODE_1));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon2",touchcontrols::RectF(4,14,6,16),"key_2",SDL_SCANCODE_2));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon3",touchcontrols::RectF(7,14,9,16),"key_3",SDL_SCANCODE_3));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon4",touchcontrols::RectF(10,14,12,16),"key_4",SDL_SCANCODE_4));

		tcGameWeapons->addControl(new touchcontrols::Button("weapon5",touchcontrols::RectF(14,14,16,16),"key_5",SDL_SCANCODE_5));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon6",touchcontrols::RectF(17,14,19,16),"key_6",SDL_SCANCODE_6));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon7",touchcontrols::RectF(20,14,22,16),"key_7",SDL_SCANCODE_7));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon8",touchcontrols::RectF(23,14,25,16),"key_8",SDL_SCANCODE_8));
		tcGameWeapons->signal_button.connect(  sigc::ptr_fun(&selectWeaponButton) );
		tcGameWeapons->setAlpha(0.8);

		//Weapon wheel
		touchcontrols::WheelSelect *wheel = new touchcontrols::WheelSelect("weapon_wheel",touchcontrols::RectF(7,2,19,14),"weapon_wheel",8);
		wheel->signal_selected.connect(sigc::ptr_fun(&weaponWheel) );
		wheel->signal_enabled.connect(sigc::ptr_fun(&weaponWheelSelected));
		tcWeaponWheel->addControl(wheel);
		tcWeaponWheel->setAlpha(0.5);
#if defined(CHOC_HEXEN) || defined(CHOC_HERETIC)
		//Inventory
		tcInventory->addControl(new touchcontrols::Button("invuse",touchcontrols::RectF(3,14,5,16),"enter",PORT_ACT_INVUSE));
		tcInventory->addControl(new touchcontrols::Button("invprev",touchcontrols::RectF(6,14,8,16),"arrow_left",PORT_ACT_INVPREV));
		tcInventory->addControl(new touchcontrols::Button("invnext",touchcontrols::RectF(8,14,10,16),"arrow_right",PORT_ACT_INVNEXT));
		tcInventory->addControl(new touchcontrols::Button("fly_up",touchcontrols::RectF(22,14,24,16),"next_weap",PORT_ACT_FLY_UP));
		tcInventory->addControl(new touchcontrols::Button("fly_down",touchcontrols::RectF(24,14,26,16),"prev_weap",PORT_ACT_FLY_DOWN));

#endif
#if defined(CHOC_STRIFE)
		//Inventory
		tcInventory->addControl(new touchcontrols::Button("invuse",touchcontrols::RectF(0,14,2,16),"enter",PORT_ACT_INVUSE));
		tcInventory->addControl(new touchcontrols::Button("invprev",touchcontrols::RectF(0,12,2,14),"arrow_left",PORT_ACT_INVPREV));
		tcInventory->addControl(new touchcontrols::Button("invnext",touchcontrols::RectF(2,12,4,14),"arrow_right",PORT_ACT_INVNEXT));
		tcInventory->addControl(new touchcontrols::Button("invdrop",touchcontrols::RectF(2,14,4,16),"arrow_down",PORT_ACT_INVDROP));
#endif

		tcInventory->signal_button.connect(  sigc::ptr_fun(&inventoryButton) );
		tcInventory->setAlpha(0.5);

		controlsContainer.addControlGroup(tcGameMain);
		controlsContainer.addControlGroup(tcGameWeapons);
		controlsContainer.addControlGroup(tcMenuMain);
#ifdef CHOC_DOOM
		controlsContainer.addControlGroup(tcWeaponWheel);
#endif
		controlsContainer.addControlGroup(tcInventory);
		controlsCreated = 1;

		tcGameMain->setXMLFile((std::string)graphics_path +  "/game_choc.xml");
		tcInventory->setXMLFile((std::string)graphics_path +  "/inventory_choc.xml");
		tcWeaponWheel->setXMLFile((std::string)graphics_path +  "/weaponwheel_choc.xml");
		tcGameWeapons->setXMLFile((std::string)graphics_path +  "/weapons_choc.xml");
	}
	else
		LOGI("NOT creating controls");


}


int inMenuLast = 1;
int inAutomapLast = 0;
void frameControls()
{
    static bool glInit = false;
    if( !glInit )
    {
        controlsContainer.initGL();
        glInit = true;
    }
	//	LOGI("frameControls");

	int inMenuNew = PortableInMenu();
	if (inMenuLast != inMenuNew)
	{
		inMenuLast = inMenuNew;
		if (!inMenuNew)
		{
			tcGameMain->setEnabled(true);
			if (enableWeaponWheel)
				tcWeaponWheel->setEnabled(true);
			tcMenuMain->setEnabled(false);
		}
		else
		{
			tcGameMain->setEnabled(false);
			tcGameWeapons->setEnabled(false);
			tcWeaponWheel->setEnabled(false);
			tcMenuMain->setEnabled(true);
		}
	}


	weaponCycle(showWeaponCycle);
	setHideSticks(!showSticks);
	controlsContainer.draw();


	//swapBuffers();

	// glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT);
}


void setTouchSettings(float alpha,float strafe,float fwd,float pitch,float yaw,int other)
{

	gameControlsAlpha = alpha;
	if (tcGameMain)
		tcGameMain->setAlpha(gameControlsAlpha);

	showWeaponCycle = other & 0x1?true:false;
	turnMouseMode   = other & 0x2?true:false;
	invertLook      = other & 0x4?true:false;
	precisionShoot  = other & 0x8?true:false;
	showSticks      = other & 0x1000?true:false;
	enableWeaponWheel  = other & 0x2000?true:false;

	if (tcWeaponWheel)
		tcWeaponWheel->setEnabled(enableWeaponWheel);


	hideTouchControls = other & 0x80000000?true:false;


	switch ((other>>4) & 0xF)
	{
	case 1:
		left_double_action = PORT_ACT_ATTACK;
		break;
	case 2:
		left_double_action = PORT_ACT_JUMP;
		break;
	default:
		left_double_action = 0;
	}

	switch ((other>>8) & 0xF)
	{
	case 1:
		right_double_action = PORT_ACT_ATTACK;
		break;
	case 2:
		right_double_action = PORT_ACT_JUMP;
		break;
	default:
		right_double_action = 0;
	}

	strafe_sens = strafe;
	forward_sens = fwd;
	pitch_sens = pitch;
	yaw_sens = yaw;

}

int quit_now = 0;

#define EXPORT_ME __attribute__ ((visibility("default")))

JNIEnv* env_;

int argc=1;
const char * argv[32];
std::string graphicpath;


std::string game_path;

const char * getGamePath()
{
	return game_path.c_str();
}

std::string home_env;


extern void Android_SetGameResolution(int width, int height);

jint EXPORT_ME
JAVA_FUNC(init) ( JNIEnv* env,	jobject thiz,jstring graphics_dir,jint mem_mb,jobjectArray argsArray,jint lowRes,jstring game_path_ )
{
#ifdef ANTI_HACK
	getGlobalClasses(env);
#endif

	env_ = env;
	/*
	if (lowRes)
		Android_SetGameResolution(320,240);
	else
		Android_SetGameResolution(640,400);
	 */
	//Android_SetGameResolution(320,200);
//	Android_SetGameResolution(320,240);
	//Android_SetGameResolution(640,480);

	argv[0] = "quake";
	int argCount = (env)->GetArrayLength( argsArray);
	LOGI("argCount = %d",argCount);
	for (int i=0; i<argCount; i++) {
		jstring string = (jstring) (env)->GetObjectArrayElement( argsArray, i);
		argv[argc] = (char *)(env)->GetStringUTFChars( string, 0);
		LOGI("arg = %s",argv[argc]);
		argc++;
	}


	game_path = (char *)(env)->GetStringUTFChars( game_path_, 0);

	LOGI("game_path = %s",getGamePath());

	//Needed for ecwolf to run
	//home_env = "HOME=/" + game_path;
	//putenv(home_env.c_str());
	setenv("HOME", getGamePath(),1);

	putenv("TIMIDITY_CFG=../timidity.cfg");

	chdir(getGamePath());


	const char * p = env->GetStringUTFChars(graphics_dir,NULL);
	graphicpath =  std::string(p);



	initControls(android_screen_width,-android_screen_height,graphicpath.c_str());

	SDL_SetSwapBufferCallBack(frameControls);

	//Now done in java to keep context etc
	//SDL_SwapBufferPerformsSwap(false);

	PortableInit(argc,argv); //Never returns!!

	return 0;
}


jint EXPORT_ME
JAVA_FUNC(frame) ( JNIEnv* env,	jobject thiz )
{

	//NOT CALLED
	/*
	LOGI("frame");

	PortableFrame();

	if (quit_now)
	{
		LOGI("frame QUIT");
		return 128;
	}

	frameControls();

	int ret = 0;

	return ret;
	 */
}

__attribute__((visibility("default"))) jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	LOGI("JNI_OnLoad");
	setTCJNIEnv(vm);

	return JNI_VERSION_1_4;
}


void EXPORT_ME
JAVA_FUNC(keypress) (JNIEnv *env, jobject obj,jint down, jint keycode, jint unicode)
{
	LOGI("keypress %d",keycode);
	if (controlsContainer.isEditing())
	{
		if (down && (keycode == SDL_SCANCODE_ESCAPE ))
			controlsContainer.finishEditing();
		return;
	}
	PortableKeyEvent(down,keycode,unicode);

}


void EXPORT_ME
JAVA_FUNC(touchEvent) (JNIEnv *env, jobject obj,jint action, jint pid, jfloat x, jfloat y)
{
	//LOGI("TOUCHED");
	controlsContainer.processPointer(action,pid,x,y);
}


void EXPORT_ME
JAVA_FUNC(doAction) (JNIEnv *env, jobject obj,	jint state, jint action)
{
	LOGI("doAction %d %d",state,action);
	if (hideTouchControls)
		if (tcGameMain)
		{
			if (tcGameMain->isEnabled())
				tcGameMain->animateOut(30);

			tcWeaponWheel->animateOut(1);
		}
	PortableAction(state,action);
}

void EXPORT_ME
JAVA_FUNC(analogFwd) (JNIEnv *env, jobject obj,	jfloat v)
{
	PortableMoveFwd(v);
}

void EXPORT_ME
JAVA_FUNC(analogSide) (JNIEnv *env, jobject obj,jfloat v)
{
	PortableMoveSide(v);
}

void EXPORT_ME
JAVA_FUNC(analogPitch) (JNIEnv *env, jobject obj,
		jint mode,jfloat v)
		{
	PortableLookPitch(mode, v);
		}

void EXPORT_ME
JAVA_FUNC(analogYaw) (JNIEnv *env, jobject obj,	jint mode,jfloat v)
{
	PortableLookYaw(mode, v);
}

void EXPORT_ME
JAVA_FUNC(setTouchSettings) (JNIEnv *env, jobject obj,	jfloat alpha,jfloat strafe,jfloat fwd,jfloat pitch,jfloat yaw,int other)
{
	setTouchSettings(alpha,strafe,fwd,pitch,yaw,other);
}

std::string quickCommandString;
jint EXPORT_ME
JAVA_FUNC(quickCommand) (JNIEnv *env, jobject obj,	jstring command)
{

	return 0;
}




void EXPORT_ME
JAVA_FUNC(setScreenSize) ( JNIEnv* env,	jobject thiz, jint width, jint height)
{
	android_screen_width = width;
	android_screen_height = height;
}


}
