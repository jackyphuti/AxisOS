#!/usr/bin/env bash
set -e

CHROOT="/build/chroot"

echo "=================================================="
echo "  Configuring Terminal Environment & Privileges   "
echo "=================================================="

# 1. Environment variables
cat << 'EOF' > "${CHROOT}/etc/environment"
PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
TERM="xterm-256color"
COLORTERM="truecolor"
EOF

# 2. Bashrc configuration
BASHRC_CONTENT='
case $- in
    *i*) ;;
      *) return;;
esac

HISTCONTROL=ignoreboth
shopt -s histappend
HISTSIZE=10000
HISTFILESIZE=20000
shopt -s checkwinsize

export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$PATH"
export TERM="xterm-256color"
export COLORTERM="truecolor"

PS1="\[\e]0;\u@\h: \w\a\]\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ "

alias ls="ls --color=auto"
alias ll="ls -alF --color=auto"
alias la="ls -A --color=auto"
alias l="ls -CF --color=auto"
alias grep="grep --color=auto"

if ! shopt -oq posix; then
  if [ -f /usr/share/bash-completion/bash_completion ]; then
    . /usr/share/bash-completion/bash_completion
  elif [ -f /etc/bash_completion ]; then
    . /etc/bash_completion
  fi
fi
'

echo "$BASHRC_CONTENT" > "${CHROOT}/home/axis/.bashrc"
echo "$BASHRC_CONTENT" > "${CHROOT}/etc/skel/.bashrc"
echo "$BASHRC_CONTENT" > "${CHROOT}/root/.bashrc"

# 3. Passwords
echo "axis:axis" | chroot "$CHROOT" chpasswd 2>/dev/null || true
echo "root:axis" | chroot "$CHROOT" chpasswd 2>/dev/null || true

# 4. Sudoers
mkdir -p "${CHROOT}/etc/sudoers.d"
cat << 'EOF' > "${CHROOT}/etc/sudoers.d/axis"
axis ALL=(ALL:ALL) NOPASSWD: ALL
Defaults secure_path="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
Defaults !use_pty
Defaults !requiretty
EOF
chmod 440 "${CHROOT}/etc/sudoers.d/axis"

# 5. Polkit rule
mkdir -p "${CHROOT}/etc/polkit-1/rules.d"
cat << 'EOF' > "${CHROOT}/etc/polkit-1/rules.d/49-axisos.rules"
polkit.addRule(function(action, subject) {
    if (subject.isInGroup("sudo") || subject.user === "axis") {
        return polkit.Result.YES;
    }
});
EOF
chmod 644 "${CHROOT}/etc/polkit-1/rules.d/49-axisos.rules"

# 6. Default terminal alternative
chroot "$CHROOT" update-alternatives --install /usr/bin/x-terminal-emulator x-terminal-emulator /usr/local/bin/axis-terminal 100 2>/dev/null || true
chroot "$CHROOT" update-alternatives --set x-terminal-emulator /usr/local/bin/axis-terminal 2>/dev/null || true

mkdir -p "${CHROOT}/usr/bin"
ln -sf /usr/local/bin/axis-terminal "${CHROOT}/usr/bin/axis-terminal"

# 7. Hide duplicate xterm launchers
for f in debian-xterm.desktop debian-uxterm.desktop xterm.desktop uxterm.desktop; do
    desk="${CHROOT}/usr/share/applications/$f"
    if [ -f "$desk" ]; then
        if ! grep -q "NoDisplay=true" "$desk"; then
            echo "NoDisplay=true" >> "$desk"
        fi
    fi
done

chroot "$CHROOT" chown -R 1000:1000 /home/axis 2>/dev/null || true

echo "[SUCCESS] AxisOS terminal environment configured completely."
