#!/install/bin/python3
"""
OriginOS Basic — boot.py

This file runs AS PID 1. init.c is a thin C stub that mounts /proc,
/sys, /dev, then execs this script directly — from this point on,
every single thing OriginOS Basic does is Python code we wrote,
not busybox, not a borrowed shell.

Rules that matter because we are PID 1:
  - This process must never exit. If it does, the kernel panics
    (nothing is left to reap zombie processes or handle signals).
  - Uncaught exceptions must not kill the process — wrap the main
    loop so a bug in a command doesn't take the whole OS down.
"""

import sys
import os
import subprocess

TOOLS_DIR = "/originos/tools"


def discover_tools():
    """Any .py file in TOOLS_DIR becomes a command, named after the file
    (minus .py). Returns {name: full_path}."""
    tools = {}
    if not os.path.isdir(TOOLS_DIR):
        return tools
    for entry in os.listdir(TOOLS_DIR):
        if entry.endswith(".py"):
            name = entry[:-3]
            tools[name] = os.path.join(TOOLS_DIR, entry)
    return tools


def run_tool(path, args):
    """Run a tool as an isolated subprocess. A crash, infinite loop, or
    bad exit in the tool can NEVER take down boot.py (PID 1) — it's a
    separate process. We just wait for it and report the result."""
    try:
        result = subprocess.run(
            ["/bin/python3", path] + args,
            stdin=sys.stdin,
            stdout=sys.stdout,
            stderr=sys.stderr,
        )
        if result.returncode != 0:
            print(f"[tool exited with code {result.returncode}]")
    except Exception as e:
        print(f"error running tool: {e}")


BANNER = r"""
   ___       _       _        ___  ____
  / _ \ _ __(_) __ _(_)_ __   / _ \/ ___|
 | | | | '__| |/ _` | | '_ \ | | | \___ \
 | |_| | |  | | (_| | | | | || |_| |___) |
  \___/|_|  |_|\__, |_|_| |_(_)___/|____/
               |___/
  OriginOS Basic — running on Python {version}
"""


def print_banner():
    version = f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"
    print(BANNER.format(version=version))


def cmd_help(args):
    print("Built-in commands:")
    print("  help          show this message")
    print("  ls [path]     list directory contents")
    print("  pwd           show current directory")
    print("  cd <path>     change directory")
    print("  cat <file>    print file contents")
    print("  ps            list running processes (from /proc)")
    print("  uname         show system info")
    print("  exit          this does nothing — PID 1 cannot exit")

    tools = discover_tools()
    if tools:
        print("\nTools (from /originos/tools):")
        for name in sorted(tools):
            print(f"  {name}")
    else:
        print(f"\nNo tools found in {TOOLS_DIR} yet.")


def cmd_ls(args):
    path = args[0] if args else "."
    try:
        for entry in sorted(os.listdir(path)):
            print(entry)
    except OSError as e:
        print(f"ls: {e}")


def cmd_pwd(args):
    print(os.getcwd())


def cmd_cd(args):
    if not args:
        print("cd: missing argument")
        return
    try:
        os.chdir(args[0])
    except OSError as e:
        print(f"cd: {e}")


def cmd_cat(args):
    if not args:
        print("cat: missing argument")
        return
    for filename in args:
        try:
            with open(filename, "r") as f:
                sys.stdout.write(f.read())
        except OSError as e:
            print(f"cat: {e}")


def cmd_ps(args):
    # Minimal /proc walk — no busybox ps, we read it ourselves.
    try:
        pids = [p for p in os.listdir("/proc") if p.isdigit()]
    except OSError as e:
        print(f"ps: {e}")
        return
    print(f"{'PID':>6}  CMD")
    for pid in sorted(pids, key=int):
        try:
            with open(f"/proc/{pid}/comm", "r") as f:
                name = f.read().strip()
        except OSError:
            name = "?"
        print(f"{pid:>6}  {name}")


def cmd_uname(args):
    info = os.uname()
    print(f"{info.sysname} {info.release} {info.machine}")
    print("OriginOS Basic (Python-native userland)")


def cmd_exit(args):
    print("PID 1 cannot exit — this would panic the kernel. Ignoring.")


COMMANDS = {
    "help": cmd_help,
    "ls": cmd_ls,
    "pwd": cmd_pwd,
    "cd": cmd_cd,
    "cat": cmd_cat,
    "ps": cmd_ps,
    "uname": cmd_uname,
    "exit": cmd_exit,
    "quit": cmd_exit,
}


def run_shell():
    print_banner()
    print("Type 'help' for available commands.\n")

    while True:
        try:
            line = input("originos# ").strip()
        except EOFError:
            # No TTY input available right now — don't exit, just wait.
            continue
        except KeyboardInterrupt:
            print()
            continue

        if not line:
            continue

        parts = line.split()
        cmd, args = parts[0], parts[1:]

        handler = COMMANDS.get(cmd)
        if handler is not None:
            try:
                handler(args)
            except Exception as e:
                # A bug in a built-in command must never take down PID 1.
                print(f"error running '{cmd}': {e}")
            continue

        tools = discover_tools()
        if cmd in tools:
            run_tool(tools[cmd], args)
            continue

        print(f"{cmd}: command not found")


def main():
    # PID 1 must never let an exception escape main() — that kills
    # the process, which kernel-panics the whole system. Wrap the
    # entire shell loop so we always recover into it.
    while True:
        try:
            run_shell()
        except Exception as e:
            print(f"boot.py: unexpected top-level error: {e}")
            print("Restarting shell loop...")


if __name__ == "__main__":
    main()