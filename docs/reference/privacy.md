---
title: Privacy Policy
description:
  How EspControl handles information in the documentation website, firmware, and web configuration interface.
---

# Privacy Policy

**Effective date: September 5, 2026**

EspControl is a source-available, non-commercial touchscreen control panel
project maintained by [jtenniswood](https://github.com/jtenniswood). This
policy explains what
information may be involved when you use the EspControl documentation website,
firmware, or built-in web configuration interface.

## The short version

EspControl does not operate an account system, advertising service, analytics
platform, or central service that collects your smart-home data. The firmware is
designed to communicate with your Home Assistant installation and other
services you choose to configure. Your device configuration is stored on your
device or in your own backups.

## The documentation website

The documentation site is hosted by GitHub Pages. When you visit it, GitHub
may process technical information such as your IP address, browser and device
information, the pages requested, and timestamps in order to deliver and
secure the site. GitHub controls that processing under its own policies; see
the [GitHub Privacy Statement](https://docs.github.com/en/site-policy/privacy-policies/github-privacy-statement).

The site does not intentionally use cookies, advertising trackers, or a
first-party analytics service. Its search is generated and run in your
browser.

Every documentation page includes a support button whose image is loaded from
Buy Me a Coffee's CDN. Loading that image may disclose your IP address and
browser request information to Buy Me a Coffee or its service providers.
Following the button takes you to Buy Me a Coffee, which has its own privacy
policy.

The install and C6 recovery pages also automatically load the USB installer
library from unpkg when the browser supports Web Serial, and the install pages
check a public firmware manifest. These browser requests may disclose normal
request metadata, such as your IP address, browser and device information, and
request time, to the relevant hosting providers. The library is not loaded on
other documentation pages unless one of these installer components is used.

The site also contains links to GitHub, Home Assistant, ESPHome, and other
third-party websites. Those sites have their own terms and privacy practices.

## The EspControl firmware and device

The firmware does not send your smart-home data to an EspControl server. It
normally exchanges data directly with your Home Assistant instance over your
local network, including entity states, names, media information, weather
data, and commands needed for the cards you configure. Home Assistant and any
integrations in your installation determine how that information is stored and
processed.

Depending on the cards and options you enable, the device may also:

- request media artwork from URLs supplied by Home Assistant or by your
  configuration;
- send requests to webhook URLs that you configure; and
- use Home Assistant's voice-assistant pipeline, which may involve a speech
  service or other provider selected in Home Assistant.

These requests are initiated by your configuration and may expose information
to the service named by the URL or integration. Review the privacy practices
of those services before enabling them.

## Firmware and asset downloads

Firmware with EspControl's update checker enabled periodically requests public
version metadata and, when an update is installed, firmware files from the
EspControl GitHub Pages site. Some builds also check the public ESPHome hosted
firmware manifest for an ESP32-C6 co-processor. These requests contain normal
network request information, such as the device's IP address and request time,
as observed by the hosting provider. They are not intended to include your
Home Assistant entity data or account credentials.

You can disable EspControl's built-in update checker where the firmware build
provides that option. Ethernet-only builds documented on this site omit the
EspControl update checker. ESPHome OTA or other update methods may still
contact the services you select.

Standard released firmware also loads the JavaScript for its built-in web
configuration interface from EspControl's GitHub Pages site. The request URL
includes the device model slug and firmware version, so the hosting provider
may receive those values along with normal browser or device connection
metadata when the interface is opened. This asset request is separate from
the firmware update checker.

Networked standard builds automatically synchronize time with the public NTP
pool at `0.pool.ntp.org`, `1.pool.ntp.org`, and `2.pool.ntp.org`. DNS and NTP
requests may disclose normal connection metadata to the pool operators. This
traffic occurs even when no optional cards or services are configured and the
EspControl update checker is disabled.

## Information you provide to the project

If you open a GitHub issue, pull request, or discussion, GitHub processes your
GitHub account information and the content you submit. Issue descriptions,
logs, screenshots, and configuration snippets may be public. Remove passwords,
tokens, Wi-Fi credentials, private URLs, and other sensitive information before
posting. Do not use a public issue to send a privacy request or other
confidential information.

## Retention and sharing

The project maintainer does not operate a database of EspControl users and
does not sell or rent personal information. Information submitted through
GitHub is retained and handled according to GitHub's policies and the
project's public repository settings. Information handled by Home Assistant,
Buy Me a Coffee, GitHub Pages, or another service you choose is governed by
that service's policy and retention practices.

## Your choices and privacy requests

You can choose whether to use automatic firmware updates, configure optional
webhooks or voice features, visit external links, or submit information to
GitHub. To remove local device settings, use a full flash erase or another
verified factory-reset procedure that clears the device's storage. A normal
reset or firmware re-flash may preserve saved configuration, including entity
IDs, webhook credentials, and other settings. This does not remove copies you
have made elsewhere.

For a question about this policy or a request concerning information directly
controlled by the project maintainer, contact [jtenniswood through GitHub](https://github.com/jtenniswood).
Please do not include private information in a public issue. Requests about
GitHub, Home Assistant, or another third-party service should be directed to
that service.

## Changes to this policy

This policy may be updated when EspControl's data practices or the services it
uses change. The effective date at the top of this page indicates when the
current version was published.
