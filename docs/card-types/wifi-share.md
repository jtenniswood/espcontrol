---
title: Wi-Fi Share
description: Share separately configured guest Wi-Fi with a scannable QR code on EspControl.
---

# Wi-Fi Share

**Wi-Fi Share** adds a card that opens a large QR code for a guest network. It is useful near an entrance, in a guest room, or anywhere visitors may need the network details without asking for the password.

It does not read, reveal, or share the Wi-Fi network used by the EspControl panel itself. You enter a separate network name and, where needed, a password.

## Set It Up

1. Select a card and choose **Wi-Fi Share**.
2. Enter the guest network name exactly as it is broadcast, including any meaningful spaces.
3. Choose **WPA/WPA2 Personal** and enter its password, or choose **Open** for a password-free network.
4. Turn on **Hidden network** only when the network does not broadcast its name.
5. Save the card, then tap it on the panel. Scan the black-and-white code with a current iPhone or Android phone.

The card title and Wi-Fi icon can be changed. The tile never displays the password or QR code itself.

## Supported Networks

- WPA/WPA2 Personal passwords of 8–63 bytes, or a 64-character hexadecimal key
- Open networks
- Hidden networks
- Unicode network names and passwords

Enterprise Wi-Fi, WEP, links, plain text QR codes, colour choices, and sharing the panel's own connection are not supported.

## Backup Safety

The password is base64url encoded only so it fits safely in the card configuration; it is **not encrypted**. Exported backup JSON files include Wi-Fi Share credentials so they can restore correctly. Keep those files private and delete old copies you no longer need.
