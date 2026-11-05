"""Small owned fixtures for the offline city exporter. Requires NumPy, like Blender."""
import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from city_materials import extract_tiles, sample_mirrored, padded_extent, padded_patch, pack_regions, pack_pages, split_triangles


class CityMaterialTests(unittest.TestCase):
    def test_panel_dividers_never_reach_material_samples(self):
        # Six distinct interiors with two-pixel saturated divider/shadow bands.
        sheet = np.zeros((256, 384, 4), dtype=np.float32)
        sheet[:] = (1, 0, 1, 1)
        colors = [(i / 8, .4, .6, 1) for i in range(6)]
        for i, color in enumerate(colors):
            x, y = i % 3 * 128, i // 3 * 128
            sheet[y + 2:y + 126, x + 2:x + 126] = color
        original = sheet.copy()
        for tile, color in zip(extract_tiles(sheet), colors):
            # Both positive and negative mirrored repetitions, crossing many joins.
            sampled = sample_mirrored(tile, 169, 117, 32, (-71, -19))
            np.testing.assert_allclose(sampled, np.broadcast_to(color, sampled.shape), atol=1e-7)
        np.testing.assert_array_equal(sheet, original)

    def test_fragments_share_field_and_repeats_meet(self):
        y, x = np.mgrid[:31, :29]
        tile = np.stack((x / 29, y / 31, (x + y) / 60, np.ones_like(x)), axis=2)
        whole = sample_mirrored(tile, 128, 128, 32, (-64, -64))
        for join in (32, 64, 96):
            np.testing.assert_array_equal(whole[:, join - 1], whole[:, join])
            np.testing.assert_array_equal(whole[join - 1], whole[join])
        fragment = sample_mirrored(tile, 27, 19, 32, (-21, 5))
        np.testing.assert_array_equal(fragment, whole[69:88, 43:70])

    def test_aligned_gutters_isolate_small_mips(self):
        # Odd-sized region next to a strongly contrasting one, with black unused area.
        gutter, w, h = 16, 37, 29
        red, green = (1, 0, 0, 1), (0, 1, 0, 1)
        atlas = np.zeros((128, 256, 4), dtype=np.float32)
        slot_w, slot_h = padded_extent(w, gutter), padded_extent(h, gutter)
        for slot, color in enumerate((red, green)):
            patch = np.broadcast_to(color, (h, w, 4)).copy()
            atlas[:slot_h, slot * slot_w:(slot + 1) * slot_w] = padded_patch(patch, gutter)
        for level in range(5):
            scale = 2 ** level
            for slot, color in enumerate((red, green)):
                xs = (slot * slot_w + gutter + np.linspace(0, w, 21)) // scale
                ys = (gutter + np.linspace(0, h, 21)) // scale
                samples = atlas[ys.astype(int)[:, None], xs.astype(int)[None, :]]
                np.testing.assert_array_equal(samples, np.broadcast_to(color, samples.shape))
            ah, aw = atlas.shape[:2]
            atlas = atlas.reshape(ah // 2, 2, aw // 2, 2, 4).mean(axis=(1, 3))

    def test_invalid_sheet_and_alignment_reject(self):
        for shape in ((10, 15, 4), (255, 384, 4), (256, 385, 4)):
            with self.assertRaises(ValueError):
                extract_tiles(np.zeros(shape))
        with self.assertRaises(ValueError):
            padded_extent(37, 3)

    def test_packing_reuses_gaps_without_overlap(self):
        # The small third region must reuse a gap left by the first tall region.
        dims = [(28, 20), (36, 12), (4, 4)]
        gutter, size = 2, 40
        positions = pack_regions(dims, size, gutter)
        self.assertIsNotNone(positions)
        occupied = np.zeros((size, size), dtype=bool)
        for (w, h), (x, y) in zip(dims, positions):
            x, y = x - gutter, y - gutter
            pw, ph = padded_extent(w, gutter), padded_extent(h, gutter)
            self.assertLessEqual(x + pw, size)
            self.assertLessEqual(y + ph, size)
            self.assertFalse(occupied[y:y + ph, x:x + pw].any())
            occupied[y:y + ph, x:x + pw] = True
        self.assertIsNone(pack_regions([(41, 1)], size, gutter))
        self.assertIsNone(pack_regions([(36, 20), (36, 20)], size, gutter))

    def test_bounded_pages_and_triangle_parts(self):
        dims = [(20, 20), (20, 20), (4, 4)]
        pages = pack_pages(dims, 32, 2)
        self.assertEqual(pages, pack_pages(dims, 32, 2))
        self.assertEqual({p[0] for p in pages}, {0, 1})
        occupied = np.zeros((2, 32, 32), dtype=bool)
        for (w, h), (page, x, y) in zip(dims, pages):
            area = occupied[page, y-2:y-2+padded_extent(h, 2), x-2:x-2+padded_extent(w, 2)]
            self.assertEqual(area.shape, (padded_extent(h, 2), padded_extent(w, 2)))
            self.assertFalse(area.any())
            area[:] = True
        for dimensions in ([(24, 24)] * 3, [(33, 1)], [(1, 33)], [(0, 1)]):
            with self.assertRaises(ValueError):
                pack_pages(dimensions, 32, 2)
        vertices = list(range(6003))
        parts = split_triangles(vertices)
        self.assertEqual([len(p) for p in parts], [6000, 3])
        self.assertEqual([v for p in parts for v in p], vertices)
        self.assertEqual(len(split_triangles(list(range(24000)))), 4)
        for count in (4, 24003):
            with self.assertRaises(ValueError):
                split_triangles(list(range(count)))
        with self.assertRaises(ValueError):
            split_triangles(vertices, capacity=1)


if __name__ == "__main__":
    unittest.main()
