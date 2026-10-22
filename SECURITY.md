# Security Policy

The AxisOS team takes the security of our operating system, desktop shell, system daemons, and package management infrastructure very seriously. We appreciate the efforts of the security research community and open-source contributors in helping keep AxisOS secure.

---

## Supported Versions

We actively provide security updates, vulnerability patches, and bug fixes for the following versions of AxisOS:

| Version | Supported | Status |
| :--- | :---: | :--- |
| **2.1.x ("Debian Edition")** | Yes | Active Release |
| **2.0.x ("Horizon v2")** | Yes | Maintenance & Critical Fixes |
| **< 2.0** | No | End of Life (Unsupported) |

---

## Reporting a Vulnerability

> [!IMPORTANT]
> **Please DO NOT report security vulnerabilities via public GitHub issues, discussions, or social media.** Public disclosure before a fix is available puts users and installations at risk.

### Preferred Method: Private Vulnerability Reporting (GitHub)
AxisOS has **GitHub Private Vulnerability Reporting** enabled:
1. Navigate to the [AxisOS Security Advisories](https://github.com/jackyphuti/AxisOS/security/advisories) page.
2. Click **"Report a vulnerability"**.
3. Fill out the advisory form with:
   - A clear description of the vulnerability.
   - Affected components (e.g. `axis-compositor`, `axis-installer`, `axis-settings`, kernel drivers).
   - Step-by-step instructions or Proof of Concept (PoC) to reproduce the issue.
   - Potential impact (e.g. privilege escalation, denial of service, remote code execution).
4. Submit the report. This creates a confidential, private advisory visible only to maintainers.

### Alternative Method: Direct Security Contact
If you cannot use GitHub's advisory system, send an email directly to:
- **Lead Maintainer**: Jacky Mpoka (`jackympoka22@gmail.com`)
- **Subject**: `[SECURITY] AxisOS Vulnerability Report: <Brief Summary>`

---

## Response & Disclosure Timeline

We adhere to coordinated vulnerability disclosure best practices:

1. **Initial Acknowledgment**: Within **48 hours** of receiving your report, a maintainer will acknowledge receipt and begin triage.
2. **Assessment & Confirmation**: Within **5 business days**, we will confirm reproduction and severity rating (CVSS score).
3. **Patch Development & Validation**: A fix will be developed in a private security fork or branch.
4. **Coordinated Release**: Once verified, the patch will be merged, and an updated package or ISO release will be published alongside an official CVE/GHSA advisory giving proper credit to the reporter.

---

## Security Best Practices for AxisOS Components

- **Desktop Shell**: Runs with native Wayland boundaries preventing cross-window snooping and unauthorized input capture.
- **Privilege Separation**: Non-interactive administration is gated strictly via secure sudo rules and Polkit policies.
- **Atomic Updates**: Utilizes Btrfs subvolumes and snapshot checkpoints to guarantee automated rollbacks if an upgrade fails.
- **Packaging (`axis`)**: Package archives are checked for filesystem containment and cryptographic checksums prior to installation.

Thank you for helping us keep AxisOS secure for everyone!
