---
title: Language
description: Choose the panel language and learn how to contribute translations for EspControl.
---

# Language

Open the panel web page, go to **Settings**, then choose **Language**.

The language setting is stored on the device as the Home Assistant select entity **Screen: Language**. It is also included when you export a backup from the panel setup page.

The selector shows languages that are included in the firmware. English, Czech, Danish, German, Spanish, Finnish, French, Hungarian, Italian, Norwegian Bokmål, Dutch, Polish, Portuguese, Brazilian Portuguese, Romanian, Slovak, Slovenian, Swedish, Turkish, and Ukrainian are included.

## Translation Files

Panel strings are kept in `common/config/strings.en.txt`. Each translated language has a matching `common/config/strings.<language-code>.txt` file.

To contribute another language:

1. Copy `common/config/strings.en.txt`.
2. Rename the copy to `common/config/strings.<language-code>.txt`, for example `strings.it.txt` for Italian.
3. Translate only the text after the `=`.
4. Keep every key before the `=` exactly the same.
5. Add the language code to the firmware language select options in `common/addon/time.yaml`.

Example:

```text
settings_language=Language
```

For another language, `settings_language` stays unchanged. Only `Language` is translated.

The strings file is for words shown on the panel screen. It does not include text that comes from the Home Assistant API, such as entity names, custom labels, option values, or media titles. It also does not include text shown only by the built-in webserver setup page.
