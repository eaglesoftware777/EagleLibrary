Eagle Library Translation Packs
Copyright (c) 2026 Eagle Software. All rights reserved.

Place custom JSON language packs in this folder.

Each pack should contain:
- code
- name
- nativeName
- rtl
- translations

Example:
{
  "code": "pt_br",
  "name": "Portuguese (Brazil)",
  "nativeName": "Português (Brasil)",
  "rtl": false,
  "translations": {
    "menu.file": "&Arquivo",
    "action.settings": "Configurações..."
  }
}

If a key is missing, Eagle Library falls back to English.
