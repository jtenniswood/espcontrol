---
title: "Home Assistant Webhook Cards"
description:
  How to use webhook cards on your EspControl panel to call HTTP URLs directly from the device.
---

# Webhook

A Webhook card is a one-tap HTTP request. It calls the URL directly from the panel, so it can trigger systems that expose simple web APIs without going through Home Assistant.

Use Webhook cards for local automation platforms such as Jeedom, openHAB, Fibaro, Node-RED, or Home Assistant webhook automations. They also work with cloud webhook services such as IFTTT and Zapier when the panel can reach the service URL.

## Setting Up a Webhook Card

1. Select a card and change its type to **Webhook**.
2. Set a **Label** - this is the text shown on the card.
3. Choose the HTTP **Method**.
4. Enter the webhook **URL**.
5. For POST, PUT, or PATCH requests, optionally enter a **Body**.
6. Optionally enter **Headers**.
7. Choose an **Icon**.

## Common Examples

### Jeedom Scenario

Jeedom scenarios and commands often use HTTP GET URLs. Paste the full Jeedom API URL into the **URL** field and leave the method as **GET**.

Example:

```text
http://jeedom.local/core/api/jeeApi.php?apikey=YOUR_KEY&type=scenario&id=56&action=start
```

### Home Assistant Webhook

Set the method to **POST** and use the webhook URL from your Home Assistant automation.

If you send JSON in the body, add this header:

```text
Content-Type: application/json
```

### IFTTT or Zapier

Set the method to **POST**. Use the service webhook URL, add a JSON body if needed, and include:

```text
Content-Type: application/json
```

## Headers

Enter headers as `Name: value` pairs. Separate multiple headers with semicolons.

Example:

```text
Content-Type: application/json; Authorization: Bearer YOUR_TOKEN
```

If you enter a JSON-looking body and do not provide a `Content-Type` header, EspControl sends `Content-Type: application/json` automatically.

## Limits and Security

Webhook details are stored in the panel configuration. Anyone who can view or edit the device configuration may be able to see URLs, keys, or tokens saved in a Webhook card.

Keep destructive actions behind local-only or private URLs where possible. Avoid using a public webhook URL for actions such as unlocking a door or opening a garage unless the receiving system adds its own protection.

The saved card configuration is limited to 255 characters, so very long URLs, headers, or request bodies may need a shorter API endpoint or an automation relay such as Node-RED.
