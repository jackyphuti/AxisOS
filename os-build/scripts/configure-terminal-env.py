#!/usr/bin/env python3
"""
AxisOS Terminal & Debian Environment Configurator
Ensures complete Debian CLI functionality:
- Full PATH (/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin) across /etc/environment, /etc/profile, and shell rc files.
- Bash completion enabled system-wide.
- Resolves bash_profile syntax errors.
- Sets axis-terminal as default x-terminal-emulator via update-alternatives.
- Configures PAM / sudo password synchronization for 'axis' and 'root' users.
"""

import os
import sys
import subprocess
import shutil

CHROOT = "/build/chroot" if os.path.exists("/build/chroot/etc") else "/"

def run_cmd(cmd, chroot=True):
    if chroot and CHROOT != "/":
        full_cmd = ["chroot", CHROOT] + cmd
    else:
        full_cmd = cmd
    print(f"--> Running: {' '.join(full_cmd)}")
    try:
        subprocess.run(full_cmd, check=True)
    except Exception as e:
        print(f"    [WARN] Command error: {e}")

def write_chroot_file(rel_path, content, mode=0o644):
    target = os.path.join(CHROOT, rel_path.lstrip("/"))
    os.makedirs(os.path.dirname(target), exist_ok=True)
    with open(target, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    os.chmod(target, mode)
    print(f"--> Wrote {target} ({mode:o})")

def main():
    print("==================================================")
    print("    AxisOS Terminal & Debian Environment Setup   ")
    print(f"    Target Root: {CHROOT}")
    print("==================================================")

    # 1. /etc/environment: Standard global PATH
    env_content = (
        'PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"\n'
        'TERM="xterm-256color"\n'
        'COLORTERM="truecolor"\n'
        'LANG="C.UTF-8"\n'
    )
    write_chroot_file("/etc/environment", env_content)

    # 2. Fix ~/.bash_profile and /etc/skel/.bash_profile
    bash_profile_content = (
        '# ~/.bash_profile: executed by bash(1) for login shells.\n'
        'if [ -f "$HOME/.bashrc" ]; then\n'
        '    . "$HOME/.bashrc"\n'
        'fi\n'
        '\n'
        '# Ensure complete Debian execution paths\n'
        'export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$PATH"\n'
        'export TERM="xterm-256color"\n'
        'export COLORTERM="truecolor"\n'
    )
    write_chroot_file("/home/axis/.bash_profile", bash_profile_content)
    write_chroot_file("/etc/skel/.bash_profile", bash_profile_content)

    # 3. Enhanced /home/axis/.bashrc and /etc/skel/.bashrc
    bashrc_content = (
        '# ~/.bashrc: executed by bash(1) for non-login shells.\n'
        'case $- in\n'
        '    *i*) ;;\n'
        '      *) return;;\n'
        'esac\n'
        '\n'
        '# Shell History\n'
        'HISTCONTROL=ignoreboth\n'
        'shopt -s histappend\n'
        'HISTSIZE=10000\n'
        'HISTFILESIZE=20000\n'
        'shopt -s checkwinsize\n'
        '\n'
        '# Complete Debian PATH\n'
        'export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$PATH"\n'
        'export TERM="xterm-256color"\n'
        'export COLORTERM="truecolor"\n'
        '\n'
        '# Color prompt with OSC window title for terminal\n'
        'PS1="\\[\\e]0;\\u@\\h: \\w\\a\\]\\[\\033[01;32m\\]\\u@\\h\\[\\033[00m\\]:\\[\\033[01;34m\\]\\w\\[\\033[00m\\]\\$ "\n'
        '\n'
        '# Standard Debian aliases\n'
        'alias ls="ls --color=auto"\n'
        'alias ll="ls -alF --color=auto"\n'
        'alias la="ls -A --color=auto"\n'
        'alias l="ls -CF --color=auto"\n'
        'alias grep="grep --color=auto"\n'
        'alias fgrep="fgrep --color=auto"\n'
        'alias egrep="egrep --color=auto"\n'
        '\n'
        '# Enable system-wide bash completion\n'
        'if ! shopt -oq posix; then\n'
        '  if [ -f /usr/share/bash-completion/bash_completion ]; then\n'
        '    . /usr/share/bash-completion/bash_completion\n'
        '  elif [ -f /etc/bash_completion ]; then\n'
        '    . /etc/bash_completion\n'
        '  fi\n'
        'fi\n'
    )
    write_chroot_file("/home/axis/.bashrc", bashrc_content)
    write_chroot_file("/etc/skel/.bashrc", bashrc_content)
    write_chroot_file("/root/.bashrc", bashrc_content)

    # 4. Set ownership of /home/axis
    run_cmd(["chown", "-R", "axis:axis", "/home/axis"], chroot=True)

    # 5. Set default passwords (axis / axis, root / axis)
    p = subprocess.Popen(["chroot", CHROOT, "chpasswd"], stdin=subprocess.PIPE, text=True)
    p.communicate("axis:axis\nroot:axis\n")
    print("--> Set passwords for axis and root to 'axis'")

    # 6. Ensure sudoers group and secure_path (NOPASSWD for live session installer)
    sudoers_axis = (
        "# AxisOS standard sudoer configuration\n"
        "axis ALL=(ALL:ALL) NOPASSWD: ALL\n"
        "Defaults secure_path=\"/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin\"\n"
        "Defaults !use_pty\n"
        "Defaults !requiretty\n"
    )
    write_chroot_file("/etc/sudoers.d/axis", sudoers_axis, mode=0o440)

    # 6b. Deploy Polkit rule allowing live session full administrative permissions
    polkit_rule = (
        "// Allow axis user and sudo group full administrative permissions in live environment\n"
        "polkit.addRule(function(action, subject) {\n"
        "    if (subject.isInGroup(\"sudo\") || subject.user === \"axis\") {\n"
        "        return polkit.Result.YES;\n"
        "    }\n"
        "});\n"
    )
    write_chroot_file("/etc/polkit-1/rules.d/49-axisos.rules", polkit_rule, mode=0o644)

    # 7. Register axis-terminal as default x-terminal-emulator
    run_cmd([
        "update-alternatives", "--install",
        "/usr/bin/x-terminal-emulator", "x-terminal-emulator",
        "/usr/local/bin/axis-terminal", "100"
    ], chroot=True)

    run_cmd([
        "update-alternatives", "--set",
        "x-terminal-emulator", "/usr/local/bin/axis-terminal"
    ], chroot=True)

    # Also symlink /usr/bin/axis-terminal -> /usr/local/bin/axis-terminal
    target_axis_bin = os.path.join(CHROOT, "usr", "bin", "axis-terminal")
    if not os.path.exists(target_axis_bin):
        try:
            os.symlink("/usr/local/bin/axis-terminal", target_axis_bin)
        except Exception:
            shutil.copyfile(os.path.join(CHROOT, "usr", "local", "bin", "axis-terminal"), target_axis_bin)

    # 8. Hide duplicate UXTerm / XTerm launchers from app menu
    apps_dir_chroot = os.path.join(CHROOT, "usr", "share", "applications")
    for dup_desktop in [
        "debian-xterm.desktop",
        "debian-uxterm.desktop",
        "xterm.desktop",
        "uxterm.desktop",
    ]:
        p_desk = os.path.join(apps_dir_chroot, dup_desktop)
        if os.path.exists(p_desk):
            try:
                with open(p_desk, "r", encoding="utf-8", errors="ignore") as f:
                    d_txt = f.read()
                if "NoDisplay=true" not in d_txt:
                    d_txt = d_txt.rstrip() + "\nNoDisplay=true\n"
                    with open(p_desk, "w", encoding="utf-8") as f:
                        f.write(d_txt)
                    print(f"--> Hidden duplicate menu launcher: {dup_desktop}")
            except Exception as e:
                print(f"    [WARN] Could not hide {dup_desktop}: {e}")

    print("\n[SUCCESS] AxisOS Debian terminal environment configured completely.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
