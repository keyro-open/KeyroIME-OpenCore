// Copyright (C) 2025-2026 Localpro株式会社 (Localpro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) v1.0 OpenCore.
// KeyroIME is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation.
//
// For commercial use licensing, custom deployment, or proprietary integrations,
// please contact Localpro株式会社 via https://localpro.jp. Unauthorized closed-source
// commercial exploitation is strictly prohibited.
/// Romaji-to-Kana ローマ字からひらがなへのマッピングモジュール
/// 標準的な日本語入力方式に沿って、基本ローマ字からひらがなへのマッピングを実装します。
use std::collections::HashMap;

/// ローマ字変換エンジン
pub struct RomajiConverter {
    /// 静的マッピング表：ローマ字 -> ひらがな
    mapping: HashMap<&'static str, &'static str>,
    /// マッピング表内の最長ローマ字長。変換ロジックとマトリクスの不整合を防ぎます。
    max_mapping_len: usize,
}

impl RomajiConverter {
    /// ローマ字変換器を初期化し、完全なマッピング表を構築します。
    pub fn new() -> Self {
        let mut mapping = HashMap::new();

        // ===== 基本単音かな =====
        // あ行
        mapping.insert("a", "あ");
        mapping.insert("i", "い");
        mapping.insert("u", "う");
        mapping.insert("e", "え");
        mapping.insert("o", "お");

        // か行
        mapping.insert("ka", "か");
        mapping.insert("ki", "き");
        mapping.insert("ku", "く");
        mapping.insert("ke", "け");
        mapping.insert("ko", "こ");

        // さ行
        mapping.insert("sa", "さ");
        mapping.insert("si", "し");
        mapping.insert("shi", "し"); // 別名
        mapping.insert("su", "す");
        mapping.insert("se", "せ");
        mapping.insert("so", "そ");

        // た行
        mapping.insert("ta", "た");
        mapping.insert("ti", "ち");
        mapping.insert("chi", "ち"); // 別名
        mapping.insert("tu", "つ");
        mapping.insert("tsu", "つ"); // 別名
        mapping.insert("te", "て");
        mapping.insert("to", "と");

        // な行
        mapping.insert("na", "な");
        mapping.insert("ni", "に");
        mapping.insert("nu", "ぬ");
        mapping.insert("ne", "ね");
        mapping.insert("no", "の");

        // は行
        mapping.insert("ha", "は");
        mapping.insert("hi", "ひ");
        mapping.insert("hu", "ふ");
        mapping.insert("fu", "ふ"); // 別名
        mapping.insert("he", "へ");
        mapping.insert("ho", "ほ");

        // ま行
        mapping.insert("ma", "ま");
        mapping.insert("mi", "み");
        mapping.insert("mu", "む");
        mapping.insert("me", "め");
        mapping.insert("mo", "も");

        // や行
        mapping.insert("ya", "や");
        mapping.insert("yu", "ゆ");
        mapping.insert("yo", "よ");

        // ら行
        mapping.insert("ra", "ら");
        mapping.insert("ri", "り");
        mapping.insert("ru", "る");
        mapping.insert("re", "れ");
        mapping.insert("ro", "ろ");

        // わ行
        mapping.insert("wa", "わ");
        mapping.insert("wi", "ゐ"); // 古語
        mapping.insert("we", "ゑ"); // 古語
        mapping.insert("wo", "を");

        // ん
        mapping.insert("n", "ん");
        mapping.insert("nn", "ん"); // 明示的な「ん」

        // が行（濁音）
        mapping.insert("ga", "が");
        mapping.insert("gi", "ぎ");
        mapping.insert("gu", "ぐ");
        mapping.insert("ge", "げ");
        mapping.insert("go", "ご");

        // ざ行（濁音）
        mapping.insert("za", "ざ");
        mapping.insert("zi", "じ");
        mapping.insert("ji", "じ"); // 別名
        mapping.insert("zu", "ず");
        mapping.insert("ze", "ぜ");
        mapping.insert("zo", "ぞ");

        // だ行（濁音）
        mapping.insert("da", "だ");
        mapping.insert("di", "ぢ");
        mapping.insert("du", "づ");
        mapping.insert("de", "で");
        mapping.insert("do", "ど");

        // ば行（濁音）
        mapping.insert("ba", "ば");
        mapping.insert("bi", "び");
        mapping.insert("bu", "ぶ");
        mapping.insert("be", "べ");
        mapping.insert("bo", "ぼ");

        // ぱ行（半濁音）
        mapping.insert("pa", "ぱ");
        mapping.insert("pi", "ぴ");
        mapping.insert("pu", "ぷ");
        mapping.insert("pe", "ぺ");
        mapping.insert("po", "ぽ");

        // ===== 拗音（小書き文字合成） =====
        mapping.extend([
            ("kya", "きゃ"),
            ("kyu", "きゅ"),
            ("kyo", "きょ"),
            ("kyi", "きぃ"),
            ("kye", "きぇ"),
            ("gya", "ぎゃ"),
            ("gyu", "ぎゅ"),
            ("gyo", "ぎょ"),
            ("gyi", "ぎぃ"),
            ("gye", "ぎぇ"),
            ("sya", "しゃ"),
            ("syu", "しゅ"),
            ("syo", "しょ"),
            ("syi", "しぃ"),
            ("sye", "しぇ"),
            ("sha", "しゃ"),
            ("shu", "しゅ"),
            ("sho", "しょ"),
            ("she", "しぇ"),
            ("zya", "じゃ"),
            ("zyu", "じゅ"),
            ("zyo", "じょ"),
            ("zyi", "じぃ"),
            ("zye", "じぇ"),
            ("jya", "じゃ"),
            ("jyu", "じゅ"),
            ("jyo", "じょ"),
            ("jyi", "じぃ"),
            ("jye", "じぇ"),
            ("ja", "じゃ"),
            ("ju", "じゅ"),
            ("jo", "じょ"),
            ("je", "じぇ"),
            ("tya", "ちゃ"),
            ("tyu", "ちゅ"),
            ("tyo", "ちょ"),
            ("tyi", "ちぃ"),
            ("tye", "ちぇ"),
            ("cya", "ちゃ"),
            ("cyu", "ちゅ"),
            ("cyo", "ちょ"),
            ("cyi", "ちぃ"),
            ("cye", "ちぇ"),
            ("cha", "ちゃ"),
            ("chu", "ちゅ"),
            ("cho", "ちょ"),
            ("che", "ちぇ"),
            ("dya", "ぢゃ"),
            ("dyu", "ぢゅ"),
            ("dyo", "ぢょ"),
            ("dyi", "ぢぃ"),
            ("dye", "ぢぇ"),
            ("nya", "にゃ"),
            ("nyu", "にゅ"),
            ("nyo", "にょ"),
            ("nyi", "にぃ"),
            ("nye", "にぇ"),
            ("hya", "ひゃ"),
            ("hyu", "ひゅ"),
            ("hyo", "ひょ"),
            ("hyi", "ひぃ"),
            ("hye", "ひぇ"),
            ("bya", "びゃ"),
            ("byu", "びゅ"),
            ("byo", "びょ"),
            ("byi", "びぃ"),
            ("bye", "びぇ"),
            ("pya", "ぴゃ"),
            ("pyu", "ぴゅ"),
            ("pyo", "ぴょ"),
            ("pyi", "ぴぃ"),
            ("pye", "ぴぇ"),
            ("mya", "みゃ"),
            ("myu", "みゅ"),
            ("myo", "みょ"),
            ("myi", "みぃ"),
            ("mye", "みぇ"),
            ("rya", "りゃ"),
            ("ryu", "りゅ"),
            ("ryo", "りょ"),
            ("ryi", "りぃ"),
            ("rye", "りぇ"),
            // KeyroIME 互換別名。独立した小書きゃゅょには xya/xyu/xyo を使用します。
            ("lya", "りゃ"),
            ("lyu", "りゅ"),
            ("lyo", "りょ"),
            ("lyi", "りぃ"),
            ("lye", "りぇ"),
        ]);

        // ===== 小文字 =====
        mapping.extend([
            ("xa", "ぁ"),
            ("xi", "ぃ"),
            ("xu", "ぅ"),
            ("xe", "ぇ"),
            ("xo", "ぉ"),
            ("la", "ぁ"),
            ("li", "ぃ"),
            ("lu", "ぅ"),
            ("le", "ぇ"),
            ("lo", "ぉ"),
            ("xya", "ゃ"),
            ("xyu", "ゅ"),
            ("xyo", "ょ"),
            ("xtu", "っ"),
            ("xtsu", "っ"),
            ("ltu", "っ"),
            ("ltsu", "っ"),
            ("xwa", "ゎ"),
            ("lwa", "ゎ"),
            ("xka", "ゕ"),
            ("lka", "ゕ"),
            ("xke", "ゖ"),
            ("lke", "ゖ"),
        ]);

        // ===== 拡張拗音と外来音 =====
        mapping.extend([
            ("ye", "いぇ"),
            ("tsa", "つぁ"),
            ("tsi", "つぃ"),
            ("tse", "つぇ"),
            ("tso", "つぉ"),
            ("thi", "てぃ"),
            ("thu", "てゅ"),
            ("the", "てぇ"),
            ("tho", "てょ"),
            ("dhi", "でぃ"),
            ("dhu", "でゅ"),
            ("dhe", "でぇ"),
            ("dho", "でょ"),
            ("twa", "とぁ"),
            ("twi", "とぃ"),
            ("twu", "とぅ"),
            ("twe", "とぇ"),
            ("two", "とぉ"),
            ("dwa", "どぁ"),
            ("dwi", "どぃ"),
            ("dwu", "どぅ"),
            ("dwe", "どぇ"),
            ("dwo", "どぉ"),
            ("fa", "ふぁ"),
            ("fi", "ふぃ"),
            ("fe", "ふぇ"),
            ("fo", "ふぉ"),
            ("fya", "ふゃ"),
            ("fyu", "ふゅ"),
            ("fyo", "ふょ"),
            ("vu", "ゔ"),
            ("va", "ゔぁ"),
            ("vi", "ゔぃ"),
            ("ve", "ゔぇ"),
            ("vo", "ゔぉ"),
            ("vya", "ゔゃ"),
            ("vyu", "ゔゅ"),
            ("vyo", "ゔょ"),
            ("kwa", "くぁ"),
            ("kwi", "くぃ"),
            ("kwe", "くぇ"),
            ("kwo", "くぉ"),
            ("gwa", "ぐぁ"),
            ("gwi", "ぐぃ"),
            ("gwe", "ぐぇ"),
            ("gwo", "ぐぉ"),
            ("qwa", "くぁ"),
            ("qwi", "くぃ"),
            ("qwu", "くぅ"),
            ("qwe", "くぇ"),
            ("qwo", "くぉ"),
        ]);

        // ===== 特殊音 =====
        // 促音（っ）は貪欲マッチ中に二重子音として動的に識別します。
        // ー 長音符号
        mapping.insert("-", "ー");

        let max_mapping_len = mapping.keys().map(|key| key.len()).max().unwrap_or(1);
        RomajiConverter {
            mapping,
            max_mapping_len,
        }
    }

