#include "android.h"
#include "../simutrans/sys/simsys.h"

extern "C" {
JNIEXPORT jstring JNICALL Java_com_simutrans_simutrans_Simutrans_getVersion(JNIEnv* env, jobject thisObject) { 
    return env->NewStringUTF(get_version()); 
} 

}