# Security Policy

## Supported Versions

The following versions of QuickSnapAudio are currently being supported with security updates.

| Version | Supported          |
| ------- | ------------------ |
| 1.0.3   | Yes                |
| 1.0.2   | No                 |
| 1.0.1   | No                 |
| < 1.0.1 | No                 |

We recommend always updating to the latest version using the built-in **Check for Updates** feature in the system tray menu.

## Reporting a Vulnerability

If you discover a security vulnerability in QuickSnapAudio, please report it responsibly.

**Do not open a public GitHub issue for security vulnerabilities.**

Instead, please report them via one of the following methods:

1. **GitHub Private Vulnerability Reporting**: Use the [Security Advisories](https://github.com/horaceV1/QuickSnapAudio/security/advisories/new) page to submit a private report directly on GitHub.
2. **Email**: Send a detailed report to the repository maintainer via the contact information on their GitHub profile.

### What to include in your report

- A clear description of the vulnerability
- Steps to reproduce the issue
- The version of QuickSnapAudio affected
- Your operating system and version
- Any potential impact or severity assessment

### What to expect

- **Acknowledgment**: We will acknowledge receipt of your report within **48 hours**.
- **Updates**: We will provide status updates at least every **7 days** while the issue is being investigated.
- **Resolution**: If the vulnerability is confirmed, we aim to release a fix within **14 days** depending on severity and complexity.
- **Credit**: With your permission, we will credit you in the release notes for the fix.

### If a vulnerability is declined

If we determine the report does not constitute a security vulnerability, we will provide a clear explanation of our reasoning. You are welcome to follow up or open a regular issue for further discussion.

## Scope

The following areas are in scope for security reports:

- The QuickSnapAudio desktop application (Windows and Linux builds)
- The auto-update mechanism (download integrity, URL handling)
- Configuration file handling and storage
- Hotkey registration and system-level interactions
- Any bundled dependencies

The following are **out of scope**:

- Vulnerabilities in third-party dependencies that are already publicly known (please check if an update is available first)
- Issues that require physical access to the user's machine
- Social engineering attacks
