
LOCAL_PATH := $(call my-dir)/../src/

include $(CLEAR_VARS)

LOCAL_MODULE    := doomretro

LOCAL_CFLAGS :=  -O0 -g -DRETRO_DOOM -Wall -Wdeclaration-after-statement  -D_GNU_SOURCE=1 -D_REENTRANT -fsigned-char


LOCAL_C_INCLUDES :=     $(SDL_INCLUDE_PATHS)  \
                        $(TOP_DIR)/MobileTouchControls \
                        $(TOP_DIR)/Clibs_OpenTouch \
                        $(LOCAL_PATH)/../mobile \

ANDROID_FILES = \
      ../../common_interface/android_jni.cpp \
      ../../common_interface/touch_interface.cpp \
      ../mobile/game_interface.c \

FILE_LIST := $(wildcard $(LOCAL_PATH)*.c)
LOCAL_SRC_FILES := $(ANDROID_FILES) $(FILE_LIST:$(LOCAL_PATH)%=%)


LOCAL_LDLIBS += -llog -lz -lGLESv1_CM

LOCAL_SHARED_LIBRARIES := touchcontrols SDL2 SDL2_mixer SDL2_image

include $(BUILD_SHARED_LIBRARY)






