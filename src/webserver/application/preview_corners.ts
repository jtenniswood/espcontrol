// Match the firmware's fourth-power corner mask, including browsers that
// cannot draw CSS corner-shape. Coordinates stay in the card's own pixels.
export function continuousCornerPath(width: number, height: number, radius: number): string {
    if (![width, height, radius].every(Number.isFinite) || radius <= 0 || width < radius * 4 || height < radius * 4) return "";
    const extent = radius * 2;
    const point = (step: number): [number, number] => {
        const angle = step * Math.PI / 64;
        return [extent * (1 - Math.sqrt(Math.cos(angle))),
            extent * (1 - Math.sqrt(Math.sin(angle)))];
    };
    const coordinate = (value: number) => value.toFixed(3);
    let path = `M ${coordinate(extent)} 0 L ${coordinate(width - extent)} 0`;
    const line = (x: number, y: number) => { path += ` L ${coordinate(x)} ${coordinate(y)}`; };
    for (let i = 31; i >= 0; --i) { const [x, y] = point(i); line(width - x, y); }
    line(width, height - extent);
    for (let i = 1; i <= 32; ++i) { const [x, y] = point(i); line(width - x, height - y); }
    line(extent, height);
    for (let i = 31; i >= 0; --i) { const [x, y] = point(i); line(x, height - y); }
    line(0, extent);
    for (let i = 1; i <= 32; ++i) { const [x, y] = point(i); line(x, y); }
    return path + " Z";
}

interface PreviewCorners {
    radius: number;
    emptyRadius: number;
    enabled: boolean;
    refresh(): void;
}

const previews = new WeakMap<HTMLElement, PreviewCorners>();

export function syncPreviewCorners(screen: HTMLElement, radius: number, emptyRadius: number, enabled: boolean): void {
    let corners = previews.get(screen);
    if (!corners) {
        let cards = new Set<HTMLElement>();
        const cache = new Map<string, { clip: string; edge: string }>();
        const edges = new WeakMap<SVGSVGElement, string>();
        const refresh = () => {
            const next = new Set(screen.querySelectorAll<HTMLElement>(".sp-btn,.sp-empty-cell"));
            for (const card of cards) if (!next.has(card)) resize?.unobserve(card);
            for (const card of next) if (!cards.has(card)) resize?.observe(card);
            cards = next;
            const screenWidth = screen.clientWidth;
            for (const card of cards) {
                const style = getComputedStyle(card);
                const empty = card.classList.contains("sp-empty-cell");
                const nominal = (empty ? settings.emptyRadius : settings.radius) * screenWidth / 100;
                const borderBox = (size: string, extras: string[]) => parseFloat(size) +
                    (style.boxSizing === "border-box" ? 0 : extras.reduce((sum, value) => sum + parseFloat(value || "0"), 0));
                const width = borderBox(style.width, [style.paddingLeft, style.paddingRight, style.borderLeftWidth, style.borderRightWidth]);
                const height = borderBox(style.height, [style.paddingTop, style.paddingBottom, style.borderTopWidth, style.borderBottomWidth]);
                if (!(width > 0 && height > 0)) continue;
                const key = `${width}:${height}:${nominal}:${settings.enabled}:${empty}`;
                let shape = cache.get(key);
                if (shape === undefined) {
                    const path = settings.enabled ? continuousCornerPath(width, height, nominal) : "";
                    const edge = path ? `<path d="${path}"/>` : `<rect width="${width}" height="${height}" rx="${nominal}"/>`;
                    shape = { clip: path ? `path("${path}")` : "", edge };
                    if (cache.size >= 64) cache.clear();
                    cache.set(key, shape);
                }
                if (card.style.clipPath !== shape.clip) card.style.clipPath = shape.clip;
                let outline = card.querySelector<SVGSVGElement>(":scope > .sp-card-outline");
                if (!outline) {
                    outline = document.createElementNS("http://www.w3.org/2000/svg", "svg");
                    outline.classList.add("sp-card-outline");
                    outline.setAttribute("aria-hidden", "true");
                    outline.setAttribute("focusable", "false");
                    outline.setAttribute("preserveAspectRatio", "none");
                    card.appendChild(outline);
                }
                const viewBox = `0 0 ${width} ${height}`;
                if (outline.getAttribute("viewBox") !== viewBox) outline.setAttribute("viewBox", viewBox);
                // Half the centered 4px stroke is clipped by the same card edge,
                // leaving an even 2px inner outline above all card content.
                if (edges.get(outline) !== shape.edge) {
                    outline.innerHTML = shape.edge;
                    edges.set(outline, shape.edge);
                }
            }
        };
        const settings: PreviewCorners = { radius, emptyRadius, enabled, refresh };
        const resize = typeof ResizeObserver === "undefined" ? null : new ResizeObserver(refresh);
        resize?.observe(screen);
        new MutationObserver(refresh).observe(screen, { childList: true, subtree: true, attributes: true, attributeFilter: ["class"] });
        corners = settings;
        previews.set(screen, corners);
    }
    corners.radius = radius;
    corners.emptyRadius = emptyRadius;
    corners.enabled = enabled;
    corners.refresh();
}
