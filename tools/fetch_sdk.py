#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""The OpenRAC mod SDK that sdk.lock pins, into .sdk/.

    python3 tools/fetch_sdk.py [--from ZIP]

downloads sdk.lock's url (or takes a local archive with --from), checks it against sdk.lock's
sha256, and unpacks it as .sdk/openrac-mod-sdk-<version>/, which CMakeLists.txt finds. An SDK
already there of that version and hash is kept. Prints the SDK's folder. Stdlib only.

A lock with no sha256 yet (a repository made from the SDK's template, whose own archive cannot name
its hash) takes the hash the release publishes beside the archive (<url>.sha256) and writes it into
sdk.lock: commit it, and every later build is held to it. This file is the SDK's
(template/tools/fetch_sdk.py); copy it again when it changes.

This mod's copy also takes an SDK that has no release yet: a lock with "git" (OpenRAC's repository)
and "commit" (a full commit id) instead of "url" and "sha256" takes the SDK's source folder, sdk/,
from that commit (that commit alone, shallow, a sparse checkout of sdk/, with git), checks that its sdk/VERSION is the lock's version,
and puts it where the archive would go. Back to a released SDK with a lock of url and sha256.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


# Attempts at the fetch (a network that drops it now and then), and the wait between them.
FETCH_ATTEMPTS = 3
FETCH_RETRY_SECONDS = 10


def fetch_commit(repo: str, commit: str, src: Path) -> bool:
    """sdk/ of the one commit of repo into src: an empty repository, the sparse checkout of sdk/ set
    before anything is fetched, then that commit alone (depth 1, its blobs as the checkout needs
    them), not the repository's history. GitHub serves a commit by its id. Tries FETCH_ATTEMPTS
    times."""
    git = ["git", "-c", "advice.detachedHead=false", "-C", str(src)]
    setup = (
        ["git", "init", "--quiet", str(src)],
        [*git, "remote", "add", "origin", repo],
        [*git, "sparse-checkout", "set", "sdk"],
    )
    for step in setup:
        if subprocess.run(step, stdout=sys.stderr).returncode != 0:
            print(f"fetch_sdk: {' '.join(step)} failed", file=sys.stderr)
            return False
    fetch = [*git, "fetch", "--quiet", "--depth", "1", "--filter=blob:none", "origin", commit]
    for attempt in range(1, FETCH_ATTEMPTS + 1):
        if subprocess.run(fetch, stdout=sys.stderr).returncode == 0:
            break
        print(f"fetch_sdk: fetching {commit[:9]} failed (attempt {attempt} of {FETCH_ATTEMPTS})", file=sys.stderr)
        if attempt == FETCH_ATTEMPTS:
            return False
        time.sleep(FETCH_RETRY_SECONDS)
    checkout = [*git, "checkout", "--quiet", "FETCH_HEAD"]
    if subprocess.run(checkout, stdout=sys.stderr).returncode != 0:
        print("fetch_sdk: checking out the fetched commit failed", file=sys.stderr)
        return False
    return True


def from_git(lock: dict) -> int:
    """The SDK of lock's commit, into .sdk/openrac-mod-sdk-<version>/; prints its folder."""
    version, repo, commit = lock["version"], lock["git"], lock["commit"].lower()
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        print("fetch_sdk: sdk.lock's commit is not a full commit id", file=sys.stderr)
        return 1
    top = f"openrac-mod-sdk-{version}"
    dest = ROOT / ".sdk"
    stamp = dest / f"{top}.commit"
    if (dest / top / "cmake").is_dir() and stamp.is_file() and stamp.read_text().strip() == commit:
        print(dest / top)
        return 0
    dest.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=dest) as tmp:
        src = Path(tmp) / "openrac"
        if not fetch_commit(repo, commit, src):
            return 1
        got = (src / "sdk" / "VERSION").read_text().strip()
        if got != version:
            print(f"fetch_sdk: the SDK at {commit[:9]} is {got}, sdk.lock says {version}", file=sys.stderr)
            return 1
        shutil.rmtree(dest / top, ignore_errors=True)
        (src / "sdk").rename(dest / top)
    stamp.write_text(commit + "\n")
    print(dest / top)
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--from", dest="source", type=Path, help="a local openrac-mod-sdk-<version>.zip")
    args = ap.parse_args(argv)
    lock = json.loads((ROOT / "sdk.lock").read_text())
    if "git" in lock:
        return from_git(lock)
    version, want = lock["version"], (lock.get("sha256") or "").lower()
    if not want:
        if args.source:
            print("fetch_sdk: sdk.lock has no sha256 to check a local archive against", file=sys.stderr)
            return 1
        listed = urllib.request.urlopen(lock["url"] + ".sha256", timeout=60).read().decode().split()
        if not listed or len(listed[0]) != 64:
            print(f"fetch_sdk: {lock['url']}.sha256 gives no SHA-256", file=sys.stderr)
            return 1
        want = listed[0].lower()
        lock["sha256"] = want
        (ROOT / "sdk.lock").write_text(json.dumps(lock, indent=2) + "\n")
        print(f"fetch_sdk: sdk.lock now pins {want}; commit it", file=sys.stderr)
    top = f"openrac-mod-sdk-{version}"
    dest = ROOT / ".sdk"
    stamp = dest / f"{top}.sha256"
    if (dest / top / "cmake").is_dir() and stamp.is_file() and stamp.read_text().strip() == want:
        print(dest / top)
        return 0
    data = args.source.read_bytes() if args.source else urllib.request.urlopen(lock["url"], timeout=60).read()
    got = hashlib.sha256(data).hexdigest()
    if got != want:
        print(f"fetch_sdk: the SDK's SHA-256 is {got}, sdk.lock pins {want}", file=sys.stderr)
        return 1
    dest.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=dest) as tmp:
        archive = Path(tmp) / f"{top}.zip"
        archive.write_bytes(data)
        with zipfile.ZipFile(archive) as z:
            for name in z.namelist():
                if name.startswith("/") or ".." in Path(name).parts or not name.startswith(top + "/"):
                    print(f"fetch_sdk: {name} is outside {top}/", file=sys.stderr)
                    return 1
            z.extractall(tmp)
        shutil.rmtree(dest / top, ignore_errors=True)
        (Path(tmp) / top).rename(dest / top)
    stamp.write_text(want + "\n")
    print(dest / top)
    return 0


if __name__ == "__main__":
    sys.exit(main())
