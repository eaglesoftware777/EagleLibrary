# Security Policy

## Supported Versions

Security fixes are provided for the latest public release of Eagle Library.

## Reporting Security Issues

Report security issues privately to:

```text
eagle@eaglesoftware.biz
```

Please include:

- affected version
- Windows version
- clear reproduction steps
- sample file only if it is safe to share

## Release Hardening

Eagle Library uses a conservative desktop security model:

- files remain in their original locations
- the database is local to the application folder
- plugin URL actions are limited to trusted HTTPS destinations
- Python hook execution is disabled unless explicitly enabled in settings
- diagnostic logs are disabled by default and stored locally when enabled
- release workflows support Authenticode signing when certificate secrets are configured

## Windows Publisher Warning

Windows shows an “Unknown publisher” warning for unsigned executables and installers.
To remove that warning for distributed builds, configure an Authenticode code-signing
certificate in the release workflow secrets:

- `WINDOWS_CODESIGN_CERT_BASE64`
- `WINDOWS_CODESIGN_CERT_PASSWORD`

Without a valid code-signing certificate, Windows cannot display Eagle Software as a
verified publisher.
