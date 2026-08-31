---
title: Wifi Sharing
description: Share separately configured guest Wifi with a scannable QR code on EspControl.
---

# Wifi Sharing

**Wifi Sharing** provides two card types for sharing a guest network. Both open the same modal with QR and text connection details:

- **Connect Card** shows a configurable title and Wifi icon.
- **QR Card** shows the scannable QR code directly on a white tile, without a title or icon.

It does not read, reveal, or share the Wifi network used by the EspControl panel itself. You enter a separate network name and, where needed, a password.

## Set It Up

1. Add **Wifi Sharing**, then choose **Connect Card** or **QR Card** from its **Type** setting.
2. Enter the guest network name exactly as it is broadcast, including any meaningful spaces.
3. Choose **WPA/WPA2 Personal** and enter its password, or choose **Open** for a password-free network.
4. Turn on **Hidden network** only when the network does not broadcast its name.
5. Save the card, then tap it on the panel. Scan the black-and-white code with a current iPhone or Android phone.

The Connect Card title and Wifi icon can be changed. The QR Card intentionally has no title or icon and uses all available tile space for the QR code. Neither card displays the password as text on the dashboard.

## Supported Networks

- WPA/WPA2 Personal passwords of 8–63 bytes, or a 64-character hexadecimal key
- Open networks
- Hidden networks
- Unicode network names and passwords

Enterprise Wifi, WEP, links, plain text QR codes, colour choices, and sharing the panel's own connection are not supported.

## Backup Safety

The password is base64url encoded only so it fits safely in the card configuration; it is **not encrypted**. Exported backup JSON files include Wifi Sharing credentials so they can restore correctly. Keep those files private and delete old copies you no longer need.

The optional `web_server_auth` package is not required for Wifi Sharing. Without it, anyone who can access the panel's web interface on your network can also retrieve the configured Wifi Sharing credentials. This exposure is intentional so the panel can share the password; enable web authentication only when you want to restrict access to the whole web interface.
