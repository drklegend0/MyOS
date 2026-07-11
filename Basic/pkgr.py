#!/usr/bin/env python3
import json
import os
import sys
import tarfile
import zipfile
import urllib.request
import time
import shutil

# --- Configuration ---
GITHUB_OWNER = "drklegend0"
GITHUB_REPO = "OriginOS"
GITHUB_BRANCH = "main"
BASIC_SUBDIR = "Basic"
RAW_BASE = f"https://raw.githubusercontent.com/{GITHUB_OWNER}/{GITHUB_REPO}/{GITHUB_BRANCH}/{BASIC_SUBDIR}"
VERSIONS_URL = f"{RAW_BASE}/versions.json"
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
# Note: Keep snapshots one level up as per your setup
SNAPSHOTS_DIR = os.path.abspath(os.path.join(CURRENT_DIR, "../"))

# --- Progress Bar Helpers ---
def print_progress(current, total, prefix='Overall'):
    length = 30
    filled = int(length * current // total)
    bar = '█' * filled + '-' * (length - filled)
    sys.stdout.write(f'\r{prefix}: |{bar}| {current}/{total} packages')
    sys.stdout.flush()
    if current == total: print()

def download_file(url, label):
    with urllib.request.urlopen(url, timeout=30) as resp:
        total_size = int(resp.headers.get('Content-Length', 0))
        downloaded = 0
        data = b""
        chunk_size = 8192
        while True:
            chunk = resp.read(chunk_size)
            if not chunk: break
            data += chunk
            downloaded += len(chunk)
            if total_size > 0:
                percent = int(50 * downloaded / total_size)
                sys.stdout.write(f"\r  {label[:20]:<20} |{'█' * percent}{'-' * (50-percent)}|")
                sys.stdout.flush()
        sys.stdout.write(f"\r  {label[:20]:<20} |{'█' * 50}| Done!\n")
        return data

# --- Core Logic ---
def get_latest_version_entry():
    print(f"Fetching {VERSIONS_URL} ...")
    with urllib.request.urlopen(VERSIONS_URL) as r:
        data = json.loads(r.read())
        latest = data.get("latest")
        for entry in data.get("Versions", []):
            if entry.get("version") == latest: return entry
    raise RuntimeError("Version not found")

def make_snapshot_dir(version):
    os.makedirs(SNAPSHOTS_DIR, exist_ok=True)
    path = os.path.join(SNAPSHOTS_DIR, f"snapshot-{version}-{int(time.time())}")
    os.makedirs(path)
    return path

def process_package(entry, snapshot_dir):
    dest_path = os.path.join(snapshot_dir, entry["dest"])
    
    # Clean up if a directory exists where a file should be
    if os.path.exists(dest_path) and os.path.isdir(dest_path) and entry.get("type") == "binary":
        shutil.rmtree(dest_path)

    if "source" in entry: # fromRepo
        url = f"{RAW_BASE}/{entry['source']}"
        data = download_file(url, entry['source'])
        os.makedirs(os.path.dirname(dest_path) or ".", exist_ok=True)
        with open(dest_path, "wb") as f: f.write(data)
    else: # fromURL
        data = download_file(entry['url'], entry['dest'])
        os.makedirs(dest_path, exist_ok=True)
        tmp = dest_path + ".tmp"
        with open(tmp, "wb") as f: f.write(data)
        
        if entry['type'] == "binary":
            with open(dest_path, "wb") as f: f.write(data)
            os.chmod(dest_path, 0o755)
        elif entry['type'] == "tar.gz":
            with tarfile.open(tmp, "r:gz") as tar:
                members = tar.getmembers()
                for m in members:
                    parts = m.name.split("/", 1)
                    m.name = parts[1] if len(parts) == 2 else parts[0]
                tar.extractall(dest_path, members=[m for m in members if m.name])
        elif entry['type'] == "zip":
            with zipfile.ZipFile(tmp, "r") as z: z.extractall(dest_path)
        os.remove(tmp)

def main():
    try:
        entry = get_latest_version_entry()
        snapshot_dir = make_snapshot_dir(entry["version"])
        packages = entry.get("packages", {})
        all_pkgs = packages.get("fromRepo", []) + packages.get("fromURL", [])
        
        print(f"Building snapshot: {entry['version']}")
        for i, pkg in enumerate(all_pkgs):
            print_progress(i, len(all_pkgs))
            process_package(pkg, snapshot_dir)
        print_progress(len(all_pkgs), len(all_pkgs))
        print(f"Done! Snapshot: {snapshot_dir}")
    except Exception as e:
        print(f"\nError: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()