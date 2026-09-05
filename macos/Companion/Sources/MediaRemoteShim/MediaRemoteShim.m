#import "MediaRemoteShim.h"

#import <dlfcn.h>

typedef void (*ECRegisterNotificationsFn)(dispatch_queue_t);
typedef void (*ECGetNowPlayingInfoFn)(dispatch_queue_t, void (^)(CFDictionaryRef _Nullable));
typedef void (*ECGetNowPlayingPIDFn)(dispatch_queue_t, void (^)(int));
typedef void (*ECGetNowPlayingIsPlayingFn)(dispatch_queue_t, void (^)(BOOL));
typedef BOOL (*ECSendCommandFn)(uint32_t, CFDictionaryRef _Nullable);

static void *ECMediaRemoteHandle;
static ECRegisterNotificationsFn ECRegisterNotifications;
static ECGetNowPlayingInfoFn ECGetNowPlayingInfo;
static ECGetNowPlayingPIDFn ECGetNowPlayingPID;
static ECGetNowPlayingIsPlayingFn ECGetNowPlayingIsPlaying;
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
    ECGetNowPlayingIsPlaying = (ECGetNowPlayingIsPlayingFn) dlsym(
        ECMediaRemoteHandle, "MRMediaRemoteGetNowPlayingApplicationIsPlaying");
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
    dispatch_async(dispatch_get_main_queue(), ^{ handler(nil, nil, nil); });
    return;
  }
  ECGetNowPlayingPID(dispatch_get_main_queue(), ^(int processIdentifier) {
    ECGetNowPlayingInfo(dispatch_get_main_queue(), ^(CFDictionaryRef value) {
      NSDictionary *info = value == NULL ? nil : [(__bridge NSDictionary *) value copy];
      NSNumber *pid = processIdentifier > 0 ? @(processIdentifier) : nil;
      if (ECGetNowPlayingIsPlaying != NULL) {
        ECGetNowPlayingIsPlaying(dispatch_get_main_queue(), ^(BOOL isPlaying) {
          handler(info, pid, @(isPlaying));
        });
      } else {
        handler(info, pid, nil);
      }
    });
  });
}

+ (BOOL)startObservingChanges:(ECMediaRemoteChangeHandler)handler {
  if (![self isAvailable]) return NO;
  [self stopObservingChanges];
  ECRegisterNotifications(dispatch_get_main_queue());
  ECObservers = [NSMutableArray array];
  NSArray<NSString *> *symbols = @[
    @"kMRMediaRemoteNowPlayingInfoDidChangeNotification",
    @"kMRMediaRemoteNowPlayingApplicationDidChangeNotification",
    @"kMRMediaRemoteNowPlayingApplicationPlaybackStateDidChangeNotification",
  ];
  for (NSString *symbol in symbols) {
    CFStringRef *notificationPointer = (CFStringRef *)dlsym(
        ECMediaRemoteHandle, symbol.UTF8String);
    if (notificationPointer == NULL || *notificationPointer == NULL) continue;
    NSString *name = (__bridge NSString *)*notificationPointer;
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
