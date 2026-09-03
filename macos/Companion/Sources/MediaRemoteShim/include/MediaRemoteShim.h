#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^ECMediaRemoteSnapshotHandler)(NSDictionary<NSString *, id> * _Nullable info,
                                              NSNumber * _Nullable processIdentifier,
                                              NSNumber * _Nullable isPlaying);
typedef void (^ECMediaRemoteChangeHandler)(void);

@interface ECMediaRemoteBridge : NSObject
+ (BOOL)isAvailable;
+ (BOOL)isCommandAvailable;
+ (void)fetchNowPlaying:(ECMediaRemoteSnapshotHandler)handler;
+ (BOOL)startObservingChanges:(ECMediaRemoteChangeHandler)handler;
+ (void)stopObservingChanges;
+ (BOOL)sendCommand:(uint32_t)command;
@end

NS_ASSUME_NONNULL_END
