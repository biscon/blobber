"""Offline SC01.2 material extraction and atlas padding; no Blender dependency."""
import numpy as np


def extract_tiles(master):
    """Discard each generated panel's border, retaining its unmodified interior.

    The retained sheet has six nominally 512-square panels. Eight source pixels
    per edge exclude the generated divider and its shadow; scale proportionally
    for other sheet resolutions. Never filter neighboring panels into a tile.
    """
    h, w = master.shape[:2]
    if h % 2 or w % 3 or min(w // 3, h // 2) < 16:
        raise ValueError("Expected a 3x2 sheet with sufficiently large equal panels")
    tiles = []
    for i in range(6):
        x0, x1 = (i % 3) * w // 3, (i % 3 + 1) * w // 3
        y0, y1 = (i // 3) * h // 2, (i // 3 + 1) * h // 2
        inset = max(1, min(x1 - x0, y1 - y0) // 64)
        tiles.append(master[y0 + inset:y1 - inset, x0 + inset:x1 - inset].copy())
    return tiles


def sample_mirrored(src, width, height, period, offset=(0, 0)):
    """Sample one continuous physical field, including negative UV coordinates."""
    sh, sw = src.shape[:2]
    xx = np.floor((np.arange(width) + offset[0] + .5) / period * sw).astype(int)
    yy = np.floor((np.arange(height) + offset[1] + .5) / period * sh).astype(int)
    xx = np.where((xx // sw) % 2, sw - 1 - xx % sw, xx % sw)
    yy = np.where((yy // sh) % 2, sh - 1 - yy % sh, yy % sh)
    return src[yy[:, None], xx[None, :]].copy()


def padded_extent(length, gutter):
    if gutter < 1 or gutter & (gutter - 1):
        raise ValueError("Gutter alignment must be a power of two")
    return (length + 2 * gutter + gutter - 1) // gutter * gutter


def padded_patch(patch, gutter):
    """Fill the entire aligned slot; no black slivers between padding and slot end."""
    h, w = patch.shape[:2]
    return np.pad(patch, ((gutter, padded_extent(h, gutter) - h - gutter),
                         (gutter, padded_extent(w, gutter) - w - gutter), (0, 0)), mode="edge")


def pack_regions(dimensions, size, gutter):
    """First-fit decreasing shelves reuse earlier gaps; bounded offline input."""
    slots = [(padded_extent(w, gutter), padded_extent(h, gutter)) for w, h in dimensions]
    result = [None] * len(slots)
    rows, bottom = [], 0
    for i in sorted(range(len(slots)), key=lambda i: (-slots[i][1], -slots[i][0], i)):
        w, h = slots[i]
        if w > size:
            return None
        row = next((r for r in rows if r[0] + w <= size and r[2] >= h), None)
        if row is None:
            if bottom + h > size:
                return None
            row = [0, bottom, h]
            rows.append(row)
            bottom += h
        result[i] = (row[0] + gutter, row[1] + gutter)
        row[0] += w
    return result


def pack_pages(dimensions, size, gutter, capacity=2):
    """Bounded deterministic atlas shelves; never shrink metric regions to fit."""
    if capacity < 1:
        raise ValueError("Atlas capacity must be positive")
    slots = [(padded_extent(w, gutter), padded_extent(h, gutter)) for w, h in dimensions]
    pages = []
    result = [None] * len(slots)
    for i in sorted(range(len(slots)), key=lambda i: (-slots[i][1], -slots[i][0], i)):
        w, h = slots[i]
        if w > size or h > size or min(dimensions[i]) <= 0:
            raise ValueError("Material region exceeds atlas dimensions")
        for page_index in range(capacity):
            if page_index == len(pages):
                pages.append([[], 0])
            page = pages[page_index]
            row = next((r for r in page[0] if r[0] + w <= size and r[2] >= h), None)
            if row is None and page[1] + h <= size:
                row = [0, page[1], h]
                page[0].append(row)
                page[1] += h
            if row is not None:
                result[i] = (page_index, row[0] + gutter, row[1] + gutter)
                row[0] += w
                break
        if result[i] is None:
            raise ValueError("Material atlas capacity exceeded")
    return result


def split_triangles(vertices, limit=6000, capacity=8):
    """Split triangle-list payloads on triangle boundaries, rejecting oversized models."""
    if len(vertices) % 3 or limit < 3 or limit % 3 or len(vertices) > 24000:
        raise ValueError("Invalid architecture triangle list")
    parts = [vertices[i:i + limit] for i in range(0, len(vertices), limit)]
    if len(parts) > capacity:
        raise ValueError("Model part capacity exceeded")
    return parts