    /// ローマ字文字列をひらがなへ変換します。
    /// 貪欲マッチ方式を使用し、長いキーから短いキーへ順に照合します。
    pub fn convert(&self, romaji: &str) -> String {
        let mut result = String::new();
        let mut position = 0;

        while position < romaji.len() {
            let remaining = &romaji[position..];
            let mut matched = false;

            // マッピングキーはすべて ASCII です。非 ASCII 入力は文字単位でそのまま維持します。
            if !remaining.as_bytes()[0].is_ascii() {
                let ch = remaining.chars().next().unwrap();
                result.push(ch);
                position += ch.len_utf8();
                continue;
            }

            // 貪欲マッチ: 長さはマッピング行列から自動決定し、xtsu/ltsu などの4文字キーにも対応します。
            for len in (1..=self.max_mapping_len.min(remaining.len())).rev() {
                if remaining.is_char_boundary(len) {
                    let substr = &remaining[..len];

                    // 特別処理: 二重子音の促音（例: kk -> っk）
                    if len == 2 && substr.chars().nth(0) == substr.chars().nth(1) {
                        let first_char = substr.chars().nth(0).unwrap();
                        // 子音のみ促音を形成できます。
                        if first_char.is_ascii_alphabetic() && !"aiueon".contains(first_char) {
                            result.push('っ');
                            position += 1;
                            matched = true;
                            break;
                        }
                    }

                    if let Some(&kana) = self.mapping.get(substr) {
                        result.push_str(kana);
                        position += len;
                        matched = true;
                        break;
                    }
                }
            }

            // 一致しない場合は元の文字を保持します。
            if !matched {
                let ch = remaining.chars().next().unwrap();
                result.push(ch);
                position += ch.len_utf8();
            }
        }

        result
    }

