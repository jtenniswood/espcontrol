import {
  deleteCardImageConfigurationFirst,
  createCardImageBackupAssetProvider,
  createCardImagesFeature,
  type CardImageItem,
  type CardImageHttpRequest,
} from "../features/card_images";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import { state } from "../state/app_instance";

export function installCardImageServiceModule(): GlobalDescriptors {
    var CARD_IMAGE_TARGET_SIZE: any = 200;
    var CARD_IMAGE_UPLOAD_MAX_BYTES: any = 45 * 1024;
    var CARD_IMAGE_MIN_QUALITY: any = 0.42;
    var cardImagesFeature = createCardImagesFeature({
        maxActiveBackgrounds: CARD_BACKGROUND_IMAGE_LIMIT,
        fetch: function (url: string, request?: CardImageHttpRequest) {
            return fetch(url, request as RequestInit);
        },
        normalizeId: function (value: unknown) { return normalizeCardBackgroundImageId(value); },
        imageUrl: function (id: string) { return cardImageUrl(id); },
        targetSize: function () { return CARD_IMAGE_TARGET_SIZE; },
        uploadMaxBytes: function () { return CARD_IMAGE_UPLOAD_MAX_BYTES; },
        minimumQuality: function () { return CARD_IMAGE_MIN_QUALITY; },
    });
    var cardImageBackupAssetProvider = createCardImageBackupAssetProvider(cardImagesFeature, {
        normalizeId: function (value: unknown) { return normalizeCardBackgroundImageId(value); },
        imageId: function (button: any) { return cardBackgroundImage(button); },
        setImageId: function (button: any, id: string) {
            setCardBackgroundImage(button, id);
        },
    });

    function listCardImages(this: any, force?: any) {
        return cardImagesFeature.list(!!force);
    }

    function cardImageLibraryInfo(this: any) {
        return cardImagesFeature.info();
    }

    function countCardImageUsage(this: any, id?: any) {
        id = normalizeCardBackgroundImageId(id);
        if (!id)
            return 0;
        var count: any = 0;
        function countButtons(this: any, buttons?: any) {
            (buttons || []).forEach(function (this: any, button?: any) {
                if (cardBackgroundImage(button) === id)
                    count++;
            });
        }
        countButtons(state.buttons);
        Object.keys(state.subpages || {}).forEach(function (this: any, key?: any) {
            var subpage: any = state.subpages[key];
            countButtons(subpage && subpage.buttons);
        });
        return count;
    }

    function resizeCardImageFile(this: any, file?: any) {
        return cardImagesFeature.resize(file);
    }

    function uploadCardImage(this: any, file?: any) {
        return cardImagesFeature.upload(file);
    }

    function renameCardImage(this: any, id?: any, name?: any) {
        return cardImagesFeature.rename(id, name);
    }

    function deleteCardImage(this: any, id?: any) {
        return cardImagesFeature.delete(id);
    }

    function deleteCardImageSafely(this: any, id?: any) {
        id = normalizeCardBackgroundImageId(id);
        if (!id)
            return Promise.reject(new Error("Invalid card image ID."));
        return deleteCardImageConfigurationFirst({
            waitForPendingPosts: function () { return postQueueIdle(); },
            resetPostError: function () { resetPostQueueError(); },
            clearReferences: function () { return clearCardImageReferences(id); },
            postsHadError: function () { return postQueueHadError(); },
            deleteImage: function () { return deleteCardImage(id); },
            rerender: function () {
                renderPreview();
                renderButtonSettings();
            },
        });
    }

    return {
        "_cardImageLibrary": liveGlobal(
            () => cardImagesFeature.cachedImages(),
            (value: unknown) => {
                cardImagesFeature.replaceCachedImages(Array.isArray(value) ? value as CardImageItem[] : []);
            },
        ),
        "_cardImageLibraryInfo": liveGlobal(
            () => cardImagesFeature.info(),
            () => { cardImagesFeature.invalidate(); },
        ),
        "CARD_IMAGE_TARGET_SIZE": liveGlobal(() => CARD_IMAGE_TARGET_SIZE, (value?: any) => { CARD_IMAGE_TARGET_SIZE = value; }),
        "CARD_IMAGE_UPLOAD_MAX_BYTES": liveGlobal(() => CARD_IMAGE_UPLOAD_MAX_BYTES, (value?: any) => { CARD_IMAGE_UPLOAD_MAX_BYTES = value; }),
        "CARD_IMAGE_MIN_QUALITY": liveGlobal(() => CARD_IMAGE_MIN_QUALITY, (value?: any) => { CARD_IMAGE_MIN_QUALITY = value; }),
        "cardImagesFeature": staticGlobal(cardImagesFeature),
        "cardImageBackupAssetProvider": staticGlobal(cardImageBackupAssetProvider),
        "listCardImages": staticGlobal(listCardImages),
        "cardImageLibraryInfo": staticGlobal(cardImageLibraryInfo),
        "countCardImageUsage": staticGlobal(countCardImageUsage),
        "resizeCardImageFile": staticGlobal(resizeCardImageFile),
        "uploadCardImage": staticGlobal(uploadCardImage),
        "renameCardImage": staticGlobal(renameCardImage),
        "deleteCardImage": staticGlobal(deleteCardImage),
        "deleteCardImageSafely": staticGlobal(deleteCardImageSafely),
    };
}
