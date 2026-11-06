import { CARD_IMAGE_STORAGE_GUIDE_URL, CARD_IMAGE_STORAGE_SETUP_MESSAGE } from "../features/card_image_storage";
import type { CardImageLibraryInfo } from "../features/card_images";

export function createCardImageStorageBanner(): HTMLElement {
  const banner = document.createElement("div");
  banner.className = "sp-info-panel sp-card-image-storage";
  banner.setAttribute("role", "status");
  banner.hidden = true;
  const icon = document.createElement("span");
  icon.className = "mdi mdi-information-outline";
  icon.setAttribute("aria-hidden", "true");
  const content = document.createElement("span");
  content.append(document.createTextNode(CARD_IMAGE_STORAGE_SETUP_MESSAGE + " "));
  const link = document.createElement("a");
  link.href = CARD_IMAGE_STORAGE_GUIDE_URL;
  link.target = "_blank";
  link.rel = "noopener noreferrer";
  link.textContent = "Image setup guide";
  content.appendChild(link);
  banner.append(icon, content);
  return banner;
}

export function syncCardImageStorageBanner(banner: HTMLElement, info: Pick<CardImageLibraryInfo, "requiresUsbFlash">): void {
  banner.hidden = !info.requiresUsbFlash;
}
