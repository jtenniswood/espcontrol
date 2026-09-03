#import "MediaRemoteShim.h"

#import <dlfcn.h>

typedef void (*ECRegisterNotificationsFn)(dispatch_queue_t);
typedef void (*ECGetNowPlayingInfoFn)(dispatch_queue_t, void (^)(CFDictionaryRef _Nullable));
typedef void (*ECGetNowPlayingPIDFn)(dispatch_queue_t, void (^)(int));
typedef BOOL (*ECSendCommandFn)(uint32_t, CFDictionaryRef _Nullable);

static void *ECMediaRemoteHandle;
static ECRegisterNotificationsFn ECRegisterNotifications;
static ECGetNowPlayingInfoFn ECGetNowPlayingInfo;
static ECGetNowPlayingPIDFn ECGetNowPlayingPID;
static ECSendCommandFn ECSendCommand;
static NSMutableArray<id> *ECObservers;

static BOOL ECLoadMediaRemote(void) {
  // Keep loading the framework independent from the optional Now Playing
  // entry points. Media commands can remain usable when Apple changes or
  // removes one of those read-only symbols.
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    ECMediaRemoteHandle = dlopen(
        "/System/Library/PrivateFrameworks/MediaRemote.framework/MediaRemote",
        RTLD_LAZY | RTLD_LOCAL);
    if (ECMediaRemoteHandle == NULL) return;
    ECRegisterNotifications = (ECRegisterNotificationsFn) dlsym(
        ECMediaRemoteHandle, "MRMediaRemoteRegisterForNowPlayingNotifications");
    ECGetNowPlayingInfo = (ECGetNowPlayingInfoFn) dlsym(
        ECMediaRemoteHandle, "MRMediaRemoteGetNowPlayingInfo");
    ECGetNowPlayingPID = (ECGetNowPlayingPIDFn) dlsym(
        ECMediaRemoteHandle, "MRMediaRemoteGetNowPlayingApplicationPID");
    ECSendCommand = (ECSendCommandFn) dlsym(
        ECMediaRemoteHandle, "MRMediaRemoteSendCommand");
  });
  return ECMediaRemoteHandle != NULL;
}

@implementation ECMediaRemoteBridge

+ (BOOL)isAvailable {
  return ECLoadMediaRemote() && ECRegisterNotifications != NULL &&
         ECGetNowPlayingInfo != NULL && ECGetNowPlayingPID != NULL;
}

+ (BOOL)isCommandAvailable {
  return ECLoadMediaRemote() && ECSendCommand != NULL;
}

+ (void)fetchNowPlaying:(ECMediaRemoteSnapshotHandler)handler {
  if (![self isAvailable]) {
    dispatch_async(dispatch_get_main_queue(), ^{ handler(nil, nil); });
    return;
  }
  ECGetNowPlayingPID(dispatch_get_main_queue(), ^(int processIdentifier) {
    ECGetNowPlayingInfo(dispatch_get_main_queue(), ^(CFDictionaryRef value) {
      NSDictionary *info = value == NULL ? nil : [(__bridge NSDictionary *) value copy];
      NSNumber *pid = processIdentifier > 0 ? @(processIdentifier) : nil;
      handler(info, pid);
    });
  });
}

+ (BOOL)startObservingChanges:(ECMediaRemoteChangeHandler)handler {
  if (![self isAvailable]) return NO;
  [self stopObservingChanges];
  ECRegisterNotifications(dispatch_get_main_queue());
  ECObservers = [NSMutableArray array];
  NSArray<NSString *> *names = @[
    @"kMRMediaRemoteNowPlayingInfoDidChangeNotification",
    @"kMRMediaRemoteNowPlayingApplicationDidChangeNotification",
    @"kMRMediaRemoteNowPlayingApplicationPlaybackStateDidChangeNotification",
  ];
  for (NSString *name in names) {
    id observer = [[NSNotificationCenter defaultCenter]
        addObserverForName:name object:nil queue:[NSOperationQueue mainQueue]
        usingBlock:^(__unused NSNotification *notification) { handler(); }];
    [ECObservers addObject:observer];
  }
  return YES;
}

+ (void)stopObservingChanges {
  for (id observer in ECObservers) {
    [[NSNotificationCenter defaultCenter] removeObserver:observer];
  }
  ECObservers = nil;
}

+ (BOOL)sendCommand:(uint32_t)command {
  if (![self isCommandAvailable]) return NO;
  return ECSendCommand(command, NULL);
}

@end