    /// 文字列が有効なローマ字か確認します（ASCII 文字のみ）。
    pub fn is_valid_romaji(input: &str) -> bool {
        input.chars().all(|c| c.is_ascii_alphabetic() || c == '-')
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_basic_romaji() {
        let converter = RomajiConverter::new();
        assert_eq!(converter.convert("a"), "あ");
        assert_eq!(converter.convert("ka"), "か");
        assert_eq!(converter.convert("sa"), "さ");
        assert_eq!(converter.convert("shi"), "し");
        assert_eq!(converter.convert("ta"), "た");
        assert_eq!(converter.convert("chi"), "ち");
    }

    #[test]
    fn test_word_conversion() {
        let converter = RomajiConverter::new();
        assert_eq!(converter.convert("koukan"), "こうかん");
        assert_eq!(converter.convert("nihon"), "にほん");
        assert_eq!(converter.convert("watashi"), "わたし");
        assert_eq!(converter.convert("arigatou"), "ありがとう");
    }

    #[test]
    fn test_youon() {
        let converter = RomajiConverter::new();
        let cases = [
            ("kya", "きゃ"),
            ("gya", "ぎゃ"),
            ("sha", "しゃ"),
            ("sya", "しゃ"),
            ("cha", "ちゃ"),
            ("cya", "ちゃ"),
            ("jya", "じゃ"),
            ("jyu", "じゅ"),
            ("jyo", "じょ"),
            ("dya", "ぢゃ"),
            ("nya", "にゃ"),
            ("hya", "ひゃ"),
            ("bya", "びゃ"),
            ("pya", "ぴゃ"),
            ("mya", "みゃ"),
            ("rya", "りゃ"),
            ("lya", "りゃ"),
        ];
        for (romaji, kana) in cases {
            assert_eq!(
                converter.convert(romaji),
                kana,
                "failed to convert {romaji}"
            );
        }
    }

    #[test]
    fn test_small_kana() {
        let converter = RomajiConverter::new();
        let cases = [
            ("xaxixuxexo", "ぁぃぅぇぉ"),
            ("lalilulelo", "ぁぃぅぇぉ"),
            ("xyaxyuxyo", "ゃゅょ"),
            ("xtuxtsultultsu", "っっっっ"),
            ("xwalwaxkalka xkelke", "ゎゎゕゕ ゖゖ"),
        ];
        for (romaji, kana) in cases {
            assert_eq!(
                converter.convert(romaji),
                kana,
                "failed to convert {romaji}"
            );
        }
    }

    #[test]
    fn test_extended_youon_and_foreign_sounds() {
        let converter = RomajiConverter::new();
        let cases = [
            ("shejeche", "しぇじぇちぇ"),
            ("tsatsitsetso", "つぁつぃつぇつぉ"),
            ("thidhitwodwi", "てぃでぃとぉどぃ"),
            ("fafifefobye", "ふぁふぃふぇふぉびぇ"),
            ("vavyuvokwagwa", "ゔぁゔゅゔぉくぁぐぁ"),
        ];
        for (romaji, kana) in cases {
            assert_eq!(
                converter.convert(romaji),
                kana,
                "failed to convert {romaji}"
            );
        }
    }

    #[test]
    fn test_preserves_non_ascii_input() {
        let converter = RomajiConverter::new();
        assert_eq!(converter.convert("かなjya"), "かなじゃ");
    }

    #[test]
    fn test_sokuon() {
        let converter = RomajiConverter::new();
        assert_eq!(converter.convert("kko"), "っこ");
        assert_eq!(converter.convert("gakkou"), "がっこう");
        assert_eq!(converter.convert("kitte"), "きって");
    }

    #[test]
    fn test_dakuon() {
        let converter = RomajiConverter::new();
        assert_eq!(converter.convert("ga"), "が");
        assert_eq!(converter.convert("ji"), "じ");
        assert_eq!(converter.convert("zu"), "ず");
        assert_eq!(converter.convert("ba"), "ば");
        assert_eq!(converter.convert("pa"), "ぱ");
    }

    #[test]
    fn test_is_valid_romaji() {
        assert!(RomajiConverter::is_valid_romaji("koukan"));
        assert!(RomajiConverter::is_valid_romaji("github"));
        assert!(!RomajiConverter::is_valid_romaji("こうかん"));
        assert!(!RomajiConverter::is_valid_romaji("hello123"));
    }
}
