#!/usr/bin/env node
// Fit a UI screenshot to the transparent screen in the supplied device template.
// Requires sharp. Usage: node compose-screen.cjs template.webp screen.png out.webp
const assert = require('node:assert/strict');
const sharp = require('sharp');

function solve(rows) {
  const n = rows.length;
  for (let col = 0; col < n; col++) {
    let pivot = col;
    for (let row = col + 1; row < n; row++) {
      if (Math.abs(rows[row][col]) > Math.abs(rows[pivot][col])) pivot = row;
    }
    assert(Math.abs(rows[pivot][col]) > 1e-12, 'Screen corners must define a plane');
    [rows[col], rows[pivot]] = [rows[pivot], rows[col]];
    const divisor = rows[col][col];
    for (let j = col; j <= n; j++) rows[col][j] /= divisor;
    for (let row = 0; row < n; row++) {
      if (row === col) continue;
      const factor = rows[row][col];
      for (let j = col; j <= n; j++) rows[row][j] -= factor * rows[col][j];
    }
  }
  return rows.map(row => row[n]);
}

async function main() {
  const [templatePath, screenPath, outputPath] = process.argv.slice(2);
  assert(templatePath && screenPath && outputPath,
    'Usage: node compose-screen.cjs template.webp screen.png output.webp');
  const { data: photo, info } = await sharp(templatePath).ensureAlpha().raw()
    .toBuffer({ resolveWithObject: true });
  const { width, height } = info;
  assert.equal(info.channels, 4);
  const isScreen = p => photo[p * 4 + 3] < 255;
  // The template's screen contains the image centre. Flood fill avoids isolated
  // pixels in the surrounding photograph affecting corner detection.
  const start = Math.floor(height / 2) * width + Math.floor(width / 2);
  assert(isScreen(start), 'Template centre must lie inside the transparent screen');
  const mask = new Uint8Array(width * height);
  const pixels = new Int32Array(width * height);
  pixels[0] = start;
  mask[start] = 1;
  let count = 1;
  let minSum = Infinity, maxSum = -Infinity, minDiff = Infinity, maxDiff = -Infinity;
  let tl, tr, br, bl;
  for (let index = 0; index < count; index++) {
    const p = pixels[index], x = p % width, y = Math.floor(p / width);
    if (x + y < minSum) { minSum = x + y; tl = [x, y]; }
    if (x + y > maxSum) { maxSum = x + y; br = [x, y]; }
    if (x - y < minDiff) { minDiff = x - y; bl = [x, y]; }
    if (x - y > maxDiff) { maxDiff = x - y; tr = [x, y]; }
    const neighbours = [];
    if (x > 0) neighbours.push(p - 1);
    if (x + 1 < width) neighbours.push(p + 1);
    if (y > 0) neighbours.push(p - width);
    if (y + 1 < height) neighbours.push(p + width);
    for (const next of neighbours) {
      if (!mask[next] && isScreen(next)) {
        mask[next] = 1;
        pixels[count++] = next;
      }
    }
  }
  assert(count > width * height * 0.1, 'Expected a substantial transparent screen mask');
  const corners = [tl, tr, br, bl];
  const uv = [[0, 0], [1, 0], [1, 1], [0, 1]];
  const equations = [];
  corners.forEach(([x, y], index) => {
    const [u, v] = uv[index];
    equations.push([x, y, 1, 0, 0, 0, -u * x, -u * y, u]);
    equations.push([0, 0, 0, x, y, 1, -v * x, -v * y, v]);
  });
  const h = solve(equations);
  function map(x, y) {
    const denominator = h[6] * x + h[7] * y + 1;
    return [(h[0] * x + h[1] * y + h[2]) / denominator,
      (h[3] * x + h[4] * y + h[5]) / denominator];
  }
  corners.forEach(([x, y], index) => {
    const point = map(x, y);
    assert(Math.hypot(point[0] - uv[index][0], point[1] - uv[index][1]) < 1e-8,
      'Projected screen corners must match the template');
  });
  const metadata = await sharp(screenPath).metadata();
  // Remove the editor preview's four-pixel simulated enclosure outline.
  const { data: screen, info: screenInfo } = await sharp(screenPath)
    .extract({ left: 4, top: 4, width: metadata.width - 8, height: metadata.height - 8 })
    .removeAlpha().raw().toBuffer({ resolveWithObject: true });
  const sw = screenInfo.width, sh = screenInfo.height;
  const output = Buffer.alloc(width * height * 3);
  for (let p = 0; p < mask.length; p++) {
    for (let c = 0; c < 3; c++) output[p * 3 + c] = photo[p * 4 + c];
  }
  for (let index = 0; index < count; index++) {
    const p = pixels[index];
    const [u, v] = map(p % width, Math.floor(p / width));
    const sx = Math.max(0, Math.min(sw - 1, u * (sw - 1)));
    const sy = Math.max(0, Math.min(sh - 1, v * (sh - 1)));
    const x0 = Math.floor(sx), y0 = Math.floor(sy);
    const x1 = Math.min(sw - 1, x0 + 1), y1 = Math.min(sh - 1, y0 + 1);
    const dx = sx - x0, dy = sy - y0;
    for (let c = 0; c < 3; c++) {
      const at = (x, y) => screen[(y * sw + x) * 3 + c];
      const sample =
        at(x0, y0) * (1 - dx) * (1 - dy) + at(x1, y0) * dx * (1 - dy) +
        at(x0, y1) * (1 - dx) * dy + at(x1, y1) * dx * dy;
      const opacity = photo[p * 4 + 3] / 255;
      output[p * 3 + c] = Math.round(sample * (1 - opacity) + photo[p * 4 + c] * opacity);
    }
  }
  await sharp(output, { raw: { width, height, channels: 3 } })
    .webp({ lossless: true }).toFile(outputPath);
  const decoded = await sharp(outputPath).removeAlpha().raw().toBuffer();
  let preserved = 0;
  for (let p = 0; p < mask.length; p++) {
    if (mask[p]) continue;
    for (let c = 0; c < 3; c++) assert.equal(decoded[p * 3 + c], photo[p * 4 + c],
      'Photograph outside the screen must remain pixel-identical');
    preserved++;
  }
  console.log(JSON.stringify({ corners, screenPixels: count, preservedPhotoPixels: preserved, outputPath }));
}

main().catch(error => { console.error(error); process.exitCode = 1; });
