// See https://www.synack.com/blog/untranslocating-apps/ and its implementation in TaskExplorer

#import <dlfcn.h>
#import "translocation.h"

#import <AppKit/AppKit.h>
#import <Security/Security.h>
#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>


//path to xattr
#define XATTR @"/usr/bin/xattr"

//path to open
#define OPEN @"/usr/bin/open"

//app kit version for OS X 10.11
#define APPKIT_VERSION_10_11 1404

//key for stdout output
#define STDOUT @"stdOutput"

//key for stderr output
#define STDERR @"stdError"

//key for exit code
#define EXIT_CODE @"exitCode"




//exec a process with args
// if 'shouldWait' is set, wait and return stdout/in and termination status
NSMutableDictionary* execTask(NSString* binaryPath, NSArray* arguments, BOOL shouldWait)
{
	//task
	NSTask* task = nil;

	//output pipe for stdout
	NSPipe* stdOutPipe = nil;

	//output pipe for stderr
	NSPipe* stdErrPipe = nil;

	//read handle for stdout
	NSFileHandle* stdOutReadHandle = nil;

	//read handle for stderr
	NSFileHandle* stdErrReadHandle = nil;

	//results dictionary
	NSMutableDictionary* results = nil;

	//output for stdout
	NSMutableData *stdOutData = nil;

	//output for stderr
	NSMutableData *stdErrData = nil;

	//init dictionary for results
	results = [NSMutableDictionary dictionary];

	//init task
	task = [NSTask new];

	//only setup pipes if wait flag is set
	if(YES == shouldWait)
	{
		//init stdout pipe
		stdOutPipe = [NSPipe pipe];

		//init stderr pipe
		stdErrPipe = [NSPipe pipe];

		//init stdout read handle
		stdOutReadHandle = [stdOutPipe fileHandleForReading];

		//init stderr read handle
		stdErrReadHandle = [stdErrPipe fileHandleForReading];

		//init stdout output buffer
		stdOutData = [NSMutableData data];

		//init stderr output buffer
		stdErrData = [NSMutableData data];

		//set task's stdout
		task.standardOutput = stdOutPipe;

		//set task's stderr
		task.standardError = stdErrPipe;
	}

	//set task's path
	task.launchPath = binaryPath;

	//set task's args
	if(nil != arguments)
	{
		//set
		task.arguments = arguments;
	}

	//wrap task launch
	@try
	{
		//launch
		[task launch];
	}
	@catch(NSException *exception)
	{
		//bail
		goto bail;
	}

	//no need to wait
	// can just bail w/ no output
	if(YES != shouldWait)
	{
		//bail
		goto bail;
	}

	//read in stdout/stderr
	while(YES == [task isRunning])
	{
		//accumulate stdout
		[stdOutData appendData:[stdOutReadHandle readDataToEndOfFile]];

		//accumulate stderr
		[stdErrData appendData:[stdErrReadHandle readDataToEndOfFile]];
	}

	//grab any leftover stdout
	[stdOutData appendData:[stdOutReadHandle readDataToEndOfFile]];

	//grab any leftover stderr
	[stdErrData appendData:[stdErrReadHandle readDataToEndOfFile]];

	//add stdout
	if(0 != stdOutData.length)
	{
		//add
		results[STDOUT] = stdOutData;
	}

	//add stderr
	if(0 != stdErrData.length)
	{
		//add
		results[STDERR] = stdErrData;
	}

	//add exit code
	results[EXIT_CODE] = [NSNumber numberWithInteger:task.terminationStatus];

bail:

	return results;
}



//check if app is translocated
// ->based on http://lapcatsoftware.com/articles/detect-app-translocation.html
NSURL* getUnTranslocatedURL()
{
	//orignal URL
	NSURL* untranslocatedURL = nil;

	//function def for 'SecTranslocateIsTranslocatedURL'
	Boolean (*mySecTranslocateIsTranslocatedURL)(CFURLRef path, bool *isTranslocated, CFErrorRef * __nullable error);

	//function def for 'SecTranslocateCreateOriginalPathForURL'
	CFURLRef __nullable (*mySecTranslocateCreateOriginalPathForURL)(CFURLRef translocatedPath, CFErrorRef * __nullable error);

	//flag for API request
	bool isTranslocated = false;

	//handle for security framework
	void *handle = NULL;

	//app path
	NSURL* appPath = nil;

	//init app's path
	appPath = [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]];

	//check ignore pre-macOS Sierra
	if(floor(NSAppKitVersionNumber) <= APPKIT_VERSION_10_11)
	{
		//bail
		goto bail;
	}

	//open security framework
	handle = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY);
	if(NULL == handle)
	{
		//bail
		goto bail;
	}

	//get 'SecTranslocateIsTranslocatedURL' API
	mySecTranslocateIsTranslocatedURL = dlsym(handle, "SecTranslocateIsTranslocatedURL");
	if(NULL == mySecTranslocateIsTranslocatedURL)
	{
		//bail
		goto bail;
	}

	//get
	mySecTranslocateCreateOriginalPathForURL = dlsym(handle, "SecTranslocateCreateOriginalPathForURL");
	if(NULL == mySecTranslocateCreateOriginalPathForURL)
	{
		//bail
		goto bail;
	}

	//invoke it
	if(true != mySecTranslocateIsTranslocatedURL((__bridge CFURLRef)appPath, &isTranslocated, NULL))
	{
		//bail
		goto bail;
	}

	//bail if app isn't translocated
	if(true != isTranslocated)
	{
		//bail
		goto bail;
	}

	//get original URL
	untranslocatedURL = (__bridge NSURL*)mySecTranslocateCreateOriginalPathForURL((__bridge CFURLRef)appPath, NULL);

//bail
bail:

	//close handle
	if(NULL != handle)
	{
		//close
		dlclose(handle);
	}

	return untranslocatedURL;
}



// restart us if translocated
bool RestartIfTranslocated()
{
	//untranslocated URL
	NSURL* untranslocatedURL = nil;

	//get original url
	untranslocatedURL = getUnTranslocatedURL();
	if(nil != untranslocatedURL)
	{
		//remove quarantine attributes of original
		execTask(XATTR, @[@"-cr", untranslocatedURL.path], NO);

		//nap
		[NSThread sleepForTimeInterval:0.5];

		//relaunch
		// use 'open' since allows two instances of app to be run
		execTask(OPEN, @[@"-n", @"-a", untranslocatedURL.path], NO);

		//this instance is done
		return 1;
	}
	return 0;
}
