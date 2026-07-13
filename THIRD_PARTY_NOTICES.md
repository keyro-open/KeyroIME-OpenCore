# Third-Party Dictionary Sources

## EDRDG JMdict and JMnedict

KeyroIME's dictionary import pipeline is designed for the Electronic Dictionary Research and Development Group (EDRDG) JMdict and JMnedict datasets.

- Project: https://www.edrdg.org/wiki/index.php/JMdict-EDICT_Dictionary_Project
- Official downloads: https://ftp.edrdg.org/pub/Nihongo/
- Licence statement: https://www.edrdg.org/edrdg/licence.html
- Licence: Creative Commons Attribution-ShareAlike 4.0 International
- Licence text: https://creativecommons.org/licenses/by-sa/4.0/

Generated dictionary assets must retain this notice. Snapshot metadata and SHA-256 hashes are recorded in `src/keyro_service/assets/dictionary_manifest.json`.

Current third-party source snapshot was generated on 2026-06-15; the release manifest was updated on 2026-06-22:

- `JMdict_e.gz`: `65a52f037d95c9bd0a1954d74825c06d5392626b71630ead431937d6e2c26adf`
- `JMnedict.xml.gz`: `56a8138c37a2ee521b1894ca6a64fcbfca62c0752b5b437c5bb0dfdf96d96fea`

The release TSV files are deterministic, size-bounded derivatives of the official snapshots. Existing product baseline entries are merged before priority filtering so regression samples remain available.

LocalPro-authored curated supplement TSV files, including the high-frequency katakana loanword asset updated on 2026-06-22, are original data authored and owned by LocalPro Co., Ltd. They are not copied from additional third-party dictionaries. Katakana coverage is cross-checked against the National Institute for Japanese Language and Linguistics public "Gairaigo" terminology pages and Digital Agency public standard-guideline terminology. These pages are terminology references only; KeyroIME does not redistribute their explanations or bulk page content.

The local import recorded in `dictionary_manifest.json` is also original data authored and owned by LocalPro Co., Ltd. The generic local filename was an internal preparation label and did not identify an external dictionary source.

- NINJAL terminology reference: https://www2.ninjal.ac.jp/gairaigo/Teian1_4/index.html
- Digital Agency terminology reference: https://www.digital.go.jp/resources/standard_guidelines

Microsoft Japanese IME is not a dictionary source for this release. Its installed proprietary `.DIC`/`.FIL` files are not copied or parsed, and candidate output is not bulk-distilled for redistribution. The documented `IFELanguage` API may be used only for local behavioral comparison.

## Excluded Sources

BCCWJ frequency-list data is not included because its general-download terms are oriented to research and educational use. KeyroIME uses JMdict priority tags (`news`, `ichi`, `spec`, `gai`, `nf`) for the import score instead.
