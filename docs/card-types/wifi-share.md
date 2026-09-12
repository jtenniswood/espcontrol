---
title: Wifi Sharing
description: Share separately configured guest Wifi with a scannable QR code on EspControl.
---

# Wifi Sharing

**Wifi Sharing** provides two card types for sharing a guest network. Both open the same modal with QR and text connection details:

- **Connect Card** shows a configurable title and Wifi icon.
- **QR Card** shows the scannable QR code directly on a white tile, without a title or icon.

It does not read, reveal, or share the Wifi network used by the EspControl panel itself. You enter a separate network name and, where needed, a password.

## Setting Up a Wifi Sharing Card

1. Add **Wifi Sharing**, then choose **Connect Card** or **QR Card** from its **Type** setting. Enter a **Name** directly below Type (default: **Connect**).
2. Under **Wifi Network**, enter the **Network name (SSID)** exactly as it is broadcast, including any meaningful spaces.
3. Choose **WPA/WPA2 Personal** and enter its password, or choose **Open** for a password-free network.
4. Turn on **Hidden network** only when the network does not broadcast its name.
5. Save the card, then tap it on the panel. Scan the black-and-white code with a current iPhone or Android phone.

The **Name** appears on the Connect Card and in the clock bar while either card’s modal is open. It is preserved when switching card types. Change the Connect Card’s Wifi icon under **Card Settings**. The QR Card tile intentionally has no title or icon and uses all available tile space for the QR code. Neither card displays the password as text on the dashboard.

## Modal Settings

Under **Modal Settings**, choose which tabs appear when you tap the card:

- **QR Code** - shows the code visitors scan to join the network.
- **Connection Details** - shows the network name and password for visitors to enter manually.

Both tabs are enabled by default. You can reorder them or hide either one, but at least one must stay enabled. These settings affect the popup; the QR Card still shows its code on the dashboard.

## Supported Networks

- WPA/WPA2 Personal passwords of 8–63 bytes, or a 64-character hexadecimal key
- Open networks
- Hidden networks
- Unicode network names and passwords

Enterprise Wifi, WEP, links, plain text QR codes, colour choices, and sharing the panel's own connection are not supported.

## Backup Safety

Wifi Sharing passwords are stored in the panel configuration and included in [backup files](/features/backup). They are **not encrypted**, so keep backups private.

Wifi Sharing works without web authentication. Anyone who can access the panel's web interface can retrieve the saved guest-network credentials. Enable web authentication if you want to restrict access to the web interface.
