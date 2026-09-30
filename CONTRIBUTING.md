# Contributing to AxisOS 🚀

Thank you for your interest in contributing to **AxisOS**! 

AxisOS is a next-generation, glassmorphic Linux operating system built on the Linux kernel, Cage Wayland compositor, an asynchronous system daemon, and a modern hardware-accelerated desktop shell. We welcome contributions from developers, designers, system engineers, kernel hackers, and technical writers worldwide.

---

## 📜 Table of Contents
1. [Code of Conduct](#-code-of-conduct)
2. [Contribution Workflow](#-contribution-workflow)
3. [Repository Structure](#-repository-structure)
4. [Development Guidelines](#-development-guidelines)
   - [Desktop Shell (TypeScript / React)](#desktop-shell-typescript--react)
   - [System Daemon (Node.js)](#system-daemon-nodejs)
   - [Systems & Kernel Drivers (C / Linux)](#systems--kernel-drivers-c--linux)
5. [Commit Message Conventions](#-commit-message-conventions)
6. [Developer Certificate of Origin (DCO) & Signed-off-by](#-developer-certificate-of-origin-dco--signed-off-by)
7. [Submitting a Pull Request](#-submitting-a-pull-request)
8. [Reporting Issues & Bugs](#-reporting-issues--bugs)
9. [Join the Community](#-join-the-community)

---

## 🕊️ Code of Conduct

We are committed to providing a welcoming, inclusive, and harassment-free environment for everyone. Please be respectful, constructive, and collaborative in all discussions, pull requests, and issue threads.

---

## 🔄 Contribution Workflow

To keep our codebase clean, stable, and robust:

> [!IMPORTANT]
> **No direct pushes to `main`**: All code contributions must be submitted via a **Pull Request (PR)** from a fork or topic branch. Automated merging to `main` is disabled to ensure every contribution undergoes manual code review and verification.

### Standard Git Workflow:
1. **Fork the Repository**: Click the **Fork** button at the top right of [jackyphuti/AxisOS](https://github.com/jackyphuti/AxisOS).
2. **Clone your Fork**:
   ```bash
   git clone https://github.com/<your-username>/AxisOS.git
   cd AxisOS
   ```
3. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/my-new-feature
   # or for bug fixes:
   git checkout -b fix/resolve-audio-crash
   ```
4. **Make Your Changes**: Follow our [Development Guidelines](#-development-guidelines).
5. **Verify Locally**:
   - Run type checks and build tests: `npm run build` in `shell/`.
   - Test daemon syntax: `node -c os-build/configs/cage-session/axisos-daemon.cjs`.
6. **Commit Your Changes**: Follow [Conventional Commits](#-commit-message-conventions).
7. **Push to Your Fork**:
   ```bash
   git push origin feature/my-new-feature
   ```
8. **Open a Pull Request**: Go to the GitHub repository and open a PR targeting the `main` branch.

---

## 📁 Repository Structure

Understanding where code lives will help you navigate and contribute effectively:

| Directory | Purpose | Key Technologies |
| :--- | :--- | :--- |
| `shell/` | The desktop environment, window manager, and native applications. | React 19, TypeScript, Vite, Tailwind CSS, Lucide Icons |
| `shell/src/apps/` | Individual system applications (Terminal, Files, Browser, Settings, Store, etc.). | React, TypeScript |
| `shell/src/services/` | Frontend API client communicating with the backend system daemon. | REST, WebSockets, TypeScript |
| `os-build/` | Linux OS image build system, kernel drivers, and systemd configs. | Bash, Debian live-build, C, systemd |
| `os-build/configs/` | Session scripts, Cage Wayland compositor configs, and daemon services. | Cage, Xwayland, systemd |
| `os-build/drivers/` | Linux device drivers (character, block, platform devices). | Linux Kernel C API |
| `os-build/updater/` | Atomic dual-partition A/B update engine (`axis update`). | POSIX C, kexec |
| `pkg-mgr/` | Native `axis` package manager runtime and format tools. | POSIX C / Shell |

---

## 🛠️ Development Guidelines

### Desktop Shell (TypeScript / React)
- **Strict Typing**: All components, hooks, and services must pass `tsc` with zero errors. Do not use `any` unless absolutely unavoidable.
- **Asynchronous Execution**: The UI thread must never block. Long-running tasks must show loading indicators, progress bars, or run in the background.
- **Responsive Layout**: Support window resizing, maximizing, and snapping gracefully without horizontal overflow or clipped text.
- **Deep Linking**: Apps should support parameters passed via window state (`params?: Record<string, any>`).

### System Daemon (Node.js)
- **Location**: `os-build/configs/cage-session/axisos-daemon.cjs`
- **Dependencies**: Keep the daemon lean with minimal external npm dependencies so it can execute reliably on minimal live-boot environments.
- **Graceful Error Handling**: Never allow an API failure or unhandled exception to terminate the daemon process. Always return JSON `{ error: string }` or appropriate HTTP status codes.
- **Security**: Validate and sanitize file paths and system command parameters before passing them to execution layers.

### Systems & Kernel Drivers (C / Linux)
- Follow the Linux Kernel Coding Style (tabs for indentation, 80-column line limits where practical).
- Properly manage memory allocations with clean cleanup paths on error conditions.
- Respect the Linux Device Model (`MODULE_DEVICE_TABLE`, `probe()`, `remove()`).

---

## 📝 Commit Message Conventions

We use [Conventional Commits](https://www.conventionalcommits.org/) to keep the commit history informative and automate changelog generation.

### Format:
```
<type>(<scope>): <short description>

[optional body explaining rationale]
```

### Types:
- `feat`: A new feature (e.g. `feat(shell): implement tabbed terminal sessions`)
- `fix`: A bug fix (e.g. `fix(file-manager): resolve permission calculation on rootfs`)
- `docs`: Documentation updates (e.g. `docs: add development and build instructions`)
- `style`: Formatting, CSS tweaks, no logic changes (e.g. `style(dock): refine glass blur effect`)
- `refactor`: Code change that neither fixes a bug nor adds a feature
- `perf`: Performance improvements
- `test`: Adding or improving tests
- `chore`: Maintenance tasks, dependency bumps, build configurations

---

## ✍️ Developer Certificate of Origin (DCO) & Signed-off-by

To ensure software provenance, legal compliance with our GPL-3.0 license, and transparency across our kernel, daemon, and desktop code, **AxisOS requires all contributions to be signed off by their real author**.

### 1. The Developer Certificate of Origin (DCO 1.1)
By adding a `Signed-off-by` trailer to your commit message, you certify that you have the legal right to submit the code under the **Developer Certificate of Origin (DCO)**:

```text
Developer Certificate of Origin
Version 1.1

Copyright (C) 2004, 2006 Open Source Development Labs, Inc.
By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

### 2. Mandatory Real Name Policy
> [!IMPORTANT]
> Your `Signed-off-by` line **must use your real legal or professional name** and an authentic, working email address. Pseudonyms, anonymous handles, or bot nicknames are not permitted.
> 
> - ✅ **Valid**: `Signed-off-by: Jane Doe <jane.doe@example.com>`
> - ❌ **Invalid**: `Signed-off-by: @xX_anon_Xx <anon@users.noreply.github.com>`
> - ❌ **Invalid**: Missing `Signed-off-by` trailer.

### 3. How to Sign Off Your Commits

#### New Commits:
Pass the `-s` (or `--signoff`) flag when committing:
```bash
git commit -s -m "feat(shell): add window snap gestures"
```
Git will automatically append your sign-off trailer:
```text
feat(shell): add window snap gestures

Signed-off-by: Real Name <real.email@example.com>
```

#### Configuring your Git Identity:
Ensure your local git configuration reflects your real name and email:
```bash
git config --global user.name "Your Real Name"
git config --global user.email "your.email@example.com"
```

#### Amending an Unsigned Commit:
If you forgot to sign off your latest commit before pushing:
```bash
git commit --amend --signoff --no-edit
```

#### Signing Off Past Commits in a Branch:
If your feature branch has multiple commits that need sign-offs:
```bash
git rebase --signoff origin/main
```

---

## 🚀 Submitting a Pull Request

When opening a Pull Request:
1. Provide a clear, descriptive title following conventional commit style.
2. Ensure **every commit** in the PR contains a valid `Signed-off-by: Real Name <email>` trailer.
3. Fill out the PR description with:
   - Summary of changes.
   - Issue/feature reference (e.g. `Closes #12`).
   - Screenshots or video clips for visual/UI changes.
   - Verification steps you performed.
4. Be responsive to review comments and code suggestions.

---

## 🐛 Reporting Issues & Bugs

If you discover a bug or have a feature idea:
- Check existing [GitHub Issues](https://github.com/jackyphuti/AxisOS/issues) to avoid duplicates.
- Open a new issue with:
  - Clear steps to reproduce the issue.
  - Expected vs. actual behavior.
  - Hardware specifications (CPU, GPU, RAM, UEFI vs BIOS).
  - Screenshots or terminal log outputs if applicable.

---

## 💬 Join the Community

- **GitHub Discussions**: Share ideas, showcase setups, and ask questions.
- **Release Milestone**: Version 2.0 "Horizon" is scheduled for December release!

Thank you for helping make AxisOS the best desktop Linux experience possible! 🌟
