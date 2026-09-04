# Security Policy

## Reporting Issues

Please do not open public issues for vulnerabilities, leaked credentials, or
private infrastructure details.

Until a dedicated security contact is published, report sensitive issues
privately to the repository owner.

## Scope

Security-sensitive areas include:

- build and release scripts
- package index generation
- upload/flash tooling
- network, TLS, MQTT, OTA, filesystem, and storage code
- any accidental exposure of private SDK paths, credentials, or internal URLs

## Public Repo Hygiene

- do not commit secrets, tokens, certificates, or private keys
- do not commit private SDK trees
- do not publish local absolute paths in public-facing instructions
- publish large binary artifacts through release assets instead of git
