import { state } from "../state/app_instance";
import type { ApplicationLayoutState } from "./application_context";
import type { ApplicationApiFeature } from "./api";
import type { ConfigPersistenceFeature } from "./config_post_api";
import {
  createCardImageBackupAssetProvider,
  createCardImagesFeature,
  deleteCardImageConfigurationFirst,
  deleteCardImageWithDeviceTransaction,
  type CardImageHttpRequest,
} from "../features/card_images";
import {
  cardBackgroundImage,
  cardImageUrl,
  normalizeCardBackgroundImageId,
  setCardBackgroundImage,
} from "./config_option_core";

export interface CardImageServiceDependencies {
  readonly layout: ApplicationLayoutState;
  readonly requestApi: Pick<ApplicationApiFeature,
    "postQueueIdle" | "resetPostQueueError" | "postQueueHadError">;
  readonly persistence: Pick<ConfigPersistenceFeature, "clearCardImageReferences">;
  fetch(url: string, request?: RequestInit): Promise<Response>;
  renderPreview(): void;
  renderButtonSettings(): void;
}

export function createCardImageService(dependencies: CardImageServiceDependencies) {
  const targetSize = 200;
  const uploadMaxBytes = 45 * 1024;
  const minimumQuality = 0.42;
  const maxActiveBackgrounds = Math.max(
    0,
    Number.parseInt(String(dependencies.layout.config.cardBackgroundImageLimit || dependencies.layout.numSlots), 10) || 0,
  );
  const cardImages = createCardImagesFeature({
    maxActiveBackgrounds,
    fetch: (url: string, request?: CardImageHttpRequest) =>
      dependencies.fetch(url, request as RequestInit),
    normalizeId: normalizeCardBackgroundImageId,
    imageUrl: cardImageUrl,
    targetSize: () => targetSize,
    uploadMaxBytes: () => uploadMaxBytes,
    minimumQuality: () => minimumQuality,
  });
  const backupAssetProvider = createCardImageBackupAssetProvider(cardImages, {
    normalizeId: normalizeCardBackgroundImageId,
    imageId: cardBackgroundImage,
    setImageId: (button, id) => { setCardBackgroundImage(button, id); },
  });

  const countUsage = (value: unknown): number => {
    const id = normalizeCardBackgroundImageId(value);
    if (!id) return 0;
    let count = 0;
    const countButtons = (buttons: readonly any[] = []) => {
      buttons.forEach((button) => { if (cardBackgroundImage(button) === id) count++; });
    };
    countButtons(state.buttons);
    Object.values(state.subpages || {}).forEach((subpage: any) => countButtons(subpage?.buttons));
    return count;
  };

  const rerender = () => {
    dependencies.renderPreview();
    dependencies.renderButtonSettings();
  };

  const deleteSafely = async (value: unknown): Promise<boolean> => {
    const id = normalizeCardBackgroundImageId(value);
    if (!id) throw new Error("Invalid card image ID.");
    await cardImages.list(true);
    if (cardImages.info().referenceTransactions) {
      return deleteCardImageWithDeviceTransaction({
        waitForPendingPosts: dependencies.requestApi.postQueueIdle,
        resetPostError: dependencies.requestApi.resetPostQueueError,
        deleteImage: () => cardImages.delete(id),
        clearLocalReferences: () => { dependencies.persistence.clearCardImageReferences(id, false); },
        rerender,
      });
    }
    return deleteCardImageConfigurationFirst({
      waitForPendingPosts: dependencies.requestApi.postQueueIdle,
      resetPostError: dependencies.requestApi.resetPostQueueError,
      clearReferences: () => dependencies.persistence.clearCardImageReferences(id),
      postsHadError: dependencies.requestApi.postQueueHadError,
      deleteImage: () => cardImages.delete(id),
      rerender,
    });
  };

  return {
    feature: cardImages,
    backupAssetProvider,
    maxActiveBackgrounds,
    list: (force?: boolean) => cardImages.list(!!force),
    info: () => cardImages.info(),
    countUsage,
    resize: (file: Blob) => cardImages.resize(file),
    upload: (file: Blob) => cardImages.upload(file),
    rename: (id: unknown, name: unknown) => cardImages.rename(id, name),
    delete: (id: unknown) => cardImages.delete(id),
    deleteSafely,
  };
}

export type CardImageService = ReturnType<typeof createCardImageService>;
