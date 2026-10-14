import { CARD_SIZE_DEFINITIONS, cardSizeClass, sizeColSpan, sizeRowSpan } from "../model/grid";

interface ResizeTarget { size: number; width: number; height: number }
interface PreviewResizeDependencies {
    document: Document;
    isConfigLocked(): boolean;
    targets(slot: number, card: HTMLElement): ResizeTarget[];
    commit(slot: number, size: number): void;
}

/** Keep the rendered card alive during the gesture; save and rerender on release. */
export function installPreviewResize(dependencies: PreviewResizeDependencies): void {
    const { document } = dependencies;
    let hoveredCard: HTMLElement | null = null;
    let hoveredDraggable = false;
    const restoreHover = () => {
        if (hoveredCard) hoveredCard.draggable = hoveredDraggable && !dependencies.isConfigLocked();
        hoveredCard = null;
    };
    const handleAt = (event: MouseEvent): HTMLElement | null => {
        const element = event.target as Element;
        const card = element.closest<HTMLElement>(".sp-btn[data-slot]");
        const handle = card?.querySelector<HTMLElement>(".sp-resize-handle");
        if (!handle) return null;
        const rect = handle.getBoundingClientRect();
        // Include the visible corner even when an overlapping card decoration receives the event.
        return event.clientX >= rect.left - 6 && event.clientX <= rect.right &&
            event.clientY >= rect.top - 6 && event.clientY <= rect.bottom ? handle : null;
    };
    const hover = (event: PointerEvent) => {
        if (document.querySelector(".sp-resizing")) return;
        const card = handleAt(event)?.closest<HTMLElement>(".sp-btn[data-slot]") || null;
        if (card === hoveredCard) return;
        restoreHover();
        if (card && !dependencies.isConfigLocked()) {
            hoveredCard = card;
            hoveredDraggable = card.draggable;
            card.draggable = false;
        }
    };
    document.addEventListener("pointerover", hover, true);
    document.addEventListener("pointermove", hover, true);
    document.addEventListener("pointerout", (event) => {
        if (hoveredCard && !hoveredCard.classList.contains("sp-resizing") &&
            !hoveredCard.contains(event.relatedTarget as Node | null)) restoreHover();
    }, true);
    document.addEventListener("mousedown", (event) => {
        if (handleAt(event) || document.querySelector(".sp-resizing")) {
            event.preventDefault();
            event.stopImmediatePropagation();
        }
    }, true);
    document.addEventListener("dragstart", (event) => {
        if ((event.target as Element).closest(".sp-resize-handle,.sp-resizing") ||
            (hoveredCard && hoveredCard.contains(event.target as Node))) {
            event.preventDefault();
            event.stopImmediatePropagation();
        }
    }, true);
    document.addEventListener("click", (event) => {
        if ((event.target as Element).closest(".sp-resize-handle")) {
            event.preventDefault();
            event.stopImmediatePropagation();
        }
    }, true);
    document.addEventListener("pointerdown", (event) => {
        const handle = handleAt(event);
        const card = handle?.closest<HTMLElement>(".sp-btn[data-slot]");
        if (!handle || !card || event.button !== 0 || !event.isPrimary) return;
        event.preventDefault();
        event.stopImmediatePropagation();
        if (dependencies.isConfigLocked() || document.querySelector(".sp-resizing")) return;
        const slot = Number(card.dataset.slot);
        const targets = dependencies.targets(slot, card);
        if (!targets.length) return;
        const rect = card.getBoundingClientRect();
        let target = targets.reduce((best, item) => distance(item, rect.width, rect.height) < distance(best, rect.width, rect.height) ? item : best);
        const previousStyle = card.getAttribute("style");
        const previousDraggable = hoveredCard === card ? hoveredDraggable : card.draggable;
        const previousSizeClasses = CARD_SIZE_DEFINITIONS.map(item => item.className).filter(Boolean);
        const originalSizeClasses = previousSizeClasses.filter(name => card.classList.contains(name));
        const grid = card.parentElement!;
        const gridStyle = document.defaultView!.getComputedStyle(grid);
        const previousRows = grid.style.gridTemplateRows;
        const previousColumns = grid.style.gridTemplateColumns;
        const cardStyle = document.defaultView!.getComputedStyle(card);
        // Fractional tracks otherwise grow to accommodate the preview's minimum size.
        grid.style.gridTemplateRows = gridStyle.gridTemplateRows;
        grid.style.gridTemplateColumns = gridStyle.gridTemplateColumns;
        // Keep the original grid footprint until release while applying size-specific visuals.
        card.style.gridRow = cardStyle.gridRow;
        card.style.gridColumn = cardStyle.gridColumn;
        card.draggable = false;
        let finished = false;
        card.classList.add("sp-resizing");
        handle.setPointerCapture(event.pointerId);
        const move = (next: PointerEvent) => {
            if (next.pointerId !== event.pointerId) return;
            if (!card.isConnected || dependencies.isConfigLocked()) { finish(false); return; }
            const width = rect.width + next.clientX - event.clientX;
            const height = rect.height + next.clientY - event.clientY;
            target = targets.reduce((best, item) => distance(item, width, height) < distance(best, width, height) ? item : best);
            card.classList.remove(...previousSizeClasses);
            const sizeClass = cardSizeClass(target.size);
            if (sizeClass) card.classList.add(sizeClass);
            card.style.width = target.width + "px";
            card.style.height = target.height + "px";
        };
        const end = (next: PointerEvent) => {
            if (next.pointerId !== event.pointerId) return;
            if (next.type === "pointerup") move(next);
            finish(next.type === "pointerup");
        };
        const key = (next: KeyboardEvent) => {
            if (next.key === "Escape") { next.preventDefault(); finish(false); }
        };
        const finish = (commit: boolean) => {
            if (finished) return;
            finished = true;
            document.removeEventListener("pointermove", move, true);
            document.removeEventListener("pointerup", end, true);
            document.removeEventListener("pointercancel", end, true);
            document.removeEventListener("keydown", key, true);
            handle.removeEventListener("lostpointercapture", cancel);
            if (handle.hasPointerCapture(event.pointerId)) handle.releasePointerCapture(event.pointerId);
            card.classList.remove("sp-resizing");
            card.classList.remove(...previousSizeClasses);
            card.classList.add(...originalSizeClasses);
            card.draggable = previousDraggable;
            if (hoveredCard === card) restoreHover();
            grid.style.gridTemplateRows = previousRows;
            grid.style.gridTemplateColumns = previousColumns;
            if (previousStyle === null) card.removeAttribute("style");
            else card.setAttribute("style", previousStyle);
            if (commit && card.isConnected && !dependencies.isConfigLocked()) dependencies.commit(slot, target.size);
        };
        const cancel = () => finish(false);
        document.addEventListener("pointermove", move, true);
        document.addEventListener("pointerup", end, true);
        document.addEventListener("pointercancel", end, true);
        document.addEventListener("keydown", key, true);
        handle.addEventListener("lostpointercapture", cancel);
    }, true);
}

function distance(target: ResizeTarget, width: number, height: number): number {
    return (target.width - width) ** 2 + (target.height - height) ** 2;
}

export function resizeTargetDimensions(size: number, cellWidth: number, cellHeight: number, gap: number): ResizeTarget {
    const cols = sizeColSpan(size);
    const rows = sizeRowSpan(size);
    return { size, width: cols * cellWidth + (cols - 1) * gap, height: rows * cellHeight + (rows - 1) * gap };
}
