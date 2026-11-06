import type { CardImageItem } from "../features/card_images";
import type { CardImageService } from "./card_image_service";
import { createCardImageStorageBanner, syncCardImageStorageBanner } from "./card_image_storage_banner";
import { cardImageUrl } from "./config_option_core";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { ControlsShellFeature } from "./controls_shell";

export interface SettingsImageLibraryFeature {
  buildCard(): HTMLElement;
}

export function createSettingsImageLibraryFeature(
  images: CardImageService,
  shell: Pick<ControlsShellFeature, "createActionButton" | "showBanner" | "isConfigLocked">,
  fields: Pick<ControlsFieldsFeature, "makeCollapsibleCard">,
): SettingsImageLibraryFeature {
  return {
    buildCard() {
      const body = document.createElement("div");
      body.className = "sp-image-library";
      const storage = createCardImageStorageBanner();
      const toolbar = document.createElement("div");
      toolbar.className = "sp-image-library-toolbar";
      const file = document.createElement("input");
      file.type = "file";
      file.accept = "image/*";
      file.hidden = true;
      const upload = shell.createActionButton("sp-action-btn sp-primary-btn", "Upload", "plus");
      const retry = shell.createActionButton("sp-action-btn sp-secondary-btn", "Retry", "restore");
      toolbar.append(file, upload, retry);
      const status = document.createElement("div");
      status.className = "sp-image-library-status";
      status.setAttribute("role", "status");
      const list = document.createElement("div");
      list.className = "sp-image-library-list";
      body.append(storage, toolbar, status, list);
      const card: HTMLElement = fields.makeCollapsibleCard("Image Library", body, true);
      card.id = "sp-image-library";
      let busy = false;
      let loading = false;
      let loadFailed = false;
      let imageActions: HTMLButtonElement[] = [];

      const canEdit = () => images.info().available && !shell.isConfigLocked();
      function syncActions() {
        const hidden = !canEdit() || busy || loading || loadFailed;
        upload.hidden = hidden;
        file.disabled = hidden;
        for (const action of imageActions) action.hidden = hidden;
        retry.hidden = !loadFailed || busy || loading;
        toolbar.hidden = upload.hidden && retry.hidden;
        body.setAttribute("aria-busy", String(busy || loading));
      }

      function render(items: readonly CardImageItem[]) {
        list.replaceChildren();
        imageActions = [];
        const info = images.info();
        syncCardImageStorageBanner(storage, info);
        status.hidden = info.requiresUsbFlash;
        status.textContent = !info.available ? "Image storage is unavailable."
          : items.length ? `${items.length} image${items.length === 1 ? "" : "s"}` : "No images uploaded yet.";
        for (const image of items) {
          const row = document.createElement("div");
          row.className = "sp-image-library-item";
          row.setAttribute("role", "group");
          row.setAttribute("aria-label", image.name || image.id);
          const thumbnail = document.createElement("img");
          thumbnail.src = cardImageUrl(image.id);
          thumbnail.alt = "";
          thumbnail.loading = "lazy";
          const details = document.createElement("div");
          details.className = "sp-image-library-details";
          const name = document.createElement("div");
          name.className = "sp-image-library-name";
          name.textContent = image.name || image.id;
          name.title = name.textContent;
          const usage = document.createElement("div");
          usage.className = "sp-image-library-usage";
          const count = images.countUsage(image.id);
          usage.textContent = count ? `Used by ${count} card${count === 1 ? "" : "s"}` : "Not used by any cards";
          details.append(name, usage);
          const actions = document.createElement("div");
          actions.className = "sp-image-library-actions";
          const rename = shell.createActionButton("sp-action-btn sp-secondary-btn", "Rename", "pencil");
          rename.addEventListener("click", () => {
            if (!canEdit() || busy || loading) return;
            const value = window.prompt("Image name", image.name || image.id);
            if (value == null || !value.trim() || value.trim() === image.name) return;
            void change("Renaming image…", "Image renamed.", () => images.rename(image.id, value.trim()));
          });
          const remove = shell.createActionButton("sp-action-btn sp-secondary-btn", "", "trash-can-outline", "Delete image");
          remove.addEventListener("click", () => {
            if (!canEdit() || busy || loading) return;
            const used = images.countUsage(image.id);
            const message = used
              ? `This image is used by ${used} card${used === 1 ? "" : "s"}. Delete it and clear those card backgrounds?`
              : `Delete ${image.name || "this image"}?`;
            if (!window.confirm(message)) return;
            void change("Deleting image…", "Image deleted.", () => images.deleteSafely(image.id));
          });
          imageActions.push(rename, remove);
          actions.append(rename, remove);
          row.append(thumbnail, details, actions);
          list.appendChild(row);
        }
      }

      async function refresh() {
        if (loading) return;
        loading = true;
        loadFailed = false;
        status.hidden = false;
        status.textContent = "Loading images…";
        syncActions();
        try {
          render(await images.list(true));
        } catch (error) {
          syncCardImageStorageBanner(storage, images.info());
          loadFailed = !images.info().requiresUsbFlash;
          status.hidden = !loadFailed;
          status.textContent = error instanceof Error ? error.message : "Could not load images.";
        } finally {
          loading = false;
          syncActions();
        }
      }

      async function change(progress: string, success: string, operation: () => Promise<unknown>) {
        if (!canEdit() || busy || loading) return;
        busy = true;
        status.hidden = false;
        status.textContent = progress;
        syncActions();
        try {
          await operation();
          shell.showBanner(success, "success");
          await refresh();
        } catch (error) {
          const message = error instanceof Error ? error.message : "Could not update the image library.";
          status.textContent = message;
          shell.showBanner(message, "error");
        } finally {
          busy = false;
          file.value = "";
          syncActions();
        }
      }

      upload.addEventListener("click", () => file.click());
      file.addEventListener("change", () => {
        const selected = file.files?.[0];
        if (selected) void change("Uploading image…", "Image uploaded.", () => images.upload(selected));
      });
      retry.addEventListener("click", () => { void refresh(); });
      card.querySelector(".card-header")!.addEventListener("click", () => {
        if (!card.classList.contains("collapsed") && !busy) void refresh();
      });
      syncActions();
      return card;
    },
  };
}
