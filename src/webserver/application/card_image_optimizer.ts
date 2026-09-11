import type { CardImageOptimizationOptions, OptimizedCardImage } from "../features/card_images";

export const optimizeCardImage = (file: Blob, options: CardImageOptimizationOptions): Promise<OptimizedCardImage> => new Promise((resolve, reject) => {
    if (!file?.type || !file.type.startsWith("image/")) {
      reject(new Error("Choose an image file."));
      return;
    }
    const targetSize = options.targetSize;
    const maxBytes = options.maxBytes;
    const minQuality = options.minimumQuality;
    const source = new Image();
    const objectUrl = URL.createObjectURL(file);
    source.onload = (): void => {
      URL.revokeObjectURL(objectUrl);
      const canvas = document.createElement("canvas");
      canvas.width = targetSize;
      canvas.height = targetSize;
      const context = canvas.getContext("2d");
      if (!context) {
        reject(new Error("Could not optimize that image."));
        return;
      }
      context.imageSmoothingEnabled = true;
      context.imageSmoothingQuality = "high";
      const scale = Math.max(targetSize / source.naturalWidth, targetSize / source.naturalHeight);
      const width = source.naturalWidth * scale;
      const height = source.naturalHeight * scale;
      context.drawImage(source, (targetSize - width) / 2, (targetSize - height) / 2, width, height);
      const finish = (blob: Blob | null, quality: number): void => {
        if (!blob || blob.size > maxBytes) {
          reject(new Error("Image is still too large after browser optimization."));
          return;
        }
        resolve(Object.assign(blob, {
          optimizedWidth: targetSize,
          optimizedHeight: targetSize,
          optimizedQuality: quality,
        }));
      };
      const encode = (quality: number): void => {
        if (canvas.toBlob) {
          canvas.toBlob((blob) => {
            if (blob && blob.size > maxBytes && quality > minQuality) {
              encode(Math.max(minQuality, quality - 0.08));
            } else {
              finish(blob, quality);
            }
          }, "image/jpeg", quality);
          return;
        }
        const data = canvas.toDataURL("image/jpeg", quality);
        const raw = atob(data.substring(data.indexOf(",") + 1));
        const bytes = new Uint8Array(raw.length);
        for (let index = 0; index < raw.length; index++) bytes[index] = raw.charCodeAt(index);
        const blob = new Blob([bytes], { type: "image/jpeg" });
        if (blob.size > maxBytes && quality > minQuality) {
          encode(Math.max(minQuality, quality - 0.08));
        } else {
          finish(blob, quality);
        }
      };
      encode(0.78);
    };
    source.onerror = (): void => {
      URL.revokeObjectURL(objectUrl);
      reject(new Error("Could not read that image."));
    };
    source.src = objectUrl;
  });

