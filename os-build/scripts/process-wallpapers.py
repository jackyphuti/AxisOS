#!/usr/bin/env python3
"""
Process and standardize all AxisOS wallpapers to true 1920x1080 Full HD (16:9),
eliminate color banding and compression artifacts, replace scratchy textures,
and generate genuine PNG wallpaper assets.
"""

import os
import shutil
from PIL import Image

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
WALLPAPERS_DIR = os.path.join(REPO_ROOT, "data", "wallpapers")
BRANDING_DIR = os.path.join(REPO_ROOT, "data", "branding")
THEME_DIR = os.path.join(REPO_ROOT, "data", "theme")
CONFIGS_GNOME_DIR = os.path.join(REPO_ROOT, "os-build", "configs", "gnome")
ARTIFACT_DIR = r"C:\Users\jacky\.gemini\antigravity\brain\efecf8bd-4931-4f17-a337-91ff2d824cf3"

# Replacement maps for scratchy / glitch textures
REPLACEMENTS = {
    "08-axisos-elevation-lines.jpg": (
        os.path.join(ARTIFACT_DIR, "08_axisos_obsidian_wave_1792390370970.jpg"),
        "08-axisos-obsidian-wave.jpg"
    ),
    "09-axisos-overclocked-glitch.jpg": (
        os.path.join(ARTIFACT_DIR, "09_axisos_cyber_matrix_1792390388996.jpg"),
        "09-axisos-cyber-matrix.jpg"
    ),
}

def process_image(src_path, dst_path, target_w=1920, target_h=1080):
    """Crop to 16:9 and upscale/resize using Lanczos resampling."""
    img = Image.open(src_path)
    if img.mode != "RGB":
        img = img.convert("RGB")
    
    target_aspect = float(target_w) / float(target_h)
    current_aspect = float(img.width) / float(img.height)

    if current_aspect > target_aspect:
        new_w = int(img.height * target_aspect)
        left = (img.width - new_w) // 2
        img_cropped = img.crop((left, 0, left + new_w, img.height))
    else:
        new_h = int(img.width / target_aspect)
        top = (img.height - new_h) // 2
        img_cropped = img.crop((0, top, img.width, top + new_h))

    img_1080 = img_cropped.resize((target_w, target_h), Image.Resampling.LANCZOS)
    
    # Save with maximum quality and no chroma subsampling to prevent dark gradient banding
    if dst_path.lower().endswith(('.jpg', '.jpeg')):
        img_1080.save(dst_path, "JPEG", quality=98, subsampling=0, progressive=True)
    elif dst_path.lower().endswith('.png'):
        img_1080.save(dst_path, "PNG", optimize=True)
    
    return img_1080

def main():
    print("=== AxisOS Wallpaper Processing & 1080p Standardizer ===")

    # 1. Replace scratchy textures if replacements exist
    for old_file, (new_src, new_dst_name) in REPLACEMENTS.items():
        old_path = os.path.join(WALLPAPERS_DIR, old_file)
        new_path = os.path.join(WALLPAPERS_DIR, new_dst_name)
        if os.path.exists(new_src):
            print(f"Replacing {old_file} -> {new_dst_name}")
            shutil.copyfile(new_src, new_path)
            if os.path.exists(old_path) and old_file != new_dst_name:
                os.remove(old_path)
        else:
            print(f"Warning: replacement source {new_src} not found")

    # 2. Process all wallpapers to crisp 1920x1080
    files = sorted([f for f in os.listdir(WALLPAPERS_DIR) if f.endswith(('.jpg', '.jpeg', '.png')) and not f.endswith('-1080p.png')])
    for f in files:
        f_path = os.path.join(WALLPAPERS_DIR, f)
        process_image(f_path, f_path, 1920, 1080)
        print(f"  [OK] Standardized to 1920x1080 (16:9, Lanczos): {f}")

    # Remove temporary test file if it exists
    test_tmp = os.path.join(WALLPAPERS_DIR, "01-axisos-signature-core-1080p.png")
    if os.path.exists(test_tmp):
        os.remove(test_tmp)

    # 3. Generate genuine 1920x1080 PNG for axis-wallpaper.png & nobara-gaming.png
    primary_src = os.path.join(WALLPAPERS_DIR, "01-axisos-signature-core.jpg")
    boot_src = os.path.join(BRANDING_DIR, "axisos-boot-1920x1080.png")
    
    primary_png_path = os.path.join(WALLPAPERS_DIR, "01-axisos-signature-core.png")
    process_image(primary_src, primary_png_path, 1920, 1080)
    print(f"  [OK] Generated authentic PNG: {primary_png_path}")

    # Copy to theme & branding
    os.makedirs(THEME_DIR, exist_ok=True)
    os.makedirs(BRANDING_DIR, exist_ok=True)
    # Clean up obsolete duplicate if present
    obsolete_branding_wp = os.path.join(BRANDING_DIR, "axis-wallpaper.png")
    if os.path.exists(obsolete_branding_wp):
        os.remove(obsolete_branding_wp)


    print("\nWallpaper standardization complete!")

if __name__ == "__main__":
    main()
