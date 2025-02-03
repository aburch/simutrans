#include "../simutrans/sys/simsys.h"
#include "../simutrans/simdebug.h"

#include <jni.h>
#include <android/log.h>
#include <string>

#include "android.h"

// Function to find the class and method ID (Must call this once on init because otherwise not found) => cache the results!
// for some reason (maybe because main SDL runs in other thread) this works only for static functions
static struct MethodIDs {
	jmethodID downloadfile;
	jclass myClass;
} methodIDs;

bool getMethodIDs(JNIEnv *env, jobject thisClass) {
	// Find the Java class
	jclass cls = env->FindClass("com/simutrans/Simutrans"); // Use fully qualified name
	if (cls == nullptr) {
		dbg->fatal("getMethodIDs()","Class not found");
		return false; // Class not found
	}

	methodIDs.myClass = static_cast<jclass>(env->NewGlobalRef(cls)); // Prevent GC

	// Find the instance method
	methodIDs.downloadfile = env->GetStaticMethodID(methodIDs.myClass, "downloadFile", "(Ljava/lang/String;Ljava/lang/String;)V");
	if (methodIDs.downloadfile == nullptr) {
		env->ExceptionClear(); // Clear the exception if the method is not found
		__android_log_print(ANDROID_LOG_ERROR, "Simutrans", "methodIDs.downloadfile not found!");
		return false; // Method not found
	}
	__android_log_print(ANDROID_LOG_INFO, "Simutrans", "methodIDs.downloadfile %p", methodIDs.downloadfile);

	return true;
}

extern "C" {

// get the version but 
JNIEXPORT jstring JNICALL Java_com_simutrans_Simutrans_getVersion(JNIEnv* env, jobject thisClass)
{
	// now we have a valid class reference => use this to get the static memberIDs ...
	getMethodIDs(env,thisClass);
	return env->NewStringUTF(get_version());
}
}


#include <SDL_system.h>

const char *download_file(const char *url, const char *filename)
{
	const char * error_msg = NULL;
	JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();

	jstring jurl = env->NewStringUTF(url);
	jstring jfilename = env->NewStringUTF(filename);


	// Call the Java method
	env->CallStaticVoidMethod(methodIDs.myClass, methodIDs.downloadfile, jurl, jfilename);

	// Check for exceptions
	if (env->ExceptionCheck()) {
		jthrowable exception = env->ExceptionOccurred();
		env->ExceptionDescribe();
		env->ExceptionClear();

		jclass exceptionCls = env->GetObjectClass(exception);
		jmethodID getMessage = env->GetMethodID(exceptionCls, "getMessage", "()Ljava/lang/String;");
		jstring message = (jstring)env->CallObjectMethod(exception, getMessage);
		error_msg = env->GetStringUTFChars(message, nullptr);
		__android_log_print(ANDROID_LOG_INFO, "Simutrans", "downloadfile from %s failed with %s", url, error_msg);
	}

	env->DeleteLocalRef(jurl);
	env->DeleteLocalRef(jfilename);

	return error_msg;
}


