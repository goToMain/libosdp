# Security Policy

There are many users using LibOSDP in some capacity in production. If you think
you found a bug that may have security implications, please follow the usual
responsible disclosure protocols. Any issue reported in this channel will be
acknowledged withing 3 business days.

If an issue has been confirmed  by a maintainer, we request the reporter to
respect a 90 day embargo period before making the issue public.

## Supported Versions

LibOSDP will support the last 2 [releases][1] for security and bug fixes.

| Version | Branch | Supported          |
| ------- | -------|------------------- |
| <= 1.5  | N/A    | :x:                |
| 2.4.x   | 2.4.x  | :white_check_mark: |
| latest  | master | :white_check_mark: |

## Reporting a Vulnerability

Please send an email to sidcha.dev@gmail.com ([GPG]([2])).

## Security Mailing List

If you are a vendor using LibOSDP in a product (or any production capacity),
please send an email to sidcha.dev@gmail.com to get added to a private mailing
list which will be used to notify about critical incidents such as
vulnerabilities and potential fixes or workarounds before the issue has been made
public.

You can also follow the [security advisories][3] page but this will be updated
only after the issue has been made public.

Note: For very obvious reasons, not everyone can be added to this list. You
should be able to prove that you are indeed using LibOSDP in production.

[1]: https://github.com/goToMain/libosdp/releases
[2]: https://github.com/sidcha.gpg
[3]: https://github.com/goToMain/libosdp/security/advisories
